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

#include <map>

#define CONFIG_PROPERTY_CHECKBOX_DECLARE(name) \
	static const bool name(void);

namespace Aseba
{
	class GeneralPage;
	class EditorPage;

	// ConfigDialog
	class ConfigDialog: public QDialog
	{
		Q_OBJECT

	public:
		// instantiate / kill the singleton
		static void init(QWidget* parent = 0);
		static void bye(void);

		static void showConfig();

		// access to properties
		// startup properties
		CONFIG_PROPERTY_CHECKBOX_DECLARE(getStartupShowHidden)
		CONFIG_PROPERTY_CHECKBOX_DECLARE(getStartupShowLineNumbers)
		CONFIG_PROPERTY_CHECKBOX_DECLARE(getStartupShowKeywordToolbar)
		// autocompletion behaviour
		CONFIG_PROPERTY_CHECKBOX_DECLARE(getAutoCompletion)

	protected:
		ConfigDialog(QWidget* parent = 0);
		~ConfigDialog();

	protected:
		static ConfigDialog* me;	// pointer to the singleton

		QPushButton* okButton;
		QPushButton* cancelButton;
		QListWidget* topicList;
		QStackedWidget* configStack;

		// individual pages
		GeneralPage* generalpage;
		EditorPage* editorpage;

		void readSettings();
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

		friend class ConfigDialog;

	protected:
		virtual void readSettings();
		virtual void writeSettings();

		virtual void flushCache();
		virtual void discardChanges();

		QCheckBox* newCheckbox(QString label, QString ID, bool checked = false);

	protected:
		template <class T> struct WidgetCache {
			WidgetCache(): widget(NULL){}
			WidgetCache(QWidget* widget, T value):widget(widget),value(value){}
			QWidget* widget;
			T value;
		};
		std::map<QString, WidgetCache<bool> > checkboxCache;

		QVBoxLayout* mainLayout;
	};

	// GeneralPage
	class GeneralPage: public ConfigPage
	{
		Q_OBJECT

	public:
		GeneralPage(QWidget* parent = 0);
		~GeneralPage(){}

		friend class ConfigDialog;

	protected:
		virtual void readSettings();
		virtual void writeSettings();
	};

	// EditorPage
	class EditorPage: public ConfigPage
	{
		Q_OBJECT

	public:
		EditorPage(QWidget* parent = 0);
		~EditorPage(){}

		friend class ConfigDialog;

	protected:
		virtual void readSettings();
		virtual void writeSettings();
	};
}

#endif // CONFIG_DIALOG_H

