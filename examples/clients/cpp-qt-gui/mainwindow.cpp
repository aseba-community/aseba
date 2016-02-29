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

#include <QMessageBox>
#include <QVector>
#include <QDebug>

#include "mainwindow.h"
#include "dashelinterface.h"

#include <mainwindow.moc>

MainWindow::MainWindow( QWidget *parent) :
	QWidget(parent)
{
	ip = new QLineEdit("127.0.0.1");
	port = new QLineEdit("33333");
	status = new QLabel(tr("Not connected"));
	connectBtn = new QPushButton(tr("Connect"));
	msgId = new QLineEdit("0");
	msgValue = new QLineEdit("0");
	sendMsg = new QPushButton(tr("Send Message"));
	quitBtn = new QPushButton(tr("Quit"));

	// Connection parameters
	QGroupBox* grp1 = new QGroupBox(tr("Connection"));
	QVBoxLayout* grp1layout = new QVBoxLayout();
	grp1->setLayout(grp1layout);
	grp1layout->addWidget(new QLabel(tr("IP")));
	grp1layout->addWidget(ip);
	grp1layout->addWidget(new QLabel(tr("Port")));
	grp1layout->addWidget(port);
	grp1layout->addWidget(status);
	grp1layout->addWidget(connectBtn);

	// UserMessage parameters
	QGroupBox* grp2 = new QGroupBox(tr("User message"));
	QVBoxLayout* grp2layout = new QVBoxLayout();
	grp2->setLayout(grp2layout);
	grp2layout->addWidget(new QLabel(tr("Message ID")));
	grp2layout->addWidget(msgId);
	grp2layout->addWidget(new QLabel(tr("Data 0")));
	grp2layout->addWidget(msgValue);
	grp2layout->addWidget(sendMsg);

	// Overall layout
	QVBoxLayout* main_layout = new QVBoxLayout();
	main_layout->addWidget(grp1);
	main_layout->addWidget(grp2);
	main_layout->addWidget(quitBtn);
	setLayout(main_layout);

	// connect widgets
	connect(connectBtn, SIGNAL(clicked()), this, SLOT(connectToDashel()));
	connect(sendMsg, SIGNAL(clicked()), this, SLOT(sendUserMessage()));
	connect(quitBtn, SIGNAL(clicked()), this, SLOT(close()));

	// connect signals from dashel
	connect(&dashelInterface, SIGNAL(messageAvailable(Aseba::UserMessage *)), SLOT(messageFromDashel(Aseba::UserMessage *)), Qt::QueuedConnection);
	connect(&dashelInterface, SIGNAL(dashelDisconnection()), SLOT(disconnectionFromDashel()), Qt::QueuedConnection);
	connect(&dashelInterface, SIGNAL(dashelConnection()), SLOT(connectionFromDashel()), Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
	dashelInterface.disconnectAseba();
}


void MainWindow::connectToDashel()
{
	dashelInterface.connectAseba(ip->text(), port->text());
}


/*
 * Example on how to broadcast a message to Aseba nodes.
 * The message ID depend on the events defined inside your Aseba code.
 * In this simple example, we send 1 data with the message (can be zero,
 * or more).
 */
void MainWindow::sendUserMessage()
{
	QVector<int> values(0);
	values.append(msgValue->text().toInt());
	dashelInterface.sendEvent(msgId->text().toInt(), values);
}


/*
 * Receive messages sent by Aseba nodes
 *
 * userMessage->source
 * 	ID of the Aseba node sending the event
 * userMessage->type
 * 	Type (EVENT_ID) of event. Depend on the node
 * userMessage->data
 * 	Data associated with the event
 */
void MainWindow::messageFromDashel(Aseba::UserMessage *message)
{
	// Process the message _here_
	// ...

	// do not remove this line
	delete message;
}


/*
 * We are now connected...
 */
void MainWindow::connectionFromDashel()
{
	status->setText(tr("Connected"));
	connectBtn->setEnabled(false);

	// If you need to initialize the nodes, perform the
	// work here

	// dashelInterface.sendEvent(...)
}

/*
 * Oops, we have lost the connection...
 */
void MainWindow::disconnectionFromDashel()
{
	status->setText(tr("Not connected"));
	connectBtn->setEnabled(true);

	QMessageBox::warning(0, tr("Oops"), tr("Disconnected from Dashel"));
}

