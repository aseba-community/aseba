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

#include <cstdlib>
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

namespace Aseba 
{
	using namespace std;
	using namespace Dashel;
	
	/** \addtogroup switch */
	/*@{*/

	//! Broadcast messages form any data stream to all others data streams including itself.
	Switch::Switch(unsigned port, bool verbose, bool dump) :
		verbose(verbose),
		dump(dump)
	{
		ostringstream oss;
		oss << "tcpin:port=" << port;
		connect(oss.str());
	}
	
	void Switch::connectionCreated(Stream *stream)
	{
		if (verbose)
		{
			dumpTime(cout);
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
			std::cout << "Read " << len + 4 << " on stream " << stream << " : ";
			for(unsigned int i = 0; i < readbuff.size(); i++)
				std::cout << (unsigned)readbuff[i] << " ";
			std::cout << std::endl;
		}
		
		// write on all connected streams
		for (StreamsSet::iterator it = dataStreams.begin(); it != dataStreams.end();++it)
		{
			Stream* destStream = *it;
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
	
	/*@}*/
};

//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba switch, connects aseba components together, usage:\n";
	stream << programName << " [options] [additional targets]*\n";
	stream << "Options:\n";
	stream << "-v, --verbose   : makes the switch verbose\n";
	stream << "-d, --dump      : makes the switch dumping all data\n";
	stream << "-p port         : listens to incoming connection on this port\n";
	stream << "Additional targets are any valid Dashel targets." << std::endl;
}

int main(int argc, char *argv[])
{
	unsigned port = ASEBA_DEFAULT_PORT;
	bool verbose = false;
	bool dump = false;
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
		else if (strcmp(arg, "-p") == 0)
		{
			arg = argv[++argCounter];
			port = atoi(arg);
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
		Aseba::Switch aswitch(port, verbose, dump);
		for (size_t i = 0; i < additionalTargets.size(); i++)
			aswitch.connect(additionalTargets[i]);
		aswitch.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.reason << " " << e.sysMessage << std::endl;
	}
	
	return 0;
}


