/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
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
#include <cstring>

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	/**
	\defgroup rec Message recorder
	*/
	/*@{*/
	
	//! A message recorder.
	//! This class saves user messages
	class Recorder : public Hub
	{
	protected:
		
		void incomingData(Stream *stream)
		{
			Message *message = Message::receive(stream);
			UserMessage *userMessage = dynamic_cast<UserMessage *>(message);
			if (userMessage)
			{
				dumpTime(cout, true);
				cout << userMessage->source << " ";
				cout << userMessage->type << " ";
				cout << userMessage->data.size() << " ";
				for (UserMessage::DataVector::const_iterator it = userMessage->data.begin(); it != userMessage->data.end(); ++it)
					cout << *it << " ";
				cout << endl;
			}
		}
	};
	
	/*@}*/
}


//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba rec, record the user messages to stdout for later replay, usage:\n";
	stream << programName << " [options] [targets]*\n";
	stream << "Options:\n";
	stream << "-h, --help      : shows this help\n";
	stream << "Targets are any valid Dashel targets." << std::endl;
}

int main(int argc, char *argv[])
{
	std::vector<std::string> targets;
	
	int argCounter = 1;
	
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		
		if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
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
		Aseba::Recorder recorder;
		for (size_t i = 0; i < targets.size(); i++)
			recorder.connect(targets[i]);
		recorder.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
