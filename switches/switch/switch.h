/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
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

#ifndef ASEBA_SWITCH
#define ASEBA_SWITCH

#include <dashel/dashel.h>
#include <map>
#include "../../common/types.h"

namespace Aseba
{
	/**
	\defgroup switch Software router of messages.
	*/
	/*@{*/

	/*!
		Route Aseba messages on the TCP part of the network.
	*/
	class Switch: public Dashel::Hub
	{
		public:
			/*! Creates the switch, listen to TCP on port.
				@param verbose should we print a notification on each message
				@param dump should we dump content of each message
				@param forward should we only forward messages instead of transmit them back to the sender
			*/
			Switch(unsigned port, bool verbose, bool dump, bool forward, bool rawTime);
			
			/*! Forwards the data received for a connections to the other ones.
				If forward is false, transmit it back to the sender too.
				@param stream the stream the packet was received from
			*/
			void forwardDataFrom(Dashel::Stream* stream);
			
			/*!	Send a dummy user message to all connected pears. */
			void broadcastDummyUserMessage();
			
			/*! Remap the node identifier for messages coming on a stream
				@param stream the stream from which node id will be remapped
				@param localId the new local node id to use
				@param targetId the target node id to remap
			*/
			void remapId(Dashel::Stream* stream, const uint16 localId, const uint16 targetId);
			
		private:
			virtual void connectionCreated(Dashel::Stream *stream);
			virtual void incomingData(Dashel::Stream *stream);
			virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);

		private:
			bool verbose; //!< should we print a notification on each message
			bool dump; //!< should we dump content of CAN messages
			bool forward; //!< should we only forward messages instead of transmit them back to the sender
			bool rawTime; //!< should displayed timestamps be of the form sec:usec since 1970
			
			//! A pair of id: local, target
			typedef std::pair<uint16, uint16> IdPair;
			//! A table allowing to remap the aseba node id of streams
			typedef std::map<Dashel::Stream*, IdPair> IdRemapTable;
			IdRemapTable idRemapTable; //!< table for remapping id
	};
	
	/*@}*/
};

#endif
