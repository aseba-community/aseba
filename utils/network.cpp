/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
		Sebastian Gerlach
	
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

/*!	\file network.cpp
	\brief Implementation of network classes: clean, endian-safe and easy to use TCP/IP sockets wrapper
*/

#include "network.h"
#include <sstream>
#include <iostream>
#include <set>
#include <cassert>
#include <cstring>
#include <cerrno>

#ifndef WIN32
	#include <netdb.h>
	#include <signal.h>
	#include <arpa/inet.h>
	#include <sys/select.h>
	#include <sys/time.h>
	#include <sys/types.h>
	#include <unistd.h>
#else
	#include <winsock2.h>
#endif

namespace Aseba
{
	std::string TCPIPAddress::format() const
	{
		std::stringstream buf;
	
		struct hostent *he;
		unsigned a2 = htonl(address);
		he = gethostbyaddr((const char *)&a2, 4, AF_INET);
	
		if (he == NULL)
		{
			struct in_addr addr;
			addr.s_addr = a2;
			buf << inet_ntoa(addr) << ":" << port;
		}
		else
		{
			buf << he->h_name << ":" << port;
		}
	
		return buf.str();
	}

	SocketHelper socketHelper;

	SocketHelper::SocketHelper()
	{
		#ifdef WIN32
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD( 2,0 );

		WSAStartup( wVersionRequested, &wsaData );
		#else
		oldHandler = signal(SIGPIPE,SIG_IGN);
		#endif
	}

	SocketHelper::~SocketHelper()
	{
		#ifdef WIN32
		WSACleanup();
		#else
		signal(SIGPIPE,oldHandler);
		#endif
	}

	TCPIPAddress SocketHelper::resolve(const std::string &name, unsigned short defPort)
	{
		TCPIPAddress address;
		hostent *he;

		std::string::size_type pos = name.find_first_of(':');
		if (pos != std::string::npos)
		{
			address.port = atoi(name.substr(pos+1, std::string::npos).c_str());
			he = gethostbyname(name.substr(0, pos).c_str());
		}
		else
		{
			address.port = defPort;
			he = gethostbyname(name.c_str());
		}

		if(he == NULL)
		{
			#ifndef WIN32
			struct in_addr addr;
			if (inet_aton(name.c_str(), &addr))
			{
				address.address = ntohl(addr.s_addr);
			}
			else
			{
				address.address = INADDR_ANY;
			}
			#else
			address.address = INADDR_ANY;
			#endif
		}
		else
		{
			address.address=ntohl(*((unsigned *)he->h_addr));
		}

		return address;
	}

	//! size_t of a newly allocated send buffer
	const size_t startSendBuffersize_t = 256;
	//! When size of send buffer is bigSendsize_t, send it, even if flush hasn't been called
	const size_t bigSendsize_t = 65536;
	//! Network server is running, set to 0 by SIGTERM
	int netRun = 1;
	
	Socket::Socket()
	{
		socket = -1;
		#ifndef TCP_CORK
		sendBuffer = (unsigned char*)malloc(startSendBuffersize_t);
		buffersize_t = startSendBuffersize_t;
		bufferPos = 0;
		#endif
	}
	
	Socket::~Socket()
	{
		if (socket >= 0)
			disconnect();
		#ifndef TCP_CORK
		free(sendBuffer);
		#endif
	}
	
	void Socket::disconnect(void)
	{
		if (socket < 0)
			return;
		
		#ifndef WIN32
		shutdown(socket, SHUT_RDWR);
		close(socket);
		#else
		shutdown(socket, SD_BOTH);
		closesocket(socket);
		#endif
		
		socket = -1;
	}
	
	void Socket::write(const void *data, const size_t size)
	{
		if (socket < 0)
			throw Exception::SocketNotConnected(this);
		
		#ifdef TCP_CORK
		send(data, size);
		#else
		if (size >= bigSendsize_t)
		{
			flush();
			send(data, size);
		}
		else
		{
			if (bufferPos + size > buffersize_t)
			{
				buffersize_t = std::max(buffersize_t * 2, bufferPos + size);
				sendBuffer = (unsigned char*)realloc(sendBuffer, buffersize_t);
			}
			memcpy(sendBuffer+bufferPos, (unsigned char *)data, size);
			bufferPos += size;
	
			if (bufferPos >= bigSendsize_t)
				flush();
		}
		#endif
	}
	
