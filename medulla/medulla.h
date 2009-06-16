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
#include <QStringList>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QMetaType>
#include <QList>
#include "../msg/msg.h"
#include "../msg/descriptions-manager.h"

typedef QList<qint16> Values;

namespace Aseba
{
	/**
	\defgroup medulla Software router of messages on TCP and D-Bus.
	*/
	/*@{*/
	
	class Hub;
	class AsebaNetworkInterface;
	
	//! DBus interface for an event filter
	class EventFilterInterface: public QObject
	{
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.EventFilter")
		
		public:
			EventFilterInterface(AsebaNetworkInterface* network) : network(network) { ListenEvent(0); }
			void emitEvent(const quint16 id, const Values& data);
			
		public slots:
			Q_SCRIPTABLE void ListenEvent(const quint16 event);
			Q_SCRIPTABLE void IgnoreEvent(const quint16 event);
			Q_SCRIPTABLE Q_NOREPLY void Free();
		
		signals:
			Q_SCRIPTABLE void Event(const quint16, const Values& );
		
		protected:
			AsebaNetworkInterface* network;
	};
	
	//! DBus interface for aseba network
	class AsebaNetworkInterface: public QDBusAbstractAdaptor, public DescriptionsManager
	{
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.AsebaNetwork")
		
		protected:
			struct RequestData
			{
				unsigned nodeId;
				unsigned pos;
				QDBusMessage reply;
			};
			
		public:
			AsebaNetworkInterface(Hub* hub);
		
		private slots:
			friend class Hub;
			void processMessage(Message *message);
			friend class EventFilterInterface;
			void listenEvent(EventFilterInterface* filter, quint16 event);
			void ignoreEvent(EventFilterInterface* filter, quint16 event);
			void filterDestroyed(EventFilterInterface* filter);
		
		public slots:
			QStringList GetNodesList() const;
			QStringList GetVariablesList(const QString& node) const;
			Q_NOREPLY void SetVariable(const QString& node, const QString& variable, const Values& data, const QDBusMessage &message) const;
			Values GetVariable(const QString& node, const QString& variable, const QDBusMessage &message);
			QDBusObjectPath CreateEventFilter();
		
		protected:
			virtual void nodeDescriptionReceived(unsigned nodeId);
			
		protected:
			Hub* hub;
			typedef QMap<QString, unsigned> NodesNamesMap;
			NodesNamesMap nodesNames;
			typedef QList<RequestData*> RequestsList;
			RequestsList pendingReads;
			typedef QMultiMap<quint16, EventFilterInterface*> EventsFiltersMap;
			EventsFiltersMap eventsFilters;
			unsigned eventsFiltersCounter;
	};
	
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
	
	/*@}*/
};

Q_DECLARE_METATYPE(Values);

#endif
