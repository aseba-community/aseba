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
#include "../../common/msg/msg.h"
#include "../../common/utils/utils.h"
#include "HttpDashelTarget.h"
#include "HttpRequest.h"
#include "HttpInterface.h"

using Aseba::NodesManager;
using Aseba::Http::DashelHttpRequest;
using Aseba::Http::HttpDashelTarget;
using Aseba::Http::HttpInterface;
using std::cerr;
using std::endl;
using std::make_pair;
using std::map;
using std::set;
using std::string;
using std::vector;

HttpDashelTarget::HttpDashelTarget(HttpInterface *interface_, const std::string& address_, Dashel::Stream *stream_) :
	interface(interface_),
	address(address_),
	stream(stream_)
{

}

HttpDashelTarget::~HttpDashelTarget()
{

}

bool HttpDashelTarget::sendEvent(const std::vector<std::string>& args)
{
	if(args.empty()) {
		return false;
	}

	size_t eventPos;

	if(interface->getProgram().getCommonDefinitions().events.contains(UTF8ToWString(args[0]), &eventPos)) {
		try {
			// build event and emit
			UserMessage::DataVector data;
			for(size_t i = 1; i < args.size(); ++i) {
				data.push_back(atoi(args[i].c_str()));
			}
			UserMessage userMessage(eventPos, data);
			userMessage.serialize(stream);
			stream->flush();
		} catch(Dashel::DashelException e) {
			if(interface->isVerbose()) {
				cerr << "Target " << address << " failed to send event message to stream: " << e.what() << endl;
			}
			return false;
		}

		return true;
	} else {
		if(interface->isVerbose()) {
			cerr << "Target " << address << " failed to send event: No event " << args[0] << endl;
		}
		return false;
	}
}

bool HttpDashelTarget::sendGetVariables(unsigned globalNodeId, const std::vector<std::string>& args, HttpRequest *request)
{
	vector<unsigned> positions;

	map<unsigned, Node>::iterator query = nodes.find(globalNodeId);
	if(query == nodes.end()) {
		if(interface->isVerbose()) {
			cerr << "Target " << address << " failed to send get variables message for node " << globalNodeId << ": No such node" << endl;
		}
		return false;
	}

	Node& node = query->second;
	assert(globalNodeId == node.globalId);

	for(vector<string>::const_iterator it(args.begin()); it != args.end(); ++it) {
		unsigned position;
		unsigned size;

		if(getVariableInfo(node, *it, position, size)) {
			try {
				// send the message
				GetVariables getVariables(node.localId, position, size);
				getVariables.serialize(stream);
				stream->flush();
			} catch(Dashel::DashelException e) {
				if(interface->isVerbose()) {
					cerr << "Target " << address << " failed to send get variables message for node " << node.globalId << " (" << node.name << ") to stream: " << e.what() << endl;
				}
				return false;
			}

			positions.push_back(position);
		} else {
			return false;
		}
	}

	if(!positions.empty()) {
		DashelHttpRequest *dashelRequest = static_cast<DashelHttpRequest *>(request);
		node.pendingVariables[positions[0]].insert(make_pair(dashelRequest->getStream(), dashelRequest));

		if(interface->isVerbose()) {
			cerr << "Target " << address << " scheduled variables request for node " << node.globalId << " (" << node.name << "): " << join(args, ", ") << endl;
		}
	}

	return true;
}

bool HttpDashelTarget::sendSetVariable(unsigned globalNodeId, const std::vector<std::string>& args)
{
	map<unsigned, Node>::iterator query = nodes.find(globalNodeId);
	if(query == nodes.end()) {
		if(interface->isVerbose()) {
			cerr << "Target " << address << " failed to send set variables message for node " << globalNodeId << ": No such node" << endl;
		}
		return false;
	}

	Node& node = query->second;
	assert(globalNodeId == node.globalId);

	if(args.empty()) {
		return false;
	}

	unsigned position;
	unsigned size;

	if(!getVariableInfo(node, args[0], position, size)) {
		return false;
	}

	try {
		// send the message
		SetVariables::VariablesVector data;
		for(size_t i = 1; i < args.size(); ++i) {
			data.push_back(atoi(args[i].c_str()));
		}

		SetVariables setVariables(node.localId, position, data);
		setVariables.serialize(stream);
		stream->flush();
	} catch(Dashel::DashelException e) {
		if(interface->isVerbose()) {
			cerr << "Target " << address << " failed to send set variables message for node " << node.globalId << " (" << node.name << ") to stream: " << e.what() << endl;
		}
		return false;
	}


	return true;
}

