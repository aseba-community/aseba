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

#include <QCoreApplication>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <set>
#include <valarray>
#include <vector>
#include <iterator>
#include "medulla.h"
#include "medulla.moc"
#include "../common/consts.h"
#include "../common/types.h"
#include "../utils/utils.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QtDebug>

namespace Aseba 
{
	using namespace std;
	using namespace Dashel;
	
	/** \addtogroup medulla */
	/*@{*/
	
	std::vector<sint16> toAsebaVector(const Values& values)
	{
		std::vector<sint16> data;
		data.reserve(values.size());
		for (int i = 0; i < values.size(); ++i)
			data.push_back(values[i]);
		return data;
	}
	
	Values fromAsebaVector(const std::vector<sint16>& values)
	{
		Values data;
		for (size_t i = 0; i < values.size(); ++i)
			data.push_back(values[i]);
		return data;
	}
	
	
	AsebaNetworkInterface::AsebaNetworkInterface(Hub* hub) :
		QDBusAbstractAdaptor(hub),
		hub(hub)
	{
		qDBusRegisterMetaType<Values>();
		
		QDBusConnection::sessionBus().registerObject("/", hub);
		QDBusConnection::sessionBus().registerService("ch.epfl.mobots.Aseba");
	}
	
	void AsebaNetworkInterface::processMessage(Message *message)
	{
		DescriptionsManager::processMessage(message);
		
		UserMessage *userMessage = dynamic_cast<UserMessage *>(message);
		if (userMessage)
		{
			// TODO: process using event filter
		}
		
		Variables *variables = dynamic_cast<Variables *>(message);
		if (variables)
		{
			const unsigned nodeId(variables->source);
			const unsigned pos(variables->start);
			for (RequestsList::iterator it = pendingReads.begin(); it != pendingReads.end(); ++it)
			{
				RequestData* request(*it);
				if (request->nodeId == nodeId && request->pos == pos)
				{
					QDBusMessage &reply(request->reply);
					Values values(fromAsebaVector(variables->variables));
					QDBusArgument arg;
					arg << values;
					reply << arg.asVariant();
					QDBusConnection::sessionBus().send(reply);
					delete request;
					pendingReads.erase(it);
					break;
				}
			}
		}
		
		delete message;
	}
	
	QStringList AsebaNetworkInterface::GetNodesList() const
	{
		QStringList list;
		for (NodesNamesMap::const_iterator it = nodesNames.begin(); it != nodesNames.end(); ++it)
		{
			list.push_back(it.key());
		}
		return list;
	}
	
	QStringList AsebaNetworkInterface::GetVariablesList(const QString& node) const
	{
		NodesNamesMap::const_iterator it(nodesNames.find(node));
		if (it != nodesNames.end())
		{
			const unsigned nodeId(it.value());
			const NodesDescriptionsMap::const_iterator descIt(nodesDescriptions.find(nodeId));
			const NodeDescription& description(descIt->second);
			QStringList list;
			for (size_t i = 0; i < description.namedVariables.size(); ++i)
			{
				list.push_back(QString::fromStdString(description.namedVariables[i].name));
			}
			return list;
		}
		else
		{
			return QStringList();
		}
	}
	
	void AsebaNetworkInterface::SetVariable(const QString& node, const QString& variable, const Values& data) const
	{
		// TODO: generate error if bad arguments
		NodesNamesMap::const_iterator it(nodesNames.find(node));
		if (it != nodesNames.end())
		{
			const unsigned nodeId(it.value());
			bool ok;
			const unsigned pos(getVariablePos(nodeId, variable.toStdString(), &ok));
			if (ok)
			{
				Message* message(new SetVariables(nodeId, pos, toAsebaVector(data)));
				hub->sendMessage(message);
				delete message;
			}
		}
	}
	
	Values AsebaNetworkInterface::GetVariable(const QString& node, const QString& variable, const QDBusMessage &message)
	{
		// TODO: generate error if bad arguments
		NodesNamesMap::const_iterator it(nodesNames.find(node));
		if (it != nodesNames.end())
		{
			const unsigned nodeId(it.value());
			bool ok;
			const unsigned pos(getVariablePos(nodeId, variable.toStdString(), &ok));
			if (ok)
			{
				RequestData *request = new RequestData;
				request->nodeId = nodeId;
				request->pos = pos;
				message.setDelayedReply(true);
				request->reply = message.createReply();
				QDBusConnection::sessionBus().send(request->reply);
				
				pendingReads.push_back(request);
			}
		}
		return Values();
	}
	
	QDBusObjectPath AsebaNetworkInterface::CreateEventFilter()
	{
		// TODO
		return QDBusObjectPath();
	}
	
