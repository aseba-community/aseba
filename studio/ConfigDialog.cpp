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

#include "ConfigDialog.h"

#include <QSettings>

#include <ConfigDialog.moc>


#define BOOL_TO_CHECKED(value)	(value == true ? Qt::Checked : Qt::Unchecked)
#define CHECKED_TO_BOOL(value)	(value == Qt::Checked ? true : false)

#define CONFIG_PROPERTY_CHECKBOX_HANDLER(name, page, keyword) \
	const bool ConfigDialog::name() { \
		if (me) \
			return me->page->checkboxCache[#keyword].value; \
		else \
			return false; \
	}


#define TITLE_SPACING		10
#define HORIZONTAL_STRUT	700

namespace Aseba
{
	CONFIG_PROPERTY_CHECKBOX_HANDLER(getStartupShowLineNumbers,	generalpage,	showlinenumbers		)
	CONFIG_PROPERTY_CHECKBOX_HANDLER(getStartupShowHidden,		generalpage,	showhidden		)
	CONFIG_PROPERTY_CHECKBOX_HANDLER(getStartupShowKeywordToolbar,	generalpage,	keywordToolbar		)
	CONFIG_PROPERTY_CHECKBOX_HANDLER(getAutoCompletion,		editorpage,	autoKeyword		)

	/*** ConfigPage ***/
	ConfigPage::ConfigPage(QString title, QWidget *parent):
		QWidget(parent)
	{
		QLabel* myTitle = new QLabel(title);
		// bold & align center
		myTitle->setAlignment(Qt::AlignCenter);
		QFont current = myTitle->font();
		current.setBold(true);
		myTitle->setFont(current);

		mainLayout = new QVBoxLayout();
		mainLayout->addWidget(myTitle);
		mainLayout->addSpacing(TITLE_SPACING);
		mainLayout->addStrut(HORIZONTAL_STRUT);
		setLayout(mainLayout);
	}

	QCheckBox* ConfigPage::newCheckbox(QString label, QString ID, bool checked)
	{
		QCheckBox* widget = new QCheckBox(label);
		widget->setCheckState(BOOL_TO_CHECKED(checked));
		WidgetCache<bool> cache(widget, checked);
		checkboxCache.insert(std::pair<QString, WidgetCache<bool> >(ID, cache));
		return widget;
	}

	void ConfigPage::readSettings()
	{
		// update cache and widgets from disk
		QSettings settings;
		// iterate on checkboxes
		for (std::map<QString, WidgetCache<bool> >::iterator it = checkboxCache.begin(); it != checkboxCache.end(); it++)
		{
			QString name = it->first;
			QCheckBox* checkbox = dynamic_cast<QCheckBox*>((it->second).widget);
			Q_ASSERT(checkbox);
			if (settings.contains(name))
			{
				bool value = settings.value(name, false).toBool();
				checkbox->setCheckState(BOOL_TO_CHECKED(value));	// update widget
				(it->second).value = value;				// update cache
			}
		}
	}

	void ConfigPage::writeSettings()
	{
		// write the cache on disk
		QSettings settings;
		// iterate on checkboxes
		for (std::map<QString, WidgetCache<bool> >::iterator it = checkboxCache.begin(); it != checkboxCache.end(); it++)
		{
			QString name = it->first;
			QCheckBox* checkbox = dynamic_cast<QCheckBox*>((it->second).widget);
			Q_ASSERT(checkbox);
			settings.setValue(name, (it->second).value);
		}
	}

	void ConfigPage::flushCache()
	{
		// sync the cache with the widgets' state
		// iterate on checkboxes
		for (std::map<QString, WidgetCache<bool> >::iterator it = checkboxCache.begin(); it != checkboxCache.end(); it++)
		{
			QCheckBox* checkbox = dynamic_cast<QCheckBox*>((it->second).widget);
			Q_ASSERT(checkbox);
			(it->second).value = CHECKED_TO_BOOL(checkbox->checkState());
		}
	}

	void ConfigPage::discardChanges()
	{
		// reload values from the cache
		// iterate on checkboxes
		for (std::map<QString, WidgetCache<bool> >::iterator it = checkboxCache.begin(); it != checkboxCache.end(); it++)
		{
			QCheckBox* checkbox = dynamic_cast<QCheckBox*>((it->second).widget);
			Q_ASSERT(checkbox);
			checkbox->setCheckState(BOOL_TO_CHECKED((it->second).value));
		}
	}


	/*** GeneralPage ***/
	GeneralPage::GeneralPage(QWidget *parent):
		ConfigPage(tr("General Setup"), parent)
	{
		QGroupBox* gb1 = new QGroupBox(tr("On startup"));
		QVBoxLayout* gb1layout = new QVBoxLayout();
		gb1->setLayout(gb1layout);
		mainLayout->addWidget(gb1);
		// Show hidden variables & functions
		gb1layout->addWidget(newCheckbox(tr("Show hidden variables"), "showhidden", false));
		// Show line numbers
		gb1layout->addWidget(newCheckbox(tr("Show line numbers"), "showlinenumbers", true));
		// Show the keyword toolbar
		gb1layout->addWidget(newCheckbox(tr("Show keyword toolbar"), "keywordToolbar", true));

		mainLayout->addStretch();
	}

	void GeneralPage::readSettings()
	{
		QSettings settings;
		settings.beginGroup("general-config");
		ConfigPage::readSettings();
		settings.endGroup();
	}

	void GeneralPage::writeSettings()
	{
		QSettings settings;
		settings.beginGroup("general-config");
		ConfigPage::writeSettings();
		settings.endGroup();
	}


	/*** EditorPage ***/
	EditorPage::EditorPage(QWidget *parent):
		ConfigPage(tr("Editor Setup"), parent)
	{
		//
		QGroupBox* gb1 = new QGroupBox(tr("Autocompletion"));
		QVBoxLayout* gb1layout = new QVBoxLayout();
		gb1->setLayout(gb1layout);
		mainLayout->addWidget(gb1);
		//
		gb1layout->addWidget(newCheckbox(tr("Keywords"), "autoKeyword", true));

		mainLayout->addStretch();
	}

	void EditorPage::readSettings()
	{
		QSettings settings;
		settings.beginGroup("editor-config");
		ConfigPage::readSettings();
		settings.endGroup();
	}

	void EditorPage::writeSettings()
	{
		QSettings settings;
		settings.beginGroup("editor-config");
		ConfigPage::writeSettings();
		settings.endGroup();
	}


	/*** ConfigDialog ***/
	void ConfigDialog::init(QWidget* parent)
	{
		if (!me)
			me = new ConfigDialog(parent);
	}

	void ConfigDialog::bye()
	{
		if (me)
		{
			delete me;
			me = NULL;
		}
	}

	void ConfigDialog::showConfig()
	{
		me->setModal(true);
		me->show();

	}

	ConfigDialog* ConfigDialog::me = NULL;

	ConfigDialog::ConfigDialog(QWidget* parent):
		QDialog(parent)
	{
		// list of topics
		topicList = new QListWidget();
		topicList->setViewMode(QListView::IconMode);
		topicList->setIconSize(QSize(64, 64));
		topicList->setMovement(QListView::Static);
		topicList->setMaximumWidth(128);
		topicList->setGridSize(QSize(100, 100));
		topicList->setMinimumHeight(300);
		// add topics
		QListWidgetItem* general = new QListWidgetItem(QIcon(":/images/exec.png"), tr("General"));
		general->setTextAlignment(Qt::AlignHCenter);
		topicList->addItem(general);
		QListWidgetItem* editor = new QListWidgetItem(QIcon(":/images/kedit.png"), tr("Editor"));
		editor->setTextAlignment(Qt::AlignHCenter);
		topicList->addItem(editor);

		// create pages
		configStack = new QStackedWidget();
		generalpage = new GeneralPage();
		configStack->addWidget(generalpage);
		editorpage = new EditorPage();
		configStack->addWidget(editorpage);

		connect(topicList, SIGNAL(currentRowChanged(int)), configStack, SLOT(setCurrentIndex(int)));
		topicList->setCurrentRow(0);

		// buttons
		okButton = new QPushButton(tr("Ok"));
		cancelButton = new QPushButton(tr("Cancel"));
		connect(okButton, SIGNAL(clicked()), SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), SLOT(reject()));

		// main layout
		QHBoxLayout* contentLayout = new QHBoxLayout();
		contentLayout->addWidget(topicList);
		contentLayout->addWidget(configStack);

		QHBoxLayout* buttonLayout = new QHBoxLayout();
		buttonLayout->addStretch();
		buttonLayout->addWidget(okButton);
		buttonLayout->addWidget(cancelButton);

		QVBoxLayout* mainLayout = new QVBoxLayout();
		mainLayout->addLayout(contentLayout);
		mainLayout->addLayout(buttonLayout);

		setLayout(mainLayout);

		readSettings();
	}

	ConfigDialog::~ConfigDialog()
	{
		writeSettings();
	}

	void ConfigDialog::accept()
	{
		// update the cache with new values
		for (int i = 0; i < configStack->count(); i++)
		{
			ConfigPage* config = dynamic_cast<ConfigPage*>(configStack->widget(i));
			if (config)
				config->flushCache();
		}
		QDialog::accept();
	}

	void ConfigDialog::reject()
	{
		// reload values from the cache
		for (int i = 0; i < configStack->count(); i++)
		{
			ConfigPage* config = dynamic_cast<ConfigPage*>(configStack->widget(i));
			if (config)
				config->discardChanges();
		}
		QDialog::reject();
	}

	void ConfigDialog::readSettings()
	{
		for (int i = 0; i < configStack->count(); i++)
		{
			ConfigPage* config = dynamic_cast<ConfigPage*>(configStack->widget(i));
			if (config)
				config->readSettings();
		}
	}

	void ConfigDialog::writeSettings()
	{
		for (int i = 0; i < configStack->count(); i++)
		{
			ConfigPage* config = dynamic_cast<ConfigPage*>(configStack->widget(i));
			if (config)
				config->writeSettings();
		}
	}

}

