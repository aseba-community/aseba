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

#include <iostream>
#include <QCoreApplication>
#include <QTimer>
#include "targetlist-qt.h"

using namespace std;

QtTargetLister::QtTargetLister(int argc, char* argv[]) :
	QCoreApplication(argc, argv)
{
	connect(&targets, SIGNAL(zeroconfTargetFound(const Aseba::Zeroconf::TargetInformation&)), SLOT(targetFound(const Aseba::Zeroconf::TargetInformation&)));
	// Browse for _aseba._tcp services on all interfaces
	targets.browse();
	// Run for 10 seconds
	QTimer::singleShot(10000, this, SLOT(quit()));
}

void QtTargetLister::targetFound(const Aseba::Zeroconf::TargetInformation& target)
{
	// output could be JSON but for now is Dashel target [Target name (DNS domain)]
	cout << target.host << ";port=" << target.port;
	cout << " [" << target.name << " (" << target.regtype+"."+target.domain << ")]" << endl;
	// also output properties, typically the DNS-encoded full host name and fields from TXT record
	for (auto const& field: target.properties)
	{
		cout << "\t" << field.first << ":";
		// ids and pids are a special case because they contain vectors of 16-bit integers
		if (field.first.rfind("ids") == field.first.size()-3)
			for (size_t i = 0; i < field.second.length(); i += 2)
				cout << " " << (int(field.second[i])<<8) + int(field.second[i+1]);
		else
			cout << " " << field.second;
		cout << endl;
	}
}

int main(int argc, char* argv[])
{
	QtTargetLister app(argc, argv);
	return app.exec();
}
