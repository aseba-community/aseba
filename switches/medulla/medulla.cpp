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
#include "../../common/consts.h"
#include "../../common/types.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QtXml>
#include <QtDebug>

#if DASHEL_VERSION_INT < 10003
#	error "You need at least Dashel version 1.0.3 to compile Medulla"
#endif // DAHSEL_VERSION_INT

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
	
	void EventFilterInterface::emitEvent(const quint16 id, const QString& name, const Values& data)
	{
		emit Event(id, name, data);
	}
	
	void EventFilterInterface::ListenEvent(const quint16 event)
	{
		network->listenEvent(this, event);
	}
	
	void EventFilterInterface::ListenEventName(const QString& name, const QDBusMessage &message)
	{
		size_t event;
		if (network->commonDefinitions.events.contains(name.toStdWString(), &event))
			network->listenEvent(this, event);
		else
			network->DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("no event named %0").arg(name)));
	}
	
	void EventFilterInterface::IgnoreEvent(const quint16 event)
	{
		network->ignoreEvent(this, event);
	}
	
	void EventFilterInterface::IgnoreEventName(const QString& name, const QDBusMessage &message)
	{
		size_t event;
		if (network->commonDefinitions.events.contains(name.toStdWString(), &event))
			network->ignoreEvent(this, event);
		else
			network->DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("no event named %0").arg(name)));
	}
	
	void EventFilterInterface::Free()
	{
		network->filterDestroyed(this);
		deleteLater();
	}
	
	AsebaNetworkInterface::AsebaNetworkInterface(Hub* hub, bool systemBus) :
		QDBusAbstractAdaptor(hub),
		hub(hub),
		systemBus(systemBus),
		eventsFiltersCounter(0)
	{
		qDBusRegisterMetaType<Values>();
		
		//FIXME: here no error handling is done, with system bus these calls can fail	
		DBusConnectionBus().registerObject("/", hub);
		DBusConnectionBus().registerService("ch.epfl.mobots.Aseba");
		
		// regular network pinging
		QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(PingNetwork()));
		timer->start(1000);
	}
	
	void AsebaNetworkInterface::processMessage(Message *message, const Dashel::Stream* sourceStream)
	{
		// send messages to Dashel peers
		hub->sendMessage(message, sourceStream);
		
		// scan this message for nodes descriptions
		NodesManager::processMessage(message);
		
		// if user message, send to D-Bus as well
		UserMessage *userMessage = dynamic_cast<UserMessage *>(message);
		if (userMessage)
		{
			sendEventOnDBus(userMessage->type, fromAsebaVector(userMessage->data));
		}
		
		// if variables, check for pending answers
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
					reply << QVariant::fromValue(values);
					DBusConnectionBus().send(reply);
					delete request;
					pendingReads.erase(it);
					break;
				}
			}
		}
		
		delete message;
	}
	
	void AsebaNetworkInterface::sendEventOnDBus(const quint16 event, const Values& data)
	{
		QList<EventFilterInterface*> filters = eventsFilters.values(event);
		QString name;
		if (event < commonDefinitions.events.size())
			name = QString::fromStdWString(commonDefinitions.events[event].name);
		else
			name = "?";
		for (int i = 0; i < filters.size(); ++i)
			filters.at(i)->emitEvent(event, name, data);
	}
	
	void AsebaNetworkInterface::listenEvent(EventFilterInterface* filter, quint16 event)
	{
		eventsFilters.insert(event, filter);
	}
	
	void AsebaNetworkInterface::ignoreEvent(EventFilterInterface* filter, quint16 event)
	{
		eventsFilters.remove(event, filter);
	}
	
	void AsebaNetworkInterface::filterDestroyed(EventFilterInterface* filter)
	{
		QList<quint16> events = eventsFilters.keys(filter);
		for (int i = 0; i < events.size(); ++i)
			eventsFilters.remove(events.at(i), filter);
	}
	
	void AsebaNetworkInterface::LoadScripts(const QString& fileName, const QDBusMessage &message)
	{
		QFile file(fileName);
		if (!file.open(QFile::ReadOnly))
		{
			DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("file %0 does not exists").arg(fileName)));
			return;
		}
		
		QDomDocument document("aesl-source");
		QString errorMsg;
		int errorLine;
		int errorColumn;
		if (!document.setContent(&file, false, &errorMsg, &errorLine, &errorColumn))
		{
			DBusConnectionBus().send(message.createErrorReply(QDBusError::Other, QString("Error in XML source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn)));
			return;
		}
		
		commonDefinitions.events.clear();
		commonDefinitions.constants.clear();
		userDefinedVariablesMap.clear();
		
		int noNodeCount = 0;
		QDomNode domNode = document.documentElement().firstChild();
		
		// FIXME: this code depends on event and contants being before any code
		bool wasError = false;
		while (!domNode.isNull())
		{
			if (domNode.isElement())
			{
				QDomElement element = domNode.toElement();
				if (element.tagName() == "node")
				{
					bool ok;
					const unsigned nodeId(getNodeId(element.attribute("name").toStdWString(), element.attribute("nodeId", 0).toUInt(), &ok));
					if (ok)
					{
						std::wistringstream is(element.firstChild().toText().data().toStdWString());
						Error error;
						BytecodeVector bytecode;
						unsigned allocatedVariablesCount;
						
						Compiler compiler;
						compiler.setTargetDescription(getDescription(nodeId));
						compiler.setCommonDefinitions(&commonDefinitions);
						bool result = compiler.compile(is, bytecode, allocatedVariablesCount, error);
						
						if (result)
						{
							typedef std::vector<Message*> MessageVector;
							MessageVector messages;
							sendBytecode(messages, nodeId, std::vector<uint16>(bytecode.begin(), bytecode.end()));
							for (MessageVector::const_iterator it = messages.begin(); it != messages.end(); ++it)
							{
								hub->sendMessage(*it);
								delete *it;
							}
							Run msg(nodeId);
							hub->sendMessage(msg);
						}
						else
						{
							DBusConnectionBus().send(message.createErrorReply(QDBusError::Failed, QString::fromStdWString(error.toWString())));
							wasError = true;
							break;
						}
						// retrieve user-defined variables for use in get/set
						userDefinedVariablesMap[element.attribute("name")] = *compiler.getVariablesMap();
					}
					else
						noNodeCount++;
				}
				else if (element.tagName() == "event")
				{
					const QString eventName(element.attribute("name"));
					const unsigned eventSize(element.attribute("size").toUInt());
					if (eventSize > ASEBA_MAX_EVENT_ARG_SIZE)
					{
						DBusConnectionBus().send(message.createErrorReply(QDBusError::Failed, QString("Event %1 has a length %2 larger than maximum %3").arg(eventName).arg(eventSize).arg(ASEBA_MAX_EVENT_ARG_SIZE)));
						wasError = true;
						break;
					}
					else
					{
						commonDefinitions.events.push_back(NamedValue(eventName.toStdWString(), eventSize));
					}
				}
				else if (element.tagName() == "constant")
				{
					commonDefinitions.constants.push_back(NamedValue(element.attribute("name").toStdWString(), element.attribute("value").toInt()));
				}
			}
			domNode = domNode.nextSibling();
		}
		
		// check if there was an error
		if (wasError)
		{
			std::wcerr << QString("There was an error while loading script %1").arg(fileName).toStdWString() << std::endl;
			commonDefinitions.events.clear();
			commonDefinitions.constants.clear();
			userDefinedVariablesMap.clear();
		}
		
		// check if there was some matching problem
		if (noNodeCount)
		{
			std::wcerr << QString("%0 scripts have no corresponding nodes in the current network and have not been loaded.").arg(noNodeCount).toStdWString() << std::endl;
		}
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

	qint16 AsebaNetworkInterface::GetNodeId(const QString& node, const QDBusMessage &message) const
	{
		NodesNamesMap::const_iterator nodeIt(nodesNames.find(node));
		if (nodeIt == nodesNames.end())
		{
			DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("node %0 does not exists").arg(node)));
			return 0;
		}
		return nodeIt.value();	
	}
	
	QString AsebaNetworkInterface::GetNodeName(const quint16 nodeId, const QDBusMessage &message) const
	{
		const QString nodeName(QString::fromStdWString(getNodeName(nodeId)));
		if (nodeName.isEmpty())
		{
			DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("node %0 does not exists").arg(nodeId)));
			return 0;
		}
		return nodeName;
	}
	
	bool AsebaNetworkInterface::IsConnected(const QString& node, const QDBusMessage &message) const
	{
		NodesNamesMap::const_iterator nodeIt(nodesNames.find(node));
		if (nodeIt == nodesNames.end())
		{
			DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("node %0 does not exists").arg(node)));
			return 0;
		}
		return nodes.find(nodeIt.value())->second.connected;
	}

	//! Send the list of variables if the node is fully known, or an empty list otherwise
	QStringList AsebaNetworkInterface::GetVariablesList(const QString& node) const
	{
		NodesNamesMap::const_iterator it(nodesNames.find(node));
		if (it != nodesNames.end())
		{
			// node defined variables
			const unsigned nodeId(it.value());
			const NodesMap::const_iterator descIt(nodes.find(nodeId));
			const Node& description(descIt->second);
			QStringList list;
			for (size_t i = 0; i < description.namedVariables.size(); ++i)
			{
				list.push_back(QString::fromStdWString(description.namedVariables[i].name));
			}
			
			// user defined variables
			const UserDefinedVariablesMap::const_iterator userVarMapIt(userDefinedVariablesMap.find(node));
			if (userVarMapIt != userDefinedVariablesMap.end())
			{
				const VariablesMap& variablesMap(*userVarMapIt);
				for (VariablesMap::const_iterator jt = variablesMap.begin(); jt != variablesMap.end(); ++jt)
				{
					list.push_back(QString::fromStdWString(jt->first));
				}
			}
			
			return list;
		}
		else
		{
			return QStringList();
		}
	}
	
	void AsebaNetworkInterface::SetVariable(const QString& node, const QString& variable, const Values& data, const QDBusMessage &message) const
	{
		// make sure the node exists
		NodesNamesMap::const_iterator nodeIt(nodesNames.find(node));
		if (nodeIt == nodesNames.end())
		{
			DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("node %0 does not exists").arg(node)));
			return;
		}
		const unsigned nodeId(nodeIt.value());
		
		unsigned pos(unsigned(-1));
		
		// check whether variable is user-defined
		const UserDefinedVariablesMap::const_iterator userVarMapIt(userDefinedVariablesMap.find(node));
		if (userVarMapIt != userDefinedVariablesMap.end())
		{
			const VariablesMap& userVarMap(userVarMapIt.value());
			const VariablesMap::const_iterator userVarIt(userVarMap.find(variable.toStdWString()));
			if (userVarIt != userVarMap.end())
			{
				pos = userVarIt->second.first;
			}
		}
		
		// if variable is not user-defined, check whether it is provided by this node
		if (pos == unsigned(-1))
		{
			bool ok;
			pos = getVariablePos(nodeId, variable.toStdWString(), &ok);
			if (!ok)
			{
				DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("variable %0 does not exists in node %1").arg(variable).arg(node)));
				return;
			}
		}
		
		SetVariables msg(nodeId, pos, toAsebaVector(data));
		hub->sendMessage(msg);
	}
	
	Values AsebaNetworkInterface::GetVariable(const QString& node, const QString& variable, const QDBusMessage &message)
	{
		// make sure the node exists
		NodesNamesMap::const_iterator nodeIt(nodesNames.find(node));
		if (nodeIt == nodesNames.end())
		{
			DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("node %0 does not exists").arg(node)));
			return Values();
		}
		const unsigned nodeId(nodeIt.value());
		
		unsigned pos(unsigned(-1));
		unsigned length(unsigned(-1));
		
		// check whether variable is user-defined
		const UserDefinedVariablesMap::const_iterator userVarMapIt(userDefinedVariablesMap.find(node));
		if (userVarMapIt != userDefinedVariablesMap.end())
		{
			const VariablesMap& userVarMap(userVarMapIt.value());
			const VariablesMap::const_iterator userVarIt(userVarMap.find(variable.toStdWString()));
			if (userVarIt != userVarMap.end())
			{
				pos = userVarIt->second.first;
				length = userVarIt->second.second;
			}
		}
		
		// if variable is not user-defined, check whether it is provided by this node
		if (pos == unsigned(-1))
		{
			bool ok1, ok2;
			pos = getVariablePos(nodeId, variable.toStdWString(), &ok1);
			length = getVariableSize(nodeId, variable.toStdWString(), &ok2);
			if (!(ok1 && ok2))
			{
				DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("variable %0 does not exists in node %1").arg(variable).arg(node)));
				return Values();
			}
		}
		
		// send request to aseba network
		{
			GetVariables msg(nodeId, pos, length);
			hub->sendMessage(msg);
		}
		
		// build bookkeeping for async reply
		RequestData *request = new RequestData;
		request->nodeId = nodeId;
		request->pos = pos;
		message.setDelayedReply(true);
		request->reply = message.createReply();
		
		pendingReads.push_back(request);
		return Values();
	}
	
	void AsebaNetworkInterface::SendEvent(const quint16 event, const Values& data)
	{
		// send event to DBus listeners
		sendEventOnDBus(event, data);
		
		// send on TCP
		UserMessage msg(event, toAsebaVector(data));
		hub->sendMessage(msg);
	}
	
	void AsebaNetworkInterface::SendEventName(const QString& name, const Values& data, const QDBusMessage &message)
	{
		size_t event;
		if (commonDefinitions.events.contains(name.toStdWString(), &event))
			SendEvent(event, data);
		else
			DBusConnectionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("no event named %0").arg(name)));
	}
	
	QDBusObjectPath AsebaNetworkInterface::CreateEventFilter()
	{
		QDBusObjectPath path(QString("/events_filters/%0").arg(eventsFiltersCounter++));
		DBusConnectionBus().registerObject(path.path(), new EventFilterInterface(this), QDBusConnection::ExportScriptableContents);
		return path;
	}
	
	void AsebaNetworkInterface::PingNetwork()
	{
		pingNetwork();
	}
	
	void AsebaNetworkInterface::sendMessage(const Message& message)
	{
		hub->sendMessage(message);
	}
	
	void AsebaNetworkInterface::nodeDescriptionReceived(unsigned nodeId)
	{
		nodesNames[QString::fromStdWString(nodes[nodeId].name)] = nodeId;
	}

	inline QDBusConnection AsebaNetworkInterface::DBusConnectionBus() const
	{
		if (systemBus)
			return QDBusConnection::systemBus();
		else
			return QDBusConnection::sessionBus();
	}
	
	// the following methods run in the main thread (event loop)
	
	Hub::Hub(unsigned port, bool verbose, bool dump, bool forward, bool rawTime, bool systemBus) :
		#ifdef DASHEL_VERSION_INT
		Dashel::Hub(verbose || dump),
		#endif // DASHEL_VERSION_INT
		verbose(verbose),
		dump(dump),
		forward(forward),
		rawTime(rawTime)
	{
		// TODO: work in progress to remove ugly delay
		AsebaNetworkInterface* network(new AsebaNetworkInterface(this, systemBus));
		QObject::connect(this, SIGNAL(messageAvailable(Message*, const Dashel::Stream*)), network, SLOT(processMessage(Message*, const Dashel::Stream*)));
		ostringstream oss;
		oss << "tcpin:port=" << port;
		Dashel::Hub::connect(oss.str());
	}
	
	void Hub::sendMessage(const Message *message, const Stream* sourceStream)
	{
		// dump if requested
		if (dump)
		{
			dumpTime(cout, rawTime);
			message->dump(wcout);
			cout << std::endl;
		}
		
		// Called from the dbus thread, not the Hub thread, need to lock	
		lock();

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

		unlock();
	}
	
	void Hub::sendMessage(const Message& message, const Stream* sourceStream)
	{
		sendMessage(&message, sourceStream);
	}
	
	// the following methods run in the blocking reception thread
	
	// In QThread main function, we just make our Dashel hub switch listen for incoming data
	void Hub::run()
	{
		Dashel::Hub::run();
	}
	
	// the following method run in the blocking reception thread
	
	void Hub::incomingData(Stream *stream)
	{
		// receive message
		Message *message(0);
		try
		{
			message = Message::receive(stream);
		}
		catch (DashelException e)
		{
			// if this stream has a problem, ignore it for now, and let Hub call connectionClosed later.
			std::cerr << "error while reading message" << std::endl;
		}
		
		// send on DBus, the receiver can delete it
		emit messageAvailable(message, stream);
	}
	
	void Hub::connectionCreated(Stream *stream)
	{
		if (verbose)
		{
			dumpTime(cout, rawTime);
			cout << "Incoming connection from " << stream->getTargetName() << endl;
		}
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
	stream << "--system        : connects medulla to the system d-bus bus\n";	
	stream << "-h, --help      : shows this help\n";
	stream << "-V, --version   : shows the version number\n";
	stream << "Additional targets are any valid Dashel targets." << std::endl;
	stream << "Report bugs to: aseba-dev@gna.org" << std::endl;
}

//! Show version
void dumpVersion(std::ostream &stream)
{
	stream << "Aseba medulla " << ASEBA_VERSION << std::endl;
	stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
	stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	Dashel::initPlugins();
	
	unsigned port = ASEBA_DEFAULT_PORT;
	bool verbose = false;
	bool dump = false;
	bool forward = true;
	bool rawTime = false;
	bool systemBus = false;
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
		else if (strcmp(arg, "--system") == 0)
		{
			systemBus = true;
		}
		else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
		{
			dumpHelp(std::cout, argv[0]);
			return 0;
		}
		else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
		{
			dumpVersion(std::cout);
			return 0;
		}
		else
		{
			additionalTargets.push_back(argv[argCounter]);
		}
		argCounter++;
	}
	
	Aseba::Hub hub(port, verbose, dump, forward, rawTime, systemBus);
	
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


