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

#include "Switch.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	// Switch::NodeWithProgram
	
	Switch::NodeWithProgram::NodeWithProgram(const TargetDescription& targetDescription):
		Node(targetDescription)
	{
	}
	
	void Switch::NodeWithProgram::compile(const CommonDefinitions& commonDefinitions)
	{
		Compiler compiler;
		compiler.setTargetDescription(this);
		compiler.setCommonDefinitions(&commonDefinitions);
		
		wistringstream is(code);
		
		success = compiler.compile(is, bytecode, allocatedVariablesCount, error);
		
		if (success)
		{
			variables = compiler.getVariablesMap();
			subroutinesNames = compiler.getSubroutinesNames();
		}
	}
	
	
	// Switch
	
	//! Dump a short description of this module and the list of arguments it eats
	void Switch::dumpArgumentsDescription(ostream &stream) const
	{
		stream << "  Core features of the switch\n";
		stream << "    -v, --verbose   : makes the switch verbose (default: silent)\n";
		stream << "    -d, --dump      : makes the switch dump the content of messages (default: do not dump)\n";
		stream << "    -p, --port port : listens to incoming Aseba connection on this port (default: 33333)\n";
		stream << "    --rawtime       : shows time in the form of sec:usec since 1970 (default: user readable)\n";
		stream << "    --duration sec  : run the switch only for a given duration (default: run forever)\n";
	}
	
	//! Give the list of arguments this module can understand
	ArgumentDescriptions Switch::describeArguments() const
	{
		return {
			{ "-v", 0 },
			{ "--verbose", 0 },
			{ "-d", 0 },
			{ "--dump", 0 },
			{ "-p", 1 },
			{ "--port", 1 },
			{ "--rawtime", 0 },
			{ "--duration", 1 }
		};
	}
	
	//! Pass all parsed arguments to this module
	void processArguments(const Arguments& arguments)
	{
		// TODO
	}
	
	// TODO: document
	void Switch::handleAutomaticReconnection(Stream* stream)
	{
		automaticReconnectionStreams.insert(stream);
	}
	
	//! Register a module to be notified of incoming messages
	void Switch::registerModuleNotification(Module* module)
	{
		notifiedModules.push_back(module);
	}
	
	//! Delegate the handling of a stream, including receiving data, to a module
	void Switch::delegateHandlingToModule(Stream* stream, Module* owner)
	{
		moduleSpecificStreams[stream] = owner;
	}
	
	void Switch::incomingData(Stream * stream)
	{
		// if it is our listen stream, answer the connection
		// TODO
		
		// if specific to a module, delegate management of the stream to the module
		auto moduleIt(moduleSpecificStreams.find(stream));
		if (moduleIt != moduleSpecificStreams.end())
		{
			moduleIt->second->incomingData(stream);
			return;
		}
		
		// it is an Aseba stream, receive message
		const unique_ptr<Message> message(Message::receive(stream));
		
		// remap identifier if message not from IDE, update tables if previously unseen id
		if (message->source != 0)
		{
			auto remapTablesIt(idRemapTables.find(stream));
			if (remapTablesIt == idRemapTables.end())
				remapTablesIt = newRemapTable(stream);
			IdRemapTable& remapTable(remapTablesIt->second);
			auto remapIt(remapTable.find(message->source));
			if (remapIt == remapTable.end())
				remapIt = newRemapEntry(remapTable, message->source);
			message->source = remapIt->second;
		}
		
		// pass message to nodes manager, which builds the node descriptions in background
		// createNode() might be called if it is a previously unseen node node
		processMessage(message.get());
		
		// allow every module a chance to process the message
		for (auto& module: notifiedModules)
			module->processMessage(*message);
		
		// resend on the network
		sendMessageWithRemap(message.get(), stream);
	}
	
	void Switch::connectionClosed(Stream * stream, bool abnormal)
	{
		// TODO: implement
	}
		
	
	Switch::Node* Switch::createNode(const TargetDescription& targetDescription)
	{
		return new NodeWithProgram(targetDescription);
	}
	
	void Switch::sendMessage(const Message& message)
	{
		broadcastMessage(&message, nullptr);
	}
	
	//! Broadcast a message without rewrite, assuming this message is not a subclass of CmdMessage
	void Switch::broadcastMessage(const Message* message, const Stream* exceptedThis)
	{
		assert(!dynamic_cast<const CmdMessage *>(message));
		
		// broadcast on all the network
		for (auto stream: dataStreams)
		{
			// skip streams that do not correspond to Aseba targets
			if (moduleSpecificStreams.find(stream) != moduleSpecificStreams.end())
				continue;
			// skip stream that was the source
			if (stream == exceptedThis)
				continue;
			try
			{
				message->serialize(stream);
				stream->flush();
			}
			catch (const Dashel::DashelException& e)
			{
				cerr << "Error while rebroadcasting to stream " << stream << " of target " << stream->getTargetName() << endl;
			}
		}
	}
	
	void Switch::sendMessageWithRemap(Message* message, const Stream* exceptedThis)
	{
		CmdMessage *cmdMessage(dynamic_cast<CmdMessage *>(message));
		if (cmdMessage)
		{
			// command message, send only to the stream who has this node, and remap node
			for (const auto& idRemapTablesKV: idRemapTables)
			{
				for (const auto& localToGlobalIdKV: idRemapTablesKV.second)
				{
					if (localToGlobalIdKV.second == cmdMessage->dest)
					{
						cmdMessage->dest = localToGlobalIdKV.first;
						Stream* stream(idRemapTablesKV.first);
						try
						{
							message->serialize(stream);
							stream->flush();
						}
						catch (const Dashel::DashelException& e)
						{
							cerr << "Error while rebroadcasting to stream " << stream << " of target " << stream->getTargetName() << endl;
						}
						return; // global identifiers are unique and each node are connected on a single stream, so we can stop here
					}
						
				}
			}
		}
		else
			broadcastMessage(message, exceptedThis);
	}
	
	Switch::IdRemapTables::iterator Switch::newRemapTable(Dashel::Stream* stream)
	{
		return idRemapTables.emplace(stream, IdRemapTable()).first;
	}
	
	Switch::IdRemapTable::iterator Switch::newRemapEntry(IdRemapTable& remapTable, unsigned localId)
	{
		unsigned globalId(localId);
		// get next free global id
		while (globalIds.find(globalId) != globalIds.end())
			++globalId;
		globalIds.insert(globalId);
		return remapTable.emplace(localId, globalId).first;
	}
	
} // namespace Aseba