	void AsebaNetworkInterface::nodeDescriptionReceived(unsigned nodeId)
	{
		nodesNames[QString::fromStdString(nodesDescriptions[nodeId].name)] = nodeId;
	}
	
	//! Broadcast messages form any data stream to all others data streams including itself.
	Hub::Hub(unsigned port, bool verbose, bool dump, bool forward, bool rawTime) :
		verbose(verbose),
		dump(dump),
		forward(forward),
		rawTime(rawTime)
	{
		AsebaNetworkInterface* network(new AsebaNetworkInterface(this));
		QObject::connect(this, SIGNAL(messageAvailable(Message*)), network, SLOT(processMessage(Message*)));
		ostringstream oss;
		oss << "tcpin:port=" << port;
		Dashel::Hub::connect(oss.str());
	}
	
	void Hub::sendMessage(Message *message, Stream* sourceStream)
	{
		// write on all connected streams
		for (StreamsSet::iterator it = dataStreams.begin(); it != dataStreams.end();++it)
		{
			Stream* destStream(*it);
			
			if ((forward) && (destStream == sourceStream))
				continue;
			
			try
			{
				message->serialize(destStream);
				destStream->flush();
			}
			catch (DashelException e)
			{
				// if this stream has a problem, ignore it for now, and let Hub call connectionClosed later.
				std::cerr << "error while writing message" << std::endl;
			}
		}
	}
	
	// In QThread main function, we just make our Dashel hub switch listen for incoming data
	void Hub::run()
	{
		Dashel::Hub::run();
	}
	
	void Hub::incomingData(Stream *stream)
	{
		Message *message;
		try
		{
			message = Message::receive(stream);
		}
		catch (DashelException e)
		{
			// if this stream has a problem, ignore it for now, and let Hub call connectionClosed later.
			std::cerr << "error while reading message" << std::endl;
		}
		
		if (dump)
		{
			dumpTime(cout, rawTime);
			message->dump(cout);
		}
		
		sendMessage(message);
		
		// the receiver can delete it
		emit messageAvailable(message);
	}
	
	void Hub::connectionCreated(Stream *stream)
	{
		if (verbose)
		{
			dumpTime(cout, rawTime);
			cout << "Incoming connection from " << stream->getTargetName() << endl;
		}
		GetDescription().serialize(stream);
		stream->flush();
	}
	
	void Hub::connectionClosed(Stream* stream, bool abnormal)
	{
		if (verbose)
		{
			dumpTime(cout);
			if (abnormal)
				cout << "Abnormal connection closed to " << stream->getTargetName() << " : " << stream->getFailReason() << endl;
			else
				cout << "Normal connection closed to " << stream->getTargetName() << endl;
		}
	}
	
	/*@}*/
};

//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba medulla, connects aseba components together and with D-Bus, usage:\n";
	stream << programName << " [options] [additional targets]*\n";
	stream << "Options:\n";
	stream << "-v, --verbose   : makes the switch verbose\n";
	stream << "-d, --dump      : makes the switch dump the content of messages\n";
	stream << "-l, --loop      : makes the switch transmit messages back to the send, not only forward them.\n";
	stream << "-p port         : listens to incoming connection on this port\n";
	stream << "--rawtime       : shows time in the form of sec:usec since 1970\n";
	stream << "-h, --help      : shows this help\n";
	stream << "Additional targets are any valid Dashel targets." << std::endl;
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	
	unsigned port = ASEBA_DEFAULT_PORT;
	bool verbose = false;
	bool dump = false;
	bool forward = true;
	bool rawTime = false;
	std::vector<std::string> additionalTargets;
	
	int argCounter = 1;
	
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		
		if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0))
		{
			verbose = true;
		}
		else if ((strcmp(arg, "-d") == 0) || (strcmp(arg, "--dump") == 0))
		{
			dump = true;
		}
		else if ((strcmp(arg, "-l") == 0) || (strcmp(arg, "--loop") == 0))
		{
			forward = false;
		}
		else if (strcmp(arg, "-p") == 0)
		{
			arg = argv[++argCounter];
			port = atoi(arg);
		}
		else if (strcmp(arg, "--rawtime") == 0)
		{
			rawTime = true;
		}
		else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
		{
			dumpHelp(std::cout, argv[0]);
			return 0;
		}
		else
		{
			additionalTargets.push_back(argv[argCounter]);
		}
		argCounter++;
	}
	
	Aseba::Hub hub(port, verbose, dump, forward, rawTime);
	
	// TODO: add d-bus
	
	try
	{
		for (size_t i = 0; i < additionalTargets.size(); i++)
			((Dashel::Hub&)hub).connect(additionalTargets[i]);
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	hub.start();
	
	return app.exec();
}


