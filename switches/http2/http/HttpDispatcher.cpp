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
		registerHandler(bind(&HttpDispatcher::handler, this, _1, _2, _3, _4), HttpMethod::method,  __VA_ARGS__);
	
	HttpDispatcher::HttpDispatcher():
		serverPort(3000)
	{
		// register all handlers
		REGISTER_HANDLER(getEventsHandler, GET, { "events" });
		REGISTER_HANDLER(getEventsHandler, GET, { "events", "{name}" });
		REGISTER_HANDLER(testHandler, GET, { "test" });
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
							LOG_VERBOSE << "HTTP | On stream " << stream->getTargetName() << ", dispatching " << toString(request.method) << " " << request.uri << " to handler" << endl;
							uriHandlerKV.second(asebaSwitch, stream, request, filledPathTemplates);
							return;
						}
					}
				}
				
				// no matching method or URI found, not found
				LOG_VERBOSE << "HTTP | On stream " << stream->getTargetName() << ", request error, " << toString(request.method) << " " << request.uri << " is not found" << endl;
				HttpResponse response;
				response.status = HttpStatus::NOT_FOUND;
				response.send(stream);
				
				// TODO; add timeout for closing unused connections for a while
				// FIXME: what about protocols incompatibilities?
			}
			catch (const HttpRequest::Error& e)
			{
				// show error
				LOG_ERROR << "HTTP | On stream " << stream->getTargetName() << ", request error " << HttpStatus::toString(e.errorCode) << " while receiving" << endl;
				// send an error
				HttpResponse response;
				response.status = e.errorCode;
				response.send(stream);
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
		
		// send all messages, including anonymous ones, to the general events SSEs
		auto generalEventsRange(eventStreams.equal_range(""));
		for (auto generalEventIt = generalEventsRange.first; generalEventIt != generalEventsRange.second; ++generalEventIt)
			// TODO send on generalEventIt->second
			;
		
		// retrieve event name from Id
		//asebaSwitch->commonDefinitions.
		// send on SSE associated to this name
		// TODO
		
	}
	
	void HttpDispatcher::registerHandler(Handler handler, const HttpMethod& method, const strings& uriPath)
	{
		handlers[method][uriPath] = handler;
	}
	
	void HttpDispatcher::getEventsHandler(Switch* asebaSwitch, Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// TODO: handle Dashel exception
		HttpResponse response;
		const auto nameIt(filledPathTemplates.find("name"));
		if (nameIt != filledPathTemplates.end())
		{
			// Check if name exists
			// TODO
		}
		else
		{
			eventStreams.emplace("", stream);
			response.setServerSideEvent();
		}
		response.send(stream);
	}
	
	void HttpDispatcher::testHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		HttpResponse response;
		string rs("{ content: \"hello world\" }");
		response.content.insert(response.content.end(), rs.begin(), rs.end());
		response.send(stream);
	}

	
} // namespace Aseba
