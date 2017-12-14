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

#include <QCloseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSizePolicy>
#include <QFileDialog>
#include <QFile>
#include <QDomElement>
#include <QDesktopWidget>
#include <QApplication>
#include <QScrollBar>
#include <QAction>
#include <QUrl>
#include <QDesktopServices>
#include <QSettings>
#include <QImageReader>
#include <QFileInfo>
#include <QDir>
#include <QtDebug>
#include <QSvgRenderer>
#include <QWebPage>
#include <QWebFrame>

#include "ThymioBlockly.h"

#include "../../TargetModels.h"
#include "../../../../common/utils/utils.h"

using namespace std;

QPixmap pixmapFromSVG(const QString& fileName, int size)
{
	QSvgRenderer renderer(fileName);
	QPixmap pixmap(size, size);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	renderer.render(&painter);
	return pixmap;
}

namespace Aseba { namespace ThymioBlockly
{
	// Visual Programming
	ThymioBlockly::ThymioBlockly(DevelopmentEnvironmentInterface *_de, bool showCloseButton, bool debugLog, bool execFeedback):
		de(_de),
		webview(new QWebView(this)),
		skipNextUpdate(false)
	{
		// Create the gui ...
		setWindowTitle(tr("Thymio Blockly Interface"));
		setMinimumSize(QSize(400,400));
		
		mainLayout = new QVBoxLayout(this);
		
		toolLayout = new  QHBoxLayout();
		//toolLayout->setHorizontalSpacing(0);
		//toolLayout->setVerticalSpacing(0);
		mainLayout->addLayout(toolLayout);
		// add back the spacing we removed
		mainLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));

		newButton = new QPushButton();
		newButton->setIcon(QIcon(":/images/icons/filenew.svgz"));
		newButton->setToolTip(tr("New"));
		newButton->setFlat(true);
		newButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(newButton);
		
		openButton = new QPushButton();
		openButton->setIcon(QIcon(":/images/icons/fileopen.svgz"));
		openButton->setToolTip(tr("Open"));
		openButton->setFlat(true);
		openButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(openButton);
		
		saveButton = new QPushButton();
		saveButton->setIcon(QIcon(":/images/icons/save.svgz"));
		saveButton->setToolTip(tr("Save"));
		saveButton->setFlat(true);
		saveButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(saveButton);
		
		saveAsButton = new QPushButton();
		saveAsButton->setIcon(QIcon(":/images/icons/saveas.svgz"));
		saveAsButton->setToolTip(tr("Save as"));
		saveAsButton->setFlat(true);
		saveAsButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(saveAsButton);
		
