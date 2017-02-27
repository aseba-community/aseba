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

#ifndef ASEBA_HTTP_JSON
#define ASEBA_HTTP_JSON

#include <vector>
#include "../../../common/json/json.hpp"

namespace Aseba
{
	using json = nlohmann::json;
	
	json parse(const std::vector<uint8_t>& rawData);
	
	void validate(const json& schema, const json& data, const std::string& context = "/");
	void validateType(const json& schema, const std::string& type, const json& data, const std::string& context);
	
	using logic_error = std::logic_error;
	struct InvalidJsonSchema: public logic_error
	{
		using logic_error::logic_error;
	};
	
} // namespace Aseba

#endif // ASEBA_HTTP_JSON
