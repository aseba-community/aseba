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

#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H

#include <QWidget>
#include <QString>
#include <QtGui>


namespace Aseba
{
	// ConfigDialog
	class ConfigDialog: public QDialog
	{
		Q_OBJECT

	public:
		ConfigDialog(QWidget* parent = 0);
		~ConfigDialog();

		void showConfig();

	protected:
		QPushButton* okButton;
		QPushButton* cancelButton;
		QListWidget* topicList;
		QStackedWidget* configStack;

		bool readSettings();
		void writeSettings();

	public slots:
		virtual void accept();
		virtual void reject();
	};

	// ConfigPage
	class ConfigPage: public QWidget
	{
		Q_OBJECT

	public:
		ConfigPage(QString title = QString(), QWidget* parent = 0);
		~ConfigPage(){}

	protected:
		QVBoxLayout* mainLayout;
	};

	// GeneralPage
	class GeneralPage: public ConfigPage
	{
		Q_OBJECT

	public:
		GeneralPage(QWidget* parent = 0);
		~GeneralPage(){}
	};

	// EditorPage
	class EditorPage: public ConfigPage
	{
		Q_OBJECT

	public:
		EditorPage(QWidget* parent = 0);
		~EditorPage(){}

	protected:
		QCheckBox* autocompletion;
	};
}

#endif // CONFIG_DIALOG_H

