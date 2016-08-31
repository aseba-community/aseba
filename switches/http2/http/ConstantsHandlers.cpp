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
#include "../Globals.h"
#include "../Switch.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	#define RQ_GET_VALUE(stream, jsonObject, fieldType, fieldName) \
		const auto _it ## fieldName(jsonObject.find(#fieldName)); \
		if (_it ## fieldName == jsonObject.end()) \
		{ \
			HttpResponse::fromPlainString("Cannot find field " #fieldName " in object" + jsonObject.dump(), HttpStatus::BAD_REQUEST).send(stream); \
			return; \
		} \
		fieldType fieldName; \
		try \
		{ \
			fieldName = _it ## fieldName.value().get<fieldType>(); \
		} \
		catch (const domain_error& e) \
		{ \
			HttpResponse::fromPlainString("Field " #fieldName " has invalid value " + _it ## fieldName.value().dump() + ": " + e.what(), HttpStatus::BAD_REQUEST).send(stream); \
			return; \
		}
	
	//! handler for GET /constants/
	void HttpDispatcher::getConstantsHandler(HandlerContext& context)
	{
		json response(json::array());
		for (const auto& constant: context.asebaSwitch->commonDefinitions.constants)
			response.push_back(WStringToUTF8(constant.name));
		HttpResponse::fromJSONString(response.dump()).send(context.stream);
	}

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
	}
	
	//! handler for POST /constants/
	void HttpDispatcher::postConstantsHandler(HandlerContext& context)
	{
		// get the name, and check presence
		RQ_GET_VALUE(context.stream, context.parsedContent, string, name);
		const wstring wname(UTF8ToWString(name));
		if (context.asebaSwitch->commonDefinitions.constants.contains(wname))
		{
			HttpResponse::fromPlainString(FormatableString("Constant %0 already exists").arg(name), HttpStatus::CONFLICT).send(context.stream);
			return;
		}
		
		// create the constant with a default value
		context.asebaSwitch->commonDefinitions.constants.push_back({wname, 0});
		const size_t position(context.asebaSwitch->commonDefinitions.constants.size() - 1);
		
		// updated the value and return an answer
		updateConstantValue(context, name, position);
	}
	
	//! handler for DELETE /constants/
	void HttpDispatcher::deleteConstantsHandler(HandlerContext& context)
	{
		// clear all constants
		context.asebaSwitch->commonDefinitions.constants.clear();
		
		// return success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);
	}

	//! handler for GET /constants/{name}
	void HttpDispatcher::getConstantHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(context, name, position))
			return;
		
		// return the constant
		const NamedValue& constant(context.asebaSwitch->commonDefinitions.constants[position]);
		HttpResponse::fromJSONString(json(
			{
				{ "id", position },
				{ "name",  name },
				{ "value", constant.value }
	
		}
		).dump()).send(context.stream);
	}

	//! handler for PUT /constants/{name}
	void HttpDispatcher::putConstantHandler(HandlerContext& context)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(context, name, position))
			return;
		
		// updated the value and return an answer
		updateConstantValue(context, name, position);
	}

	//! handler for DELETE /constants/{name}
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
	
	//! set the value of a content from the request
	void HttpDispatcher::updateConstantValue(HandlerContext& context, const string& name, size_t position)
	{
		// get the value, and check bounds
		RQ_GET_VALUE(context.stream, context.parsedContent, int, value);
		
		// make sure the value is in a valid range
		if (!validateInt16Value(context, name, value))
			return;
		
		// sets the value
		context.asebaSwitch->commonDefinitions.constants[position].value = value;
		
		// return answer
		HttpResponse::fromJSONString(json(
			{
				{ "id", position },
				{ "name",  name },
				{ "value", value }
			}
		).dump()).send(context.stream);
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
	
	//! Return true if the value is valid, return false and send an error if not
	bool HttpDispatcher::validateInt16Value(HandlerContext& context, const string& name, int value)
	{
		if (value < -32768 || value > 32767)
		{
			HttpResponse::fromPlainString(FormatableString("Value %0 of constant %1 is outside bounds [-32768:32767]").arg(value).arg(name), HttpStatus::BAD_REQUEST).send(context.stream);
			return false;
		}
		return true;
	}

} // namespace Aseba
