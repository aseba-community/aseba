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
#include "../../common/utils/utils.h"
#include "../../common/utils/FormatableString.h"
#include "HttpRequest.h"
#include "DashelUtils.h"
#include "Globals.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	void readRequestLine(Stream* stream, string& method, string& uri, string& protocol, strings& tokenizedUri)
	{
		const string requestLine(readLine(stream));
		const vector<string> parts(split<string>(requestLine, " "));

		if (parts.size() != 3)
			throw HttpRequest::Error(FormatableString("Header line does not have three parts, it has %0 parts").arg(parts.size()), HttpStatus::BAD_REQUEST);

		method = parts[0];
		uri = parts[1];
		protocol = trim(parts[2]);
		
		const set<string> supportedMethods = {
			"GET",
			"PUT",
			"POST",
			"OPTIONS",
			"DELETE"
		};
		if (supportedMethods.find(method) == supportedMethods.end())
			throw HttpRequest::Error(FormatableString("Unsupported method %0").arg(method), HttpStatus::METHOD_NOT_ALLOWED);

		const set<string> supportedProtocolVersions = {
			"HTTP/1.0",
			"HTTP/1.1"
		};
		if (supportedProtocolVersions.find(protocol) == supportedProtocolVersions.end())
			throw HttpRequest::Error(FormatableString("Unsupported HTTP protocol version %0").arg(method), HttpStatus::HTTP_VERSION_NOT_SUPPORTED);

		// Also allow %2F as URL part delimiter (see Scratch v430)
		string::size_type n = 0;
		while((n = uri.find("%2F", n)) != string::npos)
		{
			uri.replace(n, 3, "/"), n += 1;
		}

		tokenizedUri = split<string>(uri, "/");
		
		// eat leading empty tokens, but leave at least one
		while (tokenizedUri.size() > 1 && tokenizedUri[0].size() == 0)
		{
			tokenizedUri.erase(tokenizedUri.begin(), tokenizedUri.begin() + 1);
		}
	}
	
	void readHeaders(Stream* stream, HttpRequest::Headers& headers)
	{
		while (true)
		{
			const string headerLine(trim(readLine(stream)));

			if (headerLine.empty()) { // header section ends with empty line
				break;
			}

			const size_t firstColon(headerLine.find(": "));

			if (firstColon != string::npos)
			{
				string header = headerLine.substr(0, firstColon);
				string value = headerLine.substr(firstColon + 2);
				headers[header] = value;
			}
			else
				throw HttpRequest::Error(FormatableString("Invalid header line: %0").arg(headerLine), HttpStatus::BAD_REQUEST);
		}
	}

	void readContent(Stream* stream, const HttpRequest::Headers& headers, vector<uint8_t>& content)
	{
		auto contentLengthIt(headers.find("Content-Length"));
		if (contentLengthIt == headers.end())
			throw HttpRequest::Error("Invalid header, missing \"Content-Length\" field", HttpStatus::BAD_REQUEST);
		
		// to make our life easier, we will require the presence of Content-Length in order to parse content
		const size_t contentLength(atoi(contentLengthIt->second.c_str()));
		if (contentLength > HttpRequest::CONTENT_BYTES_LIMIT)
			throw HttpRequest::Error(FormatableString("Request content length is %0 which exceeds limit %1").arg(contentLength).arg(HttpRequest::CONTENT_BYTES_LIMIT), HttpStatus::BAD_REQUEST);
			
		if (contentLength > 0)
		{
			content.resize(contentLength);
			stream->read(&content[0], contentLength);
		}
	}
	
	//! Receive the HTTP request and parse its headers and content.
	HttpRequest HttpRequest::receive(Dashel::Stream* stream)
	{
		string method;
		string uri;
		strings tokenizedUri;
		string protocol;
		readRequestLine(stream, method, uri, protocol, tokenizedUri);
		
		Headers headers;
		readHeaders(stream, headers);
		
		vector<uint8_t> content;
		readContent(stream, headers, content);
		
		HttpRequest request = {
			method,
			uri,
			tokenizedUri,
			protocol,
			headers,
			content
		};
		
		if (_globals.verbose)
		{
			cerr << " Received HTTP request" << endl;
			request.dump(cerr);
		}
		
		return request;
	}

	//! Write the request as JSON
	void HttpRequest::dump(ostream& os)
	{
		// TODO: write json
	}
};
