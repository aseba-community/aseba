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
#include <algorithm>
#include <cctype>
#include "HttpResponse.h"
#include "../Globals.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;

	//! Constructor, protected to force the use of factories that set correct content-type
	HttpResponse::HttpResponse(const HttpStatus::Code status):
		protocol(HttpProtocol::HTTP_1_1),
		status(status)
	{
		// these can be overwritten later
		headers.emplace("Content-Length", "");
		headers.emplace("Access-Control-Allow-Origin", "*");
	}
	
	//! Factory, from status only
	HttpResponse HttpResponse::fromStatus(const HttpStatus::Code status)
	{
		return HttpResponse(status);
	}
	
	//! Factory, from a plain string
	HttpResponse HttpResponse::fromPlainString(const std::string& content, const HttpStatus::Code status)
	{
		HttpResponse response(status);
		response.headers.emplace("Content-Type", "text/plain; charset=UTF-8");
		response.content.insert(response.content.end(), content.begin(), content.end());
		return response;
	}
	
	//! Factory, from an UTF-8 encoded HTML string
	HttpResponse HttpResponse::fromHTMLString(const std::string& content, const HttpStatus::Code status)
	{
		HttpResponse response(status);
		response.headers.emplace("Content-Type", "text/html; charset=UTF-8");
		response.content.insert(response.content.end(), content.begin(), content.end());
		return response;
	}
	
	//! Factory, from a JSON string
	HttpResponse HttpResponse::fromJSONString(const std::string& content, const HttpStatus::Code status)
	{
		HttpResponse response(status);
		response.headers.emplace("Content-Type", "application/json"); // JSON content is UTF-8 encoded
		response.content.insert(response.content.end(), content.begin(), content.end());
		return response;
	}
	
	//! Factory, from a JSON object
	HttpResponse HttpResponse::fromJSON(const json& content, const HttpStatus::Code status)
	{
		HttpResponse response(status);
		response.headers.emplace("Content-Type", "application/json"); // JSON content is UTF-8 encoded
		const string serializedContent(content.dump());
		response.content.insert(response.content.end(), serializedContent.begin(), serializedContent.end());
		return response;
	}
	
	//! Factory, creates a Server-Side Event declaration for the client
	HttpResponse HttpResponse::createSSE()
	{
		HttpResponse response;
		response.headers["Content-Type"] = "text/event-stream";
		response.headers["Cache-Control"] = "no-cache";
		response.headers["Connection"] = "keep-alive";
		return response;
	}
	
	//! Return a header or an empty string if not present
	std::string HttpResponse::getHeader(const std::string& header) const
	{
		const auto headerIt(headers.find(header));
		if (headerIt != headers.end())
			return headerIt->second;
		else
			return "";
	}

	//! Send the response
	void HttpResponse::send(Dashel::Stream* stream)
	{
		ostringstream reply;
		
		// write status
		reply << toString(protocol) << " " << unsigned(status) << " " << HttpStatus::toString(status) << "\r\n";
		
		// write headers
		for (const auto& headerKV: headers)
		{
			if (headerKV. first == "Content-Length") // override with actual size
			{
				if (getHeader("Content-Type") != "text/event-stream") // but only if this is not an event stream
					reply << headerKV.first << ": " << content.size() << "\r\n";
			}
			else
				reply << headerKV.first << ": " << headerKV.second << "\r\n";
		}
		reply << "\r\n";

		// write reply
		const std::string replyString(reply.str());
		stream->write(replyString.c_str(), replyString.size());

		// send content payload
		stream->write(&content[0], content.size());
		stream->flush();
		
		if (status >= 400)
		{
			LOG_ERROR << "HTTP | On stream " << stream->getTargetName() << ", request error " << status << " " << HttpStatus::toString(status) << ": " << string(reinterpret_cast<const char*>(&content[0]), content.size()) << endl;
		}
		else
		{
			LOG_VERBOSE << "HTTP | On stream " << stream->getTargetName() << ", sent response " << status << " and " << content.size() << " byte(s) payload" << endl;
		}
		LOG_DUMP << "HTTP | On stream " << stream->getTargetName() << ": " << json(*this) << endl;
	}
	
	//! Write the response as JSON
	HttpResponse::operator json() const
	{
		// if content is printable, show as string, otherwise as int array
		json serializedContent;
		if (all_of(content.begin(), content.end(), [](uint8_t c) { return isprint(c); }))
			serializedContent = string(reinterpret_cast<const char*>(&content[0]), content.size());
		else
			serializedContent = content;
		return {
			{ "protocol", toString(protocol) },
			{ "status", HttpStatus::toString(status) },
			{ "headers", headers },
			{ "content", serializedContent }
		};
	}
	
} // namespace Aseba
