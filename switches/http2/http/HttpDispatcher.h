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

#ifndef ASEBA_SWITCH_DISPATCHER_HTTP
#define ASEBA_SWITCH_DISPATCHER_HTTP

#include <functional>
#include <unordered_map>
#include "../Module.h"
#include "../Switch.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace Aseba
{
	//! The HTTP dispatcher provides an HTTP REST interface to the Aseba network
	class HttpDispatcher: public Module
	{
	protected:
		// types for calling handlers
		
		//! A Handler takes a pointer to the switch, the stream, the request, the filled path templates map in the URI.
		typedef std::map<std::string, std::string> PathTemplateMap;
		
		//! All information a handler needs to have to process a request
		struct HandlerContext
		{
			Switch * const asebaSwitch;
			Dashel::Stream * const stream;
			const HttpRequest& request;
			const PathTemplateMap &filledPathTemplates;
			const json parsedContent;
		};
		//! A callback to an handler for HTTP requests
		typedef std::function<void(HandlerContext&)> Handler;
		
		//! A (node id, node pointer) pair
		typedef std::pair<unsigned, Switch::NodeWithProgram*> NodeEntry;
		
		// types for selecting which handler to call
		
		//! A pair of Handler function and its documentation
		typedef std::pair<Handler, json> HandlerDescription;
		//! Map of splitted URI (including path templates) to handlers
		typedef std::map<strings, HandlerDescription> URIHandlerMap;
		//! Map of method to map of splitted URI to handlers
		typedef std::map<HttpMethod, URIHandlerMap> HandlersMap;
	
		// types for reading variables' values
		
		//! The result of a search for a variable
		struct VariableSearchResult
		{
			bool found; //! whether the search was successful (node and variable's name found)
			unsigned nodeId; //! node identifier holding the variable
			unsigned pos; //! address of the variable inside the node
			unsigned size; //! size of the variable
		};
		//! The key indexing a pending variable read request
		struct VariableRequestKey
		{
			unsigned nodeId; //!< the node containing the variable being read
			unsigned pos; //!< the address within the node of the variable being read
			//! Comparison operator, equal if both nodeId and pos are equal
			bool operator== (const VariableRequestKey& that) const { return nodeId == that.nodeId && pos == that.pos; }
		};
		//! Hash function for VariableRequestKey
		struct VariableRequestKeyHash
		{
			size_t operator()(const VariableRequestKey& key) const;
		};
		//! Pending variable read operations for this dispatcher
		//! We use a multimap as the same variable might have been requested more than once by several clients, and therefore
		//! should be returned to each of them.
		//! Note that unordered_multimap does not specify a policy for equal keys, so under heavy load and
		//! a LIFO policy, the earliest clients might starve.
		//! If this prove a problem in practice, we'll swith to a unordered_map pointing to a dequeue.
		typedef std::unordered_multimap<VariableRequestKey, Dashel::Stream*, VariableRequestKeyHash> PendingReads;
		
		// types for SSE
		
		//! Multimap of a named Aseba event to Dashel streams, if name is empty it means all events
		typedef std::multimap<std::string, Dashel::Stream*> EventStreams;
		
	public:
		HttpDispatcher();
		
		virtual std::string name() const;
		virtual void dumpArgumentsDescription(std::ostream &stream) const;
		virtual ArgumentDescriptions describeArguments() const;
		virtual void processArguments(Switch* asebaSwitch, const Arguments& arguments);
		
		virtual bool connectionCreated(Switch* asebaSwitch, Dashel::Stream * stream);
		virtual void incomingData(Switch* asebaSwitch, Dashel::Stream * stream);
		virtual void connectionClosed(Switch* asebaSwitch, Dashel::Stream * stream);
		virtual void processMessage(Switch* asebaSwitch, const Message& message);
		
	protected:
		void registerHandler(const Handler& handler, const HttpMethod& method, const strings& uriPath);
		void registerHandler(const Handler& handler, const HttpMethod& method, const strings& uriPath, const json& apidoc);
		void resolveReferences(json& object) const;
		NodeEntry findNode(HandlerContext& context) const;
		
		// options and test
		void optionsHandler(HandlerContext& context);
		void getTestHandler(HandlerContext& context);
		void getApiDocs(HandlerContext& context);
		virtual json buildApiDocs();
		
		// TODO: add handler support class
		
		// constants, in ConstantsHandlers.cpp
		// handlers
		void registerConstantsHandlers();
		void postConstantHandler(HandlerContext& context);
		void getConstantsHandler(HandlerContext& context);
		void deleteConstantsHandler(HandlerContext& context);
		//void putConstantsHandler(HandlerContext& context)
		void getConstantHandler(HandlerContext& context);
		void deleteConstantHandler(HandlerContext& context);
		void putConstantHandler(HandlerContext& context);
		// support
		void updateConstantValue(HandlerContext& context, const std::string& name, size_t position, bool created);
		bool findConstant(HandlerContext& context, std::string& name, size_t& position);
		
		// events, in EventsHandlers.cpp
		// handlers
		void registerEventsHandlers();
		void postEventHandler(HandlerContext& context);
		void getEventsHandler(HandlerContext& context);
		void deleteEventsHandler(HandlerContext& context);
		void getEventHandler(HandlerContext& context);
		void deleteEventHandler(HandlerContext& context);
		void putEventHandler(HandlerContext& context);
		// support
		void updateEventValue(HandlerContext& context, const std::string& name, size_t position, bool created);
		bool findEvent(HandlerContext& context, std::string& name, size_t& position);
		
		// nodes, in NodesHandlers.cpp
		void registerNodesHandlers();
		// handlers
		void getNodesHandler(HandlerContext& context);
		void getNodesNodeHandler(HandlerContext& context);
		void getNodesNodeDescriptionHandler(HandlerContext& context);
		void putNodesNodeProgramHandler(HandlerContext& context);
		void getNodesNodeProgramHandler(HandlerContext& context);
		void putNodesNodeVariablesVariableHandler(HandlerContext& context);
		void getNodesNodeVariablesVariableHandler(HandlerContext& context);
		// support
		json jsonNodeDescription(const NodeEntry& node) const;
		json jsonNodeProgram(const NodeEntry& node) const;
		json jsonNodeCompilationResult(const NodeEntry& node) const;
		json jsonNodeSymbolEventsVector(const NodeEntry& node) const;
		json jsonNodeNativeEvents(const NodeEntry& node) const;
		json jsonNodeNativeFunctions(const NodeEntry& node) const;
		json jsonNodeNativeVariables(const NodeEntry& node) const;
		json jsonNodeSymbolTable(const NodeEntry& node) const;
		json jsonNodeSymbolVariables(const NodeEntry& node) const;
		json jsonNodeSymbolSubroutines(const NodeEntry& node) const;
		VariableSearchResult findVariable(HandlerContext& context) const;
		
		// debugs, in DebugsHandlers.cpp
		void registerDebugsHandlers();
		// handlers
		void getDebugsHandler(HandlerContext& context);
		void getDebugsNodeHandler(HandlerContext& context);
		void postDebugsNodeHandler(HandlerContext& context);
		// support
		json jsonDebugsNodeHandler(const NodeEntry& node) const;
		
		// streams
		void getStreamsEventsHandler(HandlerContext& context);
		
	protected:
		unsigned serverPort; //!< port this server is listening on
		const json definitions; //!< the JSON models used in the API
		HandlersMap handlers; //!< handlers for known method + URI couples
		PendingReads pendingReads; //!< pending variable read requests
		EventStreams eventStreams; //!< ongoing SSE for Aseba events
	};
	
} // namespace Aseba

#endif // ASEBA_SWITCH_DISPATCHER_HTTP
