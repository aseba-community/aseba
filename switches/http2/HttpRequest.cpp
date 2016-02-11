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
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../../common/utils/utils.h"

using Aseba::Http::DashelHttpRequest;
using Aseba::Http::DashelHttpResponse;
using Aseba::Http::HttpRequest;
using Aseba::Http::HttpResponse;
using std::cerr;
using std::endl;
using std::map;
using std::string;
using std::vector;

HttpRequest::HttpRequest() :
	verbose(false),
	valid(false),
	blocking(false),
	response(NULL)
{

}

HttpRequest::~HttpRequest()
{
	delete response;
}


bool HttpRequest::receive()
{
	valid = false;

	if(!readRequestLine()) {
		return false;
	}

	if(!readHeaders()) {
		return false;
	}

	if(!readContent()) {
		return false;
	}

	valid = true;

	if(verbose) {
		cerr << this << " Received valid " << getProtocol() << " " << getMethod() << " request for " << getUri() << " with " << content.size() << " byte(s) payload ";

		cerr << "(headers:";

		map<string, string>::const_iterator end = headers.end();
		for(map<string, string>::const_iterator iter = headers.begin(); iter != end; ++iter) {
			cerr << " " << iter->first << "=" << iter->second;
		}

		cerr << ")" << endl;
	}

	return true;
}

HttpResponse& HttpRequest::respond()
{
	if(response == NULL) {
		response = createResponse();
		response->setVerbose(verbose);
	}

	return *response;
}

bool HttpRequest::readRequestLine()
{
	string requestLine = readLine();
	vector<string> parts = split<string>(requestLine, " ");

	if(parts.size() != 3) {
		if(verbose) {
			cerr << this << " Header line doesn't have three parts" << endl;
		}
		return false;
	}

	method = parts[0];
	uri = parts[1];
	protocol = trim(parts[2]);

	if(!(method.find("GET", 0) == 0 || method.find("PUT", 0) == 0 || method.find("POST", 0) == 0 || method.find("OPTIONS", 0) == 0)) {
		if(verbose) {
			cerr << this << " Unsupported method" << endl;
		}
		return false;
	}

	if(!(protocol == "HTTP/1.0" || protocol == "HTTP/1.1")) {
		if(verbose) {
			cerr << this << " Unsupported HTTP protocol version" << endl;
		}
		return false;
	}

	// Also allow %2F as URL part delimiter (see Scratch v430)
	std::string::size_type n = 0;
	while((n = uri.find("%2F", n)) != std::string::npos) {
		uri.replace(n, 3, "/"), n += 1;
	}

	tokens = split<string>(uri, "/");
	// eat leading empty tokens, but leave at least one
	while(tokens.size() > 1 && tokens[0].size() == 0) {
		tokens.erase(tokens.begin(), tokens.begin() + 1);
	}

	return true;
}

bool HttpRequest::readHeaders()
{
	headers.clear();

	while(true) {
		const string headerLine(trim(readLine()));

		if(headerLine.empty()) { // header section ends with empty line
			break;
		}

		size_t firstColon = headerLine.find(": ");

		if(firstColon != string::npos) {
			string header = headerLine.substr(0, firstColon);
			string value = headerLine.substr(firstColon + 2);

			headers[header] = value;
		} else {
			if(verbose) {
				cerr << this << " Invalid header line: " << headerLine << endl;
			}
			return false;
		}
	}

	return true;
}

bool HttpRequest::readContent()
{
	// to make our life easier, we will require the presence of Content-Length in order to parse content
	int contentLength = atoi(headers["Content-Length"].c_str());
	contentLength = (contentLength > CONTENT_BYTES_LIMIT) ? CONTENT_BYTES_LIMIT : contentLength; // truncate at limit

	if(contentLength > 0) {
		char *buffer = new char[contentLength];
		readRaw(buffer, contentLength);
		content = string(buffer, contentLength);
		delete[] buffer;
	} else {
		content = "";
	}

	return true;
}

DashelHttpRequest::DashelHttpRequest(Dashel::Stream *stream_) :
	stream(stream_)
{


}

DashelHttpRequest::~DashelHttpRequest()
{

}

HttpResponse *DashelHttpRequest::createResponse()
{
	return new DashelHttpResponse(this);
}

std::string DashelHttpRequest::readLine()
{
	char c;
	std::string line;
	do {
		stream->read(&c, 1);
		line += c;
	} while(c != '\n');

	return line;
}

void DashelHttpRequest::readRaw(char *buffer, int size)
{
	stream->read(buffer, size);
}
