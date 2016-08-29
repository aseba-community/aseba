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

#ifndef ASEBA_HTTP_JSON_PARSER
#define ASEBA_HTTP_JSON_PARSER

#include <iostream>
#include <functional>
#include <stdexcept>
#include <map>
#include <string>
#include <vector>
#include "../../../common/utils/FormatableString.h"
#include "../../../common/utils/utils.h"

namespace Aseba
{
	struct JSONParserException: public std::runtime_error
	{
		JSONParserException(const std::string& what_arg): std::runtime_error(what_arg) {}
	};
	
	
	std::string parseJSONStringOrNumber(std::istream& stream);

	template<typename Value>
	std::vector<Value> parseJSONVector(std::istream& stream, std::function<Value(std::istream&)> valueParser)
	{
		std::vector<Value> result;
		// TODO
		return result;
	}

	template<typename Value>
	std::map<std::string, Value> parseJSONMap(std::istream& stream, std::function<Value(std::istream&)> valueParser)
	{
		int c;
		
		// eat everything until { included
		while (((c = stream.get()) != EOF) && (c != '{'))
			if (!std::isspace(c))
				throw JSONParserException("Invalid JSON collection, non-whitespace before starting {");
		if (c != '{')
			throw JSONParserException("Invalid JSON collection, missing starting {");
		
		std::map<std::string, Value> result;
		
		// eat everything until } excluded
		while (((c = stream.get()) != EOF) && (c != '}'))
		{
			// read key
			std::string key;
			do
			{
				key.push_back(c);
			}
			while (((c = stream.get()) != EOF) && (c != ':'));
			if (c != ':')
				throw JSONParserException("Invalid JSON collection, missing : after key name");
			key = trim(key);
			
			// read value
			result.emplace(key, valueParser(stream));
			
			// read , or }
			while (((c = stream.get()) != EOF) && (c != ',') && (c != '}'))
				if (!std::isspace(c))
					throw JSONParserException("Invalid JSON collection, non-whitespace after value");
			if (c == '}')
				break;
			else if (c == ',')
				;
			else
				throw JSONParserException("Invalid JSON collection, missing } or , after key value");
		}
		return result;
	}

	//parseJSon<map<string, string>> parseJSon(istringstream stream, bind(parseJSon<string>, string));
} // namespace Aseba

#endif // ASEBA_HTTP_JSON_PARSER
