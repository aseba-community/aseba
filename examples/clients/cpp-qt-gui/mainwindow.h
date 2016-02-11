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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

#include "dashelinterface.h"

class MainWindow : public QWidget
{
	Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = 0);
		~MainWindow();

	protected:
		DashelInterface dashelInterface;

		//
		QLineEdit* ip;
		QLineEdit* port;
		QLabel* status;
		QPushButton* connectBtn;
		//
		QLineEdit* msgId;
		QLineEdit* msgValue;
		QPushButton* sendMsg;
		//
		QPushButton* quitBtn;

	protected slots:
		void connectToDashel();
		void sendUserMessage();

		// interface with Dashel
		void connectionFromDashel();
		void disconnectionFromDashel();
		void messageFromDashel(Aseba::UserMessage *message);
};

#endif // MAINWINDOW_H
