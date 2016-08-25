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
#include "../../../common/utils/FormatableString.h"
#include "HttpStatus.h"

namespace Aseba
{
	using namespace std;
	
	//! Construct an exception telling a code is invalid
	HttpStatus::InvalidCode::InvalidCode(Code code):
		runtime_error(FormatableString("Invalid code %0").arg(code))
	{
	}
	
	//! List of names of all known HTTP status codes
	static const map<HttpStatus::Code, string> statusNames = {
		{ HttpStatus::OK, "OK" },
		{ HttpStatus::CREATED, "Created" },
		{ HttpStatus::BAD_REQUEST, "Bad Request" },
		{ HttpStatus::FORBIDDEN, "Forbidden" },
		{ HttpStatus::NOT_FOUND, "Not Found" },
		{ HttpStatus::METHOD_NOT_ALLOWED, "Method Not Allowed" },
		{ HttpStatus::REQUEST_TIMEOUT, "Request Timeout" },
		{ HttpStatus::INTERNAL_SERVER_ERROR, "Internal Server Error" },
		{ HttpStatus::NOT_IMPLEMENTED, "Not Implemented" },
		{ HttpStatus::SERVICE_UNAVAILABLE, "Service Unavailable" },
		{ HttpStatus::HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"}
	};
	
	//! Convert an HTTP status code into a string
	string HttpStatus::toString(Code code)
	{
		const auto statusIt(statusNames.find(code));
		if (statusIt == statusNames.end())
			throw InvalidCode(code);
		return statusIt->second;
	}
	
} // namespace Aseba
