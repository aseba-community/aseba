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

HttpResponse::HttpResponse(const HttpRequest *originatingRequest_) :
	originatingRequest(originatingRequest_),
	verbose(false),
	status(HTTP_STATUS_OK)
{
	// add some default headers, these can be overwritten later
	setHeader("Content-Length", "");
	setHeader("Content-Type", "application/json");
	setHeader("Access-Control-Allow-Origin", "*");

	if(originatingRequest->getHeader("Connection") == "keep-alive") {
		setHeader("Connection", "keep-alive");
	}
}

HttpResponse::~HttpResponse()
{

}

void HttpResponse::send()
{
	// send reply with status and headers first
	ostringstream reply;

	addStatusReply(reply);
	addHeadersReply(reply);

	std::string replyString(reply.str());
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

	reply << " " << status << " ";

	switch(status) {
		case HTTP_STATUS_OK:
			reply << "OK";
		break;
		case HTTP_STATUS_CREATED:
			reply << "Created";
		break;
		case HTTP_STATUS_BAD_REQUEST:
			reply << "Bad Request";
		break;
		case HTTP_STATUS_FORBIDDEN:
			reply << "Forbidden";
		break;
		case HTTP_STATUS_NOT_FOUND:
			reply << "Not Found";
		break;
		case HTTP_STATUS_REQUEST_TIMEOUT:
			reply << "Request Timeout";
		break;
		case HTTP_STATUS_INTERNAL_SERVER_ERROR:
			reply << "Internal Server Error";
		break;
		case HTTP_STATUS_NOT_IMPLEMENTED:
			reply << "Not Implemented";
		break;
		case HTTP_STATUS_SERVICE_UNAVAILABLE:
			reply << "Service Unavailable";
		break;
		default:
			reply << "Unknown";
		break;
	}

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
