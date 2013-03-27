/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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

/*
 
Current issues:
- improve layout

Possible issues:
- udev not available on tablet?
 
*/

#include <signal.h>
#include <QApplication>
#include <QCoreApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QString>
#include <QLocale>
#include <QLibraryInfo>
#include <QDebug>
#include <iostream>
#include <stdexcept>
#include "DashelTarget.h"
#include "../transport/dashel_plugins/dashel-plugins.h"
#include "ThymioVPLStandalone.h"

using namespace std;

void usage(const char *execName)
{
	cout << "Usage: " << execName << " (OPTIONS|TARGET)* " << endl;
	cout << "  where" << endl;
	cout << "OPTION is one of:" << endl;
	cout << " -h      this help" << endl;
	cout << "TARGET is a Dashel target (the last one is always considered)" << endl;
}

/**
	\defgroup thymiovpl VPL-only container application
*/

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	Dashel::initPlugins();
	
	// Information used by QSettings with default constructor
	QCoreApplication::setOrganizationName("EPFL-LSRO-Mobots");
	QCoreApplication::setOrganizationDomain("mobots.epfl.ch");
	QCoreApplication::setApplicationName("Thymio VPL");

	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	
	// override dashel signal handling
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	
	QString commandLineTarget;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] != '-')
		{
			commandLineTarget = argv[i];
			break;
		}
		else if (argv[i][1] != 0)
		{
			QString cmd(&(argv[i][1]));
			if (cmd == "h")
			{
				usage(argv[0]);
				return 0;
			}
		}
	}

	// translation files are loaded in DashelTarget.cpp
	QTranslator qtTranslator;
	app.installTranslator(&qtTranslator);

	QTranslator translator;
	app.installTranslator(&translator);

	QTranslator compilerTranslator;
	app.installTranslator(&compilerTranslator);
	
	
	// start normal aseba studio
	try
	{
		QVector<QTranslator*> translators;
		translators.push_back(&qtTranslator);
		translators.push_back(&translator);
		translators.push_back(&compilerTranslator);
		
		Aseba::ThymioVPLStandalone vpl(translators, commandLineTarget);
		vpl.show();
		return app.exec();
	}
	catch (std::runtime_error e)
	{
		std::cerr << "Program terminated with runtime error: " << e.what() << std::endl;
		return -1;
	}
}


