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

#include <ostream>
#include <functional>
#include "HttpDispatcher.h"
#include "HttpStatus.h"
#include "../Globals.h"
#include "../Switch.h"

namespace Aseba
{
	using namespace std;
	using namespace std::placeholders;
	using namespace Dashel;
	
	#define REGISTER_HANDLER(handler, method, ...) \
		registerHandler(bind(&HttpDispatcher::handler, this, _1), HttpMethod::method,  __VA_ARGS__);
		
	#define RQ_MAP_GET_FIELD(itName, mapName, fieldName) \
		const auto itName(mapName.find(#fieldName)); \
		if (itName == mapName.end()) \
		{ \
			HttpResponse::fromPlainString("Cannot find field " #fieldName " in request content", HttpStatus::BAD_REQUEST).send(stream); \
			return; \
		}
	
	HttpDispatcher::HttpDispatcher():
		serverPort(3000)
	{
		// register all handlers
		REGISTER_HANDLER(optionsHandler, OPTIONS, {});
		REGISTER_HANDLER(getTestHandler, GET, { "test" });
		REGISTER_HANDLER(getApiDocs, GET, { "apidocs" });
		REGISTER_HANDLER(getConstantsHandler, GET, { "constants" });
		REGISTER_HANDLER(putConstantsHandler, PUT, { "constants" });
		REGISTER_HANDLER(postConstantsHandler, POST, { "constants" },
						 R"({"tags":["Definitions – Constants"],"summary":"Create Constant Definition","description":"","operationId":"POST-constant-definition","responses":{"201":{"description":"","schema":{"$ref":"#/definitions/constant-definition-full"}},"400":{"description":"","schema":{"type":"string"},"examples":{"application/json":"Invalid value"}},"403":{"description":"","schema":{"type":"string"},"examples":{"application/json":"Invalid constant name"}},"409":{"description":"","schema":{"type":"string"},"examples":{"application/json":"Constant already exists"}}},"consumes":["application/json"],"produces":["application/json"],"parameters":[{"name":"body","in":"body","schema":{"$ref":"#/definitions/constant-definition-partial"}}]})"_json);
		REGISTER_HANDLER(deleteConstantsHandler, DELETE, { "constants" });
		REGISTER_HANDLER(getConstantHandler, GET, { "constants", "{name}" });
		REGISTER_HANDLER(putConstantHandler, PUT, { "constants", "{name}" });
		REGISTER_HANDLER(deleteConstantHandler, DELETE, { "constants", "{name}" });
		REGISTER_HANDLER(getEventsHandler, GET, { "events" });
		REGISTER_HANDLER(getEventsHandler, GET, { "events", "{name}" });
	}
	
	string HttpDispatcher::name() const
	{
		return "HTTP Dispatcher";
	}
	
	void HttpDispatcher::dumpArgumentsDescription(ostream &stream) const
	{
		stream << "  HTTP module, provides access to the network from a web application\n";
		stream << "    -w, --http      : listens to incoming HTTP connections on this port (default: 3000)\n";
	}
	
	ArgumentDescriptions HttpDispatcher::describeArguments() const
	{
		return {
			{ "-w", 1 },
			{ "--http", 1 }
		};
	}
	
	void HttpDispatcher::processArguments(Switch* asebaSwitch, const Arguments& arguments)
	{
		strings args;
		if (arguments.find("-w", &args) || arguments.find("--http", &args))
			serverPort = stoi(args[0]);
		// TODO: add an option to avoid creating an HTTP port, or maybe not creating this module?
		ostringstream oss;
		oss << "tcpin:port=" << serverPort;
		try
		{
			Stream* serverStream(asebaSwitch->connect(oss.str()));
			LOG_VERBOSE << "HTTP | HTTP Listening stream on " << serverStream->getTargetName() << endl;
		}
		catch (const Dashel::DashelException& e)
		{
			LOG_ERROR << "HTTP | Error trying to open server stream " << oss.str() << ": " << e.what() << endl;
		}
	}
	
	//! Take ownership of the stream if it was created by our server stream
	bool HttpDispatcher::connectionCreated(Switch* asebaSwitch, Dashel::Stream * stream)
	{
		if (stream->getTarget().isSet("connectionPort") &&
			stoi(stream->getTargetParameter("connectionPort")) == serverPort)
			return true;
		return false;
	}
	
	//! Receive request and dispatch to the relevant handler, which will have to answer by itself or add the stream to an SSE set
	void HttpDispatcher::incomingData(Switch* asebaSwitch, Dashel::Stream * stream)
	{
		try // for DashelException
		{
			try // for HttpRequest::Error
			{
				// receive request
				HttpRequest request(HttpRequest::receive(stream));
				
				// check handler map for given method
				const auto uriHandlerMapIt(handlers.find(request.method));
				if (uriHandlerMapIt != handlers.end())
				{
					const auto& uriHandlerMap(uriHandlerMapIt->second);
					// look for a handler to dispatch
					for (const auto& uriHandlerKV: uriHandlerMap)
					{
						const strings& uri(uriHandlerKV.first);
						// length must match
						if (uri.size() != request.tokenizedUri.size())
							continue;
						PathTemplateMap filledPathTemplates;
						size_t i = 0;
						while (i < uri.size())
						{
							const string& folder(uri[i]);
							const string& requestedFolder(request.tokenizedUri[i]);
							const size_t l(folder.length());
							if (l > 2 && folder[0] == '{' && folder[l-1] == '}')
							{
								// template to fill
								filledPathTemplates[folder.substr(1, l-2)] = requestedFolder;
							}
							else if (folder != requestedFolder)
							{
								// mismatch keyword
								break;
							}
							++i;
						}
						if (i == uri.size())
						{
							// the match is complete, dispatch
							try
							{
								// create context and deserialize JSON content, if any
								HandlerContext context({
									asebaSwitch,
									stream,
									request,
									filledPathTemplates,
									request.content.size() > 0 ? parse(request.content) : json()
								});
								//LOG_VERBOSE << "HTTP | On stream " << stream->getTargetName() << ", dispatching " << toString(request.method) << " " << request.uri << " to handler" << endl;
								// dispatch
								uriHandlerKV.second(context);
							}
							catch (const invalid_argument& e)
							{
								HttpResponse::fromPlainString(string("Malformed JSON in query: ") + e.what(), HttpStatus::BAD_REQUEST).send(stream);
							}
							return;
						}
					}
				}
				
				// no matching method or URI found, not found
				HttpResponse::fromPlainString(FormatableString("No handler found for %0 %1").arg(toString(request.method)).arg(request.uri), HttpStatus::NOT_FOUND).send(stream);
				
				// TODO; add timeout for closing unused connections for a while
				// FIXME: what about protocols incompatibilities?
			}
			catch (const HttpRequest::Error& e)
			{
				// send an error
				HttpResponse::fromPlainString(FormatableString("Error %0 while reading request").arg(HttpStatus::toString(e.errorCode)), e.errorCode).send(stream);
				// fail stream
				stream->fail(DashelException::Unknown, 0, FormatableString("HTTP request error: %0").arg(e.what()).c_str());
			}
		}
		catch (const Dashel::DashelException& e)
		{
			LOG_ERROR << "HTTP | Error while processing stream " << stream << " of target " << stream->getTargetName() << ": " << e.what() << endl;
		}
	}
	
	void HttpDispatcher::connectionClosed(Switch* asebaSwitch, Dashel::Stream * stream)
	{
		// Remove the SSE associated to this stream
		auto eventStreamIt(eventStreams.begin()); 
		while (eventStreamIt != eventStreams.end())
		{
			if (eventStreamIt->second == stream)
				eventStreamIt = eventStreams.erase(eventStreamIt);
			else
				++eventStreamIt;
		}
	}
	
	void HttpDispatcher::processMessage(Switch* asebaSwitch, const Message& message)
	{
		// TODO: handle answers for requests of variables
		
		// consider user messages for forwarding to SSE
		const UserMessage* userMessage(dynamic_cast<const UserMessage*>(&message));
		if (!userMessage)
			return;
		
		// find name and build event
		string eventName;
		if (userMessage->type >= asebaSwitch->commonDefinitions.events.size())
			eventName = to_string(userMessage->type);
		else
			eventName = WStringToUTF8(asebaSwitch->commonDefinitions.events[userMessage->type].name);
		string sseEventEvent = "event: " + eventName + "\r\n\r\n";
		string sseEventData("data: [");
		unsigned i(0);
		for (auto v: userMessage->data)
		{
			if (i++ > 0)
				sseEventData += ", ";
			sseEventData += to_string(v);
		}
		sseEventData += "]\r\n\r\n";
			
		// send all messages with the name of the event to the general events SSEs
		auto generalEventsRange(eventStreams.equal_range(""));
		for (auto generalEventIt = generalEventsRange.first; generalEventIt != generalEventsRange.second; ++generalEventIt)
		{
			Stream *stream(generalEventIt->second);
			stream->write(sseEventEvent.c_str(), sseEventEvent.length());
			stream->write(sseEventData.c_str(), sseEventData.length());
			stream->flush();
		}
		
		// send message to the registered event with the right name
		auto specificEventsRange(eventStreams.equal_range(eventName));
		for (auto specificEventIt = specificEventsRange.first; specificEventIt != specificEventsRange.second; ++specificEventIt)
		{
			Stream *stream(specificEventIt->second);
			stream->write(sseEventData.c_str(), sseEventData.length());
			stream->flush();
		}
	}
	
	void HttpDispatcher::registerHandler(const Handler& handler, const HttpMethod& method, const strings& uriPath)
	{
		handlers[method][uriPath] = handler;

		std::stringstream opId;
		opId << toString(method);
		for (auto elt: uriPath) // copy elt so can remove braces
		{
			elt.erase(std::remove_if(elt.begin(), elt.end(), [](char c){ return c=='}'||c=='{'; }), elt.end());
			opId << "-" << elt;
		}
		apidocs[method][uriPath] = json{ {"operationId", opId.str()} };
	}
	
	void HttpDispatcher::registerHandler(const Handler& handler, const HttpMethod& method, const strings& uriPath, const json& apidoc)
	{
		handlers[method][uriPath] = handler;
		apidocs[method][uriPath] = apidoc;
	}

	void HttpDispatcher::optionsHandler(HandlerContext& context)
	{
		HttpResponse response(HttpResponse::fromStatus());
		response.headers.emplace("Access-Control-Allow-Methods", "GET, PUT, POST, DELETE, OPTIONS");
		response.headers.emplace("Access-Control-Allow-Headers", "Content-Type");
		response.send(context.stream);
	}
	
	void HttpDispatcher::getTestHandler(HandlerContext& context)
	{
		HttpResponse::fromHTMLString("{ content: \"hello world\" }").send(context.stream);
	}
	
	void HttpDispatcher::getApiDocs(HandlerContext& context)
	{
		json response = R"({"swagger":"2.0","schemes":["http"],"host":"localhost:3000","info":{"version":"1","title":"Aseba","description":"REST API for Aseba"},"parameters":{"trait:serverSentEventStream:todo":{"name":"todo","in":"query","required":false,"type":"integer"}},"responses":{"trait:serverSentEventStream:200":{"description":"stream of server-sent events","schema":{"type":"string"}}}})"_json;
		for (const auto& handlerMapKV: handlers)
		{
			for (const auto& handlerKV: handlerMapKV.second)
			{
				if (handlerKV.first.size() == 0)
					break;
				// build uri string with slash delimiters
				std::stringstream uri;
				for (auto elt: handlerKV.first) // copy so can modify
					uri << "/" << elt;
				// method key must be lower case for OAS 2.0
				auto method = toString(handlerMapKV.first);
				std::transform(method.begin(), method.end(), method.begin(), ::tolower);

				// apidoc for this operation should have been registered
				json doc = apidocs[handlerMapKV.first][handlerKV.first];
				// patch this operation into the apidocs
				response["paths"][uri.str()][method] = doc;
			}
		}
		HttpResponse::fromJSON(response).send(context.stream);
	}

	
} // namespace Aseba
