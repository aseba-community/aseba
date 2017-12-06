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

#include "AboutDialog.h"
#include "../consts.h"
#include "dashel/dashel.h"
#include <version.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QSvgRenderer>
#include <QPainter>
#ifdef HAVE_QWT
	#include <qwt_global.h>
#endif // HAVE_QWT
#include <QtDebug>

namespace Aseba
{
	AboutBox::AboutBox(QWidget* parent, const Parameters& parameters):
		QDialog(parent)
	{
		// title entry
		auto titleLayout = new QHBoxLayout();
		QSvgRenderer iconRenderer(parameters.iconFile);
		QPixmap iconPixmap(64, 64);
		QPainter iconPainter(&iconPixmap);
		iconRenderer.render(&iconPainter);
		auto iconLabel = new QLabel();
		iconLabel->setPixmap(iconPixmap);
		titleLayout->addWidget(iconLabel);
		titleLayout->addSpacing(16);
		auto titleLabel = new QLabel(QString(
			"<p style=\"font-size: large\">%1</p><p>%2</p>")
			.arg(parameters.applicationName)
			.arg(tr("Version %1").arg(ASEBA_VERSION))
		);
		titleLayout->addWidget(titleLabel);
		titleLayout->addStretch();
		
		// create widget for holding tabs
		auto tabs = new QTabWidget();
		
		// about tab
		const QString aboutText =
			"<p>" + parameters.description + "</p>" +
			(parameters.helpUrl.isEmpty() ? "" : "<p>" + tr("You can find more information online at <a href=\"%1\">%2</a>.").arg(parameters.helpUrl.toString()).arg(parameters.helpUrl.toString(QUrl::RemoveScheme|QUrl::RemoveUserInfo|QUrl::StripTrailingSlash).remove(0,2)) + "</p>" ) +
			"<p>" + tr("This program is part of Aseba, a set of tools which allow beginners to program robots easily and efficiently. For more information about Aseba, visit %1.").arg("<a href=\"http://aseba.io\">aseba.io</a>") + "</p>" +
			"<p>" + tr("(c) 2006-2017 <a href=\"http://stephane.magnenat.net\">Stéphane Magnenat</a> and <a href=\"https://github.com/aseba-community/aseba/blob/master/authors.txt\">other contributors</a>. See tabs \"Authors\" and \"Thanks To\" for more information.") + " " +
			tr("Aseba is open-source licensed under the <a href=\"https://www.gnu.org/licenses/lgpl.html\">LGPL version 3</a>.") + "</p>"
		;
		auto aboutTextLabel = new QLabel(aboutText);
		aboutTextLabel->setWordWrap(true);
		aboutTextLabel->setOpenExternalLinks(true);
		auto aboutEnclosingLayout = new QHBoxLayout;
		aboutEnclosingLayout->addWidget(aboutTextLabel);
		auto aboutEnclosingWidget = new QWidget;
		aboutEnclosingWidget->setLayout(aboutEnclosingLayout);
		tabs->addTab(aboutEnclosingWidget, tr("About"));
		
		// usage text
		if (!parameters.usage.isEmpty())
		{
			auto usageTextLabel = new QLabel(parameters.usage);
			auto usageEnclosingLayout = new QHBoxLayout;
			usageEnclosingLayout->addWidget(usageTextLabel);
			auto usageEnclosingWidget = new QWidget;
			usageEnclosingWidget->setLayout(usageEnclosingLayout);
			tabs->addTab(usageEnclosingWidget, tr("Usage"));
		}
		
		// author informations
		// TODO: fill information
		tabs->addTab(new QWidget, tr("Authors"));
		tabs->addTab(new QWidget, tr("Thanks To"));
		
		// libraries tab
		const QString welcomeText = tr("The following software are used in Aseba:");
		const QString libEntryText = tr("<b><a href=\"%3\">%1</a></b> version %2");
		const QString asebaBuildInfo = tr("build version %1, protocol version %2").arg(ASEBA_BUILD_VERSION).arg(ASEBA_PROTOCOL_VERSION);
		const QString dashelStreamInfo = tr("supported stream types: %1").arg(QString::fromStdString(Dashel::streamTypeRegistry.list()));
		const QString liStart = "<li style=\"margin-top:5px; margin-bottom:5px;\">";
		const QString libraryText =
			welcomeText + "<ul>" +
			liStart + libEntryText.arg("Aseba").arg(ASEBA_VERSION).arg("http://aseba.io") + "<br/>" +
			asebaBuildInfo + "</li>" + 
			liStart + libEntryText.arg("Dashel").arg(DASHEL_VERSION).arg("http://aseba-community.github.io/dashel/") + "<br/>" +
			dashelStreamInfo + "</li>" +
			liStart + libEntryText.arg("Qt").arg(qVersion()).arg("https://www.qt.io/") + "</li>" +
#ifdef HAVE_QWT
			liStart + libEntryText.arg("Qwt").arg(QWT_VERSION_STR).arg("http://qwt.sourceforge.net/") + "</li>" +
#endif // HAVE_QWT
#ifdef HAVE_ENKI
			liStart + libEntryText.arg("Enki").arg("2.0~pre").arg("https://github.com/enki-community/enki") + "</li>" +
#endif // HAVE_ENKI
			"</ul>"
		;
		//qDebug() << libraryText;
		auto libraryTextLabel = new QLabel(libraryText);
		libraryTextLabel->setOpenExternalLinks(true);
		auto libraryEnclosingLayout = new QHBoxLayout;
		libraryEnclosingLayout->addWidget(libraryTextLabel);
		auto libraryEnclosingWidget = new QWidget;
		libraryEnclosingWidget->setLayout(libraryEnclosingLayout);
		
		// add elements into a tab widget
		tabs->addTab(libraryEnclosingWidget, tr("Libraries"));
		
		// close button
		auto buttonLayout = new QHBoxLayout();
		buttonLayout->addStretch();
		auto closeButton = new QPushButton(QIcon::fromTheme("window-close"), tr("close"));
		buttonLayout->addWidget(closeButton);
		// TODO: provide icon on resource path for Windows and macOS
		
		// add all elements to a main layout
		auto mainLayout = new QVBoxLayout();
		mainLayout->addLayout(titleLayout);
		mainLayout->addSpacing(8);
		mainLayout->addWidget(tabs);
		mainLayout->addLayout(buttonLayout);
		setLayout(mainLayout);
		
		// set window title and minimum size
		setWindowTitle(tr("About %1").arg(parameters.applicationName));
		//resize(640, 460);
		// uncomment to be able to resize the dialog
		// setSizeGripEnabled(true);
		
		// connections
		connect(closeButton, SIGNAL(clicked()), SLOT(close()));
	}
	
	
} // namespace Aseba
