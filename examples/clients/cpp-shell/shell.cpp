/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <sstream>
#include <iterator>
#include <cstdlib>

#if defined(_WIN32) && defined(__MINGW32__)
  /* This is a workaround for MinGW32, see libxml/xmlexports.h */
  #define IN_LIBXML
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "shell.h"
#include "common/utils/utils.h"
#include "transport/dashel_plugins/dashel-plugins.h"

using namespace std;
using namespace Dashel;
using namespace Aseba;

// code of the shell

Shell::Shell(const char* target):
	// connect to a Dashel stdin stream
	shellStream(connect("stdin:")),
	// connect to the Aseba target
	targetStream(connect(target))
{
	// first prompt
	wcerr << "> ";
}

bool Shell::isValid() const
{
	return shellStream && targetStream;
}

bool Shell::run1s()
{
	int timeout(1000);
	UnifiedTime startTime;
	while (timeout > 0)
	{
		if (!step(timeout))
			return false;
		timeout -= (Aseba::UnifiedTime() - startTime).value;
	}
	return true;
}

void Shell::sendMessage(const Message& message)
{
	message.serialize(targetStream);
	targetStream->flush();
}

void Shell::nodeDescriptionReceived(unsigned nodeId)
{
	wcerr << '\r';
	wcerr << "Received description for node " << getNodeName(nodeId) << endl;
	wcerr << endl;
	wcerr << "> " << UTF8ToWString(curShellCmd);
}

void Shell::incomingData(Dashel::Stream *stream)
{
	// dispatch to correct method in function of stream
	if (stream == shellStream)
		incomingShellData(stream);
	if (stream == targetStream)
		incomingTargetData(stream);
}

void Shell::incomingTargetData(Dashel::Stream *stream)
{
	// receive message
	Message *message = 0;
	try
	{
		// deserialize message using Aseba::Message::receive() static function
		message = Message::receive(stream);
	}
	catch (DashelException e)
	{
		// if this stream has a problem, ignore it for now,
		// and let Hub call connectionClosed later.
		wcerr << "error while receiving message: " << e.what() << endl;
		return;
	}

	// pass message to description manager, which builds
	// the node descriptions in background
	NodesManager::processMessage(message);
	
	// if variables, print
	const Variables *variables(dynamic_cast<Variables *>(message));
	if (variables)
	{
		// as this is a shell, we just print the result of the variable
		// message as we assumes that it was linked to the last
		// GetVariables request, but in case a third-party is requesting
		// variables there might be many response before the one corresponding
		// to the query. In that case, the variables->start value
		// must be checked against the variable map
		wcerr << '\r';
		for (size_t i = 0; i < variables->variables.size(); ++i)
			wcerr << variables->variables[i] << " ";
		wcerr << endl;
		wcerr << "> " << UTF8ToWString(curShellCmd);
	}
	
	// if user event, print
	const UserMessage *userMessage(dynamic_cast<UserMessage *>(message));
	if (userMessage)
	{
		wcerr << '\r';
		if (userMessage->type < commonDefinitions.events.size())
			wcerr << commonDefinitions.events[userMessage->type].name;
		else
			wcerr << "unknown event " << userMessage->type;
		wcerr << ": ";
		for (size_t i = 0; i < userMessage->data.size(); ++i)
			wcerr << userMessage->data[i] << " ";
		wcerr << endl;
		wcerr << "> " << UTF8ToWString(curShellCmd);
	}
	
	delete message;
}

void Shell::incomingShellData(Dashel::Stream *stream)
{
	// read one character
	char c;
	stream->read(&c, 1);
	
	// ignore \r on Windows
	#ifdef WIN32
	if (c == '\r')
		return;
	#endif // WIN32
	
	// if end of line, execute command, else store character
	if (c == '\n')
		processShellCmd();
	else
		curShellCmd += c;
}