		spacer1 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer1);
		
		firstSeparator = new QFrame();
		firstSeparator->setFrameShape(QFrame::VLine);
		firstSeparator->setFrameShadow(QFrame::Sunken);
		toolLayout->addWidget(firstSeparator);
		
		spacer2 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer2);

		spacer3 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer3);

		runButton = new QPushButton();
		runButton->setIcon(QIcon(":/images/icons/play.svgz"));
		runButton->setToolTip(tr("Load & Run"));
		runButton->setFlat(true);
		runButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(runButton);
		
		spacerRunStop = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacerRunStop);

		stopButton = new QPushButton();
		stopButton->setIcon(QIcon(":/images/icons/stop.svgz"));
		stopButton->setToolTip(tr("Stop"));
		stopButton->setFlat(true);
		stopButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(stopButton);
		
		spacer4 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer4);
		
		spacer5 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer5);
		
		secondSeparator = new QFrame();
		secondSeparator->setFrameShape(QFrame::VLine);
		secondSeparator->setFrameShadow(QFrame::Sunken);
		toolLayout->addWidget(secondSeparator);
		
		spacer6 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer6);

		helpButton = new QPushButton();
		helpButton->setIcon(QIcon(":/images/icons/info.svgz"));
		helpButton->setToolTip(tr("Help"));
		helpButton->setFlat(true);
		helpButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(helpButton);
		
		snapshotButton = new QPushButton();
		snapshotButton->setIcon(QIcon(":/images/icons/screenshot.svgz"));
		snapshotButton->setToolTip(tr("Screenshot"));
		snapshotButton->setFlat(true);
		snapshotButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(snapshotButton);

		connect(newButton, SIGNAL(clicked()), this, SLOT(newFile()));
		connect(openButton, SIGNAL(clicked()), this, SLOT(openFile()));
		connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
		connect(saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));
		
		connect(runButton, SIGNAL(clicked()), this, SLOT(run()));
		connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
		connect(helpButton, SIGNAL(clicked()), this, SLOT(openHelp()));
		connect(snapshotButton, SIGNAL(clicked()), this, SLOT(saveSnapshot()));
		
		horizontalLayout = new QHBoxLayout();
		mainLayout->addLayout(horizontalLayout);

		sceneLayout = new QVBoxLayout();

		const QString lang(de->getTarget()->getLanguage().left(2));
		if(lang == "de") {
			webview->load(QUrl("qrc:/plugins/ThymioBlockly/blockly/tests/asebastudio.de.html"));
        } else if(lang == "fr") {
			webview->load(QUrl("qrc:/plugins/ThymioBlockly/blockly/tests/asebastudio.fr.html"));
		} else if(lang == "es") {
			webview->load(QUrl("qrc:/plugins/ThymioBlockly/blockly/tests/asebastudio.es.html"));
		} else if(lang == "it") {
			webview->load(QUrl("qrc:/plugins/ThymioBlockly/blockly/tests/asebastudio.it.html"));
		} else {
			webview->load(QUrl("qrc:/plugins/ThymioBlockly/blockly/tests/asebastudio.html"));
		}
		webview->setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::NonCosmeticDefaultPen | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
		webview->setContextMenuPolicy(Qt::CustomContextMenu);

		QWebPage *page = webview->page();
		QWebFrame *frame = page->mainFrame();
		connect(frame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(initAsebaJavascriptInterface()));
		connect(&asebaJavascriptInterface, SIGNAL(compiled(const QString&)), this, SLOT(codeUpdated(const QString&)));
		connect(&asebaJavascriptInterface, SIGNAL(saved(const QString&)), this, SLOT(workspaceSaved(const QString&)));
		connect(webview, SIGNAL(loadFinished(bool)), this, SLOT(blocklyLoaded()));

		sceneLayout->addWidget(webview);

		horizontalLayout->addLayout(sceneLayout);

		// window properties
		setWindowModality(Qt::ApplicationModal);
		setWindowIcon(QIcon(":/images/icons/thymioblockly.svgz"));
	}
	
	ThymioBlockly::~ThymioBlockly()
	{
		saveGeometryIfVisible();
	}
	
	void ThymioBlockly::openHelp() const
	{
		QDesktopServices::openUrl(QUrl(tr("http://aseba.wikidot.com/en:blocklyprogramming")));
	}
	
	void ThymioBlockly::saveSnapshot() const
	{
		QSettings settings;
		
		QString initialFileName(settings.value("ThymioVPL/snapshotFileName", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString());
		if (initialFileName.isEmpty() && !de->openedFileName().isEmpty())
		{
			const QFileInfo pf(de->openedFileName());
			initialFileName = (pf.absolutePath() + QDir::separator() + pf.baseName() + ".png");
		}
		QString selectedFilter;
		QString fileName(QFileDialog::getSaveFileName(0,
			tr("Export program as image"), initialFileName, "Images (*.png *.jpg *.bmp *.ppm *.tiff)", &selectedFilter));
		
		if (fileName.isEmpty())
			return;
		
		// save file name to settings
		settings.setValue("ThymioBlockly/snapshotFileName", fileName);

		// image
		if (fileName.lastIndexOf(".") < 0)
			fileName += ".png";
		
		QImage image(webview->rect().size(), QImage::Format_ARGB32_Premultiplied);
		{ // paint into image
			QPainter painter(&image);
			webview->render(&painter);
		}
		image.save(fileName);
	}
		
	QWidget* ThymioBlockly::createMenuEntry()
	{
		QPushButton *vplButton = new QPushButton(QIcon(":/images/icons/thymioblockly.svgz"), tr("Launch Blockly"));
		vplButton->setIconSize(QSize(32,32));
		connect(vplButton, SIGNAL(clicked()), SLOT(newFile()));
		return vplButton;
	}
	
	void ThymioBlockly::closeAsSoonAsPossible()
	{
		close();
	}
	
	//! prevent recursion of changes triggered by VPL itself
	void ThymioBlockly::clearSceneWithoutRecompilation()
	{
		skipNextUpdate = true;
		asebaJavascriptInterface.requestReset();
	}
	
	void ThymioBlockly::showAtSavedPosition()
	{
		QSettings settings;
		restoreGeometry(settings.value("ThymioBlockly/geometry").toByteArray());
		QWidget::show();
	}

	void ThymioBlockly::newFile()
	{
		if(de->newFile())
		{
			asebaJavascriptInterface.requestReset();
			showAtSavedPosition();
		}
	}

	void ThymioBlockly::openFile()
	{
		de->openFile();
	}
	
	bool ThymioBlockly::save()
	{
		asebaJavascriptInterface.requestSave();
		de->saveFile(false);
		return true;
	}
	
	bool ThymioBlockly::saveAs()
	{
		asebaJavascriptInterface.requestSave();
		de->saveFile(true);
		return true;
	}

	bool ThymioBlockly::closeFile()
	{
		if(!isVisible()) {
			return true;
		}
		
		if(preDiscardWarningDialog(true))
		{
			de->clearOpenedFileName(true);
			clearSceneWithoutRecompilation();
			return true;
		}
		else
		{
			de->clearOpenedFileName(true);
			return false;
		}
	}
	
	void ThymioBlockly::run()
	{
		if(runButton->isEnabled())
		{
			de->loadAndRun();
		}
	}

	void ThymioBlockly::stop()
	{
		de->stop();
		const unsigned leftSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.left.target");
		de->setVariableValues(leftSpeedVarPos, VariablesDataVector(1, 0));
		const unsigned rightSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.right.target");
		de->setVariableValues(rightSpeedVarPos, VariablesDataVector(1, 0));
	}
	
	void ThymioBlockly::initAsebaJavascriptInterface()
	{
		QWebPage *page = webview->page();
		QWebFrame *frame = page->mainFrame();
		frame->addToJavaScriptWindowObject("asebaJavascriptInterface", &asebaJavascriptInterface);
	}

	void ThymioBlockly::codeUpdated(const QString& code)
	{
		if(skipNextUpdate) {
			skipNextUpdate = false;
		} else {
			de->displayCode(QStringList() << code, 0);
		}
	}

	void ThymioBlockly::workspaceSaved(const QString& xml)
	{
		currentSavedXml = xml;
	}

	void ThymioBlockly::blocklyLoaded()
	{
		if (!currentSavedXml.isEmpty())
		{
			asebaJavascriptInterface.requestLoad(currentSavedXml);
		}
	}

	void ThymioBlockly::closeEvent ( QCloseEvent * event )
	{
		if (!closeFile())
			event->ignore();
		else
			saveGeometryIfVisible();
	}
	
	void ThymioBlockly::saveGeometryIfVisible()
	{
		if (isVisible())
		{
			QSettings settings;
			settings.setValue("ThymioBlockly/geometry", saveGeometry());
		}
	}
	
	bool ThymioBlockly::preDiscardWarningDialog(bool keepCode)
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Warning"));
		msgBox.setText(tr("The Blockly document has been modified.<p>Do you want to save the changes?"));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Save);
		
		int ret = msgBox.exec();
		switch (ret)
		{
			case QMessageBox::Save:
			{
				if (save())
					return true;
				else
					return false;
			}
			case QMessageBox::Discard:
			{
				return true;
			}
			case QMessageBox::Cancel:
			default:
				return false;
		}

		return false;
	}
	
	QDomDocument ThymioBlockly::saveToDom() const
	{
		asebaJavascriptInterface.requestSave();
		if (currentSavedXml.isEmpty() || currentSavedXml == "<xml></xml>") {
			return QDomDocument();
		}
		else {
			QDomDocument document("tool-plugin-data");
			if (document.setContent(currentSavedXml)) {
				return document;
			}
			else {
				// this is an error - I'm not sure if it still occurs under any circumstance, but if it does, some diagnostic output may be helpful
				printf("ThymioBlockly::saveToDom(): invalid XML content\n%s\n", currentSavedXml.toUtf8().constData());
				// document is in an invalid state now that would make MainWindow::saveFile crash, return a null document instead
				return QDomDocument();
			}
		}
	}
	
	void ThymioBlockly::aboutToLoad()
	{
		saveGeometryIfVisible();
		hide();
	}

	void ThymioBlockly::loadFromDom(const QDomDocument& document, bool fromFile)
	{
		currentSavedXml = document.toString();

		// remove <!DOCTYPE tool-plugin-data>, blockly can't handle it...
		currentSavedXml.remove("<!DOCTYPE tool-plugin-data>");
		
		asebaJavascriptInterface.requestLoad(currentSavedXml);
		
		showAtSavedPosition();
	}
	
	void ThymioBlockly::codeChangedInEditor()
	{
		if(!isVisible())
		{
			clearSceneWithoutRecompilation();
		}
	}
	
	bool ThymioBlockly::isModified() const
	{
		return false;
	}
	
	qreal ThymioBlockly::getViewScale() const
	{
		return 1.0;
	}
	
	void ThymioBlockly::resizeEvent( QResizeEvent *event)
	{
		// compute size of elements for toolbar
		const float toolbarWidgetCount(16);
		// get width of combox box element (not content)
		int desiredIconSize((
			event->size().width() -
			(
				/*(toolbarWidgetCount-1) * 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth) + */
				toolbarWidgetCount * 2 * style()->pixelMetric(QStyle::PM_ButtonMargin) +
				style()->pixelMetric(QStyle::PM_LayoutLeftMargin) +
				style()->pixelMetric(QStyle::PM_LayoutRightMargin) +
				6 // vertical lines
				/*#ifdef Q_WS_MAC
				55 // safety factor, as it seems that metrics do miss some space
				#else // Q_WS_MAC
				20
				#endif // Q_WS_MAC
				//20 // safety factor, as it seems that metrics do miss some space*/
			)
		) / (toolbarWidgetCount));
		
		desiredIconSize = qMin(desiredIconSize, event->size().height()/14);
		
		// set toolbar
		const QSize tbIconSize(QSize(desiredIconSize, desiredIconSize));
		const QSize importantIconSize(tbIconSize * 1.41);
		newButton->setIconSize(tbIconSize);
		openButton->setIconSize(tbIconSize);
		saveButton->setIconSize(tbIconSize);
		saveAsButton->setIconSize(tbIconSize);
		runButton->setIconSize(importantIconSize);
		stopButton->setIconSize(importantIconSize);
		helpButton->setIconSize(tbIconSize);
		snapshotButton->setIconSize(tbIconSize);
		spacerRunStop->changeSize(desiredIconSize/4, desiredIconSize);
		toolLayout->invalidate();
	}
	
} } // namespace ThymioVPL / namespace Aseba
