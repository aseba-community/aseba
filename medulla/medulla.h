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

#ifndef ASEBA_MEDULLA
#define ASEBA_MEDULLA

#include <dashel/dashel.h>
#include <QThread>
#include "../msg/msg.h"

namespace Aseba
{
	/**
	\defgroup medulla Software router of messages on TCP and D-Bus.
	*/
	/*@{*/
	
	/*!
		Route Aseba messages on the TCP part of the network.
	*/
	class Hub: public QThread, public Dashel::Hub
	{
		Q_OBJECT
		
		public:
			/*! Creates the hub, listen to TCP on port.
				@param verbose should we print a notification on each message
				@param dump should we dump content of each message
				@param forward should we only forward messages instead of transmit them back to the sender
			*/
			Hub(unsigned port, bool verbose, bool dump, bool forward, bool rawTime);
			
		signals:
			void messageAvailable(Message *message);
			
		public slots:
			void sendMessage(Message *message, Dashel::Stream* sourceStream = 0);
			
		private:
			virtual void run();
			virtual void connectionCreated(Dashel::Stream *stream);
			virtual void incomingData(Dashel::Stream *stream);
			virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
		
		private:
			bool verbose; //!< should we print a notification on each message
			bool dump; //!< should we dump content of CAN messages
			bool forward; //!< should we only forward messages instead of transmit them back to the sender
			bool rawTime; //!< should displayed timestamps be of the form sec:usec since 1970
	};
	
	class AsebaDBusInterface: public QObject
	{
		Q_OBJECT
	};
	
	class EventFilterInterface: public QObject
	{
		Q_OBJECT
	};
	
	/*@}*/
};

#endif
