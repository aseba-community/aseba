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

#ifndef HELP_VIEWER_H
#define HELP_VIEWER_H

#include <QtHelp/QHelpEngine>
#include <QWidget>
#include <QString>
#include <QPushButton>
#include <QTextBrowser>
#include <QModelIndex>

namespace Aseba
{
	class HelpBrowser;

	class HelpViewer: public QWidget
	{
		Q_OBJECT

	public:
		HelpViewer(QWidget* parent = 0);
		~HelpViewer();

		enum helpType {
			USERMANUAL,
			STUDIO,
			LANGUAGE
		};

		static const QString DEFAULT_LANGUAGE;

		void setupWidgets();
		void setupConnections();
		
		void setLanguage(const QString& lang = DEFAULT_LANGUAGE);
		void showHelp(helpType type);

	protected:
		bool selectLanguage(const QString& reqLang);
		bool readSettings();
		void writeSettings();

		QHelpEngine *helpEngine;
		HelpBrowser* viewer;
		QPushButton* previous;
		QPushButton* next;
		QPushButton* home;
		QString language;
		bool helpFound;
		const QString tmpHelpSubDir;
		const QString tmpHelpFileNameHC;
		const QString tmpHelpFileNameCH;

	protected slots:
		void previousClicked();
		void backwardAvailable(bool state);
		void nextClicked();
		void forwardAvailable(bool state);
		void homeClicked();
		void sourceChanged(const QUrl& src);
	};

	class HelpBrowser: public QTextBrowser
	{
	public:
		HelpBrowser(QHelpEngine* helpEngine, QWidget* parent = 0);
		virtual void setSource(const QUrl& url);
		virtual QVariant loadResource(int type, const QUrl& url);

	protected:
		QHelpEngine* helpEngine;
	};
}

#endif // HELP_VIEWER_H

