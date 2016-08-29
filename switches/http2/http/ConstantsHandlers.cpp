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
#include "JsonParser.h"
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
		string content("[");
		for (const auto& constant: asebaSwitch->commonDefinitions.constants)
			content += " \"" + WStringToUTF8(constant.name) + "\",";
		if (!asebaSwitch->commonDefinitions.constants.empty())
			content.pop_back();
		content += "]";
		HttpResponse::fromJSONString(content).send(stream);
	}

	void HttpDispatcher::putConstantsHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// TODO
	}
	
	void HttpDispatcher::postConstantsHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// parse JSON content
		membuf sbuf(reinterpret_cast<const char*>(&request.content[0]), request.content.size());
		istream is(&sbuf);
		map<string, string> constantDescription;
		try
		{
			constantDescription = parseJSONMap<string>(is, parseJSONStringOrNumber);
		}
		catch (const JSONParserException& e)
		{
			HttpResponse::fromPlainString(string("Malformed JSON in query: ") + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		
		// if constant's name is not provided, respond an error and return
		RQ_MAP_GET_FIELD(nameIt, constantDescription, name);
		
		// if constant does already exist, respond an error
		const string& name(nameIt->second);
		const wstring wname(UTF8ToWString(name));
		if (asebaSwitch->commonDefinitions.constants.contains(wname))
		{
			HttpResponse::fromPlainString(FormatableString("Constant %0 already exists").arg(name), HttpStatus::CONFLICT).send(stream);
			return;
		}
		
		// if constant's value is not provided, respond an error and return
		RQ_MAP_GET_FIELD(valueIt, constantDescription, value);
		
		// check the value and its bounds
		int value;
		try
		{
			value = stoi(valueIt->second);
		}
		catch (const invalid_argument& e)
		{
			HttpResponse::fromPlainString("Invalid value field " + valueIt->second + ": " + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		if (value < -32768 || value > 32767)
		{
			HttpResponse::fromPlainString(FormatableString("Value %0 of constant %1 is outside bounds [-32768:32767]").arg(value).arg(name), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		
		// everything is ok, add constant
		asebaSwitch->commonDefinitions.constants.push_back({wname, value});
		const size_t position(asebaSwitch->commonDefinitions.constants.size()-1);
		HttpResponse::fromJSONString(FormatableString("{ id: %0, name: \"%1\", value: %2 }").arg(position).arg(name).arg(value)).send(stream);
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
		HttpResponse::fromJSONString(FormatableString("{ id: %0, name: \"%1\", value: %2 }").arg(position).arg(WStringToUTF8(constant.name)).arg(constant.value)).send(stream);
	}

	void HttpDispatcher::putConstantHandler(Switch* asebaSwitch, Dashel::Stream* stream, const HttpRequest& request, const PathTemplateMap &filledPathTemplates)
	{
		// find the constant
		string name;
		size_t position(0);
		if (!findConstant(asebaSwitch, stream, request, filledPathTemplates, name, position))
			return;
		
		// parse JSON content
		membuf sbuf(reinterpret_cast<const char*>(&request.content[0]), request.content.size());
		istream is(&sbuf);
		map<string, string> constantDescription;
		try
		{
			constantDescription = parseJSONMap<string>(is, parseJSONStringOrNumber);
		}
		catch (const JSONParserException& e)
		{
			HttpResponse::fromPlainString(string("Malformed JSON in query: ") + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		
		// if constant's value is not provided, respond an error and return
		RQ_MAP_GET_FIELD(valueIt, constantDescription, value);
		
		// check the value and its bounds
		int value;
		try
		{
			value = stoi(valueIt->second);
		}
		catch (const invalid_argument& e)
		{
			HttpResponse::fromPlainString("Invalid value field " + valueIt->second + ": " + e.what(), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		if (value < -32768 || value > 32767)
		{
			HttpResponse::fromPlainString(FormatableString("Value %0 of constant %1 is outside bounds [-32768:32767]").arg(value).arg(name), HttpStatus::BAD_REQUEST).send(stream);
			return;
		}
		
		// everything is ok, update constant's value
		asebaSwitch->commonDefinitions.constants[position].value = value;
		HttpResponse::fromJSONString(FormatableString("{ id: %0, name: \"%1\", value: %2 }").arg(position).arg(name).arg(value)).send(stream);
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
