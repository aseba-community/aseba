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

namespace Aseba
{
	const QString HelpViewer::DEFAULT_LANGUAGE = "en";

	HelpViewer::HelpViewer(QWidget* parent):
		QWidget(parent),
		tmpHelpSubDir(QString("aseba-studio-%1").arg(QCoreApplication::applicationPid())),
		tmpHelpFileNameHC(QDir::temp().filePath(tmpHelpSubDir + "/aseba-doc.qhc")),
		tmpHelpFileNameCH(QDir::temp().filePath(tmpHelpSubDir + "/aseba-doc.qch"))
	{
		// expanding help files into tmp
		QDir::temp().mkdir(tmpHelpSubDir);
 		QFile(":aseba-doc.qhc").copy(tmpHelpFileNameHC);
		QFile(":aseba-doc.qch").copy(tmpHelpFileNameCH);
		
		// open files from tmp
		helpEngine = new QHelpEngine(tmpHelpFileNameHC, this);
		if (helpEngine->setupData() == false)
		{
			helpFound = false;
			QString message = tr("The help file %0 was not loaded successfully. " \
				"The error was: %1." \
				"The help file should be available in the temporary directory of your system. " \
				"Please check your installation, or report a bug.").arg(tmpHelpFileNameHC).arg(helpEngine->error());
			QMessageBox::warning(this, tr("Help file not found"), message);
		}
		else
			helpFound = true;
	}

	HelpViewer::~HelpViewer()
	{
		writeSettings();
		// remove help files from tmp
		QFile(tmpHelpFileNameHC).remove();
		QFile(tmpHelpFileNameCH).remove();
		QDir::temp().rmdir(tmpHelpSubDir);
	}
	
	void HelpViewer::setupWidgets()
	{
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

		// restore window state, if available
		if (readSettings() == false)
			resize(800, 500);

		setWindowTitle(tr("Aseba Studio Help"));
	}
	
	void HelpViewer::setupConnections()
	{
		connect(previous, SIGNAL(clicked()), this, SLOT(previousClicked()));
		connect(next, SIGNAL(clicked()), this, SLOT(nextClicked()));
		connect(home, SIGNAL(clicked()), this, SLOT(homeClicked()));
		connect(viewer, SIGNAL(backwardAvailable(bool)), this, SLOT(backwardAvailable(bool)));
		connect(viewer, SIGNAL(forwardAvailable(bool)), this, SLOT(forwardAvailable(bool)));
		connect(helpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl&)), viewer, SLOT(setSource(const QUrl&)));
		connect(viewer, SIGNAL(sourceChanged(const QUrl&)), this, SLOT(sourceChanged(const QUrl&)));
	}

	void HelpViewer::setLanguage(const QString& lang)
	{
		if (!helpFound)
			return;

		// check if the language is part of the existing filters
		if (selectLanguage(lang) == false)
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
	
	bool HelpViewer::selectLanguage(const QString& reqLang)
	{
		const QStringList& langList(helpEngine->customFilters());
		for (QStringList::const_iterator it = langList.constBegin(); it != langList.constEnd(); ++it)
		{
			const QString& availableLang(*it);
			if (reqLang.startsWith(availableLang))
			{
				helpEngine->setCurrentFilter(availableLang);
				this->language = availableLang;
				return true;
			}
		}
		return false;
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

	void HelpViewer::sourceChanged(const QUrl& src)
	{
		QModelIndex item = helpEngine->contentWidget()->indexOf(src);
		if (item.isValid())
			helpEngine->contentWidget()->setCurrentIndex(item);
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

