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

#include <dashel/streams.h>

namespace Aseba
{
	/**
	\defgroup switch Software router of messages
	
	Connects to a physical CAN interface if present.
	*/
	/*@{*/

	/*! Route Aseba messages on the TCP part of the network.
		The switch is not thread-safe because it stores a pointer to the CAN stream
		outside this object. This is required to interface with the aseba CAN
		C library.
	*/
	class Switch: public Streams::Server
	{
		public:
			/*! Creates the switch, listen to TCP on port and try to open CAN interface.
				@param port TCP port to listen to
				@param canTarget the name of the CAN target
				@param verbose should we print a notification on each message
				@param dump should we dump content of CAN messages
			*/
			Switch(int port, const char* canTarget, bool verbose, bool dump);
			
			/*! Forwards the data received for a connections to the other ones.
				@param stream the stream the packet was received from
			*/
			void forwardDataFrom(Streams::Stream* stream);

			/*! Reads a CAN frame from the CAN socket and sends it to the
				aseba CAN network layer.
			*/
			void manageCanFrame();
			
		private:
			virtual void incomingConnection(Streams::Stream *stream);
			virtual void incomingData(Streams::Stream *stream);
			virtual void connectionClosed(Streams::Stream *stream);

		private:
			std::string canTarget; //!< target for CAN
			bool verbose; //!< should we print a notification on each message
			bool dump; //!< should we dump content of CAN messages
	};
	
	/*@}*/
};

#endif
