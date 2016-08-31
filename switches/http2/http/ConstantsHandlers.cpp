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
	using namespace Dashel;
	
	#define RQ_MAP_GET_FIELD(itName, mapName, fieldName) \
		const auto itName(mapName.find(#fieldName)); \
		if (itName == mapName.end()) \
		{ \
			HttpResponse::fromPlainString("Cannot find field " #fieldName " in request content", HttpStatus::BAD_REQUEST).send(stream); \
			return; \
		}
	
	void HttpDispatcher::getConstantsHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		json response(json::array());
		for (const auto& constant: asebaSwitch->commonDefinitions.constants)
			response.push_back(WStringToUTF8(constant.name));
		HttpResponse::fromJSONString(response.dump()).send(stream);
	}

	void HttpDispatcher::putConstantsHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// TODO
	}
	
	void HttpDispatcher::postConstantsHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// parse JSON content
		json constantDescription;
		try
		{
			constantDescription = parse(request.content);
		}
		catch (const invalid_argument& e)
		{
			HttpResponse::fromPlainString(string("Malformed JSON in query: ") + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		
		// if constant's name is not provided, respond an error and return
		RQ_MAP_GET_FIELD(nameIt, constantDescription, name);
		
		// if constant does already exist, respond an error
		string name;
		try
		{
			name = nameIt.value().get<string>();
		}
		catch (const domain_error& e)
		{
			HttpResponse::fromPlainString("Invalid name field " + nameIt.value().dump() + ": " + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		const wstring wname(UTF8ToWString(name));
		if (asebaSwitch->commonDefinitions.constants.contains(wname))
		{
			HttpResponse::fromPlainString(FormatableString("Constant %0 already exists").arg(name), HttpStatus::CONFLICT).send(stream);
			return;
		}
		
		// updated the value and return an answer
		updateConstantValue(asebaSwitch, stream, constantDescription, name, wname);
	}
	
	void HttpDispatcher::updateConstantValue(Switch* asebaSwitch, Dashel::Stream* stream, const json& constantDescription, const string& name, const wstring& wname, size_t position)
	{
		// if constant's value is not provided, respond an error and return
		RQ_MAP_GET_FIELD(valueIt, constantDescription, value);
		
		// check the value and its bounds
		int value;
		try
		{
			value = valueIt.value().get<int>();
		}
		catch (const domain_error& e)
		{
			HttpResponse::fromPlainString("Invalid value field " + valueIt.value().dump() + ": " + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		if (value < -32768 || value > 32767)
		{
			HttpResponse::fromPlainString(FormatableString("Value %0 of constant %1 is outside bounds [-32768:32767]").arg(value).arg(name), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		
		// everything is ok, add constant
		if (position == size_t(-1))
		{
			asebaSwitch->commonDefinitions.constants.push_back({wname, value});
			position = asebaSwitch->commonDefinitions.constants.size() - 1;
		}
		else
		{
			asebaSwitch->commonDefinitions.constants[position] = {wname, value};
		}
		
		// return answer
		HttpResponse::fromJSONString(json(
			{
				{ "id", position },
				{ "name",  name },
				{ "value", value }
			}
		).dump()).send(stream);
	}
	
	void HttpDispatcher::deleteConstantsHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		asebaSwitch->commonDefinitions.constants.clear();
		return HttpResponse::fromPlainString("All constants erased").send(stream);
	}

	void HttpDispatcher::getConstantHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(asebaSwitch, stream, request, filledPathTemplates, name, position))
			return;
		
		// return the constant
		const NamedValue& constant(asebaSwitch->commonDefinitions.constants[position]);
		HttpResponse::fromJSONString(FormatableString("{ \"id\": %0, \"name\": \"%1\", \"value\": %2 }").arg(position).arg(WStringToUTF8(constant.name)).arg(constant.value)).send(stream);
	}

	void HttpDispatcher::putConstantHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(asebaSwitch, stream, request, filledPathTemplates, name, position))
			return;
		
		// parse JSON content
		json constantDescription;
		try
		{
			constantDescription = parse(request.content);
		}
		catch (const invalid_argument& e)
		{
			HttpResponse::fromPlainString(string("Malformed JSON in query: ") + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		
		// updated the value and return an answer
		// FIXME: duplication of conversion to wstring
		updateConstantValue(asebaSwitch, stream, constantDescription, name, UTF8ToWString(name), position);
	}

	void HttpDispatcher::deleteConstantHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(asebaSwitch, stream, request, filledPathTemplates, name, position))
			return;
		
		// delete the constant
		asebaSwitch->commonDefinitions.constants.erase(asebaSwitch->commonDefinitions.constants.begin() + position);
		
		// return success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(stream);
	}
	
	//! try to find a constant, return true and update position if found, return false and send an error if not found
	bool HttpDispatcher::findConstant(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates, string& name, size_t& position)
	{
		// retrieve name
		const auto templateIt(filledPathTemplates.find("name"));
		assert(templateIt != filledPathTemplates.end());
		name = templateIt->second;
		
		// if constant does not exist, respond an error
		position = 0;
		if (!asebaSwitch->commonDefinitions.constants.contains(UTF8ToWString(name), &position))
		{
			HttpResponse::fromPlainString(FormatableString("Constant %0 does not exist").arg(name), HttpStatus::NOT_FOUND).send(stream);
			return false;
		}
		
		return true;
	}

} // namespace Aseba
