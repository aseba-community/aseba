/*
	A Qt binding for Aseba that relies on DBus

	Supports event sending/receiving, Aseba scripts Loading, as well as setting/getting an Aseba variable.

	Authors: Frank Bonnet, Stefan Witwicki
	Copyright (C) 2007--2016

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

// You need to add qdbus in the configuration file of the project (.pro)
// CONFIG += qdbus

// This code uses C++11

#include <iostream>
#include <unistd.h>
#include "dbusinterface.h"
#include <QTest>

using namespace std;
using namespace Aseba;

// global pointer to an object of class DBusInterface, which serves as the interface to our Aseba network
DBusInterface *ThymioIIInterface;

// Example of a custom callback function, to connect to ThymioIIInterface
// Note: connected callbacks need not be global (see std::function documentation in C++11 for more information)
void my_callback(const Values& event_values)
{
	cout << "\nThymio encountered an obstacle " << DBusInterface::toString(event_values) << endl;

	// the bearing of the obstacle determines the sound played
	qint16 max_index=0;
	qint16 max=-1;
	for(int i=0; i<event_values.size(); i++)
	{
		if(event_values[i] > max)
		{
			max = event_values[i];
			max_index = i;
		}
	}
	Values sound_index({max_index});
	ThymioIIInterface->sendEventName("PlaySound", sound_index);
}


int main(int argc, char *argv[])
{
	// Qt init
	QCoreApplication a(argc, argv);

	// interface instantiation
	cout << "\n-attempting DBus connection-" << endl;
	ThymioIIInterface = new DBusInterface();

	// load an Aseba script onto Thymio II
	cout << "\n-loading an aesl script onto the robot-" << endl;
	ThymioIIInterface->loadScript("ScriptDBusThymio.aesl");

	// set Aseba variables
	cout << "\n-setting wheel speeds to move forward-" << endl;
	ThymioIIInterface->setVariable("thymio-II", "motor.left.target", Values({100}));
	ThymioIIInterface->setVariable("thymio-II", "motor.right.target", Values({100}));

	QTest::qSleep(5000);

	// send an Event
	cout << "\n-sending stop event-" << endl;
	Values event_args({});
	ThymioIIInterface->sendEventName("Stop", event_args);

	// flag an envent to be listened to, and connect its occurence with your own callback funtion
	cout << "\n-listening for ObstacleDetected events-" << endl;
	ThymioIIInterface->connectEvent("ObstacleDetected", my_callback);

	return a.exec();
}