	void Socket::flush(void)
	{
		if (socket < 0)
			throw Exception::SocketNotConnected(this);
		
		#ifdef TCP_CORK
		int flag = 0;
		setsockopt(socket, IPPROTO_TCP, TCP_CORK, &flag , sizeof(flag));
		flag = 1;
		setsockopt(socket, IPPROTO_TCP, TCP_CORK, &flag , sizeof(flag));
		#else
		send(sendBuffer, bufferPos);
		bufferPos = 0;
		#endif
	}
	
	void Socket::send(const void *data, size_t size)
	{
		if (socket < 0)
			throw Exception::SocketNotConnected(this);
		
		unsigned char *ptr = (unsigned char *)data;
		size_t left = size;
		
		while (left)
		{
			#ifndef WIN32
			ssize_t len = ::send(socket, ptr, left, 0);
			#else
			ssize_t len = ::send(socket, (const char *)ptr, left, 0);
			#endif
			
			if (len < 0)
			{
				close(socket);
				socket = -1;
				throw Exception::SocketError(this, errno);
			}
			else if (len == 0)
			{
				close(socket);
				socket = -1;
				throw Exception::SocketDisconnected(this);
			}
			else
			{
				ptr += len;
				left -= len;
			}
		}
	}
	
	void Socket::read(void *data, size_t size)
	{
		if (socket < 0)
			throw Exception::SocketNotConnected(this);
			
		unsigned char *ptr = (unsigned char *)data;
		size_t left = size;
	
		while (left)
		{
			#ifndef WIN32
			ssize_t len = recv(socket, ptr, left, 0);
			#else
			ssize_t len = recv(socket, (char *)ptr, left, 0);
			#endif
			
			if (len < 0)
			{
				close(socket);
				socket = -1;
				throw Exception::SocketError(this, errno);
			}
			else if (len == 0)
			{
				close(socket);
				socket = -1;
				throw Exception::SocketDisconnected(this);
			}
			else
			{
				ptr += len;
				left -= len;
			}
		}
	}
	
