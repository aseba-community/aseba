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

#include <ostream>
#include "HttpDispatcher.h"
#include "../Globals.h"
#include "../Switch.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	HttpDispatcher::HttpDispatcher():
		serverPort(3000)
	{
		
	}
	
	string HttpDispatcher::name() const
	{
		return "HTTP Dispatcher";
	}
	
	void HttpDispatcher::dumpArgumentsDescription(ostream &stream) const
	{
		stream << "  HTTP module, provides access to the network from a web application\n";
		stream << "    -w, --http      : listens to incoming HTTP connections on this port (default: 3000)\n";
	}
	
	ArgumentDescriptions HttpDispatcher::describeArguments() const
	{
		return {
			{ "-w", 1 },
			{ "--http", 1 }
		};
	}
	
	void HttpDispatcher::processArguments(Switch* asebaSwitch, const Arguments& arguments)
	{
		strings args;
		if (arguments.find("-w", &args) || arguments.find("--http", &args))
			serverPort = stoi(args[0]);
		// TODO: add an option to avoid creating an HTTP port, or maybe not creating this module?
		// TODO: catch the connection exception for HTTP port, decide what to do
		ostringstream oss;
		oss << "tcpin:port=" << serverPort;
		Stream* serverStream(asebaSwitch->connect(oss.str()));
		LOG_VERBOSE << "HTTP | HTTP Listening stream on " << serverStream->getTargetName() << endl;
	}
	
	bool HttpDispatcher::connectionCreated(Switch* asebaSwitch, Dashel::Stream * stream)
	{
		// take ownership of the stream if it was created by our server stream
		if (stream->getTarget().isSet("connectionPort") &&
			stoi(stream->getTargetParameter("connectionPort")) == serverPort)
			return true;
		return false;
	}
	
	void HttpDispatcher::incomingData(Switch* asebaSwitch, Dashel::Stream * stream)
	{
		HttpRequest request(HttpRequest::receive(stream));
		
		
		HttpResponse response;
		string rs("<html><body>hello world</body></html>");
		response.content.insert(response.content.end(), rs.begin(), rs.end());
		
		response.send(stream);
		
		// TODO: handle exceptions
		// TODO: dispatch
	}
	
	void HttpDispatcher::connectionClosed(Switch* asebaSwitch, Dashel::Stream * stream)
	{
		
	}
	
	void HttpDispatcher::processMessage(Switch* asebaSwitch, const Message& message)
	{
		
	}
	
} // namespace Aseba
