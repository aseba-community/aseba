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

#include <map>
#include "../../common/utils/FormatableString.h"
#include "HttpMethod.h"

namespace Aseba
{
	using namespace std;
	
	//! List of names of all known HTTP methods
	static const map<HttpMethod, string> methodNames = {
		{ HttpMethod::GET, "GET" },
		{ HttpMethod::PUT, "PUT" },
		{ HttpMethod::POST, "POST" },
		{ HttpMethod::OPTIONS, "OPTIONS" },
		{ HttpMethod::DELETE, "DELETE" }
	};
	
	//! Return a string representing the method
	std::string toString(const HttpMethod& method)
	{
		const auto methodIt(methodNames.find(method));
		if (methodIt == methodNames.end())
			abort(); // if this arrives, the table above is not in sync with the enum content
		return methodIt->second;
	}
	
	//! Return the method from a string, or throw InvalidHttpMethod if it is not found
	HttpMethod fromMethodString(const std::string& method)
	{
		for (const auto& methodKV: methodNames)
			if (methodKV.second == method)
				return methodKV.first;
		throw InvalidHttpMethod(FormatableString("Invalid method %0").arg(method));
	}

} // namespace Aseba