void Shell::processShellCmd()
{
	// split command using whitespaces
	const strings args(split(curShellCmd));
	if (args.empty())
		return;
	
	// following command, do different things
	if (args[0] == "ls")
	{
		if (args.size() == 1)
			listNodes();
		else
			listVariables(args);
	}
	else if (args[0] == "get")
	{
		getVariable(args);
	}
	else if (args[0] == "set")
	{
		setVariable(args);
	}
	else if (args[0] == "emit")
	{
		emit(args);
	}
	else if (args[0] == "load")
	{
		load(args);
	}
	else if (args[0] == "stop")
	{
		stop(args);
	}
	else if (args[0] == "run")
	{
		run(args);
	}
	else if (args[0] == "quit")
	{
		Hub::stop();
		return;
	}
	else
	{
		if (args[0] != "help")
		{
			wcerr << "Unknown command: " << UTF8ToWString(curShellCmd) << endl;
			wcerr << "Valid commands are:" << endl;
		}
		else
			wcerr << "Available commands are: " << endl;
		wcerr << "  ls             list all nodes on network" << endl;
		wcerr << "  ls NODE_NAME   list the variables of a node" << endl;
		wcerr << "  get NODE_NAME VAR_NAME  get the value of a variable" << endl;
		wcerr << "  set NODE_NAME VAR_NAME VAR_DATA+  set the value of a variable" << endl;
		wcerr << "  emit EVENT_NAME EVENT_DATA*  emit an event along with payload" << endl;
		wcerr << "  load FILENAME  load an aesl file" << endl;
		wcerr << "  stop NODE_NAME stop the execution on a node" << endl;
		wcerr << "  run NODE_NAME  run the execution on a node" << endl;
		wcerr << "  quit           quit shell" << endl;
		wcerr << "  help           show this summary" << endl;
	}
	
	// read for new command
	curShellCmd = "";
	wcerr << endl << "> ";
}

void Shell::listNodes()
{
	for (NodesMap::const_iterator it(nodes.begin()); it != nodes.end(); ++it)
		wcerr << (it->second).name << endl;
}

void Shell::listVariables(const strings& args)
{
	// get target description
	if (args.size() < 2)
	{
		wcerr << "missing argument" << endl;
		return;
	}
	bool ok;
	const unsigned nodeId(getNodeId(UTF8ToWString(args[1]), 0, &ok));
	if (!ok)
	{
		wcerr << "invalid node name " << UTF8ToWString(args[1]) << endl;
		return;
	}
	const TargetDescription* desc(getDescription(nodeId));
	if (!desc)
		return;
	
	// dump target description
	wcerr << "Target name " << desc->name << endl;
	wcerr << "protocol version " << desc->protocolVersion << endl; 
	wcerr << "bytecode size " << desc->bytecodeSize << endl;
	wcerr << "variables size " << desc->variablesSize << endl;
	wcerr << "stack size " << desc->stackSize << endl;
	wcerr << "variables:" << endl;
	// if we have a result from the compiler for this node...
	const NodeNameToVariablesMap::const_iterator allVarMapIt(allVariables.find(args[1]));
	if (allVarMapIt != allVariables.end())
	{
		// ... use it
		const VariablesMap& varMap(allVarMapIt->second);
		VariablesMap::const_iterator it(varMap.begin());
		for (;it != varMap.end(); ++it)
			wcerr << "  " << it->first << " " << it->second.second << endl;
	}
	else
	{
		// ... otherwise shows variables from the target description
		for (size_t i=0; i<desc->namedVariables.size(); ++i)
		{
			const TargetDescription::NamedVariable& var(desc->namedVariables[i]);
			wcerr << "  " << var.name << " " << var.size << endl;
		}
	}
	wcerr << "local events: " << endl;
	for (size_t i=0; i<desc->localEvents.size(); ++i)
	{
		const TargetDescription::LocalEvent &event(desc->localEvents[i]);
		wcerr << "  " << event.name << " - " << event.description << endl;
	}
	wcerr << "native functions:" << endl;
	for (size_t i=0; i<desc->nativeFunctions.size(); ++i)
	{
		const TargetDescription::NativeFunction& func(desc->nativeFunctions[i]);
		wcerr << "  " << func.name << "(";
		for (size_t j=0; j<func.parameters.size(); ++j)
		{
			const TargetDescription::NativeFunctionParameter& param(func.parameters[j]);
			wcerr << param.name << "[";
			if (param.size > 0)
				wcerr << param.size;
			else
				wcerr << "T<" << -param.size << ">";
			wcerr << "]";
			if (j+1 != func.parameters.size())
				wcerr << ", ";
		}
		wcerr << ") - ";
		wcerr << func.description << endl;
	}
}

