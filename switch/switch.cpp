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
#include <iterator>
#include "switch.h"
#include "../common/consts.h"
#include "../utils/utils.h"
#include "../can/can.h"
#include "imxcan.h"

namespace Aseba 
{
	using namespace std;
	using namespace Streams;
	
	/** \addtogroup switch */
	/*@{*/

	Switch::Switch(int port, const char* canTarget, bool verbose, bool dump) :
		canTarget(canTarget),
		verbose(verbose),
		dump(dump)
	{
		ostringstream oss;
		oss << "tcp:port=" << port;
		listen(oss.str());
		
		try
		{
			listen(canTarget);
		}
		catch (InvalidTargetDescription e)
		{
			cerr << "Invalid CAN target " <<  canTarget << endl;
		}
		catch (ConnectionError e)
		{
			cerr << "Cannot open CAN target " <<  canTarget << endl;
		}
	}
	
	extern Stream* canStream;
	
	void Switch::incomingConnection(Stream *stream)
	{
		if (stream->getTargetName() == canTarget)
		{
			canStream = stream;
			::AsebaCanInit(IMX_CAN_ID, sendFrame, isFrameRoom, receivedPacketDropped, sentPacketDropped);
			cout << "CAN connection to " << stream->getTargetName() << endl;
		}
		else
		{
			if (verbose)
			{
				dumpTime(cout);
				cout << "Incoming connection from " << stream->getTargetName() << endl;
			}
		}
	}
	
	void Switch::incomingData(Stream *stream)
	{
		if (stream == canStream)
			manageCanFrame();
		else
			forwardDataFrom(stream);
	}
	
	void Switch::connectionClosed(Stream *stream)
	{
		if (stream == canStream)
			canStream = 0;
		if (verbose)
		{
			dumpTime(cout);
			cout << "Connection closed to " << stream->getTargetName() << endl;
		}
	}
	
	void Switch::forwardDataFrom(Stream *stream)
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
			std::cout << "Read on stream ";
			for(unsigned int i = 0; i < readbuff.size(); i++)
				std::cout << (unsigned)readbuff[i] << " ";
			std::cout << std::endl;
		}
		
		// write on all connected streams
		for (StreamsList::iterator it = transferStreams.begin(); it != transferStreams.end();++it)
		{
			Stream* destStream = *it;
			if (destStream != canStream)
			{
				try
				{
					destStream->write(&len, 2);
					destStream->write(&readbuff[0], len + 4);
					destStream->flush();
				}
				catch (StreamException e)
				{
					// if this stream has a problem, ignore it for now, and let next select disconnect it
				}
			}
		}
		
		// write on the CAN stream too
		if (canStream)
		{
			// we drop the source when we send on CAN
			while (::AsebaCanSend(&readbuff[2], len + 2) == 0)
				// we sleep if CAN queue is full
				usleep(10000);
		}
	}
	
	void Switch::manageCanFrame()
	{
		
		/*	1) read the 8 bytes
			2) add them to the current AsebaCanPacket (AsebaCanFrameReceived)
			3) if the AsebaCanPacket is complete, forward it ! */
		/*
		// TODO: adapt this to new translator
		// read on CAN BUS
		imxCANFrame receivedFrame;
		Aseba::read(canSocket, &receivedFrame, sizeof(imxCANFrame));
		
		if (dump)
			receivedFrame.dump(std::cout);
		
		// check if it is what we have written ourself (could happen according to Philippe)
		if (receivedFrame.header.id == IMX_CAN_ID)
		{
			if (verbose)
				std::cout << "dropping CAN ID from IMX_CAN_ID" << std::endl;
			return;
		}
		
		// translation to aseba canFrame
		CanFrame asebaCanFrame = receivedFrame;
		::AsebaCanFrameReceived(&asebaCanFrame);
		
		// forward all completed aseba net packets
		while (true)
		{
			uint8 asebaCanRecvBuffer[ASEBA_MAX_PACKET_SIZE];
			uint16 source;
			uint16 packetSize = ::AsebaCanRecv(asebaCanRecvBuffer, ASEBA_MAX_PACKET_SIZE, &source);
			if (packetSize <= 0)
				break;
			
			// forward completed packet
			for(std::set<int>::iterator it = connectList.begin(); it != connectList.end(); ++it)
			{
				try
				{
					uint16 netPacketSize = packetSize - 2;	// type is not counted on net size
					Aseba::write(*it, &netPacketSize, 2);
					Aseba::write(*it, &source, 2);
					Aseba::write(*it, asebaCanRecvBuffer, packetSize);
				}
				catch (Exception::FileDescriptorClosed e)
				{
					closeConnection(*it);
					return;
				}
			}
		}
		*/
	}
	
	/*@}*/
};

//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba switch, connects aseba components together, usage:\n";
	stream << programName << " <can> [options]\n";
	stream << "Options:" << std::endl;
	stream << "-v, --verbose:	makes the switch verbose" << std::endl;
	stream << "-p port:		listens to incoming connection on this port" << std::endl;	
}

int main(int argc, char *argv[])
{
	unsigned int port = ASEBA_DEFAULT_PORT;
	bool verbose = false;
	const char* canTarget = "ser";
	
	int argCounter = 1;
	
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		
		if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0))
		{
			verbose = true;
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
			if (argCounter == 1)
				canTarget = argv[1];
			else
			{
				std::cout << "error: bad argument" << std::endl;
				dumpHelp(std::cout, argv[0]);
				return 1;
			}
		}
		argCounter++;
	}
	
	Aseba::Switch aswitch(port, canTarget, verbose, false);
	aswitch.run();
	
	return 0;
}


