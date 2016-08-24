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
#include "Globals.h"

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
	
	Switch::Switch():
		printMessageContent(false),
		runDuration(-1)
	{
	}
	
	//! Dump a short description of this module and the list of arguments it eats
	void Switch::dumpArgumentsDescription(ostream &stream) const
	{
		stream << "  Core features of the switch\n";
		stream << "    -v, --verbose   : makes the switch verbose (default: silent)\n";
		stream << "    -d, --dump      : makes the switch dump the content of messages, implies verbose (default: do not dump)\n";
		stream << "    -p, --port port : listens to incoming Aseba connection on this port (default: 33333)\n";
		stream << "    --rawtime       : shows time in the form of sec:usec since 1970 (default: user readable)\n";
		stream << "    --duration sec  : run the switch only for a given duration in s (default: run forever)\n";
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
	void Switch::processArguments(const Arguments& arguments)
	{
		// global arguments
		_globals.verbose = arguments.find("-v") || arguments.find("--verbose");
		_globals.rawTime = arguments.find("--rawtime");
		
		// switch-specific arguments
		printMessageContent = arguments.find("-d") || arguments.find("--dump");
		_globals.verbose = _globals.verbose || printMessageContent;
		strings args;
		if (arguments.find("--duration", &args))
			runDuration = stoi(args[0]);
		
		// open tcpin port for Aseba Dashel streams
		unsigned port(33333);
		if (arguments.find("-p", &args) || arguments.find("--port", &args))
			port = stoi(args[0]);
		ostringstream oss;
		oss << "tcpin:port=" << port;
		Stream* serverStream(connect(oss.str()));
		LOG_VERBOSE << "Core | Aseba listening stream on " << serverStream->getTargetName() << endl;
	}
	
	//! Run the Switch until a termination signal was caught or a specific given duration has elapsed
	void Switch::run()
	{
		int elapsedTime(0);
		while (run1s())
		{
			// count time and stop if maximum is reached
			++elapsedTime;
			if (runDuration >= 0 && elapsedTime > runDuration)
				break;
		}
	}
	
	//! Run the Dashel::Hub for 1 second, return true if execution should continue, false if a termination signal was caught
	bool Switch::run1s()
	{
		// get initial time
		UnifiedTime startTime;
		
		// attempt to reconnect disconnected targets
		reconnectDisconnectedTargets();
		
		// see if new nodes have appeared or reconnected
		pingNetwork();
		
		// run switch for one second
		const int duration(1000);
		int timeout(duration);
		while (timeout > 0)
		{
			if (!step(timeout))
				return false;
			timeout = duration - (UnifiedTime() - startTime).value;
		}
		return true;
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
	
	void Switch::connectionCreated(Stream *stream)
	{
		LOG_VERBOSE << "Core | Incoming connection from " << stream->getTargetName() << endl;
	}
	
	void Switch::incomingData(Stream * stream)
	{
		// if specific to a module, delegate management of the stream to the module
		auto moduleIt(moduleSpecificStreams.find(stream));
		if (moduleIt != moduleSpecificStreams.end())
		{
			moduleIt->second->incomingData(stream);
			return;
		}
		
		// it is an Aseba stream, receive message
		const unique_ptr<Message> message(Message::receive(stream));
		
		// TODO: block list nodes from clients
		
		// remap identifier if message not from IDE, update tables if previously unseen id
		if (message->source != 0)
		{
			auto remapTablesIt(idRemapTables.find(stream->getTargetName()));
			if (remapTablesIt == idRemapTables.end())
				remapTablesIt = newRemapTable(stream);
			IdRemapTable& remapTable(remapTablesIt->second);
			auto remapIt(remapTable.find(message->source));
			if (remapIt == remapTable.end())
				remapIt = newRemapEntry(remapTable, stream, message->source);
			message->source = remapIt->second;
		}
		
		// dump message if enabled
		if (printMessageContent)
		{
			dumpTime(cout, _globals.rawTime);
			cout << "Core | Message from " << stream->getTargetName() << " : ";
			message->dump(wcout);
			wcout << endl;
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
		// is this stream associated with a module?
		auto streamModuleMapIt(moduleSpecificStreams.find(stream));
		if (streamModuleMapIt != moduleSpecificStreams.end())
		{
			// yes, de-associate
			moduleSpecificStreams.erase(streamModuleMapIt);
		}
		// no, is it a data stream (and therefore an Aseba stream)?
		else if (dataStreams.find(stream) != dataStreams.end())
		{
			// yes, add to reconnection list
			toReconnectTargets.insert(stream->getTargetName());
			// clear the global id to stream entries
			for (auto& globalIdStreamKV: globalIdStreamMap)
			{
				if (globalIdStreamKV.second == stream)
					globalIdStreamKV.second = nullptr;
			}
		}
		// no, it is a tcpin stream, do nothing
		else
			return;
		
		if (abnormal)
			LOG_VERBOSE << "Core | Abnormal connection closed to " << stream->getTargetName() << " : " << stream->getFailReason() << endl;
		else
			LOG_VERBOSE << "Core | Normal connection closed to " << stream->getTargetName() << endl;
	}
		
	
	Switch::Node* Switch::createNode(const TargetDescription& targetDescription)
	{
		return new NodeWithProgram(targetDescription);
	}
	
	void Switch::sendMessage(const Message& message)
	{
		// dump message if enabled
		if (printMessageContent)
		{
			dumpTime(cout, _globals.rawTime);
			cout << "Core | Message from self : ";
			message.dump(wcout);
			wcout << endl;
		}
		
		broadcastMessage(&message, nullptr);
	}
	
	//! Broadcast a message without rewrite, assuming this message is not a subclass of CmdMessage
	void Switch::broadcastMessage(const Message* message, const Stream* exceptedThis)
	{
		assert(!dynamic_cast<const CmdMessage *>(message) || (dynamic_cast<const CmdMessage *>(message)->source == 0));
		
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
				LOG_ERROR << "Core | Error while rebroadcasting to stream " << stream << " of target " << stream->getTargetName() << endl;
			}
		}
	}
	
	void Switch::sendMessageWithRemap(Message* message, const Stream* exceptedThis)
	{
		CmdMessage *cmdMessage(dynamic_cast<CmdMessage *>(message));
		if (cmdMessage)
		{
			// find the attached stream
			auto streamIt(globalIdStreamMap.find(cmdMessage->dest));
			if (streamIt == globalIdStreamMap.end())
				return; // no stream attached, thus ignore
			// command message, send only to the stream who has this node, and remap node
			for (const auto& idRemapTablesKV: idRemapTables)
			{
				for (const auto& localToGlobalIdKV: idRemapTablesKV.second)
				{
					if (localToGlobalIdKV.second == cmdMessage->dest)
					{
						cmdMessage->dest = localToGlobalIdKV.first;
						
						Stream* stream(streamIt->second);
						if (!stream)
							return; // stream is nullptr, meaning the target is currently disconnected, thus ignore
						try
						{
							message->serialize(stream);
							stream->flush();
						}
						catch (const Dashel::DashelException& e)
						{
							LOG_ERROR << "Core | Error while rebroadcasting to stream " << stream << " of target " << stream->getTargetName() << endl;
						}
						return; // global identifiers are unique and each node are connected on a single stream, so we can stop here
					}
				}
			}
			// the global id was found in the stream map, but not in the remap tables, this is an error
			assert(false);
		}
		else
			broadcastMessage(message, exceptedThis);
	}
	
	//! Create a new Id remap table for this stream
	Switch::IdRemapTables::iterator Switch::newRemapTable(Stream* stream)
	{
		return idRemapTables.emplace(stream->getTargetName(), IdRemapTable()).first;
	}
	
	//! Create a new entry in the remapTable for stream for new node
	Switch::IdRemapTable::iterator Switch::newRemapEntry(IdRemapTable& remapTable, Stream* stream, unsigned localId)
	{
		unsigned globalId(localId);
		// get next free global id
		while (globalIdStreamMap.find(globalId) != globalIdStreamMap.end())
			++globalId;
		
		// log if effective remapping
		if (localId != globalId)
		{
			LOG_VERBOSE << "Core | Remapping local Id " << localId << " to global Id " << globalId << " for stream " << stream->getTargetName() << endl;
		}
		
		// fill remap tables
		globalIdStreamMap.emplace(globalId, stream);
		return remapTable.emplace(localId, globalId).first;
	}
	
	//! Attempt to reconnect disconnected targets
	void Switch::reconnectDisconnectedTargets()
	{
		auto targetIt(toReconnectTargets.begin());
		while (targetIt != toReconnectTargets.end())
		{
			const string& target(*targetIt);
			try
			{
				// attempt reconnection
				Stream* stream(connect(target));
				
				// update global id to stream table
				const auto idRemapTableIt(idRemapTables.find(target));
				assert(idRemapTableIt != idRemapTables.end());
				const IdRemapTable& idRemapTable(idRemapTableIt->second);
				for (const auto& localToGlobalIdKV: idRemapTable)
				{
					const unsigned globalId(localToGlobalIdKV.second);
					globalIdStreamMap[globalId] = stream;
				}
				
				LOG_VERBOSE << "Core | Reconnected to target " << target << endl;
				
				targetIt = toReconnectTargets.erase(targetIt);
			}
			catch(const Dashel::DashelException& e)
			{
				LOG_VERBOSE << "Core | Failed to reconnect to target " << target << endl;
				
				++targetIt;
			}
		}
	}

	//! Return whether the given stream is an Aseba stream handled by this Switch
// 	bool Switch::isAsebaStream(Dashel::Stream* stream) const
// 	{
// 		if (moduleSpecificStreams.find(stream) != moduleSpecificStreams.end())
// 			return false;
// 		if (dataStreams.find(stream) == dataStreams.end())
// 			return false;
// 		return true;
// 	}
	
} // namespace Aseba
