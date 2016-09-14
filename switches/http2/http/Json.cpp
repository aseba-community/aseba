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

#include <regex>
#include "../../../common/utils/utils.h"
#include "../../../common/utils/FormatableString.h"
#include "Json.h"

namespace Aseba
{
	using namespace std;
	
	//! Parse rawData as a string into JSON
	json parse(const vector<uint8_t>& rawData)
	{
		membuf sbuf(reinterpret_cast<const char*>(&rawData[0]), rawData.size());
		istream is(&sbuf);
		return json::parse(is);
	}
	
	//! Valide data as JSON schema (see http://json-schema.org/latest/json-schema-core.html)
	//! context is used to produced informative dump
	void validate(const json& schema, const json& data, const string& context)
	{
		// constant type-to-string map
		const map<json::value_t, string> typeToString{
			{ json::value_t::null, "null" },
			{ json::value_t::boolean, "boolean" },
			{ json::value_t::number_integer, "integer number" },
			{ json::value_t::number_float, "float number" },
			{ json::value_t::object, "object" },
			{ json::value_t::array, "array" },
			{ json::value_t::string, "string" }
		};
		
		// do type-specific checks
		const string type = schema.at("type");
		// TODO: currently this validator does not support multiple types (type as array)
		if (type == "array")
		{
			// check it is array
			if (!data.is_array())
				throw InvalidJsonSchema("In " + context + ", wrong JSON type, expected array, found " + typeToString.at(data.type()));
			
			// check number of items
			auto minItemsIt(schema.find("minItems"));
			if ((minItemsIt != schema.end()) && (data.size() < minItemsIt->get<size_t>()))
				throw InvalidJsonSchema(FormatableString("In %0, number of elements %1 is smaller than minimum %2").arg(context).arg(data.size()).arg(minItemsIt->get<int>()));
			auto maxItemsIt(schema.find("maxItems"));
			if ((maxItemsIt != schema.end()) && (data.size() > maxItemsIt->get<size_t>()))
				throw InvalidJsonSchema(FormatableString("In %0, number of elements %1 is smaller than maximum %2").arg(context).arg(data.size()).arg(maxItemsIt->get<int>()));
			
			// iterate the items
			const json itemSchema(schema.at("items"));
			unsigned i(0);
			for (const auto& item: data)
			{
				validate(itemSchema, item, context + to_string(i++) + "/");
			}
		}
		else if (type == "boolean")
		{
			// check it is boolean
			if (!data.is_boolean())
				throw InvalidJsonSchema("In " + context + ", wrong JSON type, expected boolean, found " + typeToString.at(data.type()));
		}
		else if (type == "integer")
		{
			// check it is number
			if (!data.is_number())
				throw InvalidJsonSchema("In " + context + ", wrong JSON type, expected integer, found " + typeToString.at(data.type()));
			
			// check bounds if given
			auto minimumIt(schema.find("minimum"));
			if ((minimumIt != schema.end()) && (data.get<int>() < minimumIt->get<int>()))
				throw InvalidJsonSchema(FormatableString("In %0, value %1 is smaller than minimum %2").arg(context).arg(data.get<int>()).arg(minimumIt->get<int>()));
			auto maximumIt(schema.find("maximum"));
			if ((maximumIt != schema.end()) && (data.get<int>() > maximumIt->get<int>()))
				throw InvalidJsonSchema(FormatableString("In %0, value %1 is larger than maximum %2").arg(context).arg(data.get<int>()).arg(maximumIt->get<int>()));
		}
		else if (type == "number")
		{
			// check it is number
			if (!data.is_number())
				throw InvalidJsonSchema("In " + context + ", wrong JSON type, expected number, found " + typeToString.at(data.type()));
			
			// check bounds if given
			auto minimumIt(schema.find("minimum"));
			if ((minimumIt != schema.end()) && (data.get<double>() < minimumIt->get<double>()))
				throw InvalidJsonSchema(FormatableString("In %0, value %1 is smaller than minimum %2").arg(context).arg(data.get<double>()).arg(minimumIt->get<double>()));
			auto maximumIt(schema.find("maximum"));
			if ((maximumIt != schema.end()) && (data.get<double>() > maximumIt->get<double>()))
				throw InvalidJsonSchema(FormatableString("In %0, value %1 is larger than maximum %2").arg(context).arg(data.get<double>()).arg(maximumIt->get<double>()));
		}
		else if (type == "null")
		{
			// check it is null
			if (!data.is_null())
				throw InvalidJsonSchema("In " + context + ", wrong JSON type, expected null, found " + typeToString.at(data.type()));
		}
		else if (type == "object")
		{
			// check it is object
			if (!data.is_object())
				throw InvalidJsonSchema("In " + context + ", wrong JSON type, expected object, found " + typeToString.at(data.type()));
			
			// check that all elements of the array are valid properties
			const json properties(schema.at("properties"));
			assert(properties.is_object());
			for (json::const_iterator it = data.begin(); it != data.end(); ++it)
			{
				// the element must exist in the schema
				const auto propertyIt(properties.find(it.key()));
				if (propertyIt == properties.end())
					throw InvalidJsonSchema("In " + context + ", invalid key \"" + it.key() + "\" in JSON object " + data.dump());
				// validate element
				validate(propertyIt.value(), it.value(), context + it.key() + "/");
			}
			
			// are some fields required?
			const auto requiredIt(schema.find("required"));
			if (requiredIt != schema.end())
			{
				const json required(*requiredIt);
				assert(required.is_array());
				for (const auto& name: required)
				{
					if (data.find(name) == data.end())
						throw InvalidJsonSchema("In " + context + ", required property \"" + name.get<string>() +  "\" not found in JSON object " + data.dump());
				}
			}
		}
		else if (type == "string")
		{
			// check it is string
			if (!data.is_string())
				throw InvalidJsonSchema("In " + context + ", wrong JSON type, expected string, found " + typeToString.at(data.type()));
			
			// check minLength
			auto minLengthIt(schema.find("minLength"));
			if ((minLengthIt != schema.end()) && (data.get<string>().size() < minLengthIt->get<size_t>()))
				throw InvalidJsonSchema(FormatableString("In %0, string \"%1\" is smaller than minimum length %2").arg(context).arg(data.get<string>()).arg(minLengthIt->get<unsigned>()));
				
			// check content using regular expression
			auto patternIt(schema.find("pattern"));
			if (patternIt != schema.end())
			{
				regex re(patternIt->get<string>());
				if (!regex_match(data.get<string>(), re))
					throw InvalidJsonSchema(FormatableString("In %0, string \"%1\" does not match pattern \"%2\"").arg(context).arg(data.get<string>()).arg(patternIt->get<string>()));
			}
		}
		else
		{
			// this means something is broken in our schema
			assert(false);
		}
	}
	
}; // namespace Aseba
