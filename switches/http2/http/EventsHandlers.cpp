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

#include "HttpDispatcher.h"
#include "HttpStatus.h"
#include "HttpHandlers.h"
#include "../Globals.h"
#include "../Switch.h"


namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	// TODO: at some point, probably move this to streams-specific file
	
	//! handler for GET /streams/events
	void HttpDispatcher::getStreamsEventsHandler(HandlerContext& context)
	{
		Switch* asebaSwitch(context.asebaSwitch);
		Stream* stream(context.stream);
		const PathTemplateMap &filledPathTemplates(context.filledPathTemplates);
		
		const auto nameIt(filledPathTemplates.find("name"));
		if (nameIt != filledPathTemplates.end())
		{
			// check if name exists
			const wstring name(UTF8ToWString(nameIt->second));
			if (asebaSwitch->commonDefinitions.events.contains(name))
			{
				// if name exists, subscribe
				LOG_VERBOSE << "HTTP | Registered stream " << stream->getTargetName() << " for SSE on specific Aseba event " << nameIt->second << endl;
				eventStreams.emplace(nameIt->second, stream);
				HttpResponse::createSSE().send(stream);
			}
			else
			{
				// otherwise produce an error
				LOG_VERBOSE << "HTTP | Stream " << stream->getTargetName() << " requested SSE registration on Aseba event " << nameIt->second << ", which does not exist" << endl;
				HttpResponse::fromPlainString(FormatableString("Event %0 does not exist").arg(nameIt->second), HttpStatus::NOT_FOUND).send(stream);
			}
		}
		else
		{
			LOG_VERBOSE << "HTTP | Registered stream " << stream->getTargetName() << " for SSE on all Aseba events" << endl;
			eventStreams.emplace("", stream);
			HttpResponse::createSSE().send(stream);
		}
	}
	
	//! Register all events-related handlers
	void HttpDispatcher::registerEventsHandlers()
	{
		REGISTER_HANDLER(deleteEventsHandler, DELETE, { "events" }, { R"#json#(
			{
				"operationId" : "DELETE-events",
				"responses" : {
					"204" : {
						"description" : "Delete All Events",
						"schema" : {
							"type" : "object"
						}
					}
				},
				"summary" : "Delete All Events",
				"tags" : [
					"Events"
				]
			})#json#"_json });
		REGISTER_HANDLER(postEventHandler, POST, { "events" }, { R"#json#(
			{
				"consumes" : [
					"application/json"
				],
				"operationId" : "POST-event",
				"parameters" : [
					{
						"in" : "body",
						"name" : "body",
						"schema" : {
							"$ref" : "#/definitions/event-definition",
							"example" : {
								"description" : "Ping all nodes with last timestamp",
								"name" : "ping",
								"parameters" : [
									{
										"description" : "Last timestamp",
										"param" : "timestamp",
										"size" : 1
									}
								],
								"size" : 1
							}
						}
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"201" : {
						"description" : "Create Event",
						"schema" : {
							"$ref" : "#/definitions/event-definition-full"
						}
					},
					"400" : {
						"description" : "Invalid event name or event size",
						"schema" : {
							"type" : "string"
						}
					},
					"409" : {
						"description" : "Event with this name already exists",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Create Event",
				"tags" : [
					"Events"
				]
			})#json#"_json });
		REGISTER_HANDLER(getEventsHandler, GET, { "events" }, { R"#json#(
			{
				"operationId" : "GET-events",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get All Events",
						"schema" : {
							"items" : {
								"$ref" : "#/definitions/event-definition-full"
							},
							"type" : "array"
						}
					}
				},
				"summary" : "Get All Events",
				"tags" : [
					"Events"
				]
			})#json#"_json });
		REGISTER_HANDLER(getEventHandler, GET, { "events", "{name}" }, { R"#json#(
			{
				"operationId" : "GET-event",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get Event",
						"schema" : {
							"$ref" : "#/definitions/event-definition-full"
						}
					},
					"404" : {
						"description" : "Unknown event",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Get Event",
				"tags" : [
					"Events"
				]
			})#json#"_json });
		REGISTER_HANDLER(putEventHandler, PUT, { "events", "{name}" }, { R"#json#(
			{
				"consumes" : [
					"application/json"
				],
				"description" : "Since the name is known from path parameter *name*, it is only supplied in the #model:EJKRFJthJFQcuf2gP request if the name should be changed.",
				"operationId" : "PUT-event",
				"parameters" : [
					{
						"in" : "body",
						"name" : "body",
						"schema" : {
							"$ref" : "#/definitions/event-definition-update",
							"example" : {
								"description" : "Ping all nodes with last timestamp pair",
								"parameters" : [
									{
										"param" : "timestamp",
										"size" : 2
									}
								],
								"size" : 2
							}
						}
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Update Event",
						"schema" : {
							"$ref" : "#/definitions/event-definition-full"
						}
					},
					"400" : {
						"description" : "Invalid size",
						"schema" : {
							"type" : "string"
						}
					},
					"404" : {
						"description" : "Unknown event",
						"schema" : {
							"type" : "string"
						}
					},
					"409" : {
						"description" : "Event with this name already exists",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Update Event",
				"tags" : [
					"Events"
				]
			})#json#"_json });
		REGISTER_HANDLER(deleteEventHandler, DELETE, { "events", "{name}" }, { R"#json#(
			{
				"operationId" : "DELETE-event",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"204" : {
						"description" : "Delete Event",
						"schema" : {
							"type" : "object"
						}
					},
					"404" : {
						"description" : "Unknown event",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Delete Event",
				"tags" : [
					"Events"
				]
			})#json#"_json });
	}
	
	// handlers
	
	//! handler for DELETE /events 
	void HttpDispatcher::deleteEventsHandler(HandlerContext& context)
	{
		// clear all events
		context.asebaSwitch->commonDefinitions.events.clear();
		
		// return success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);
	}

	//! handler for POST /events -> application/json
	void HttpDispatcher::postEventHandler(HandlerContext& context)
	{
		const string name(context.parsedContent["name"].get<string>());
		const wstring wname(UTF8ToWString(name));
		if (context.asebaSwitch->commonDefinitions.events.contains(wname))
		{
			HttpResponse::fromPlainString(FormatableString("Event %0 already exists").arg(name), HttpStatus::CONFLICT).send(context.stream);
			return;
		}
		
		// create the event with a default size
		context.asebaSwitch->commonDefinitions.events.push_back({wname, 0});
		const size_t position(context.asebaSwitch->commonDefinitions.events.size() - 1);
		
		// update the size and return an answer
		updateEventValue(context, name, position, true);
	}

	//! handler for GET /events -> application/json
	void HttpDispatcher::getEventsHandler(HandlerContext& context)
	{
		json response(json::array());
		unsigned i(0);
		for (const auto& event: context.asebaSwitch->commonDefinitions.events)
			response.push_back({
				{ "id", i++ },
				{ "name", WStringToUTF8(event.name) },
				{ "size", event.value }
			});
		HttpResponse::fromJSON(response).send(context.stream);
	}
	
	//! handler for GET /events/{event} -> application/json
	void HttpDispatcher::getEventHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findEvent(context, name, position))
			return;
		
		// return the constant
		const NamedValue& event(context.asebaSwitch->commonDefinitions.events[position]);
		HttpResponse::fromJSON({
			{ "id", position },
			{ "name",  name },
			{ "size", event.value }
	
		}).send(context.stream);
	}

	//! handler for DELETE /events/{name} -> application/json
	void HttpDispatcher::deleteEventHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findEvent(context, name, position))
			return;
		
		// delete the constant
		NamedValuesVector& events(context.asebaSwitch->commonDefinitions.events);
		events.erase(events.begin() + position);
		
		// return success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);
	}

	//! handler for PUT /events/{name} -> application/json
	void HttpDispatcher::putEventHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findEvent(context, name, position))
			return;
		
		// updated the value and return an answer
		updateEventValue(context, name, position, false);
	}

	// support

	//! set the value of an event from the request
	void HttpDispatcher::updateEventValue(HandlerContext& context, const string& name, size_t position, bool created)
	{
		// sets the value
		const int value(context.parsedContent["size"].get<int>());
		context.asebaSwitch->commonDefinitions.events[position].value = value;
		
		// return answer
		HttpResponse::fromJSON({
			{ "id", position },
			{ "name",  name },
			{ "size", value }
		}, created ? HttpStatus::CREATED : HttpStatus::OK).send(context.stream);
	}
	
	//! try to find an event, return true and update position if found, return false and send an error if not found
	bool HttpDispatcher::findEvent(HandlerContext& context, string& name, size_t& position)
	{
		// retrieve name
		const auto templateIt(context.filledPathTemplates.find("name"));
		assert(templateIt != context.filledPathTemplates.end());
		name = templateIt->second;
		
		// if constant does not exist, respond an error
		position = 0;
		if (!context.asebaSwitch->commonDefinitions.events.contains(UTF8ToWString(name), &position))
		{
			HttpResponse::fromPlainString(FormatableString("Event %0 does not exist").arg(name), HttpStatus::NOT_FOUND).send(context.stream);
			return false;
		}
		
		return true;
	}
	
} // namespace Aseba
