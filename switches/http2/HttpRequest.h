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

#ifndef ASEBA_HTTP_REQUEST
#define ASEBA_HTTP_REQUEST

#include <map>
#include <string>
#include <vector>
#include <dashel/dashel.h>

namespace Aseba
{
	class HttpResponse; // forward declaration

	/**
	 * Represents HTTP requests being parsed and responded to.
	 *
	 * Incoming requests start out without a HTTP response and are stalled in the HTTP interface
	 * until a response was created by using the HTTP request's respond() method. Since a request
	 * can have only one response, calling respond() repeatedly will simply return a reference
	 * to the same response, which makes it very easy to add things like multiple output headers.
	 */
	class HttpRequest
	{
	public:
		//! Headers are a map of key-value as strings
		typedef std::map<std::string, std::string> Headers;
		
		//! We limit the size of the request's content
		static const int CONTENT_BYTES_LIMIT = 40000;
		
		//! An HTTP error, providing a status code
		struct Error: public std::runtime_error
		{
			//! Create the error form a message and an HTTP status code
			Error(const std::string& whatArg, unsigned errorCode): std::runtime_error(whatArg), errorCode(errorCode) {}
			const unsigned errorCode; //!< 4xx/5xx error code
		};

	public:
		static HttpRequest receive(Dashel::Stream* stream);
		void dump(std::ostream& os);
	
	public:
		const std::string method;
		const std::string uri;
		const std::vector<std::string> tokenizedUri;
		const std::string protocol;
		const Headers headers;
		const std::vector<uint8_t> content;
	};
	
} // namespace Aseba

#endif // ASEBA_HTTP_REQUEST
