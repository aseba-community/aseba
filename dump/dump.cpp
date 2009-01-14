/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
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
	private:
		bool rawTime; //!< should displayed timestamps be of the form sec:usec since 1970
	
	public:
		Dump(const string& target, bool rawTime) :
			rawTime(rawTime)
		{
			cout << "Connected to " << connect(target)->getTargetName() << endl;
		}
	
	protected:
		
		void incomingData(Stream *stream)
		{
			Message *message = Message::receive(stream);
			
			dumpTime(cout, rawTime);
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
	bool rawTime = false;
	
	if (argc >= 2)
		target = argv[1];
	// TODO: fix this with a more generic command line handling, see switch
	if ((argc >= 3) && (strcmp(argv[2], "--rawtime") == 0))
	{
		rawTime = true;
	}
	
	try
	{
		Aseba::Dump dump(target, rawTime);
		dump.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
