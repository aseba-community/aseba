/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details

	This example is based on a first work of Olivier Marti (2010 - 2011).
	Stripped down & cleaned version by Florian Vaussard (2013).
	
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

#include "dashelinterface.h"

#include <QMessageBox>
#include <QDebug>


DashelInterface::DashelInterface() :
	stream(0), isRunning(false), isConnected(false)
{
}

DashelInterface::~DashelInterface()
{
}

// Connect to any kind of valid Dashel target (TCP, serial, CAN,...)
void DashelInterface::connectAseba(const QString& dashelTarget)
{
	dashelParams = dashelTarget;
	isRunning = true;
	start();
}

// Connect through a TCP socket
void DashelInterface::connectAseba(const QString& ip, const QString& port)
{
	connectAseba("tcp:" + ip + ";port=" + port);
}

// Cleanly disconnect
void DashelInterface::disconnectAseba()
{
	isRunning = false;
	Dashel::Hub::stop();
	wait();
}

// Message coming from a node.
// Consider _only_ UserMessage. Discard other types of messages (debug, etc.)
void DashelInterface::incomingData(Dashel::Stream *stream)
{
	Aseba::Message *message = Aseba::Message::receive(stream);
	Aseba::UserMessage *userMessage = dynamic_cast<Aseba::UserMessage *>(message);

	if (userMessage)
		emit messageAvailable(userMessage);
	else
		delete message;
}

// Send a UserMessage with ID 'id', and optionnally some data values
void DashelInterface::sendEvent(unsigned id, const QVector<int>& values)
{
	if (this->isConnected)
	{
		Aseba::VariablesDataVector data(values.size());
		QVectorIterator<int> it(values);
		unsigned i = 0;
		while (it.hasNext())
			data[i++] = it.next();
		Aseba::UserMessage(id, data).serialize(stream);
		stream->flush();
	}
}

// Dashel connection was closed
void DashelInterface::connectionClosed(Dashel::Stream* stream, bool abnormal)
{
		Q_UNUSED(stream);
		Q_UNUSED(abnormal);
		emit dashelDisconnection();
		this->stream = 0;
}

// internals
void DashelInterface::run()
{
	while (1)
	{
		try
		{
			stream = Dashel::Hub::connect(dashelParams.toStdString());

			emit dashelConnection();
			qDebug() << "Connected to target: " << dashelParams;
			isConnected = true;
			break;
		}
		catch (Dashel::DashelException e)
		{
			qDebug() << "Cannot connect to target: " << dashelParams;
			sleep(1000000L);	// 1s
		}

	}

	while (isRunning)
		Dashel::Hub::run();
}

