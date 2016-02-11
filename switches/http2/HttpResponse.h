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
			typedef enum {
				HTTP_STATUS_OK = 200,
				HTTP_STATUS_CREATED = 201,
				HTTP_STATUS_BAD_REQUEST = 400,
				HTTP_STATUS_FORBIDDEN = 403,
				HTTP_STATUS_NOT_FOUND = 404,
				HTTP_STATUS_REQUEST_TIMEOUT = 408,
				HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
				HTTP_STATUS_NOT_IMPLEMENTED = 501,
				HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
			} HttpStatus;

			HttpResponse(const HttpRequest *originatingRequest);
			virtual ~HttpResponse();

			/**
			 * Sends the HTTP response using the output method defined in an inheriting class.
			 */
			virtual void send();

			virtual const HttpRequest *getOriginatingRequest() const { return originatingRequest; }
			virtual void setVerbose(bool verbose) { this->verbose = verbose; }

			virtual void setStatus(HttpStatus status) { this->status = status; }
			virtual HttpStatus getStatus() const { return status; }
			virtual void setHeader(const std::string& header, const std::string& value) { headers[header] = value; }
			virtual const std::map<std::string, std::string>& getHeaders() const { return headers; }
			virtual void setContent(const std::string& content) { this->content = content; }
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
			virtual void addStatusReply(std::ostringstream& reply);
			virtual void addHeadersReply(std::ostringstream& reply);

			virtual void writeRaw(const char *buffer, int length) = 0;

		private:
			const HttpRequest *originatingRequest;
			bool verbose;

			HttpStatus status;
			std::map<std::string, std::string> headers;
			std::string content;
	};

	class DashelHttpResponse : public HttpResponse
	{
		public:
			DashelHttpResponse(DashelHttpRequest *originatingRequest) :
				HttpResponse(originatingRequest),
				stream(originatingRequest->getStream())
			{

			}

			virtual ~DashelHttpResponse() {}

		protected:
			virtual void writeRaw(const char *buffer, int length)
			{
				stream->write(buffer, length);
				stream->flush();
			}

		private:
			Dashel::Stream *stream;
	};
} }

#endif
