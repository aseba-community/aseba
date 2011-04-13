/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

#include <signal.h>
#include <QApplication>
#include <QCoreApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QString>
#include <QLocale>
#include <QLibraryInfo>
#include <iostream>
#include <stdexcept>
#include "MainWindow.h"

/**
	\defgroup studio Integrated Development Editor
*/

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	// Information used by QSettings with default constructor
	QCoreApplication::setOrganizationName("EPFL-LSRO-Mobots");
	QCoreApplication::setOrganizationDomain("mobots.epfl.ch");
	QCoreApplication::setApplicationName("Aseba Studio");

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
	}
	
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);
	
	QTranslator translator;
	translator.load(QString(":/asebastudio_") + QLocale::system().name());
	app.installTranslator(&translator);
	
	try
	{
		QVector<QTranslator*> translators;
		translators.push_back(&qtTranslator);
		translators.push_back(&translator);
		Aseba::MainWindow window(translators, commandLineTarget);
		window.show();
		return app.exec();
	}
	catch (std::runtime_error e)
	{
		std::cerr << "Program terminated with runtime error: " << e.what() << std::endl;
		return -1;
	}
}


