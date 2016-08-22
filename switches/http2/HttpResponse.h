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

#include "HttpRequest.h"

namespace Aseba { namespace Http
{
	/**
	 * Represents HTTP responses being sent as a reply to incoming HTTP requests.
	 */
	class HttpResponse
	{
	public:
		//! Headers are a map of key-value as strings
		typedef std::map<std::string, std::string> Headers;
		
	public:
		HttpResponse();
		void send(Dashel::Stream* stream);
		void dump(std::ostream& os);

	protected:
		virtual void addStatusReply(std::ostringstream& reply);
		virtual void addHeadersReply(std::ostringstream& reply);

		virtual void writeRaw(const char *buffer, int length) = 0;

	public:
		HttpProtocol protocol;
		HttpStatus status;
		Headers headers;
		std::vector<uint8_t> content;
	};
} }

#endif
