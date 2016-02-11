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

#ifndef DASHELINTERFACE_H
#define DASHELINTERFACE_H

#include <QThread>
#include <QVector>
#include <QString>

#include <dashel/dashel.h>
#include "common/msg/msg.h"
#include "common/msg/NodesManager.h"


class DashelInterface : public QThread, public Dashel::Hub
{
	Q_OBJECT

	public:
		DashelInterface();
		~DashelInterface();
		void connectAseba(const QString& dashelTarget);
		void connectAseba(const QString& ip, const QString& port);
		void disconnectAseba();

		void sendEvent(unsigned id, const QVector<int> &values = QVector<int>(0));

	signals:
		void messageAvailable(Aseba::UserMessage *message);
		void dashelDisconnection();
		void dashelConnection();

	protected:
		// from QThread
		virtual void run();

		// from Dashel::Hub
		virtual void incomingData(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);

		// members
		Dashel::Stream* stream;
		QString dashelParams;
		bool isRunning;
		bool isConnected;
};

#endif // DASHELINTERFACE_H
