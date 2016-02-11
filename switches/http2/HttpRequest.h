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

namespace Aseba { namespace Http
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
			static const int CONTENT_BYTES_LIMIT = 40000;

			HttpRequest();
			virtual ~HttpRequest();

			/**
			 * Receive the HTTP request and parse its headers and content.
			 *
			 * Returns true if successful.
			 */
			virtual bool receive();

			/**
			 * Returns a HTTP response for this object. Repeated calls to this method will
			 * always return the same HTTP response reference. The HTTP request is stalled
			 * in the HTTP interface until this method is called for the first time.
			 */
			virtual HttpResponse& respond();

			virtual bool isResponseReady() const { return !blocking && response != NULL; }

			virtual void setVerbose(bool verbose) { this->verbose = verbose; }
			virtual bool isValid() const { return valid; }
			virtual void setBlocking(bool blocking) { this->blocking = blocking; }
			virtual bool isBlocking() { return blocking; }

			virtual const std::string& getMethod() const { return method; }
			virtual const std::string& getUri() const { return uri; }
			virtual const std::string& getProtocol() const { return protocol; }
			virtual const std::vector<std::string>& getTokens() const { return tokens; }
			virtual const std::map<std::string, std::string>& getHeaders() const { return headers; }
			virtual const std::string& getContent() const { return content; }

			std::string getHeader(const std::string& header) const {
				std::map<std::string, std::string>::const_iterator query = headers.find(header);

				if(query != headers.end()) {
					return query->second;
				} else {
					return "";
				}
			}

		protected:
			virtual bool readRequestLine();
			virtual bool readHeaders();
			virtual bool readContent();

			virtual HttpResponse *createResponse() = 0;

			virtual std::string readLine() = 0;
			virtual void readRaw(char *buffer, int size) = 0;

		private:
			static std::string trim(const std::string& s)
			{
				static const char *whitespaceChars = "\n\r\t ";
				std::string::size_type start = s.find_first_not_of(whitespaceChars);
				std::string::size_type end = s.find_last_not_of(whitespaceChars);

				return start != std::string::npos ? s.substr(start, 1 + end - start) : "";
			}

			bool verbose;
			bool valid;
			bool blocking;
			HttpResponse *response;

			std::string method;
			std::string uri;
			std::string protocol;
			std::vector<std::string> tokens;
			std::map<std::string, std::string> headers;
			std::string content;
	};

	class DashelHttpRequest : public HttpRequest
	{
		public:
    		DashelHttpRequest(Dashel::Stream *stream);
			virtual ~DashelHttpRequest();

			virtual Dashel::Stream *getStream() { return stream; }

		protected:
			virtual HttpResponse *createResponse();

			virtual std::string readLine();
			virtual void readRaw(char *buffer, int size);

		private:
			Dashel::Stream *stream;
	};
} }

#endif