void Shell::getVariable(const strings& args)
{
	// check that there is the correct number of arguments
	if (args.size() != 3)
	{
		wcerr << "wrong number of arguments, usage: get NODE_NAME VAR_NAME" << endl;
		return;
	}
	
	// get node id, variable position and length
	unsigned nodeId, pos;
	const bool exists(getNodeAndVarPos(args[1], args[2], nodeId, pos));
	if (!exists)
		return;
	bool ok;
	const unsigned length(getVariableSize(nodeId, UTF8ToWString(args[2]), &ok));
	if (!ok)
		return;

	// send the message
	GetVariables getVariables(nodeId, pos, length);
	getVariables.serialize(targetStream);
	targetStream->flush();
}

void Shell::setVariable(const strings& args)
{
	// check that there are enough arguments
	if (args.size() < 4)
	{
		wcerr << "missing argument, usage: set NODE_NAME VAR_NAME VAR_DATA+" << endl;
		return;
	}
	
	// get node id, variable position and length
	unsigned nodeId, pos;
	const bool exists(getNodeAndVarPos(args[1], args[2], nodeId, pos));
	if (!exists)
		return;
	
	// send the message
	SetVariables::VariablesVector data;
	for (size_t i=3; i<args.size(); ++i)
		data.push_back(atoi(args[i].c_str()));
 	SetVariables setVariables(nodeId, pos, data);
	setVariables.serialize(targetStream);
	targetStream->flush();
}

void Shell::emit(const strings& args)
{
	// check that there are enough arguments
	if (args.size() < 2)
	{
		wcerr << "missing argument, usage: emit EVENT_NAME EVENT_DATA*" << endl;
		return;
	}
	size_t pos;
	if (!commonDefinitions.events.contains(UTF8ToWString(args[1]), &pos))
	{
		wcerr << "event " << UTF8ToWString(args[1]) << " is unknown" << endl;
		return;
	}
	
	// build event and emit
	UserMessage::DataVector data;
	for (size_t i=2; i<args.size(); ++i)
		data.push_back(atoi(args[i].c_str()));
	UserMessage userMessage(pos, data);
	userMessage.serialize(targetStream);
	targetStream->flush();
}

