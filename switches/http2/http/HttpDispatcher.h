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
#include "../Module.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Json.h"

namespace Aseba
{
	class HttpDispatcher: public Module
	{
	protected:
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
		
		// options and test
		void optionsHandler(HandlerContext& context);
		void getTestHandler(HandlerContext& context);
		
		// TODO: add handler support class
		
		// constants, in ConstantsHandlers.cpp
		// handlers
		void getConstantsHandler(HandlerContext& context);
		void putConstantsHandler(HandlerContext& context);
		void postConstantsHandler(HandlerContext& context);
		void deleteConstantsHandler(HandlerContext& context);
		void getConstantHandler(HandlerContext& context);
		void putConstantHandler(HandlerContext& context);
		void deleteConstantHandler(HandlerContext& context);
		// support
		void updateConstantValue(HandlerContext& context, const std::string& name, size_t position);
		bool findConstant(HandlerContext& context, std::string& name, size_t& position);
		bool validateInt16Value(HandlerContext& context, const std::string& name, int value);
		
		// events
		void getEventsHandler(HandlerContext& context);
		
	protected:
		unsigned serverPort; //!< port this server is listening on
		
		//! Multimap of a named Aseba event to Dashel streams, if name is empty it means all events
		typedef std::multimap<std::string, Dashel::Stream*> EventStreams;
		//! Ongoing SSE for Aseba events
		EventStreams eventStreams;
		//! Map of splitted URI (including path templates) to handlers
		typedef std::map<strings, Handler> URIHandlerMap;
		//! Map of method to map of splitted URI to handlers
		typedef std::map<HttpMethod, URIHandlerMap> HandlersMap;
		//! Handlers for known method + URI couples
		HandlersMap handlers;
	};
}

#endif // ASEBA_SWITCH_DISPATCHER_HTTP