	bool NetworkClient::connect(const std::string &host, unsigned short port)
	{
		if (isConnected())
		{
			disconnect();
			connectionClosed();
		}
	
		remoteAddress = socketHelper.resolve(host, port);
		socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (socket == -1)
		{
			std::cerr << "NetworkClient::NetworkClient(" << host << ", " << port << ") : can't create socket" << std::endl;
			assert(false);
		}
	
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(remoteAddress.port);
		addr.sin_addr.s_addr = htonl(remoteAddress.address);
		if (::connect(socket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		{
			std::cerr << "NetworkClient::NetworkClient(" << host << ", " << port << ") : can't connect to host" << std::endl;
			close(socket);
			socket = -1;
			return false;
		}
		else
		{
			#ifdef TCP_CORK
			int flag = 1;
			setsockopt(socket, IPPROTO_TCP, TCP_CORK, &flag , sizeof(flag));
			#endif
			connectionEstablished();
			return true;
		}
	}
		
	#ifndef WIN32
	//! Called when SIGTERM arrives, halts NetworkServer run
	void termHandler(int t)
	{
		netRun = 0;
	}
	#endif
	
	void NetworkClient::run(void)
	{
		#ifndef WIN32
		signal(SIGTERM, termHandler);
		#endif
		
		while (netRun)
		{
			step(true);
		}
		
		#ifndef WIN32
		signal(SIGTERM, SIG_DFL);
		#endif
	}
	
	bool NetworkClient::step(bool block, long timeout)
	{
		if (!isConnected())
			return false;
		
		fd_set rfds;
		int nfds;
		FD_ZERO(&rfds);
		FD_SET(socket, &rfds);
		nfds = socket;
		
		int ret;
		if (block)
		{
			ret = select(nfds+1, &rfds, NULL, NULL, NULL);
		}
		else
		{
			struct timeval t;
			t.tv_sec = 0;
			t.tv_usec = timeout;
			ret = select(nfds+1, &rfds, NULL, NULL, &t);
		}
		
		if (ret < 0)
		{
			std::cerr << "NetworkClient::step(" << block << ") : select error" << std::endl;
			assert(false);
		}
	
		if (FD_ISSET(socket, &rfds))
		{
			try
			{
				char c;
				do
				{
					incomingData();
				} while (recv(socket, &c, 1, MSG_PEEK | MSG_DONTWAIT) == 1);
			}
			catch (Exception::SocketException e)
			{
				connectionClosed();
				netRun = false;
			}
			return true;
		}
		else
			return false;
	}

	NetworkServer::NetworkServer()
	{
		serverSocket = 0;
	}
	
	NetworkServer::~NetworkServer()
	{
		shutdown();
	}
	
	bool NetworkServer::listen(unsigned short port)
	{
		shutdown();
		
		serverSocket = new Socket();
		assert(serverSocket);
	
		char localName[128];
		gethostname(localName,128);
		serverSocket->remoteAddress = socketHelper.resolve(localName, port);
	
		serverSocket->socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (serverSocket->socket == -1)
		{
			std::cerr << "NetworkServer::NetworkServer(" << port << ") : can't create socket" << std::endl;
			assert(false);
		}
	
		int flag = 1;
		if (setsockopt(serverSocket->socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag)) < 0)
		{
			std::cerr << "NetworkServer::NetworkServer(" << port << ") : can't enable SO_REUSEADDR" << std::endl;
			assert(false);
		}
	
		sockaddr_in addr;
		memset (&addr, 0, sizeof (addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(serverSocket->remoteAddress.port);
		if (bind(serverSocket->socket, (struct sockaddr *) &addr, sizeof (addr)) < 0)
		{
			std::cerr << "NetworkServer::NetworkServer(" << port << ") : can't bind server socket" << std::endl;
			delete serverSocket;
			serverSocket = 0;
			return false;
		}
	
		::listen(serverSocket->socket, 10);
		return true;
	}
	
	void NetworkServer::shutdown()
	{
		for (std::list<Socket *>::iterator it = clients.begin(); it != clients.end(); ++it)
			delete (*it);
		if (serverSocket)
			delete serverSocket;
	}
	
	void NetworkServer::step(bool block, long timeout)
	{
		fd_set rfds;
		int nfds = 0;
		FD_ZERO(&rfds);
	
		// add client sockets
		for (std::list<Socket *>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			int fd = (*it)->socket;
			FD_SET(fd, &rfds);
			nfds = std::max(fd, nfds);
		}
	
		// add server socket
		FD_SET(serverSocket->socket, &rfds);
		nfds = std::max(serverSocket->socket, nfds);
	
		// select
		int ret;
		if (block)
		{
			ret = select(nfds+1, &rfds, NULL, NULL, NULL);
		}
		else
		{
			struct timeval t;
			t.tv_sec = 0;
			t.tv_usec = timeout;
			ret = select(nfds+1, &rfds, NULL, NULL, &t);
		}
		
		if (ret<0)
		{
			std::cerr << "NetworkServer::step(" << block << ") : select error" << std::endl;
			assert(false);
		}
	
		// read client sockets
		for (std::list<Socket *>::iterator it = clients.begin(); it != clients.end();)
		{
			std::list<Socket *>::iterator oldIt = it;
			++it;
			int fd = (*oldIt)->socket;
			if (FD_ISSET(fd, &rfds))
			{
				try
				{
					char c;
					do
					{
						incomingData(*oldIt);
					} while (recv((*oldIt)->socket, &c, 1, MSG_PEEK | MSG_DONTWAIT) == 1);
				}
				catch (Exception::SocketException e)
				{
					connectionClosed(e.socket);
					closeSocket(e.socket);
				}
			}
		}
	
		// read server socket
		if (FD_ISSET(serverSocket->socket, &rfds))
		{
			Socket *socket = new Socket();
			assert(socket);
	
			struct sockaddr_in clientAddr;
			socklen_t l = sizeof (clientAddr);
			memset  (&clientAddr, 0, l);
			int newSocket = accept (serverSocket->socket, (struct sockaddr *)&clientAddr, &l);
			if (newSocket < 0)
			{
				std::cerr << "NetworkServer::step(" << block << ") : can't accept new connection" << std::endl;
				assert(false);
			}
			else
			{
				socket->socket = newSocket;
				socket->remoteAddress.port = ntohs(clientAddr.sin_port);
				socket->remoteAddress.address = ntohl(clientAddr.sin_addr.s_addr);
				
				#ifdef TCP_CORK
				int flag = 1;
				setsockopt(socket->socket, IPPROTO_TCP, TCP_CORK, &flag , sizeof(flag));
				#endif
			}
			
			clients.push_back(socket);
			
			try
			{
				incomingConnection(socket);
			}
			catch (Exception::SocketException e)
			{
				connectionClosed(e.socket);
				closeSocket(e.socket);
			}
		}
	}
	
	void NetworkServer::run(void)
	{
		#ifndef WIN32
		signal(SIGTERM, termHandler);
		#endif
		
		while (netRun)
		{
			step(true);
		}
		
		#ifndef WIN32
		signal(SIGTERM, SIG_DFL);
		#endif
	}
	
	void NetworkServer::closeSocket(Socket *s)
	{
		clients.remove(s);
		delete s;
	}
}