void Shell::load(const strings& args)
{
	// check that there is the correct number of arguments
	if (args.size() != 2)
	{
		wcerr << "wrong number of arguments, usage: load FILENAME" << endl;
		return;
	}
	
	// open document
	const string& fileName(args[1]);
	xmlDoc *doc(xmlReadFile(fileName.c_str(), NULL, 0));
	if (!doc)
	{
		wcerr << "cannot read XML from file " << UTF8ToWString(fileName) << endl;
		return;
	}
    xmlNode *domRoot(xmlDocGetRootElement(doc));
	
	// clear existing data
	commonDefinitions.events.clear();
	commonDefinitions.constants.clear();
	allVariables.clear();
	
	// load new data
	int noNodeCount(0);
	bool wasError(false);
	if (!xmlStrEqual(domRoot->name, BAD_CAST("network")))
	{
		wcerr << "root node is not \"network\", XML considered invalid" << endl;
		wasError = true;
	}
	else for (xmlNode *domNode(xmlFirstElementChild(domRoot)); domNode; domNode = domNode->next)
	{
		if (domNode->type == XML_ELEMENT_NODE)
		{
			// an Aseba node, which contains a virtual machine
			if (xmlStrEqual(domNode->name, BAD_CAST("node")))
			{
				// get attributes, child and content
				xmlChar *name(xmlGetProp(domNode, BAD_CAST("name")));
				if (!name)
				{
					wcerr << "missing \"name\" attribute in \"node\" entry" << endl;
				}
				else
				{
					const string _name((const char *)name);
					xmlChar * text(xmlNodeGetContent(domNode));
					if (!text)
					{
						wcerr << "missing text in \"node\" entry" << endl;
					}
					else
					{
						// got the identifier of the node and compile the code
						unsigned preferedId(0);
						xmlChar *storedId = xmlGetProp(domNode, BAD_CAST("nodeId"));
						if (storedId)
							preferedId = unsigned(atoi((char*)storedId));
						bool ok;
						unsigned nodeId(getNodeId(UTF8ToWString(_name), preferedId, &ok));
						if (ok)
						{
							if (!compileAndSendCode(UTF8ToWString((const char *)text), nodeId, _name))
								wasError = true;
						}
						else
							noNodeCount++;
						
						// free attribute and content
						xmlFree(text);
					}
					xmlFree(name);
				}
			}
			// a global event
			else if (xmlStrEqual(domNode->name, BAD_CAST("event")))
			{
				// get attributes
				xmlChar *name(xmlGetProp(domNode, BAD_CAST("name")));
				if (!name)
					wcerr << "missing \"name\" attribute in \"event\" entry" << endl;
				xmlChar *size(xmlGetProp(domNode, BAD_CAST("size"))); 
				if (!size)
					wcerr << "missing \"size\" attribute in \"event\" entry" << endl;
				// add event
				if (name && size)
				{
					int eventSize(atoi((const char *)size));
					if (eventSize > ASEBA_MAX_EVENT_ARG_SIZE)
					{
						wcerr << "Event " << name << " has a length " << eventSize << "larger than maximum" <<  ASEBA_MAX_EVENT_ARG_SIZE << endl;
						wasError = true;
						break;
					}
					else
					{
						commonDefinitions.events.push_back(NamedValue(UTF8ToWString((const char *)name), eventSize));
					}
				}
				// free attributes
				if (name)
					xmlFree(name);
				if (size)
					xmlFree(size);
			}
			// a global constant
			else if (xmlStrEqual(domNode->name, BAD_CAST("constant")))
			{
				// get attributes
				xmlChar *name(xmlGetProp(domNode, BAD_CAST("name")));
				if (!name)
					wcerr << "missing \"name\" attribute in \"constant\" entry" << endl;
				xmlChar *value(xmlGetProp(domNode, BAD_CAST("value"))); 
				if (!value)
					wcerr << "missing \"value\" attribute in \"constant\" entry" << endl;
				// add constant if attributes are valid
				if (name && value)
				{
					commonDefinitions.constants.push_back(NamedValue(UTF8ToWString((const char *)name), atoi((const char *)value)));
				}
				// free attributes
				if (name)
					xmlFree(name);
				if (value)
					xmlFree(value);
			}
			else
				wcerr << "Unknown XML node seen in .aesl file: " << domNode->name << endl;
		}
	}

	// release memory
	xmlFreeDoc(doc);
	
	// check if there was an error
	if (wasError)
	{
		wcerr << "There was an error while loading script " << UTF8ToWString(fileName) << endl;
		commonDefinitions.events.clear();
		commonDefinitions.constants.clear();
		allVariables.clear();
	}
	
	// check if there was some matching problem
	if (noNodeCount)
	{
		wcerr << noNodeCount << " scripts have no corresponding nodes in the current network and have not been loaded." << endl;
	}
}

