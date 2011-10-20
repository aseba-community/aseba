/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

#include "HelpViewer.h"

#include <QtHelp/QHelpContentWidget>
#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFile>
#include <QIODevice>
#include <QUrl>
#include <QVariant>
#include <QSettings>
#include <QDesktopServices>
#include <QMessageBox>

#include <QDebug>

#include <HelpViewer.moc>


// define the name of the help file
#ifndef HELP_FILE
	#define HELP_FILE	"aseba-doc.qhc"
#endif

// define a user path to the help file (try after the application binary path)
#ifndef HELP_USR_PATH
	#define HELP_USR_PATH	"/usr/share/doc/aseba/"
#endif


namespace Aseba
{
	QString HelpViewer::DEFAULT_LANGUAGE = "en";

	HelpViewer::HelpViewer(QWidget* parent):
		QWidget(parent)
	{
		QString helpFile;

		// search for the help file
		QDir helpPath = QDir(QCoreApplication::applicationDirPath());
		QDir helpUsrPath = QDir(HELP_USR_PATH);
		if (helpPath.exists(HELP_FILE))
		{
			// use the application binary directory
			helpFound = true;
			helpFile = helpPath.absoluteFilePath(HELP_FILE);
		}
		else if (helpUsrPath.exists(HELP_FILE))
		{
			// use the user-given path
			helpFound = true;
			helpFile = helpUsrPath.absoluteFilePath(HELP_FILE);
		}
		else
		{
			// not found!
			helpFound = false;
			helpFile = "";
			QString message = tr("The help file %0 has not been found. " \
					     "It should be located in the same directory as the binary of Aseba Studio. " \
					     "Please check your installation, or report a bug.").arg(HELP_FILE);
			QMessageBox::warning(this, tr("Help file not found"), message);
		}

		helpEngine = new QHelpEngine(helpFile, this);
		helpEngine->setupData();

		// navigation buttons
		previous = new QPushButton(tr("Previous"));
		previous->setEnabled(false);
		next = new QPushButton(tr("Next"));
		next->setEnabled(false);
		home = new QPushButton(tr("Home"));
		QHBoxLayout* buttonLayout = new QHBoxLayout();
		buttonLayout->addWidget(previous);
		buttonLayout->addWidget(home);
		buttonLayout->addWidget(next);
		buttonLayout->addStretch();

		// QTextBrower
		viewer = new HelpBrowser(helpEngine);

		// set default language
		setLanguage();

		// help layout
		QSplitter *helpSplitter = new QSplitter(Qt::Horizontal);
		helpSplitter->addWidget(helpEngine->contentWidget());
		helpSplitter->addWidget(viewer);
		helpSplitter->setStretchFactor(helpSplitter->indexOf(viewer), 1);

		// main layout
		QVBoxLayout* mainLayout = new QVBoxLayout();
		mainLayout->addLayout(buttonLayout);
		//mainLayout->addWidget(viewer);
		mainLayout->addWidget(helpSplitter);

		setLayout(mainLayout);

		// connect
		connect(previous, SIGNAL(clicked()), this, SLOT(previousClicked()));
		connect(next, SIGNAL(clicked()), this, SLOT(nextClicked()));
		connect(home, SIGNAL(clicked()), this, SLOT(homeClicked()));
		connect(viewer, SIGNAL(backwardAvailable(bool)), this, SLOT(backwardAvailable(bool)));
		connect(viewer, SIGNAL(forwardAvailable(bool)), this, SLOT(forwardAvailable(bool)));
		connect(helpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl&)), viewer, SLOT(setSource(const QUrl&)));

		// restore window state, if available
		if (readSettings() == false)
			resize(800, 500);

		setWindowTitle(tr("Aseba Studio Help"));
	}

	HelpViewer::~HelpViewer()
	{
		writeSettings();
	}

	void HelpViewer::setLanguage(QString lang)
	{
		if (!helpFound)
			return;

		// check if the language is part of the existing filters
		if (helpEngine->customFilters().contains(lang))
		{
			helpEngine->setCurrentFilter(lang);
			this->language = lang;
		}
		else
		{
			// rollback to default language
			QString message = tr("The help filter for the langauge \"%0\" has not been found. " \
					     "Falling back to the default language (%1). " \
					     "This is probably a bug, please report it.").arg(lang).arg(DEFAULT_LANGUAGE);
			QMessageBox::warning(this, tr("Help filter not found"), message);
			helpEngine->setCurrentFilter(DEFAULT_LANGUAGE);
			this->language = DEFAULT_LANGUAGE;
		}
	}

	void HelpViewer::showHelp(helpType type)
	{
		if (!helpFound)
		{
			this->show();	// show anyway, but will be blank
			return;
		}

		QString filename = "qthelp:///doc/doc/" + this->language + "_";
		switch (type)
		{
			// help files generated by the wikidot parser
			case HelpViewer::USERMANUAL:
				filename += "asebausermanual.html";
				break;
			case HelpViewer::STUDIO:
				filename += "asebastudio.html";
				break;
			case HelpViewer::LANGUAGE:
				filename += "asebalanguage.html";
				break;
		}

		QUrl result = helpEngine->findFile(QUrl(filename));
		viewer->setSource(result);
		viewer->setWindowTitle(tr("Aseba Studio Help"));
		this->show();
	}

	bool HelpViewer::readSettings()
	{
		bool result;

		QSettings settings;
		result = restoreGeometry(settings.value("HelpViewer/geometry").toByteArray());
		move(settings.value("HelpViewer/position").toPoint());
		return result;
	}

	void HelpViewer::writeSettings()
	{
		QSettings settings;
		settings.setValue("HelpViewer/geometry", saveGeometry());
		settings.setValue("HelpViewer/position", pos());
	}


	void HelpViewer::previousClicked()
	{
		viewer->backward();
	}

	void HelpViewer::backwardAvailable(bool state)
	{
		previous->setEnabled(state);
	}

	void HelpViewer::nextClicked()
	{
		viewer->forward();
	}

	void HelpViewer::forwardAvailable(bool state)
	{
		next->setEnabled(state);
	}

	void HelpViewer::homeClicked()
	{
		showHelp(HelpViewer::USERMANUAL);
	}


	HelpBrowser::HelpBrowser(QHelpEngine* helpEngine, QWidget* parent):
		QTextBrowser(parent), helpEngine(helpEngine)
	{
		setReadOnly(true);
	}

	void HelpBrowser::setSource(const QUrl &url)
	{
		if (url.scheme() == "http")
		{
			QDesktopServices::openUrl(url);
		}
		else
			QTextBrowser::setSource(url);
	}

	QVariant HelpBrowser::loadResource(int type, const QUrl& url)
	{
		//qDebug() << "Loading " << url;

		// from Qt Quarterly 28:
		// "Using QtHelp to Lend a Helping Hand"
		if (url.scheme() == "qthelp")
			return QVariant(helpEngine->fileData(url));
		else
			return QTextBrowser::loadResource(type, url);
	}
}

