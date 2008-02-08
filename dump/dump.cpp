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
#include <dashel/dashel.h>
#include <time.h>
#include <iostream>

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	/**
	\defgroup dump Message dumper
	*/
	/*@{*/
	
	//! A simple message dumper.
	//! This class calls Aseba::Message::dump() for each message
	class Dump : public Hub
	{
	public:
		Dump(const string& target)
		{
			connect(target);
		}
	
	protected:
		void incomingConnection(Stream *stream)
		{
			cout << "Connected to " << stream->getTargetName() << endl;
		}
		
		void incomingData(Stream *stream)
		{
			Message *message = Message::receive(stream);
			
			dumpTime(cout);
			if (message)
				message->dump(cout);
			else
				cout << "unknown message received";
			cout << std::endl;
		}
		
		void connectionClosed(Stream *stream, bool abnormal)
		{
			dumpTime(cerr);
			cout << "Connection closed to " << stream->getTargetName();
			if (abnormal)
				cout << " : " << stream->getFailReason();
			cout << endl;
			stop();
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
	catch(Dashel::DashelException e)
	{
		std::cerr << e.reason << " " << e.sysMessage << std::endl;
	}
	
	return 0;
}
