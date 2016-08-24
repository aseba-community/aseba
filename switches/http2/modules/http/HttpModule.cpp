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
#include "HttpModule.h"
#include "../../Globals.h"
#include "../../Switch.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	HttpModule::HttpModule():
		serverPort(3000)
	{
		
	}
	
	string HttpModule::name() const
	{
		return "HTTP";
	}
	
	void HttpModule::dumpArgumentsDescription(ostream &stream) const
	{
		stream << "  HTTP module, provides access to the network from a web application\n";
		stream << "    -w, --http      : listens to incoming HTTP connections on this port (default: 3000)\n";
	}
	
	ArgumentDescriptions HttpModule::describeArguments() const
	{
		return {
			{ "-w", 1 },
			{ "--http", 1 }
		};
	}
	
	void HttpModule::processArguments(Switch* asebaSwitch, const Arguments& arguments)
	{
		strings args;
		if (arguments.find("-w", &args) || arguments.find("--http", &args))
			serverPort = stoi(args[0]);
		// TODO: add an option to avoid creating an HTTP port
		// TODO: catch the connection exception for HTTP port, decide what to do
		ostringstream oss;
		oss << "tcpin:port=" << serverPort;
		Stream* serverStream(asebaSwitch->connect(oss.str()));
		LOG_VERBOSE << "HTTP | HTTP Listening stream on " << serverStream->getTargetName() << endl;
	}
	
	bool HttpModule::connectionCreated(Dashel::Stream * stream)
	{
		// take ownership of the stream if it was created by our server stream
		if (stream->getTarget().isSet("connectionPort") &&
			stoi(stream->getTargetParameter("connectionPort")) == serverPort)
			return true;
		return false;
	}
	
	void HttpModule::incomingData(Dashel::Stream * stream)
	{
		
	}
	
	void HttpModule::connectionClosed(Dashel::Stream * stream)
	{
		
	}
	
	void HttpModule::processMessage(const Message& message)
	{
		
	}
	
} // namespace Aseba
