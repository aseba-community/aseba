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

#ifndef ASEBA_SWITCH
#define ASEBA_SWITCH

#include <set>

namespace Aseba
{
	/*! Route Aseba messages on the TCP part of the network. */
	class Switch
	{
		public:
			/*! create the switch, listen to TCP on port and try to open CAN interface.
			 * @param port TCP port to listen to
			 * @param canIface the name of the CAN interface
			 * @param verbose should we print a notification on each message
			 * @param dump should we dump content of CAN messages
			 */
			Switch(int port, const char *canIface, bool verbose, bool dump);
			
			/*! release all ressources */
			~Switch();

			/*! connects a socket to the CAN interface if available
			 * and then initializes the aseba CAN network layer
			 * @param canIface the name of the CAN interface
			 * @return true if the socket could be connected, false otherwise
			 */
			bool canConnect(const char *canIface);

			/*! builds a list of file descriptors for the select call
			 */
			void buildSelectList();

			/*! actual main running loop of the switch, never returns
			 */
			void run();

			/*! manage the sockets when something is readable on the
			 * listening list accoding to select()
			 */
			void manageSockets();

			/*! adds a new socket to our listening list */
			void newConnection();
	
			/*! closes a connection in the listening list */
			void closeConnection(const int socketNb);

			/*! forwards the data received for a connections to the other ones
			 * @param socketNb the fd the packet was received from
			 */
			void forwardDataFrom(const int socketNb);

			/*! reads a CAN frame from the CAN socket and sends it to the
			 * aseba CAN network layer
			 */
			void manageCanFrame();

		private:
			/*! The socket file descritor we are listening to */
			int serverSocket;
			
			/*! the CAN socket fd */
			int canSocket;

			/*! socket file descriptor needed by select */
			int highSocket;

			/*! Socket file descriptors we want to wake up for, using select() */
			fd_set socketsRead;

			/*! List of connected sockets (fd) */
			std::set<int> connectList;

			/*! true if CAN interface is connected */
			bool canConnected;
			
			/*! should we print a notification on each message */
			bool verbose;

			/*! should we dump content of CAN messages */
			bool dump;
	};
	
};

#endif
