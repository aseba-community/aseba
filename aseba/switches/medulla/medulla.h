/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

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

#ifndef ASEBA_MEDULLA
#define ASEBA_MEDULLA

#include <dashel/dashel.h>
#include <QThread>
#include <QStringList>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QMetaType>
#include <QList>
#include "common/msg/msg.h"
#include "common/msg/NodesManager.h"
#include "compiler/compiler.h"

typedef QList<qint16> Values;

namespace Aseba {
/**
\defgroup medulla Software router of messages on TCP and D-Bus.
*/
/*@{*/

class Hub;
class AsebaNetworkInterface;

//! DBus interface for an event filter
class EventFilterInterface : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.EventFilter")

public:
    EventFilterInterface(AsebaNetworkInterface* network) : network(network) {
        ListenEvent(0);
    }
    void emitEvent(const uint16_t id, const QString& name, const Values& data);

public slots:
    Q_SCRIPTABLE Q_NOREPLY void ListenEvent(const uint16_t event);
    Q_SCRIPTABLE Q_NOREPLY void ListenEventName(const QString& name, const QDBusMessage& message);
    Q_SCRIPTABLE Q_NOREPLY void IgnoreEvent(const uint16_t event);
    Q_SCRIPTABLE Q_NOREPLY void IgnoreEventName(const QString& name, const QDBusMessage& message);
    Q_SCRIPTABLE Q_NOREPLY void Free();

signals:
    Q_SCRIPTABLE void Event(const uint16_t, const QString& name, const Values& values);
    Q_SCRIPTABLE void Test0(const uint16_t);
    Q_SCRIPTABLE void Test1(const QString&);
    Q_SCRIPTABLE void Test2(const Values&);
    Q_SCRIPTABLE void Test0_1(const uint16_t, const QString&);
    Q_SCRIPTABLE void Test0_2(const uint16_t, const Values&);
    Q_SCRIPTABLE void Test1_2(const QString&, const Values&);

protected:
    AsebaNetworkInterface* network;
};

//! DBus interface for aseba network
class AsebaNetworkInterface : public QDBusAbstractAdaptor, public NodesManager {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.AsebaNetwork")

protected:
    struct RequestData {
        unsigned nodeId;
        unsigned pos;
        QDBusMessage reply;
    };

public:
    AsebaNetworkInterface(Hub* hub, bool systemBus);

private slots:
    friend class Hub;
    void processMessage(Message* message, const Dashel::Stream* sourceStream);
    friend class EventFilterInterface;
    void sendEventOnDBus(const uint16_t event, const Values& data);
    void listenEvent(EventFilterInterface* filter, uint16_t event);
    void ignoreEvent(EventFilterInterface* filter, uint16_t event);
    void filterDestroyed(EventFilterInterface* filter);

public slots:
    Q_NOREPLY void LoadScripts(const QString& fileName, const QDBusMessage& message);
    QStringList GetNodesList() const;
    qint16 GetNodeId(const QString& node, const QDBusMessage& message) const;
    QString GetNodeName(const uint16_t nodeId, const QDBusMessage& message) const;
    bool IsConnected(const QString& node, const QDBusMessage& message) const;
    QStringList GetVariablesList(const QString& node) const;
    Q_NOREPLY void SetVariable(const QString& node, const QString& variable, const Values& data,
                               const QDBusMessage& message) const;
    Values GetVariable(const QString& node, const QString& variable, const QDBusMessage& message);
    Q_NOREPLY void SendEvent(const uint16_t event, const Values& data);
    Q_NOREPLY void SendEventName(const QString& name, const Values& data, const QDBusMessage& message);
    QDBusObjectPath CreateEventFilter();
    void PingNetwork();

protected:
    virtual void sendMessage(const Message& message);
    virtual void nodeDescriptionReceived(unsigned nodeId);
    QDBusConnection DBusConnectionBus() const;

protected:
    Hub* hub;
    CommonDefinitions commonDefinitions;
    typedef QMap<QString, unsigned> NodesNamesMap;
    NodesNamesMap nodesNames;
    typedef QMap<QString, VariablesMap> UserDefinedVariablesMap;
    UserDefinedVariablesMap userDefinedVariablesMap;
    typedef QList<RequestData*> RequestsList;
    RequestsList pendingReads;
    typedef QMultiMap<uint16_t, EventFilterInterface*> EventsFiltersMap;
    EventsFiltersMap eventsFilters;
    bool systemBus;
    unsigned eventsFiltersCounter;
};

/*!
    Route Aseba messages on the TCP part of the network.

    This thread only *receives* messages.
    All dispatch, including forwarding, is done in the main thread called by
    the AsebaNetworkInterface class.
*/
class Hub : public QThread, public Dashel::Hub {
    Q_OBJECT

public:
    /*! Creates the hub, listen to TCP on port, and creates a DBus interace.
        @param port port on which to listen for incoming connections
        @param verbose should we print a notification on each message
        @param dump should we dump content of each message
        @param forward should we only forward messages instead of transmit them back to the sender
        @param rawTime should the time be printed as integer
    */
    Hub(unsigned port, bool verbose, bool dump, bool forward, bool rawTime, bool systemBus);

    /*! Sends a message to Dashel peers.
        Does not delete the message, should be called by the main thread.
        @param message aseba message to send
        @param sourceStream originate of the message, if from Dashel.
    */
    void sendMessage(const Message* message, const Dashel::Stream* sourceStream = 0);
    /*! Sends a message to Dashel peers.
        Convenience overload
    */
    void sendMessage(const Message& message, const Dashel::Stream* sourceStream = 0);

signals:
    void messageAvailable(Message* message, const Dashel::Stream* sourceStream);

private:
    virtual void run();
    virtual void connectionCreated(Dashel::Stream* stream);
    virtual void incomingData(Dashel::Stream* stream);
    virtual void connectionClosed(Dashel::Stream* stream, bool abnormal);

private:
    bool verbose;  //!< should we print a notification on each message
    bool dump;     //!< should we dump content of CAN messages
    bool forward;  //!< should we only forward messages instead of transmit them back to the sender
    bool rawTime;  //!< should displayed timestamps be of the form sec:usec since 1970
};

/*@}*/
};  // namespace Aseba

Q_DECLARE_METATYPE(Values);

#endif
