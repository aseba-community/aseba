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

#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h> 
#include <net/if.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <iostream>
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
	extern int canfd;

	Switch::Switch(int port, const char *canIface, bool verbose, bool dump) :
		verbose(verbose),
		dump(dump)
	{
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSocket < 0)
		{
			std::cerr << "Cannot create server socket: " << strerror(errno) << std::endl;
			exit(1);
		}
		int on = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		
		// The local address of the serverSocket
		struct sockaddr_in loc_addr;
		loc_addr.sin_family = AF_INET;
		loc_addr.sin_port = htons(port);
		loc_addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(serverSocket, (struct sockaddr *)(&loc_addr), sizeof(struct sockaddr_in)) < 0)
		{
			std::cerr << "Cannot bind server socket: " << strerror(errno) << std::endl;
			close(serverSocket);
			exit(2);
		}
		
		canConnected = canConnect(canIface);
	}
	
	Switch::~Switch()
	{
		if (canConnected)
		{
			shutdown(canSocket, SHUT_RDWR);
			close(canSocket);
		}
		shutdown(serverSocket, SHUT_RDWR);
		close (serverSocket);
	}
	
	bool Switch::canConnect(const char *canIface)
	{
		Switch::canSocket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
		if (canSocket < 0) 
		{
			std::cerr << "Cannot create CAN socket: " << strerror(errno) << std::endl;
			if (verbose)
			{
				if (errno == EPERM)
					std::cerr << "Try to run it as root" << std::endl;
				std::cerr << "Nothing is going to be transfered to the CAN modules" << std::endl;
			}
			return false;
		}
		
		struct ifreq req;
		strcpy(req.ifr_name, canIface);
		if (ioctl(canSocket, SIOCGIFINDEX, &req))
		{
			std::cerr << "Cannot ioctl CAN socket: " << strerror(errno) << std::endl;
			if (verbose)
			{
				if (errno == ENODEV)
					//we have no can interface
					std::cerr << "CAN interface " << canIface << " does not exist" << std::endl;
				std::cerr << "Nothing is going to be transfered to the CAN modules" << std::endl;
			}
			close(canSocket);
			return false;
		}
		else
		{
			struct sockaddr_ll sal;
			sal.sll_protocol = htons(ETH_P_ALL);
			sal.sll_family = AF_PACKET;
			sal.sll_ifindex = req.ifr_ifindex;
			if (bind(canSocket, (struct sockaddr *) &sal, sizeof(struct sockaddr_ll))) 
			{
				std::cerr << "Cannot bind CAN socket: " << strerror(errno) << std::endl;
				if (verbose)
					std::cerr << "Nothing is going to be transfered to the CAN modules" << std::endl;
				close(canSocket);
				return false;
			}
			
			canfd = canSocket;
			::AsebaCanInit(IMX_CAN_ID, sendFrame, isFrameRoom, receivedPacketDropped, sentPacketDropped);
			return true;
		}
	}
	
	void Switch::buildSelectList()
	{

		/* First put together fd_set for select(), which will
		consist of the serverSocket variable in case a new connection
		is coming in, plus all the sockets we have already
		accepted. */
		
		/* FD_ZERO() clears out the fd_set called sockets, so that
		it doesn't contain any file descriptors. */
		FD_ZERO(&socketsRead);

		/* FD_SET() adds the file descriptor "serverSocket" to the fd_set,
		so that select() will return if a connection comes in
		on that socket (which means you have to do accept(), etc. */
		FD_SET(serverSocket, &socketsRead);
		highSocket = serverSocket;
		
		// We add the CAN socket to the sockets to be listened
		if (canConnected)
		{
			FD_SET(canSocket, &socketsRead);
			highSocket = canSocket;
		}

		/* Loops through all the possible connections and adds
		those sockets to the fd_set */
		for(std::set<int>::iterator it = connectList.begin(); it != connectList.end(); ++it)
		{
			FD_SET(*it, &socketsRead);
			if (*it > highSocket)
				highSocket = *it;
		}
	}
	
	void Switch::run()
	{
		listen(serverSocket, ASEBA_MAX_SWITCH_CONN);
		if (verbose)
		{
			dumpTime(std::cout);
			std::cout << "Aseba switch waiting for connection" << std::endl;
		}
		
		while (1)
		{
			int nbAdd = (canConnected ? 1 : 2);
			buildSelectList();
			
			// Timeout for select
			struct timeval timeout;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			int rdySockets = select(highSocket+nbAdd, &socketsRead, (fd_set *) 0, (fd_set *) 0, &timeout);
			if (rdySockets < 0)
			{
				dumpTime(std::cerr);
				std::cerr << "Select returned an error" << std::endl;
				exit(3);
			}
			else if (rdySockets == 0)
			{
				// Nothing to read
				//std::cout << "." << std::endl;
			}
			else
			{
				//std::cout << "read" << std::endl;
				manageSockets();
				//std::cout << "readSockets" << std::endl;
			}
		}
	}
	
	void Switch::manageSockets()
	{
		/* If a client is trying to connect() to our listening
		serverSocket, select() will consider that as the socket
		being 'readable'. Thus, if the listening socket is
		part of the fd_set, we need to accept a new connection. */
		if (FD_ISSET(serverSocket, &socketsRead))
			newConnection();
		
		/* if we have a frame to read on the can socket, we manage it */
		if (canConnected && FD_ISSET(canSocket, &socketsRead))
			manageCanFrame();

		/* Now check connectList for available data */
		/* Run through our sockets and check to see if anything
		happened with them, if so 'service' them. */
		for (std::set<int>::iterator it = connectList.begin(); it != connectList.end(); ++it)
		{
			if (FD_ISSET(*it, &socketsRead))
			{
				try
				{
					forwardDataFrom(*it);
				}
				catch (Exception::FileDescriptorClosed e)
				{
					closeConnection(e.fd);
					
					// we got exception, so socket list is dirty, get out of this loop
					return;
				}
			}
		}
	}
	
	void Switch::newConnection()
	{
		/* We have a new connection coming in!  We'll
		try to find a spot for it in connectlist. */
		struct sockaddr_in sockAddr;
		socklen_t sockAddrLen = sizeof (struct sockaddr_in);
		int connection = accept(serverSocket, (sockaddr *)&sockAddr, &sockAddrLen);
		if (connection < 0)
		{
			dumpTime(std::cerr);
			std::cerr << "Cannot accept connection: " << strerror(errno) << std::endl;
			return;
		}
		
		connectList.insert(connection);
		if (verbose)
		{
			dumpTime(std::cout);
			std::cout << "Connection accepted from " << inet_ntoa(sockAddr.sin_addr) << " : FD " << connection << std::endl;
		}
	}
	
	void Switch::forwardDataFrom(const int socketNb)
	{
		// max packet length is 65533
		// packet source and packet type is not counted in len,
		// thus read buffer is of size len + 4
		uint16 len;
		
		// read the transfer size
		Aseba::read(socketNb, &len, 2);
		
		// allocate the read buffer and do socket read
		std::valarray<uint8> readbuff((uint8)0, len + 4);
		Aseba::read(socketNb, &readbuff[0], len + 4);
		
		if (dump)
		{
			std::cout << "Read on socket ";
			for(unsigned int i = 0; i < readbuff.size(); i++)
				std::cout << (unsigned)readbuff[i] << " ";
			std::cout << std::endl;
		}
		
		// write on all connected sockets
		for (std::set<int>::iterator it = connectList.begin(); it != connectList.end(); ++it)
		{
			Aseba::write(*it, &len, 2);
			Aseba::write(*it, &readbuff[0], len + 4);
		}
		
		// write on the canSocket too
		if (canConnected)
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
	}
	
	void Switch::closeConnection(const int socketNb)
	{
		if (verbose)
		{
			dumpTime(std::cout);
			std::cout << "Connection closed: FD " << socketNb << std::endl;
		}
		shutdown(socketNb, SHUT_RDWR);
		close (socketNb);
		connectList.erase(socketNb);
	}
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
	char *canIface = "can0";
	
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
				canIface = argv[1];
			else
			{
				std::cout << "error: bad argument" << std::endl;
				dumpHelp(std::cout, argv[0]);
				return 1;
			}
		}
		argCounter++;
	}
	
	Aseba::Switch aswitch(port, canIface, verbose, false);
	aswitch.run();
	
	return 0;
}