bool HttpDashelTarget::compileAndRunCode(unsigned globalNodeId, const std::string& code, std::string& errorString)
{
	map<unsigned, Node>::iterator query = nodes.find(globalNodeId);
	if(query == nodes.end()) {
		if(interface->isVerbose()) {
			cerr << "Target " << address << " failed to compile and run code for node " << globalNodeId << ": No such node" << endl;
		}
		return false;
	}

	Node& node = query->second;
	assert(globalNodeId == node.globalId);

	// compile code
	std::wistringstream is(UTF8ToWString(code));
	Error error;
	BytecodeVector bytecode;
	unsigned allocatedVariablesCount;

	Compiler compiler;
	compiler.setTargetDescription(getDescription(node.localId));
	compiler.setCommonDefinitions(&interface->getProgram().getCommonDefinitions());

	if(compiler.compile(is, bytecode, allocatedVariablesCount, error)) {
		try {
			// send bytecode
			sendBytecode(stream, node.localId, std::vector<uint16>(bytecode.begin(), bytecode.end()));

			// run node
			Run msg(node.localId);
			msg.serialize(stream);
			stream->flush();
		} catch(Dashel::DashelException e) {
			errorString = e.what();
			if(interface->isVerbose()) {
				cerr << "Target " << address << " failed to send bytecode and run message for node " << node.globalId << " (" << node.name << ") to stream: " << e.what() << endl;
			}
			return false;
		}


		// retrieve user-defined variables for use in get/set
		node.variablesMap = *compiler.getVariablesMap();
		return true;
	} else {
		errorString = WStringToUTF8(error.toWString());
		if(interface->isVerbose()) {
			cerr << "Target " << address << " failed to compile program for node " << globalNodeId << " (" << node.name << "): " << errorString << endl;
		}

		return false;
	}
}

bool HttpDashelTarget::removePendingVariable(unsigned globalNodeId, unsigned start)
{
	map<unsigned, Node>::iterator query = nodes.find(globalNodeId);
	if(query == nodes.end()) {
		if(interface->isVerbose()) {
			cerr << "Target " << address << " failed to remove pending variable for node " << globalNodeId << ": No such node" << endl;
		}
		return false;
	}

	Node& node = query->second;
	assert(globalNodeId == node.globalId);

	return node.pendingVariables.erase(start) == 1;
}

std::set<const HttpDashelTarget::Node *> HttpDashelTarget::getNodesByName(const std::string& name) const
{
	set<const HttpDashelTarget::Node *> result;

	// todo: maybe make this more efficient by caching node access by name?
	map<unsigned, Node>::const_iterator end = nodes.end();
	for(map<unsigned, Node>::const_iterator iter = nodes.begin(); iter != end; ++iter) {
		const Node& node = iter->second;

		if(node.name == name) {
			result.insert(&node);
		}
	}

	return result;
}

const HttpDashelTarget::Node *HttpDashelTarget::getNodeById(unsigned globalNodeId) const
{
	map<unsigned, Node>::const_iterator query = nodes.find(globalNodeId);
	if(query != nodes.end()) {
		return &query->second;
	} else {
		return NULL;
	}
}

const HttpDashelTarget::Node *HttpDashelTarget::getNodeByLocalId(unsigned localNodeId) const
{
	map<unsigned, unsigned>::const_iterator query = globalIds.find(localNodeId);
	if(query != globalIds.end()) {
		return getNodeById(query->second);
	} else {
		return NULL;
	}
}

bool HttpDashelTarget::getVariableInfo(const Node& node, const std::string& variableName, unsigned& position, unsigned& size)
{
	VariablesMap::const_iterator query = node.variablesMap.find(UTF8ToWString(variableName));
	if(query != node.variablesMap.end()) {
		position = query->second.first;
		size = query->second.second;
		return true;
	}

	// if variable is not user-defined, check whether it is provided by this node
	bool ok;
	position = getVariablePos(node.localId, UTF8ToWString(variableName), &ok);
	if(ok) {
		size = getVariableSize(node.localId, UTF8ToWString(variableName), &ok);

		if(ok) {
			return true;
		}
	}

	if(interface->isVerbose()) {
		cerr << "Target " << address << " failed to find variable '" << variableName << "' address for node " << node.globalId << " (" << node.name << ")" << endl;
	}
	return false;
}

void HttpDashelTarget::sendMessage(const Message& message)
{
	message.serialize(stream);
	stream->flush();
}

void HttpDashelTarget::nodeDescriptionReceived(unsigned localNodeId)
{
	std::map<unsigned, unsigned>::iterator query = globalIds.find(localNodeId);
	if(query == globalIds.end()) {
		Node node;
		node.localId = localNodeId;
		node.name = WStringToUTF8(getNodeName(localNodeId));
		node.globalId = interface->registerNode(this, localNodeId);
		nodes[node.globalId] = node;
		globalIds[node.localId] = node.globalId;

		if(interface->isVerbose()) {
			cerr << "Target " << address << " received description for node " << node.globalId << " (" << node.name << ")" << endl;
		}

		std::vector<sint16> data;
		data.push_back(node.globalId);

		interface->notifyEventSubscribers("connect", data);
	} else if(interface->isVerbose()) {
		cerr << "Target " << address << " received description for local node id " << localNodeId << ", but that node was already registered to node " << query->second << endl;
	}
}
