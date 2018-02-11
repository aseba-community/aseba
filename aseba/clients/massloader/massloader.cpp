/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2013:
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

#include <memory>
#include <cassert>
#include <iostream>
#include <dashel/dashel.h>
#include "common/consts.h"
#include "common/msg/msg.h"
#include "common/msg/NodesManager.h"
#include "common/utils/utils.h"
#include "compiler/compiler.h"
#include "transport/dashel_plugins/dashel-plugins.h"
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDomDocument>

namespace Aseba 
{
	using namespace Dashel;
	using namespace std;

	class MassLoader: public Hub, public NodesManager
	{
	protected:
		QString fileName;
		bool once;
		Stream* stream;

	public:
		MassLoader(const QString& fileName, bool once):fileName(fileName),once(once),stream(nullptr) {}
		void loadToTarget(const std::string& target);

	protected:
		// from Hub
		virtual void connectionCreated(Stream *stream);
		virtual void incomingData(Stream *stream);
		virtual void connectionClosed(Stream *stream, bool abnormal);

		// from NodesManager
		virtual void sendMessage(const Message& message);
		virtual void nodeDescriptionReceived(unsigned nodeId);

		// self
		void waitMs(int duration);
	};

	void MassLoader::loadToTarget(const std::string& target)
	{
		bool first(true);
		while (true)
		{
			if (first)
				first = false;
			else
				UnifiedTime(100).sleep();

			try
			{
				// attempt to open stream
				stream = connect(target);
				// verify that stream is valid
				if (!stream)
					continue;

				// TODO: make this timeout a parameter
				// wait 1 s
				waitMs(1000);

				// verify that stream is still valid
				if (!stream)
					continue;

				// requests descriptions
				pingNetwork();
				// then run
				run();
				// if we were interested to load only once, step for 1 second at most, then run for 1 second, and then stop execution
				if (once)
				{
					// wait max 1s until Hub::stop() is called
					step(1000);
					// wait 1s more
					waitMs(1000);
					return;
				}
			}
			catch (const DashelException& e)
			{
				// ignore Dsahel exceptions
			}

		}
	}

	void MassLoader::connectionCreated(Stream *stream)
	{
		//cerr << "Connection created to " << stream->getTargetName() << endl;
	}

	void MassLoader::incomingData(Stream *stream)
	{
		try
		{
			// get message
			unique_ptr<Message> message(Message::receive(stream));
			/*wcerr << L"  msg: ";
			message->dump(wcerr);
			wcerr << endl;*/
			// process it
			processMessage(message.get());
		}
		catch (const DashelException& e)
		{
			wcerr << L"Error while reading data: " << e.what() << endl;
		}
	}

	void MassLoader::connectionClosed(Stream *stream, bool abnormal)
	{
		//cerr << "Connection closed to " << stream->getTargetName() << endl;
		assert(stream == this->stream);
		this->stream = nullptr;
		stop();
		reset();
	}

	void MassLoader::sendMessage(const Message& message)
	{
		message.serialize(stream);
		stream->flush();
	}

	void MassLoader::nodeDescriptionReceived(unsigned nodeId)
	{
		//cerr << "Description received" << endl;
		assert(stream);

		// we have a new node description, load file to see if there is code for it
		CommonDefinitions commonDefinitions;

		// open file
		QFile file(fileName);
		if (!file.open(QFile::ReadOnly))
		{
			wcerr << QString("Cannot open file %0").arg(fileName).toStdWString() << endl;
			return;
		}
		// load document
		QDomDocument document("aesl-source");
		QString errorMsg;
		int errorLine;
		int errorColumn;
		if (!document.setContent(&file, false, &errorMsg, &errorLine, &errorColumn))
		{
			wcerr << QString("Error in XML source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn).toStdWString() << endl;
			return;
		}

		// FIXME: this code depends on event and contants being before any code
		QDomNode domNode = document.documentElement().firstChild();
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
							sendBytecode(stream, nodeId, std::vector<uint16_t>(bytecode.begin(), bytecode.end()));
							Run(nodeId).serialize(stream);
							stream->flush();
							wcerr << QString("! %1 bytecodes loaded to target %0, you can disconnect target !").arg(element.attribute("name")).arg(bytecode.size()).toStdWString() << endl;
							if (once) // if we have to run only once, stop now
								stop();
						}
						else
						{
							wcerr << L"Compilation error: " << error.toWString() << endl;
							break;
						}
					}
				}
				else if (element.tagName() == "event")
				{
					const QString eventName(element.attribute("name"));
					const unsigned eventSize(element.attribute("size").toUInt());
					if (eventSize > ASEBA_MAX_EVENT_ARG_SIZE)
					{
						wcerr << QString("Event %1 has a length %2 larger than maximum %3").arg(eventName).arg(eventSize).arg(ASEBA_MAX_EVENT_ARG_SIZE).toStdWString() << endl;
						break;
					}
					else
					{
						commonDefinitions.events.push_back(NamedValue(eventName.toStdWString(), eventSize));
					}
				}
				else if (element.tagName() == "constant")
				{
					commonDefinitions.constants.push_back(NamedValue(element.attribute("name").toStdWString(), element.attribute("value").toUInt()));
				}
			}
			domNode = domNode.nextSibling();
		}
	}

	void MassLoader::waitMs(int duration)
	{
		UnifiedTime now;
		while (true)
		{
			const UnifiedTime::Value delta((UnifiedTime() - now).value);
			if (delta >= duration)
				break;
			step(delta);
		}
	}
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	Dashel::initPlugins();

	QString target(ASEBA_DEFAULT_TARGET);

	if (app.arguments().size() < 2)
	{
		std::wcerr << L"Usage: " << app.arguments().first().toStdWString() << L" [--once] filename [target]" << std::endl;
		return 1;
	}

	const bool once = (app.arguments().at(1) == "--once");
	const int fileNameArgPos(once ? 2 : 1);
	const int targeetArgPos(once ? 3 : 2);

	if (app.arguments().size() > targeetArgPos)
		target = app.arguments().at(targeetArgPos);

	Aseba::MassLoader massLoader(app.arguments().at(fileNameArgPos), once);
	massLoader.loadToTarget(target.toStdString());
	return 0;
}
