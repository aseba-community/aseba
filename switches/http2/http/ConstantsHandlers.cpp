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
	
	//! Register all constants-related handlers
	void HttpDispatcher::registerConstantsHandlers()
	{
		REGISTER_HANDLER(postConstantHandler, POST, { "constants" }, { R"#json#(
			{
				"consumes" : [
					"application/json"
				],
				"operationId" : "POST-constant",
				"parameters" : [
					{
						"in" : "body",
						"name" : "body",
						"schema" : {
							"$ref" : "#/definitions/constant-definition",
							"example" : {
								"description" : "Lowest address from which to start numbering",
								"name" : "base.address",
								"value" : 100
							}
						}
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"201" : {
						"description" : "Create Constant",
						"schema" : {
							"$ref" : "#/definitions/constant-definition"
						}
					},
					"400" : {
						"description" : "Invalid name or invalid value",
						"schema" : {
							"type" : "string"
						}
					},
					"409" : {
						"description" : "Constant already exists",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Create Constant",
				"tags" : [
					"Constants"
				]
			})#json#"_json });
		REGISTER_HANDLER(getConstantsHandler, GET, { "constants" }, { R"#json#(
			{
				"operationId" : "GET-constants",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get All Constants",
						"schema" : {
							"items" : {
								"$ref" : "#/definitions/constant-definition"
							},
							"type" : "array"
						}
					}
				},
				"summary" : "Get All Constants",
				"tags" : [
					"Constants"
				]
			})#json#"_json });
		REGISTER_HANDLER(deleteConstantsHandler, DELETE, { "constants" }, { R"#json#(
			{
				"operationId" : "DELETE-constants",
				"responses" : {
					"204" : {
						"description" : "Delete All Constants",
						"schema" : {
							"type" : "object"
						}
					}
				},
				"summary" : "Delete All Constants",
				"tags" : [
					"Constants"
				]
			})#json#"_json });
		REGISTER_HANDLER(getConstantHandler, GET, { "constants", "{name}" }, { R"#json#(
			{
				"operationId" : "GET-constant",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get Constant",
						"schema" : {
							"$ref" : "#/definitions/constant-definition"
						}
					},
					"404" : {
						"description" : "Unknown constant",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Get Constant",
				"tags" : [
					"Constants"
				]
			})#json#"_json });
		REGISTER_HANDLER(deleteConstantHandler, DELETE, { "constants", "{name}" }, { R"#json#(
			{
				"operationId" : "DELETE-constant",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"204" : {
						"description" : "Delete Constant",
						"schema" : {
							"type" : "object"
						}
					},
					"404" : {
						"description" : "Unknown constant",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Delete Constant",
				"tags" : [
					"Constants"
				]
			})#json#"_json });
		REGISTER_HANDLER(putConstantHandler, PUT, { "constants", "{name}" }, { R"#json#(
			{
				"consumes" : [
					"application/json"
				],
				"description" : "Since the name is known from path parameter *name*, it is only supplied in the #model:PjNWMPh4ACC7kxrss request if the name should be changed.",
				"operationId" : "PUT-constant",
				"parameters" : [
					{
						"in" : "body",
						"name" : "body",
						"schema" : {
							"$ref" : "#/definitions/constant-definition-update",
							"example" : {
								"value" : 501
							}
						}
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Update Constant",
						"schema" : {
							"$ref" : "#/definitions/constant-definition"
						}
					},
					"400" : {
						"description" : "Invalid value",
						"schema" : {
							"type" : "string"
						}
					},
					"404" : {
						"description" : "Unknown constant",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Update Constant",
				"tags" : [
					"Constants"
				]
			})#json#"_json });
	}

	// handlers

	//! handler for POST /constants -> application/json
	void HttpDispatcher::postConstantHandler(HandlerContext& context)
	{
		const string name(context.parsedContent["name"].get<string>());
		const wstring wname(UTF8ToWString(name));
		if (context.asebaSwitch->commonDefinitions.constants.contains(wname))
		{
			HttpResponse::fromPlainString(FormatableString("Constant %0 already exists").arg(name), HttpStatus::CONFLICT).send(context.stream);
			return;
		}
		
		// create the constant with a default value
		context.asebaSwitch->commonDefinitions.constants.push_back({wname, 0});
		const size_t position(context.asebaSwitch->commonDefinitions.constants.size() - 1);
		
		// update the value and return an answer
		updateConstantValue(context, name, position, true);
	}

	//! handler for GET /constants -> application/json
	void HttpDispatcher::getConstantsHandler(HandlerContext& context)
	{
		json response(json::array());
		unsigned i(0);
		for (const auto& constant: context.asebaSwitch->commonDefinitions.constants)
			response.push_back({
				{ "id", i++ },
				{ "name", WStringToUTF8(constant.name) },
				{ "value", constant.value}
			});
		HttpResponse::fromJSON(response).send(context.stream);
	}

	//! handler for DELETE /constants 
	void HttpDispatcher::deleteConstantsHandler(HandlerContext& context)
	{
		// clear all constants
		context.asebaSwitch->commonDefinitions.constants.clear();
		
		// return success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);
	}
	
	/*
	// TODO: add to scheme
	//! handler for PUT /constants/
	void HttpDispatcher::putConstantsHandler(HandlerContext& context)
	{
		// make sure we have an array
		if (!context.parsedContent.is_array())
		{
			HttpResponse::fromPlainString("Not a JSON array", HttpStatus::BAD_REQUEST).send(context.stream);
			return;
		}
		
		// check the validity of each element of the array
		for (const auto& element: context.parsedContent)
		{
			// get name and value
			RQ_GET_VALUE(context.stream, element, string, name);
			RQ_GET_VALUE(context.stream, element, int, value);
			
			// make sure the value is in a valid range
			if (!validateInt16Value(context, name, value))
				return;
		}
		
		// clear all constants
		context.asebaSwitch->commonDefinitions.constants.clear();
		
		// add each constant from the request's content
		for (const auto& element: context.parsedContent)
		{
			// get name and value
			RQ_GET_VALUE(context.stream, element, string, name);
			RQ_GET_VALUE(context.stream, element, int, value);
			context.asebaSwitch->commonDefinitions.constants.push_back({ UTF8ToWString(name), value });
		}
		
		// return success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);
	}*/

	//! handler for GET /constants/{name} -> application/json
	void HttpDispatcher::getConstantHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(context, name, position))
			return;
		
		// return the constant
		const NamedValue& constant(context.asebaSwitch->commonDefinitions.constants[position]);
		HttpResponse::fromJSON({
			{ "id", position },
			{ "name",  name },
			{ "value", constant.value }
	
		}).send(context.stream);
	}

	//! handler for DELETE /constants/{name} -> application/json
	void HttpDispatcher::deleteConstantHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(context, name, position))
			return;
		
		// delete the constant
		NamedValuesVector& constants(context.asebaSwitch->commonDefinitions.constants);
		constants.erase(constants.begin() + position);
		
		// return success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);
	}

	//! handler for PUT /constants/{name} -> application/json
	void HttpDispatcher::putConstantHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(context, name, position))
			return;
		
		// updated the value and return an answer
		updateConstantValue(context, name, position, false);
	}

	// support

	//! set the value of a content from the request
	void HttpDispatcher::updateConstantValue(HandlerContext& context, const string& name, size_t position, bool created)
	{
		// sets the value
		const int value(context.parsedContent["value"].get<int>());
		context.asebaSwitch->commonDefinitions.constants[position].value = value;
		
		// return answer
		HttpResponse::fromJSON({
			{ "id", position },
			{ "name",  name },
			{ "value", value }
		}, created ? HttpStatus::CREATED : HttpStatus::OK).send(context.stream);
	}
	
	//! try to find a constant, return true and update position if found, return false and send an error if not found
	bool HttpDispatcher::findConstant(HandlerContext& context, string& name, size_t& position)
	{
		// retrieve name
		const auto templateIt(context.filledPathTemplates.find("name"));
		assert(templateIt != context.filledPathTemplates.end());
		name = templateIt->second;
		
		// if constant does not exist, respond an error
		position = 0;
		if (!context.asebaSwitch->commonDefinitions.constants.contains(UTF8ToWString(name), &position))
		{
			HttpResponse::fromPlainString(FormatableString("Constant %0 does not exist").arg(name), HttpStatus::NOT_FOUND).send(context.stream);
			return false;
		}
		
		return true;
	}

} // namespace Aseba
