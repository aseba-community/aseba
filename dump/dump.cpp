/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

#include "../msg/msg.h"
#include "../utils/utils.h"
#include <dashel/dashel.h>
#include <time.h>
#include <iostream>
#include <cstring>

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
		Dump(bool rawTime) :
			rawTime(rawTime)
		{
		}
	
	protected:
		
		void connectionCreated(Stream *stream)
		{
			dumpTime(cout, rawTime);
			cout << stream->getTargetName()  << " connection created." << endl;
		}
		
		void incomingData(Stream *stream)
		{
			Message *message = Message::receive(stream);
			
			dumpTime(cout, rawTime);
			cout << stream->getTargetName()  << " ";
			if (message)
				message->dump(wcout);
			else
				cout << "unknown message received";
			cout << endl;
		}
		
		void connectionClosed(Stream *stream, bool abnormal)
		{
			dumpTime(cout);
			cout << stream->getTargetName() << " connection closed";
			if (abnormal)
				cout << " : " << stream->getFailReason();
			cout << "." << endl;
		}
	};
	
	/*@}*/
}


//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba dump, print the content of aseba messages, usage:\n";
	stream << programName << " [options] [targets]*\n";
	stream << "Options:\n";
	stream << "--rawtime       : shows time in the form of sec:usec since 1970\n";
	stream << "-h, --help      : shows this help\n";
	stream << "Targets are any valid Dashel targets." << std::endl;
}

int main(int argc, char *argv[])
{
	bool rawTime = false;
	std::vector<std::string> targets;
	
	int argCounter = 1;
	
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		
		if (strcmp(arg, "--rawtime") == 0)
		{
			rawTime = true;
		}
		else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
		{
			dumpHelp(std::cout, argv[0]);
			return 0;
		}
		else
		{
			targets.push_back(argv[argCounter]);
		}
		argCounter++;
	}
	
	if (targets.empty())
		targets.push_back(ASEBA_DEFAULT_TARGET);
	
	try
	{
		Aseba::Dump dump(rawTime);
		for (size_t i = 0; i < targets.size(); i++)
			dump.connect(targets[i]);
		dump.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