bool Shell::compileAndSendCode(const wstring& source, unsigned nodeId, const string& nodeName)
{
	// compile code
	std::wistringstream is(source);
	Error error;
	BytecodeVector bytecode;
	unsigned allocatedVariablesCount;
	
	Compiler compiler;
	compiler.setTargetDescription(getDescription(nodeId));
	compiler.setCommonDefinitions(&commonDefinitions);
	bool result = compiler.compile(is, bytecode, allocatedVariablesCount, error);
	
	if (result)
	{
		// send bytecode
		sendBytecode(targetStream, nodeId, std::vector<uint16>(bytecode.begin(), bytecode.end()));
		// run node
		Run msg(nodeId);
		msg.serialize(targetStream);
		targetStream->flush();
		// retrieve user-defined variables for use in get/set
		allVariables[nodeName] = *compiler.getVariablesMap();
		return true;
	}
	else
	{
		wcerr << "compilation for node " << UTF8ToWString(nodeName) << " failed: " << error.toWString() << endl;
		return false;
	}
}

void Shell::stop(const strings& args)
{
	// check arguments
	if (args.size() != 2)
	{
		wcerr << "wrong number of arguments, usage: stop NODE_NAME" << endl;
		return;
	}
	bool ok;
	const unsigned nodeId(getNodeId(UTF8ToWString(args[1]), 0, &ok));
	if (!ok)
	{
		wcerr << "invalid node name " << UTF8ToWString(args[1]) << endl;
		return;
	}
	
	// build stop message and send
	Stop stopMsg(nodeId);
	stopMsg.serialize(targetStream);
	targetStream->flush();
}

void Shell::run(const strings& args)
{
	// check arguments
	if (args.size() != 2)
	{
		wcerr << "wrong number of arguments, usage: run NODE_NAME" << endl;
		return;
	}
	bool ok;
	const unsigned nodeId(getNodeId(UTF8ToWString(args[1]), 0, &ok));
	if (!ok)
	{
		wcerr << "invalid node name " << UTF8ToWString(args[1]) << endl;
		return;
	}
	
	// build run message and send
	Run runMsg(nodeId);
	runMsg.serialize(targetStream);
	targetStream->flush();
}

bool Shell::getNodeAndVarPos(const string& nodeName, const string& variableName, unsigned& nodeId, unsigned& pos) const
{
	// make sure the node exists
	bool ok;
	nodeId = getNodeId(UTF8ToWString(nodeName), 0, &ok);
	if (!ok)
	{
		wcerr << "invalid node name " << UTF8ToWString(nodeName) << endl;
		return false;
	}
	pos = unsigned(-1);

	// check whether variable is knwon from a compilation, if so, get position
	const NodeNameToVariablesMap::const_iterator allVarMapIt(allVariables.find(nodeName));
	if (allVarMapIt != allVariables.end())
	{
		const VariablesMap& varMap(allVarMapIt->second);
		const VariablesMap::const_iterator varIt(varMap.find(UTF8ToWString(variableName)));
		if (varIt != varMap.end())
			pos = varIt->second.first;
	}
	
	// if variable is not user-defined, check whether it is provided by this node
	if (pos == unsigned(-1))
	{
		bool ok;
		pos = getVariablePos(nodeId, UTF8ToWString(variableName), &ok);
		if (!ok)
		{
			wcerr << "variable " << UTF8ToWString(variableName) << " does not exists in node " << UTF8ToWString(nodeName);
			return false;
		}
	}
	return true;
}

int main(int argc, char *argv[])
{
	// check command line arguments
	if (argc != 2)
	{
		wcerr << "Usage: " << argv[0] << " TARGET" << endl;
		return 1;
	}
	
	// initialize Dashel plugins
	Dashel::initPlugins();
	
	// create the shell
	Shell shell(argv[1]);
	// check whether connection was successful
	if (!shell.isValid())
	{
		wcerr << "Connection failure" << endl;
		return 2;
	}
	
	// run the Dashel Hub
	do {
		shell.pingNetwork();
	} while (shell.run1s());
	
	return 0;
}
