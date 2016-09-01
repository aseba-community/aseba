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

#ifndef ASEBA_HTTP_RESPONSE
#define ASEBA_HTTP_RESPONSE

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <dashel/dashel.h>
#include "HttpStatus.h"
#include "HttpMethod.h"
#include "HttpProtocol.h"
#include "HttpResponse.h"
#include "Json.h"

namespace Aseba
{
	//! Represents HTTP responses being sent as a reply to incoming HTTP requests.
	class HttpResponse
	{
	public:
		//! Headers are a map of key-value as strings
		typedef std::map<std::string, std::string> Headers;
		
	protected:
		HttpResponse(const HttpStatus::Code status = HttpStatus::OK);
		
	public:
		static HttpResponse fromStatus(const HttpStatus::Code status = HttpStatus::OK);
		static HttpResponse fromPlainString(const std::string& content, const HttpStatus::Code status = HttpStatus::OK);
		static HttpResponse fromHTMLString(const std::string& content, const HttpStatus::Code status = HttpStatus::OK);
		static HttpResponse fromJSONString(const std::string& content, const HttpStatus::Code status = HttpStatus::OK);
		static HttpResponse fromJSON(const json& content, const HttpStatus::Code status = HttpStatus::OK);
		static HttpResponse createSSE();
		
		void send(Dashel::Stream* stream);
		operator json() const;
		
		std::string getHeader(const std::string& header) const;

	public:
		HttpProtocol protocol;
		HttpStatus::Code status;
		Headers headers;
		std::vector<uint8_t> content;
	};
} // namespace Aseba


#endif
