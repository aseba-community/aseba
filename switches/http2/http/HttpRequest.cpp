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
#include <cctype>
#include <algorithm>
#include <string>
#include "../../../common/utils/utils.h"
#include "../../../common/utils/FormatableString.h"
#include "HttpRequest.h"
#include "../DashelUtils.h"
#include "../Globals.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;

	void readRequestLine(Stream* stream, HttpMethod& method, string& uri, HttpProtocol& protocol, strings& tokenizedUri)
	{
		const string requestLine(readLine(stream));
		const vector<string> parts(split<string>(requestLine, " "));

		// do we have the right number of elements in the header?
		if (parts.size() != 3)
			throw HttpRequest::Error(FormatableString("Header line does not have three parts, it has %0 parts").arg(parts.size()), HttpStatus::BAD_REQUEST);

		// read the method
		try
		{
			method = fromMethodString(parts[0]);
		}
		catch (const InvalidHttpMethod& e)
		{
			throw HttpRequest::Error(FormatableString("Unsupported method %0").arg(e.what()), HttpStatus::METHOD_NOT_ALLOWED);
		}

		// read the URI
		uri = parts[1];
		// Also allow %2F as URL part delimiter (see Scratch v430)
		string::size_type n = 0;
		while ((n = uri.find("%2F", n)) != string::npos)
		{
			uri.replace(n, 3, "/"), n += 1;
		}
		tokenizedUri = split<string>(uri, "/");
		// eat leading empty tokens, but leave at least one
		while (tokenizedUri.size() > 1 && tokenizedUri[0].size() == 0)
		{
			tokenizedUri.erase(tokenizedUri.begin(), tokenizedUri.begin() + 1);
		}

		// read the protocol
		try
		{
			protocol = fromProtocolString(trim(parts[2]));
		}
		catch (const InvalidHttpProtocol& e)
		{
			throw HttpRequest::Error(FormatableString("Unsupported HTTP protocol version %0").arg(e.what()), HttpStatus::HTTP_VERSION_NOT_SUPPORTED);
		}
	}

	void readHeaders(Stream* stream, HttpRequest::Headers& headers)
	{
		while (true)
		{
			const string headerLine(trim(readLine(stream)));

			if (headerLine.empty())
			{ // header section ends with empty line
				break;
			}

			const size_t firstColon(headerLine.find(": "));

			if (firstColon != string::npos)
			{
				string header = headerLine.substr(0, firstColon);
				transform(header.begin(), header.end(), header.begin(), ::tolower);
				string value = headerLine.substr(firstColon + 2);
				headers[header] = value;
			}
			else
				throw HttpRequest::Error(FormatableString("Invalid header line: %0").arg(headerLine), HttpStatus::BAD_REQUEST);
		}
	}

	void readContent(Stream* stream, const HttpRequest::Headers& headers, vector<uint8_t>& content)
	{
		// if this function is called, there must be content to read
		auto contentLengthIt(headers.find("content-length"));
		assert(contentLengthIt != headers.end());
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
		HttpMethod method;
		string uri;
		strings tokenizedUri;
		HttpProtocol protocol;
		readRequestLine(stream, method, uri, protocol, tokenizedUri);

		Headers headers;
		readHeaders(stream, headers);

		vector<uint8_t> content;
		if (headers.find("content-length") != headers.end())
			readContent(stream, headers, content);

		HttpRequest request = {
			method,
			uri,
			tokenizedUri,
			protocol,
			headers,
			content
		};

		LOG_VERBOSE << "HTTP | On stream " << stream->getTargetName() << ", received " << toString(method) << " " << uri << " with " << content.size() << " byte(s) payload" << endl;
		LOG_DUMP << "HTTP | On stream " << stream->getTargetName() << ": " << json(request) << endl;

		return request;
	}

	//! Write the request as JSON
	HttpRequest::operator json() const
	{
		// if content is printable, show as string, otherwise as int array
		json serializedContent;
		if (all_of(content.begin(), content.end(), [](uint8_t c) { return isprint(c); }))
			serializedContent = string(reinterpret_cast<const char*>(&content[0]), content.size());
		else
			serializedContent = content;
		return {
			{ "method", toString(method) },
			{ "protocol", toString(protocol) },
			{ "uri", uri },
			{ "headers", headers },
			{ "content", serializedContent }
		};
	}

	//! Return a header or an empty string if not present
	std::string HttpRequest::getHeader(const std::string& header) const
	{
		string lcHeader;
		transform(header.begin(), header.end(), back_inserter(lcHeader), ::tolower);
		const auto headerIt(headers.find(lcHeader));
		if (headerIt != headers.end())
			return headerIt->second;
		else
			return "";
	}
};
