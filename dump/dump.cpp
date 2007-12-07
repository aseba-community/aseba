/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../msg/msg.h"
#include "../utils/utils.h"
#include <dashel/streams.h>
#include <time.h>
#include <iostream>

namespace Aseba
{
	using namespace Streams;
	using namespace std;
	
	/**
	\defgroup dump Message dumper
	*/
	/*@{*/
	
	//! A simple message dumper.
	//! This class calls Aseba::Message::dump() for each message
	class Dump : public Client
	{
	public:
		Dump(const string& target) : Client(target)
		{
		}
	
	protected:
		virtual void incomingData(Stream *stream)
		{
			Message *message = Message::receive(stream);
			
			dumpTime(cout);
			if (message)
				message->dump(cout);
			else
				cout << "unknown message received";
			cout << std::endl;
		}
		
		virtual void connectionClosed(Stream *stream)
		{
			dumpTime(cerr);
			cout << stream->getTargetName() << " closed connection" << endl;
		}
	};
	
	/*@}*/
}

int main(int argc, char *argv[])
{
	const char *target = ASEBA_DEFAULT_TARGET;
	
	if (argc >= 2)
		target = argv[1];
	
	try
	{
		Aseba::Dump dump(target);
		dump.run();
	}
	catch (Streams::InvalidTargetDescription e)
	{
		std::cerr << "Invalid target description " << target << std::endl;
	}
	catch (Streams::ConnectionError e)
	{
		std::cerr << "Can't connect to " << target << std::endl;
	}
	
	
	return 0;
}
