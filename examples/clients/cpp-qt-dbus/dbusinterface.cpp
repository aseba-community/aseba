/*
	A Qt binding for Aseba that relies on DBus

	Supports event sending/receiving, Aseba scripts Loading, as well as setting/getting an Aseba variable.

	Authors: Frank Bonnet, Stefan Witwicki
	Copyright (C) 2007--2015

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

#include "dbusinterface.h"
#include <iostream>

namespace Aseba
{
	// Create DBus connection with the interface ch.epfl.mobots.AsebaNetwork
	DBusInterface::DBusInterface() :
		bus(QDBusConnection::sessionBus()),
		callbacks({}),
		dbus_iface("ch.epfl.mobots.Aseba", "/", "ch.epfl.mobots.AsebaNetwork",bus)
	{
		checkConnection();

		// setup event filter
		QDBusMessage eventfilterMessage = dbus_iface.call("CreateEventFilter");
		QDBusObjectPath eventfilterPath = eventfilterMessage.arguments().at(0).value<QDBusObjectPath>();
		eventfilterInterface = new QDBusInterface("ch.epfl.mobots.Aseba", eventfilterPath.path(), "ch.epfl.mobots.EventFilter",bus);
		if(!bus.connect("ch.epfl.mobots.Aseba",
						eventfilterPath.path(),
						"ch.epfl.mobots.EventFilter",
						"Event",
						this,
						SLOT(dispatchEvent(const QDBusMessage&))))
		{
			std::cout << "failed to connect eventfilter signal to dispatchEvent slot!";
		}
	}


	// Convert  QList<qint16> Values to QVariant
	QVariant DBusInterface::valuetoVariant(const Values& value)
	{
		QDBusArgument qdbarg;
		qdbarg << value;
		QVariant qdbvar;
		qdbvar.setValue(qdbarg);
		return qdbvar;
	}

	// Convert  QDBusMessage to QList<qint16> Values, extracting data at a particular index of the construct
	Values DBusInterface::dBusMessagetoValues(QDBusMessage dbmess,int index)
	{
		QDBusArgument read = dbmess.arguments().at(index).value<QDBusArgument>();
		Values values;
		read >> values;
		return values;
	}

	// Display a list of qint's nicely as a string
	std::string DBusInterface::toString(Values v)
	{
		std::string out = "[";
		for (int i=0;i<v.size();i++)
		{
			out += std::to_string(v[i]);
			if(i <(v.size()-1))
			{
				out += ",";
			}
		}
		out += "]";
		return out;
	}

	// Check if the connection was estalished
	bool DBusInterface::checkConnection()
	{
		if (!QDBusConnection::sessionBus().isConnected())
		{
			fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
							"To start it, run:\n"
							"\teval `dbus-launch --auto-syntax`\n");
			std::cout<<"erreur"<<std::endl;
			return false;
		}
		std::cout<<"You are connected to the D-Bus session bus"<<std::endl;

		QDBusMessage nodelist=dbus_iface.call("GetNodesList");

		for (int i=0;i<nodelist.arguments().size();++i)
		{
			nodeList<<nodelist.arguments().at(i).value<QString>();
		}

		displayNodeList();

		return true;
	}


	//Display the list of Aseba Nodes connected on the Aseba Network
	void DBusInterface::displayNodeList()
	{
		qDebug()<<"Aseba nodes list: ";
		qDebug()<<nodeList;
	}

	//Load an Aseba script (.aesl) on an Aseba Node
	void DBusInterface::loadScript(const QString& script)
	{
		// check to ensure that we can find the file
		QFile file(script);
		QFileInfo fi(script);
		// include absolute path
		QString path_to_script = fi.absoluteFilePath();
		if(!file.exists())
		{
			qDebug() << "Cannot find file: " << path_to_script << ".";
		}
		else
		{
			// load the script
			dbus_iface.call("LoadScripts",path_to_script);
		}
	}

	//Get an Aseba variable from a Aseba node
	Values DBusInterface::getVariable(const QString& node, const QString& variable)
	{
		return dBusMessagetoValues(dbus_iface.call( "GetVariable",node, variable),0);
	}

	//Set an Aseba variable from a Aseba node
	void DBusInterface::setVariable(const QString& node, const QString& variable,Values& value)
	{
		dbus_iface.call("SetVariable",node,variable,valuetoVariant(value));
	}

	// Flag an event to listen for, and associate callback function (passed by pointer)
	void DBusInterface::connectEvent(const QString& eventName, std::function<void(Values)> callback)
	{
		// associate callback with event name
		callbacks[eventName] = callback;

		// listen
		eventfilterInterface->call("ListenEventName", eventName);
	}

	//Send Aseba Event using the ID of the Event
	void DBusInterface::sendEvent(quint16 eventID, Values& value)
	{
		QDBusArgument argument;
		argument<<eventID;
		QVariant variant;
		variant.setValue(argument);
		dbus_iface.call("SendEvent",variant,valuetoVariant(value));
	}

	//Send Aseba Event using the name of the Event
	void DBusInterface::sendEventName(const QString& eventName, const Values& value)
	{
		dbus_iface.call("SendEventName",eventName,valuetoVariant(value));
	}

	//Callback (slot) used to retrieve subscribed event information
	void DBusInterface::dispatchEvent(const QDBusMessage& message)
	{
		// unpack event
		QString eventReceivedName= message.arguments().at(1).value<QString>();
		Values eventReceivedValues = dBusMessagetoValues(message,2);
		// find and trigger matching callback
		if( callbacks.count(eventReceivedName) > 0)
		{
			callbacks[eventReceivedName](eventReceivedValues);
		}
	}
}
