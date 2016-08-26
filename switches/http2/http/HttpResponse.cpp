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
#include "../Globals.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;

	//! Constructor, sets status to OK and sets content type to JSON
	HttpResponse::HttpResponse():
		protocol(HttpProtocol::HTTP_1_1),
		status(HttpStatus::OK)
	{
		// these can be overwritten later
		headers["Content-Length"] = "";
		headers["Content-Type"] = "application/json";
		headers["Access-Control-Allow-Origin"] = "*";
	}

	//! Set headers to make this response Server-Side Event declaration for the client
	void HttpResponse::setServerSideEvent()
	{
		headers["Content-Type"] = "text/event-stream";
		headers["Cache-Control"] = "no-cache";
		headers["Connection"] = "keep-alive";
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
		
		if (_globals.dump)
		{
			dumpTime(cout, _globals.rawTime);
			cout << "HTTP | On stream " << stream->getTargetName() << ", sent ";
			dump(cout);
			cout << endl;
		}
		else
			LOG_VERBOSE << "HTTP | On stream " << stream->getTargetName() << ", sent response " << status << " and " << content.size() << " byte(s) payload" << endl;
	}
	
	//! Write the response as JSON
	void HttpResponse::dump(ostream& os)
	{
		os << "{ ";
		os << "protocol: \"" << toString(protocol) << "\", ";
		os << "status: \"" << HttpStatus::toString(status) << "\", ";
		os << "headers: { ";
		unsigned i(0);
		for (auto p: headers)
		{
			if (i++ != 0)
				os << ", ";
			os << p.first << ": \"" << p.second << "\"";
		}
		os << " }";
		if (content.size() > 0)
		{
			// note: as this is debug dump, the escaping of " and non-printable characters in content is not handled
			os << ", content: \"";
			os.write(reinterpret_cast<const char*>(&content[0]), content.size());
			os << "\"";
		}
		os << " }";
	}
	
} // namespace Aseba
