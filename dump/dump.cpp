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
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <iostream>

int main(int argc, char *argv[])
{
	const char *address = "localhost";
	uint16_t port = ASEBA_DEFAULT_PORT;
	
	if (argc >= 2)
		address = argv[1];
	if (argc >= 3)
		port = static_cast<uint16_t>(atoi(argv[2]));
	
	// fill server description structure
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	
	// get server ip
	struct hostent *hostp = gethostbyname(address);
	if (!hostp)
	{
		Aseba::dumpTime(std::cerr);
		std::cerr << "Cannot get address of server: " << strerror(errno) << std::endl;
		return 1;
	}
	memcpy(&serverAddress.sin_addr, hostp->h_addr, sizeof(serverAddress.sin_addr));
	
	// create socket
	int socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0)
	{
		Aseba::dumpTime(std::cerr);
		std::cerr << "Cannot create socket: " << strerror(errno) << std::endl;
		return 2;
	}
	
	// connect
	int ret = connect(socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if (ret < 0)
	{
		Aseba::dumpTime(std::cerr);
		std::cerr << "Connection to server failed: " << strerror(errno) << std::endl;
		return 3;
	}
	
	try
	{
		while (true)
		{
			Aseba::Message *message = Aseba::Message::receive(socket);
			
			Aseba::dumpTime(std::cout);
			if (message)
				message->dump(std::cout);
			else
				std::cout << "unknown message received";
			std::cout << std::endl;
		}
	}
	catch (Aseba::Exception::FileDescriptorClosed e)
	{
		Aseba::dumpTime(std::cerr);
		std::cerr << "Server closed connection" << std::endl;
		close(socket);
	}
	
	return 0;
}
