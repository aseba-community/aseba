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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <set>
#include <valarray>
#include <vector>
#include <iterator>
#include "switch.h"
#include "../common/consts.h"
#include "../common/types.h"
#include "../utils/utils.h"
#include "../msg/msg.h"

namespace Aseba 
{
	using namespace std;
	using namespace Dashel;
	
	/** \addtogroup switch */
	/*@{*/

	//! Broadcast messages form any data stream to all others data streams including itself.
	Switch::Switch(unsigned port, bool verbose, bool dump, bool forward, bool rawTime) :
		verbose(verbose),
		dump(dump),
		forward(forward),
		rawTime(rawTime)
	{
		ostringstream oss;
		oss << "tcpin:port=" << port;
		connect(oss.str());
	}
	
	void Switch::connectionCreated(Stream *stream)
	{
		if (verbose)
		{
			dumpTime(cout, rawTime);
			cout << "Incoming connection from " << stream->getTargetName() << endl;
		}
	}
	
	void Switch::incomingData(Stream *stream)
	{
		// max packet length is 65533
		// packet source and packet type is not counted in len,
		// thus read buffer is of size len + 4
		uint16 len;
		
		// read the transfer size
		stream->read(&len, 2);
		
		// allocate the read buffer and do socket read
		std::valarray<uint8> readbuff((uint8)0, len + 4);
		stream->read(&readbuff[0], len + 4);
		
		if (dump)
		{
			dumpTime(cout, rawTime);
			std::cout << "Read " << std::dec << len + 4 << " on stream " << stream << " : ";
			for(unsigned int i = 0; i < readbuff.size(); i++)
				std::cout << std::hex << (unsigned)readbuff[i] << " ";
			std::cout << std::endl;
		}
		
		// write on all connected streams
		for (StreamsSet::iterator it = dataStreams.begin(); it != dataStreams.end();++it)
		{
			Stream* destStream = *it;
			
			if ((forward) && (destStream == stream))
				continue;
			
			try
			{
				destStream->write(&len, 2);
				destStream->write(&readbuff[0], len + 4);
				destStream->flush();
			}
			catch (DashelException e)
			{
				// if this stream has a problem, ignore it for now, and let Hub call connectionClosed later.
				std::cerr << "error while writing" << std::endl;
			}
		}
	}
	
	void Switch::connectionClosed(Stream *stream, bool abnormal)
	{
		if (verbose)
		{
			dumpTime(cout);
			if (abnormal)
				cout << "Abnormal connection closed to " << stream->getTargetName() << " : " << stream->getFailReason() << endl;
			else
				cout << "Normal connection closed to " << stream->getTargetName() << endl;
		}
	}
	
	void Switch::broadcastDummyUserMessage()
	{
		Aseba::UserMessage uMsg;
		uMsg.type = 0;
		//for (int i = 0; i < 80; i++)
		for (StreamsSet::iterator it = dataStreams.begin(); it != dataStreams.end();++it)
		{
			uMsg.serialize(*it);
			(*it)->flush();
		}
	}
	
	/*@}*/
};

//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba switch, connects aseba components together, usage:\n";
	stream << programName << " [options] [additional targets]*\n";
	stream << "Options:\n";
	stream << "-v, --verbose   : makes the switch verbose\n";
	stream << "-d, --dump      : makes the switch dump all data\n";
	stream << "-l, --loop      : makes the switch transmit messages back to the send, not only forward them.\n";
	stream << "-p port         : listens to incoming connection on this port\n";
	stream << "--rawtime       : shows time in the form of sec:usec since 1970\n";
	stream << "-h, --help      : shows this help\n";
	stream << "Additional targets are any valid Dashel targets." << std::endl;
}

int main(int argc, char *argv[])
{
	unsigned port = ASEBA_DEFAULT_PORT;
	bool verbose = false;
	bool dump = false;
	bool forward = true;
	bool rawTime = false;
	std::vector<std::string> additionalTargets;
	
	int argCounter = 1;
	
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		
		if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0))
		{
			verbose = true;
		}
		else if ((strcmp(arg, "-d") == 0) || (strcmp(arg, "--dump") == 0))
		{
			dump = true;
		}
		else if ((strcmp(arg, "-l") == 0) || (strcmp(arg, "--loop") == 0))
		{
			forward = false;
		}
		else if (strcmp(arg, "-p") == 0)
		{
			arg = argv[++argCounter];
			port = atoi(arg);
		}
		else if (strcmp(arg, "--rawtime") == 0)
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
			additionalTargets.push_back(argv[argCounter]);
		}
		argCounter++;
	}
	
	try
	{
		Aseba::Switch aswitch(port, verbose, dump, forward, rawTime);
		for (size_t i = 0; i < additionalTargets.size(); i++)
			aswitch.connect(additionalTargets[i]);
		/*
		Uncomment this and comment aswitch.run() to flood all pears with dummy user messages
		while (1)
		{
			aswitch.step(10);
			aswitch.broadcastDummyUserMessage();
		}*/
		aswitch.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}


