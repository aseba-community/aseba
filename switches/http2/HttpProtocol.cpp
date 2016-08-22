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
#include "HttpProtocol.h"

namespace Aseba
{
	using namespace std;
	
	//! List of names of all known HTTP protocols
	static const map<HttpProtocol, string> protocolNames = {
		{ HttpProtocol::HTTP_1_0, "HTTP/1.0" },
		{ HttpProtocol::HTTP_1_1, "HTTP/1.1" }
	};
	
	//! Return a string representing the protocol
	std::string toString(const HttpProtocol& protocol)
	{
		const auto protocolIt(protocolNames.find(protocol));
		if (protocolIt == protocolNames.end())
			abort(); // if this arrives, the table above is not in sync with the enum content
		return protocolIt->second;
	}
	
	//! Return the protocol from a string, or throw InvalidHttpProtocol if it is not found
	HttpProtocol fromProtocolString(const std::string& protocol)
	{
		for (const auto& protocolKV: protocolNames)
			if (protocolKV.second == protocol)
				return protocolKV.first;
		throw InvalidHttpProtocol(FormatableString("Invalid protocol %0").arg(protocol));
	}

} // namespace Aseba