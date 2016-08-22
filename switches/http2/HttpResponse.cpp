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

#include <iostream>
#include "HttpResponse.h"

using Aseba::Http::HttpRequest;
using Aseba::Http::HttpResponse;
using std::cerr;
using std::endl;
using std::map;
using std::ostringstream;
using std::string;

//! Constructor, sets status to OK and sets content type to JSON
HttpResponse::HttpResponse():
	status(HTTP_STATUS_OK)
{
	// these can be overwritten later
	headers["Content-Length"] = "";
	headers["Content-Type"] = "application/json";
	headers["Access-Control-Allow-Origin"] = "*";
}

//! Set this response as Server-Side Event
void HttpResponse::setSSE()
{
	headers["Content-Type"] = "text/event-stream";
	headers["Cache-Control"] = "no-cache";
	headers["Connection"] = "keep-alive";
}

//! Send the response
void HttpResponse::send(Dashel::Stream* stream)
{
	// send reply with status and headers first
	ostringstream reply;

	addStatusReply(reply);
	addHeadersReply(reply);

	const std::string replyString(reply.str());
	writeRaw(replyString.c_str(), replyString.size());

	// send content payload second
	writeRaw(content.c_str(), content.size());

	if(verbose) {
		cerr << getOriginatingRequest() << " Sent HTTP response with status " << status << " and " << content.size() << " byte(s) payload" << endl;
	}
}

void HttpResponse::addStatusReply(std::ostringstream& reply)
{
	if(originatingRequest->getProtocol() == "HTTP/1.0" || originatingRequest->getProtocol() == "HTTP/1.1") {
		reply << originatingRequest->getProtocol();
	} else {
		reply << "HTTP/1.1";
	}

	reply << " " << status << " " << toString(status);

	reply << "\r\n";
}

void HttpResponse::addHeadersReply(std::ostringstream& reply)
{
	map<string, string>::const_iterator end = headers.end();
	for(map<string, string>::const_iterator iter = headers.begin(); iter != end; ++iter) {
		if(iter->first == "Content-Length") { // override with actual size
			if(getHeader("Content-Type") != "text/event-stream") { // but only if this is not an event stream
				reply << iter->first << ": " << content.size() << "\r\n";
			}
		} else {
			reply << iter->first << ": " << iter->second << "\r\n";
		}
	}

	reply << "\r\n";
}
