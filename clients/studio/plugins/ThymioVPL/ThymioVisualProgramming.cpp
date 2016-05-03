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
#include <QSvgGenerator>
#include <QtDebug>

#include "ThymioVisualProgramming.h"
#include "ResizingView.h"
#include "Block.h"
#include "EventBlocks.h"
#include "ActionBlocks.h"
#include "Scene.h"
#include "Buttons.h"
#include "Style.h"
#include "Utils.h"
#include "UsageLogger.h"

#include "../../TargetModels.h"
#include "../../../../common/utils/utils.h"


// for backtrace
//#include <execinfo.h>

using namespace std;

namespace Aseba { namespace ThymioVPL
{
	// Visual Programming
	ThymioVisualProgramming::ThymioVisualProgramming(DevelopmentEnvironmentInterface *_de, bool showCloseButton, bool debugLog, bool execFeedback):
		debugLog(debugLog),
		execFeedback(execFeedback),
		de(_de),
		scene(new Scene(this)),
		loading(false),
		undoPos(-1),
		runAnimFrame(0),
		runAnimTimer(0)
	{
		runAnimFrames.resize(17);
		
		// Create the gui ...
		setWindowTitle(tr("Thymio Visual Programming Language"));
		setMinimumSize(QSize(400,400));
		
		mainLayout = new QVBoxLayout(this);
		
		toolLayout = new  QGridLayout();
		toolLayout->setHorizontalSpacing(0);
		toolLayout->setVerticalSpacing(0);
		mainLayout->addLayout(toolLayout);
		// add back the spacing we removed
		mainLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));

		newButton = new QPushButton();
		newButton->setIcon(QIcon(":/images/icons/filenew.svgz"));
		newButton->setToolTip(tr("New"));
		newButton->setFlat(true);
		newButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(newButton,0,0);
		
		openButton = new QPushButton();
		openButton->setIcon(QIcon(":/images/icons/fileopen.svgz"));
		openButton->setToolTip(tr("Open"));
		openButton->setFlat(true);
		openButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(openButton,0,1);
		
		saveButton = new QPushButton();
		saveButton->setIcon(QIcon(":/images/icons/save.svgz"));
		saveButton->setToolTip(tr("Save"));
		saveButton->setFlat(true);
		saveButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(saveButton,1,0);
		
		saveAsButton = new QPushButton();
		saveAsButton->setIcon(QIcon(":/images/icons/saveas.svgz"));
		saveAsButton->setToolTip(tr("Save as"));
		saveAsButton->setFlat(true);
		saveAsButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(saveAsButton,1,1);
		
		spacer1 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer1,0,2,2,1);
		
		firstSeparator = new QFrame();
		firstSeparator->setFrameShape(QFrame::VLine);
		firstSeparator->setFrameShadow(QFrame::Sunken);
		toolLayout->addWidget(firstSeparator,0,3,2,1);
		
		spacer2 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer2,0,4,2,1);
		
		undoButton = new QPushButton();
		undoButton->setIcon(QIcon(":/images/icons/edit-undo.svgz"));
		undoButton->setToolTip(tr("Undo"));
		undoButton->setFlat(true);
		undoButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		undoButton->setEnabled(false);
		undoButton->setShortcut(QKeySequence::Undo);
		toolLayout->addWidget(undoButton,0,5,2,1);
		
		redoButton = new QPushButton();
		redoButton->setIcon(QIcon(":/images/icons/edit-redo.svgz"));
		redoButton->setToolTip(tr("Redo"));
		redoButton->setFlat(true);
		redoButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		redoButton->setEnabled(false);
		redoButton->setShortcut(QKeySequence::Redo);
		toolLayout->addWidget(redoButton,0,6,2,1);
		
		spacer3 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer3,0,7,2,1);

		runButton = new QPushButton();
		runButton->setIcon(QIcon(":/images/icons/play.svgz"));
		runButton->setToolTip(tr("Load & Run"));
		runButton->setFlat(true);
		runButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(runButton,0,8,2,1);
		
		spacerRunStop = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacerRunStop,0,9,2,1);

		stopButton = new QPushButton();
		stopButton->setIcon(QIcon(":/images/icons/stop.svgz"));
		stopButton->setToolTip(tr("Stop"));
		stopButton->setFlat(true);
		stopButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(stopButton,0,10,2,1);
		
		spacer4 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer4,0,11,2,1);
		
		advancedButton = new QPushButton();
		advancedButton->setIcon(QIcon(":/images/icons/vpl-advanced_off.svgz"));
		advancedButton->setToolTip(tr("Advanced mode"));
		advancedButton->setFlat(true);
		advancedButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(advancedButton,0,12,2,1);
		
		spacer5 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer5,0,13,2,1);
		
		secondSeparator = new QFrame();
		secondSeparator->setFrameShape(QFrame::VLine);
		secondSeparator->setFrameShadow(QFrame::Sunken);
		toolLayout->addWidget(secondSeparator,0,14,2,1);
		
		spacer6 = new QSpacerItem(1,1,QSizePolicy::Expanding);
		toolLayout->addItem(spacer6,0,15,2,1);

		helpButton = new QPushButton();
		helpButton->setIcon(QIcon(":/images/icons/info.svgz"));
		helpButton->setToolTip(tr("Help"));
		helpButton->setFlat(true);
		helpButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(helpButton,0,16);
		
		snapshotButton = new QPushButton();
		snapshotButton->setIcon(QIcon(":/images/icons/screenshot.svgz"));
		snapshotButton->setToolTip(tr("Screenshot"));
		snapshotButton->setFlat(true);
		snapshotButton->setStyleSheet("QPushButton:pressed { border: 1px solid white }");
		toolLayout->addWidget(snapshotButton,1,16);

		connect(newButton, SIGNAL(clicked()), this, SLOT(newFile()));
		connect(openButton, SIGNAL(clicked()), this, SLOT(openFile()));
		connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
		connect(saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));
		connect(undoButton, SIGNAL(clicked()), this, SLOT(undo()));
		connect(redoButton, SIGNAL(clicked()), this, SLOT(redo()));
		
		connect(runButton, SIGNAL(clicked()), this, SLOT(run()));
		connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
		connect(advancedButton, SIGNAL(clicked()), this, SLOT(toggleAdvancedMode()));
		connect(helpButton, SIGNAL(clicked()), this, SLOT(openHelp()));
		connect(snapshotButton, SIGNAL(clicked()), this, SLOT(saveSnapshot()));
		
		horizontalLayout = new QHBoxLayout();
		mainLayout->addLayout(horizontalLayout);
		
		// events
		eventsLayout = new QVBoxLayout();

		eventButtons.push_back(new BlockButton("button", this));
		eventButtons.push_back(new BlockButton("prox", this));
		eventButtons.push_back(new BlockButton("proxground", this));
		eventButtons.push_back(new BlockButton("acc", this));
		eventButtons.push_back(new BlockButton("clap", this));
		eventButtons.push_back(new BlockButton("timeout", this));
		
		eventsLabel = new QLabel(tr("Events"));
		eventsLabel->setStyleSheet("QLabel { font-size: 10pt; font-weight: bold; }");
		#ifdef Q_WS_MACX
		eventsLayout->setSpacing(20);
		#else // Q_WS_MACX
		eventsLayout->setSpacing(10);
		#endif // Q_WS_MACX
		eventsLayout->setAlignment(Qt::AlignTop);
		eventsLayout->addWidget(eventsLabel);
		BlockButton* button;
		foreach (button, eventButtons)
		{
			eventsLayout->addWidget(button);
			connect(button, SIGNAL(clicked()), SLOT(addEvent()));
			connect(button, SIGNAL(contentChanged()), scene, SLOT(recompile()));
			connect(button, SIGNAL(undoCheckpoint()), SLOT(pushUndo()));
		}
		
		eventButtons[eventButtons.size()-1]->hide(); // timeout
		
		horizontalLayout->addLayout(eventsLayout);
		
		sceneLayout = new QVBoxLayout();

		// compilation
		compilationResultImage = new QLabel();
		compilationResult = new QLabel(tr("Compilation success."));
		compilationResult->setStyleSheet("QLabel { font-size: 10pt; }");
		const int fontSize(QFontMetrics(compilationResult->font()).lineSpacing());
		
		compilationResultImage->setPixmap(pixmapFromSVG(":/images/vpl/success.svgz", fontSize*1.2));
		compilationResult->setWordWrap(true);
		compilationResult->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
		
		showCompilationError = new QPushButton(tr("show line"));
		showCompilationError->setStyleSheet("QPushButton { font-size: 10pt; }");
		showCompilationError->hide();
		connect(showCompilationError, SIGNAL(clicked()), SLOT(showErrorLine()));

		compilationResultLayout = new QHBoxLayout;
		compilationResultLayout->addWidget(compilationResultImage);  
		compilationResultLayout->addWidget(compilationResult,10000);
		compilationResultLayout->addWidget(showCompilationError);
		sceneLayout->addLayout(compilationResultLayout);

		// view
		view = new ResizingView(scene);
		view->setRenderHint(QPainter::Antialiasing);
		view->setAcceptDrops(true);
		view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		sceneLayout->addWidget(view);
		Q_ASSERT(scene->setsCount() > 0);
		view->centerOn(*scene->setsBegin());
		
		connect(scene, SIGNAL(contentRecompiled()), SLOT(processCompilationResult()));
		connect(scene, SIGNAL(undoCheckpoint()), SLOT(pushUndo()));
		connect(scene, SIGNAL(highlightChanged()), SLOT(processHighlightChange()));
		connect(scene, SIGNAL(modifiedStatusChanged(bool)), SIGNAL(modifiedStatusChanged(bool)));
		
		connect(de->getTarget(), SIGNAL(userEvent(unsigned, const VariablesDataVector)), SLOT(userEvent(unsigned, const VariablesDataVector)));
		
		/*zoomSlider = new QSlider(Qt::Horizontal);
		zoomSlider->setRange(1,10);
		zoomSlider->setValue(1);
		zoomSlider->setInvertedAppearance(true);
		zoomSlider->setInvertedControls(true);
		sceneLayout->addWidget(zoomSlider);
		
		connect(zoomSlider, SIGNAL(valueChanged(int)), scene, SLOT(updateZoomLevel()));*/

		horizontalLayout->addLayout(sceneLayout);
     
		// actions
		actionsLayout = new QVBoxLayout();

		actionButtons.push_back(new BlockButton("move", this));
		actionButtons.push_back(new BlockButton("colortop", this));
		actionButtons.push_back(new BlockButton("colorbottom", this));
		actionButtons.push_back(new BlockButton("sound", this));
		actionButtons.push_back(new BlockButton("timer", this));
		actionButtons.push_back(new BlockButton("setstate", this));
		
		actionsLabel = new QLabel(tr("Actions"));
		actionsLabel->setStyleSheet("QLabel { font-size: 10pt; font-weight: bold; }");
		#ifdef Q_WS_MACX
		actionsLayout->setSpacing(20);
		#else // Q_WS_MACX
		actionsLayout->setSpacing(10);
		#endif // Q_WS_MACX
		actionsLayout->setAlignment(Qt::AlignTop);
		actionsLayout->addWidget(actionsLabel);
		foreach (button, actionButtons)
		{
			actionsLayout->addWidget(button);
			connect(button, SIGNAL(clicked()), SLOT(addAction()));
			connect(button, SIGNAL(contentChanged()), scene, SLOT(recompile()));
			connect(button, SIGNAL(undoCheckpoint()), SLOT(pushUndo()));
		}
		
		actionButtons[actionButtons.size()-2]->hide(); // timer
		actionButtons[actionButtons.size()-1]->hide(); // set state

		horizontalLayout->addLayout(actionsLayout);
		
		// window properties
		setWindowModality(Qt::ApplicationModal);
		setWindowIcon(QIcon(":/images/icons/thymiovpl.svgz"));
		
		// save initial state
		pushUndo();
	}
	
	ThymioVisualProgramming::~ThymioVisualProgramming()
	{
		saveGeometryIfVisible();
	}
	
	void ThymioVisualProgramming::openHelp() const
	{
		USAGE_LOG(logOpenHelp());
		QDesktopServices::openUrl(QUrl(tr("http://aseba.wikidot.com/en:thymiovpl")));
	}
	
	void ThymioVisualProgramming::saveSnapshot() const
	{
		QSettings settings;
		
		USAGE_LOG(logSaveSnapshot());
		QString initialFileName(settings.value("ThymioVPL/snapshotFileName", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString());
		if (initialFileName.isEmpty() && !de->openedFileName().isEmpty())
		{
			const QFileInfo pf(de->openedFileName());
			initialFileName = (pf.absolutePath() + QDir::separator() + pf.baseName() + ".png");
		}
		QString selectedFilter;
		QString fileName(QFileDialog::getSaveFileName(0,
			tr("Export program as image"), initialFileName, "Images (*.png *.jpg *.bmp *.ppm *.tiff);;Scalable Vector Graphics (*.svg)", &selectedFilter));
		
		if (fileName.isEmpty())
			return;
		
		// save file name to settings
		settings.setValue("ThymioVPL/snapshotFileName", fileName);
		
		if (selectedFilter.at(0) == 'S')
		{
			// SVG
			if (fileName.lastIndexOf(".") < 0)
				fileName += ".svg";
			
			QSvgGenerator generator;
			generator.setFileName(fileName);
			generator.setSize(scene->sceneRect().size().toSize());
			generator.setViewBox(scene->sceneRect());
			generator.setTitle(QString("VPL program %0").arg(de->openedFileName()));
			generator.setDescription("This image was generated with Thymio VPL from Aseba, get it at http://thymio.org");
			
			QPainter painter(&generator);
			scene->render(&painter);
		}
		else
		{
			// image
			if (fileName.lastIndexOf(".") < 0)
				fileName += ".png";
			
			QImage image(scene->sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
			{ // paint into image
				QPainter painter(&image);
				scene->render(&painter);
			}
			image.save(fileName);
		}
	}
	
	void ThymioVisualProgramming::setColors(QComboBox *comboBox)
	{
		assert(comboBox);
		for (unsigned i=0; i<Style::blockColorsCount(); ++i)
			comboBox->addItem(drawColorScheme(Style::blockColor("event", i), Style::blockColor("state", i), Style::blockColor("action", i)), "");
	}
	
	QPixmap ThymioVisualProgramming::drawColorScheme(const QColor& eventColor, const QColor& stateColor, const QColor& actionColor)
	{
		QPixmap pixmap(192,58);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		
		painter.setBrush(eventColor);
		painter.drawRoundedRect(0,0,54,54,4,4);
		
		painter.setBrush(stateColor);
		painter.drawRoundedRect(66,0,54,54,4,4);
		
		painter.setBrush(actionColor);
		painter.drawRoundedRect(130,0,54,54,4,4);
		
		return pixmap;
	}
	
	QWidget* ThymioVisualProgramming::createMenuEntry()
	{
		QPushButton *vplButton = new QPushButton(QIcon(":/images/icons/thymiovpl.svgz"), tr("Launch VPL"));
		vplButton->setIconSize(QSize(32,32));
		connect(vplButton, SIGNAL(clicked()), SLOT(showVPLModal()));
		return vplButton;
	}
	
	void ThymioVisualProgramming::closeAsSoonAsPossible()
	{
		close();
	}
	
	//! prevent recursion of changes triggered by VPL itself
	void ThymioVisualProgramming::clearSceneWithoutRecompilation()
	{
		loading = true;
		toggleAdvancedMode(false, true);
		scene->reset();
		clearUndo();
		loading = false;
	}
	
	void ThymioVisualProgramming::showAtSavedPosition()
	{
		QSettings settings;
		restoreGeometry(settings.value("ThymioVisualProgramming/geometry").toByteArray());
		QWidget::show();
	}

	void ThymioVisualProgramming::showVPLModal()
	{
		if (de->newFile())
		{
			toggleAdvancedMode(false, true);
			scene->reset();
			clearUndo();
			setupGlobalEvents();
			showAtSavedPosition();
			processCompilationResult();
		}
	}
	
	void ThymioVisualProgramming::setupGlobalEvents()
	{
		CommonDefinitions commonDefinitions;
		commonDefinitions.events.push_back(NamedValue(L"pair_run", 1));
		de->setCommonDefinitions(commonDefinitions);
	}
	
	void ThymioVisualProgramming::newFile()
	{
		if (de->newFile())
		{
			USAGE_LOG(logNewFile());
			scene->reset();
			clearUndo();
			setupGlobalEvents();
			processCompilationResult();
		}
	}

	void ThymioVisualProgramming::openFile()
	{
		USAGE_LOG(logOpenFile());
		de->openFile();
	}
	
	bool ThymioVisualProgramming::save()
	{
		USAGE_LOG(logSave());
		return de->saveFile(false);
	}
	
	bool ThymioVisualProgramming::saveAs()
	{
		USAGE_LOG(logSaveAs());
		return de->saveFile(true);
	}

	bool ThymioVisualProgramming::closeFile()
	{
		USAGE_LOG(logCloseFile());
		if (!isVisible())
			return true;
		
		if (scene->isEmpty() || preDiscardWarningDialog(true)) 
		{
			de->clearOpenedFileName(scene->isModified());
			clearHighlighting(true);
			clearSceneWithoutRecompilation();
			return true;
		}
		else
		{
			de->clearOpenedFileName(scene->isModified());
			return false;
		}
	}
	
	void ThymioVisualProgramming::showErrorLine()
	{
		if (!scene->compilationResult().isSuccessful())
			view->ensureVisible(scene->getSetRow(scene->compilationResult().errorLine));
	}
	
	void ThymioVisualProgramming::setColorScheme(int index)
	{
		Style::blockSetCurrentColorIndex(index);
		
		scene->update();
		updateBlockButtonImages();
	}
	
	void ThymioVisualProgramming::updateBlockButtonImages()
	{
		for(QList<BlockButton*>::iterator itr(eventButtons.begin());
			itr != eventButtons.end(); ++itr)
			(*itr)->updateBlockImage(scene->getAdvanced());

		for(QList<BlockButton*>::iterator itr(actionButtons.begin());
			itr != actionButtons.end(); ++itr)
			(*itr)->updateBlockImage(scene->getAdvanced());
	}
	
	void ThymioVisualProgramming::run()
	{
		USAGE_LOG(logRun());
		if(runButton->isEnabled())
		{
			de->loadAndRun();
			stopRunButtonAnimationTimer();
		}
	}

	void ThymioVisualProgramming::stop()
	{
		USAGE_LOG(logStop());
		de->stop();
		const unsigned leftSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.left.target");
		de->setVariableValues(leftSpeedVarPos, VariablesDataVector(1, 0));
		const unsigned rightSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.right.target");
		de->setVariableValues(rightSpeedVarPos, VariablesDataVector(1, 0));
	}
	
	void ThymioVisualProgramming::toggleAdvancedMode()
	{
		toggleAdvancedMode(!scene->getAdvanced());
	}
	
	void ThymioVisualProgramming::toggleAdvancedMode(bool advanced, bool force, bool ignoreSceneCheck)
	{
		if (!ignoreSceneCheck && (scene->getAdvanced() == advanced))
			return;
		if (advanced)
		{
			advancedButton->setIcon(QIcon(":/images/icons/vpl-advanced_on.svgz"));
			eventButtons[eventButtons.size()-1]->show(); // timeout
			actionButtons[actionButtons.size()-2]->show(); // timer
			actionButtons[actionButtons.size()-1]->show(); // set state
			scene->setAdvanced(true);
		}
		else
		{
			bool doClear(true);
			if (!force && scene->isAnyAdvancedFeature())
			{
				if (QMessageBox::warning(this, tr("Returning to simple mode"),
					tr("You are currently using states. Returning to simple mode will discard any state filter or state setting card.<p>Are you sure you want to continue?"),
				QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No)
					doClear = false;
			}
			
			if (doClear)
			{
				advancedButton->setIcon(QIcon(":/images/icons/vpl-advanced_off.svgz"));
				eventButtons[eventButtons.size()-1]->hide(); // timeout
				actionButtons[actionButtons.size()-2]->hide(); // timer
				actionButtons[actionButtons.size()-1]->hide(); // set state
				scene->setAdvanced(false);
			}
		}
		updateBlockButtonImages();
	}
	
	void ThymioVisualProgramming::closeEvent ( QCloseEvent * event )
	{
		if (!closeFile())
			event->ignore();
		else
			saveGeometryIfVisible();
	}
	
	void ThymioVisualProgramming::saveGeometryIfVisible()
	{
		if (isVisible())
		{
			QSettings settings;
			settings.setValue("ThymioVisualProgramming/geometry", saveGeometry());
		}
	}
	
	bool ThymioVisualProgramming::preDiscardWarningDialog(bool keepCode)
	{
		if (!scene->isModified())
			return true;
		
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Warning"));
		msgBox.setText(tr("The VPL document has been modified.<p>Do you want to save the changes?"));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Save);
		
		int ret = msgBox.exec();
		switch (ret)
		{
			case QMessageBox::Save:
			{
				//clearHighlighting(keepCode);
				if (save())
					return true;
				else
					return false;
			}
			case QMessageBox::Discard:
			{
				//clearHighlighting(keepCode);
				return true;
			}
			case QMessageBox::Cancel:
			default:
				return false;
		}

		return false;
	}
	
	void ThymioVisualProgramming::clearHighlighting(bool keepCode)
	{
		if (keepCode && scene->compilationResult().isSuccessful())
			de->displayCode(scene->getCode(), -1);
		else
			de->displayCode(QList<QString>(), -1);
	}

	QDomDocument ThymioVisualProgramming::saveToDom() const 
	{
		// if VPL is in its virign state, do not return a valid document
		// to prevent erasing of text-base code
		if (scene->isEmpty())
			return QDomDocument();
		
		QDomDocument document("tool-plugin-data");
		
		QDomElement vplroot = document.createElement("vplroot");
		vplroot.setAttribute("xml-format-version", 1);
		document.appendChild(vplroot);

		vplroot.appendChild(scene->serialize(document));

		scene->setModified(false);

		return document;
	}
	
	void ThymioVisualProgramming::aboutToLoad()
	{
		clearHighlighting(false);
		saveGeometryIfVisible();
		hide();
	}

	void ThymioVisualProgramming::loadFromDom(const QDomDocument& document, bool fromFile) 
	{
		const QDomElement vplroot(document.documentElement());
		const QString version(vplroot.attribute("xml-format-version"));
		
		if (version.isNull())
		{
			loadFromDom(transformDomToVersion1(document), fromFile);
			return;
		}
		if (version != "1")
		{
			QMessageBox::warning(0, tr("Incompatible Version"), tr("This file is incompatible with this version of ThymioVPL. It might not work correctly."));
		}
		
		loading = true;
		
		// load program
		const QDomElement programElement(vplroot.firstChildElement("program"));
		if (!programElement.isNull())
		{
			scene->deserialize(programElement);
			toggleAdvancedMode(scene->getAdvanced(), true, true);
		}
		
		scene->setModified(!fromFile);
		scene->recompileWithoutSetModified();
		
		if (!scene->isEmpty())
		{
			setupGlobalEvents();
			showAtSavedPosition();
		}
		
		clearUndo();
		
		loading = false;
		
		// once loaded, process the result of compilation if there is anything to compile
		if (!scene->isEmpty())
			processCompilationResult();
	}
	
	QDomDocument ThymioVisualProgramming::transformDomToVersion1(const QDomDocument& document0)
	{
		const QDomElement vplroot0(document0.documentElement());
		const QDomElement settings0(vplroot0.firstChildElement("settings"));
		
		QDomDocument document1;
		
		QDomElement vplroot1(document1.createElement("vplroot"));
		document1.appendChild(vplroot1);
		vplroot1.setAttribute("xml-format-version", "1");
		
		QDomElement settings1(document1.createElement("settings"));
		vplroot1.appendChild(settings1);
		settings1.setAttribute("color-scheme", settings0.attribute("color-scheme"));
		
		QDomElement program1(document1.createElement("program"));
		vplroot1.appendChild(program1);
		program1.setAttribute("advanced_mode", settings0.attribute("advanced-mode") == "true");
		
		// track duplicate events
		QMap<QString, QDomElement> setsByEvent;
		
		for (QDomElement buttonset0(vplroot0.firstChildElement("buttonset")); !buttonset0.isNull(); buttonset0 = buttonset0.nextSiblingElement("buttonset"))
		{
			QDomElement set1(document1.createElement("set"));
			
			QString eventName(buttonset0.attribute("event-name"));
			if (!eventName.isNull())
			{
				if (eventName == "tap")
				{
					// tap event was renamed to acc
					eventName = "acc";
				}
				
				QDomElement event1(document1.createElement("block"));
				set1.appendChild(event1);
				event1.setAttribute("type", "event");
				event1.setAttribute("name", eventName);
				for (unsigned i(0);; ++i)
				{
					QString value(buttonset0.attribute(QString("eb%0").arg(i)));
					if (value.isNull())
						break;
					if (eventName == "prox" || eventName == "proxground")
					{
						// values 1 and 2 have exchanged semantics
						if (value == "1")
							value = "2";
						else if (value == "2")
							value = "1";
					}
					event1.setAttribute(QString("value%0").arg(i), value);
				}
			}
			
			int state(buttonset0.attribute("state").toInt());
			if (state >= 0)
			{
				QDomElement state1(document1.createElement("block"));
				set1.appendChild(state1);
				state1.setAttribute("type", "state");
				state1.setAttribute("name", "statefilter");
				for (unsigned i(0); i < 4; ++i)
				{
					const unsigned value((state >> (i * 2)) & 0x3);
					state1.setAttribute(QString("value%0").arg(i), value);
				}
			}
			
			{
				// check for duplicate events
				QString setKey;
				QTextStream setKeyStream(&setKey);
				set1.save(setKeyStream, 0);
				if (!setsByEvent.contains(setKey))
				{
					// new event => insert the new set in the document
					setsByEvent.insert(setKey, set1);
					program1.appendChild(set1);
				}
				else
				{
					// duplicate event => reuse previously-inserted set
					set1 = setsByEvent.value(setKey);
				}
			}

			QString actionName(buttonset0.attribute("action-name"));
			if (!actionName.isNull())
			{
				if (actionName == "statefilter")
				{
					// statefilter action was renamed to setstate
					actionName = "setstate";
				}
				
				QDomElement action1(document1.createElement("block"));
				set1.appendChild(action1);
				action1.setAttribute("type", "action");
				action1.setAttribute("name", actionName);
				for (unsigned i(0);; ++i)
				{
					QString value(buttonset0.attribute(QString("ab%0").arg(i)));
					if (value.isNull())
						break;
					action1.setAttribute(QString("value%0").arg(i), value);
				}
			}
		}
		
		return document1;
	}
	
	void ThymioVisualProgramming::codeChangedInEditor()
	{
		if (!isVisible() && !loading)
		{
			clearSceneWithoutRecompilation();
		}
	}
	
	bool ThymioVisualProgramming::isModified() const
	{
		return scene->isModified();
	}
	
	qreal ThymioVisualProgramming::getViewScale() const
	{
		return view->getScale();
	}
	
	void ThymioVisualProgramming::pushUndo()
	{
		/* Useful code for printing backtrace
		#ifdef Q_OS_LINUX
		// dump call stack
		const size_t SIZE(100);
		void *buffer[SIZE];
		char **strings;
		int nptrs = backtrace(buffer, SIZE);
		printf("backtrace() returned %d addresses\n", nptrs);
		strings = backtrace_symbols(buffer, nptrs);
		if (strings == NULL) {
			perror("backtrace_symbols");
			exit(EXIT_FAILURE);
		}
		for (int j = 0; j < nptrs; j++)
			printf("%s\n", strings[j]);
		printf("\n");
		#endif // Q_OS_LINUX
		*/
		
		undoStack.push(scene->toString());
		undoPos = undoStack.size()-2;
		undoButton->setEnabled(undoPos >= 0);
		
		if (undoStack.size() > 50)
			undoStack.pop_front();
		
		redoButton->setEnabled(false);
	}
	
	void ThymioVisualProgramming::clearUndo()
	{
		undoButton->setEnabled(false);
		redoButton->setEnabled(false);
		undoStack.clear();
		undoPos = -1;
		pushUndo();
	}
	
	void ThymioVisualProgramming::undo()
	{
		if (undoPos < 0)
			return;
		
		scene->fromString(undoStack[undoPos]);
		toggleAdvancedMode(scene->getAdvanced(), true, true);
		scene->setModified(true);
		scene->recompileWithoutSetModified();

		--undoPos;
		
		undoButton->setEnabled(undoPos >= 0);
		redoButton->setEnabled(true);
	}
	
	void ThymioVisualProgramming::redo()
	{
		if (undoPos >= undoStack.size()-2)
			return;
		
		++undoPos;
		
		scene->fromString(undoStack[undoPos+1]);
		toggleAdvancedMode(scene->getAdvanced(), true, true);
		scene->setModified(true);
		scene->recompileWithoutSetModified();
		
		undoButton->setEnabled(true);
		redoButton->setEnabled(undoPos < undoStack.size()-2);
	}

	void ThymioVisualProgramming::processCompilationResult()
	{
		if (loading)
			return;
		
		const Compiler::CompilationResult compilation(scene->compilationResult());
		compilationResult->setText(compilation.getMessage(scene->getAdvanced()));
		const int fontSize(QFontMetrics(compilationResult->font()).lineSpacing());
		if (compilation.isSuccessful())
		{
			compilationResult->setStyleSheet("QLabel { font-size: 10pt; }");
			compilationResultImage->setPixmap(pixmapFromSVG(":/images/vpl/success.svgz", fontSize*1.2));
			de->displayCode(scene->getCode(), scene->getSelectedSetCodeId());
			runButton->setEnabled(true);
			// content changed but not uploaded, show animation
			startRunButtonAnimationTimer();
			showCompilationError->hide();
			emit compilationOutcome(true);
		}
		else if ((compilation.errorType == Compiler::MISSING_EVENT) && (scene->isSetLast(compilation.errorLine)))
		{
			compilationResult->setText(tr("Please add an event"));
			compilationResult->setStyleSheet("QLabel { font-size: 10pt; }");
			compilationResultImage->setPixmap(pixmapFromSVG(":/images/vpl/missing_block.svgz", fontSize*1.2));
			runButton->setEnabled(false);
			// error, cannot upload, stop animation
			stopRunButtonAnimationTimer();
			showCompilationError->show();
			emit compilationOutcome(false);
		}
		else if ((compilation.errorType == Compiler::MISSING_ACTION) && (scene->isSetLast(compilation.errorLine)))
		{
			compilationResult->setText(tr("Please add an action"));
			compilationResult->setStyleSheet("QLabel { font-size: 10pt; }");
			compilationResultImage->setPixmap(pixmapFromSVG(":/images/vpl/missing_block.svgz", fontSize*1.2));
			runButton->setEnabled(false);
			// error, cannot upload, stop animation
			stopRunButtonAnimationTimer();
			showCompilationError->show();
			emit compilationOutcome(false);
		}
		else
		{
			compilationResult->setStyleSheet("QLabel { font-size: 10pt; color: rgb(231,19,0); }");
			compilationResultImage->setPixmap(pixmapFromSVG(":/images/vpl/error.svgz", fontSize*1.2));
			runButton->setEnabled(false);
			// error, cannot upload, stop animation
			stopRunButtonAnimationTimer();
			showCompilationError->show();
			emit compilationOutcome(false);
		}
	}
	
	void ThymioVisualProgramming::processHighlightChange()
	{
		if (scene->compilationResult().isSuccessful())
			de->displayCode(scene->getCode(), scene->getSelectedSetCodeId());
	}
	
	void ThymioVisualProgramming::userEvent(unsigned id, const VariablesDataVector& data)
	{
		// if hidden, do not react
		if (isHidden())
			return;
		
		// here we can add react to incoming events
		if (id == 2)
			USAGE_LOG(logTabletData(data));
		else if (id > 2)
			USAGE_LOG(logUserEvent(id,data));
		
		// a set was executed on target, highlight in program if the code in the robot is the same as in this editor
		if (id == 0 && runButton->isEnabled() && !runAnimTimer)
		{
			const unsigned index(data[0]);
			assert (index < scene->setsCount());
			(*(scene->setsBegin() + index))->blink();
		}
	}
	
	void ThymioVisualProgramming::addEvent()
	{
		BlockButton* button(polymorphic_downcast<BlockButton*>(sender()));
		view->ensureVisible(scene->addEvent(button->getName()));
	}
	
	void ThymioVisualProgramming::addAction()
	{
		BlockButton* button(polymorphic_downcast<BlockButton*>(sender()));
		view->ensureVisible(scene->addAction(button->getName()));
	}
	
	static QImage blend(const QImage& firstImage, const QImage& secondImage, qreal ratio)
	{
		// only attempt to blend images of similar sizes and formats
		if (firstImage.size() != secondImage.size())
			return QImage();
		if (firstImage.format() != secondImage.format())
			return QImage();
		if ((firstImage.format() != QImage::Format_ARGB32) && (firstImage.format() != QImage::Format_ARGB32_Premultiplied))
			return QImage();
		
		// handcoded blend, because Qt painter-based function have problems on many platforms
		QImage destImage(firstImage.size(), firstImage.format());
		const quint32 a(ratio*255.);
		const quint32 oma(255-a);
		for (int y=0; y<firstImage.height(); ++y)
		{
			const quint32 *pF(reinterpret_cast<const quint32*>(firstImage.scanLine(y)));
			const quint32 *pS(reinterpret_cast<const quint32*>(secondImage.scanLine(y)));
			quint32 *pD(reinterpret_cast<quint32*>(destImage.scanLine(y)));
			
			for (int x=0; x<firstImage.width(); ++x)
			{
				const quint32 vF(*pF++);
				const quint32 vS(*pS++);
				
				*pD++ = 
					(((((vF >> 24) & 0xff) * a + ((vS >> 24) & 0xff) * oma) & 0xff00) << 16) |
					(((((vF >> 16) & 0xff) * a + ((vS >> 16) & 0xff) * oma) & 0xff00) <<  8) |
					(((((vF >>  8) & 0xff) * a + ((vS >>  8) & 0xff) * oma) & 0xff00)      ) |
					(((((vF >>  0) & 0xff) * a + ((vS >>  0) & 0xff) * oma) & 0xff00) >>  8);
			}
		}
		
		return destImage;
	}
	
	#ifndef Q_OS_WIN
	
	void ThymioVisualProgramming::regenerateRunButtonAnimation(const QSize& iconSize)
	{
		// load the play animation
		// first image
		QImageReader playReader(":/images/icons/play.svgz");
		playReader.setScaledSize(iconSize);
		const QImage playImage(playReader.read());
		// last image
		QImageReader playRedReader(":/images/icons/play-green.svgz");
		playRedReader.setScaledSize(iconSize);
		const QImage playRedImage(playRedReader.read());
		
		const int count(runAnimFrames.size());
		for (int i = 0; i<count; ++i)
		{
			qreal alpha((1.+cos(qreal(i)*M_PI/qreal(count)))*.5);
			runAnimFrames[i] = QPixmap::fromImage(blend(playImage, playRedImage, alpha));
		}
	}
	
	#endif // Q_OS_WIN
	
	//! Start the run button animation timer, safe even if the timer is already running
	void ThymioVisualProgramming::startRunButtonAnimationTimer()
	{
		if (!runAnimTimer)
			runAnimTimer = startTimer(50);
	}
	
	//! Stop the run button animation timer, safe even if the timer is not running
	void ThymioVisualProgramming::stopRunButtonAnimationTimer()
	{
		if (runAnimTimer)
		{
			killTimer(runAnimTimer);
			runAnimTimer = 0;
			#ifdef Q_OS_WIN
			runButton->setIcon(QIcon(":/images/icons/play.svgz"));
			#else // Q_OS_WIN
			runButton->setIcon(runAnimFrames[0]);
			#endif // Q_OS_WIN
		}
	}
	
	float ThymioVisualProgramming::computeScale(QResizeEvent *event, int desiredToolbarIconSize)
	{
		// FIXME: scale computation should be updated 
		// desired sizes for height
		const int idealContentHeight(6*256);
		const int uncompressibleHeight(
			max(actionsLabel->height(), eventsLabel->height()) +
			desiredToolbarIconSize * 2 + 
			4 * style()->pixelMetric(QStyle::PM_ButtonMargin) + 
			4 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth) +
			style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) +
			6 * 10 +
			2 * style()->pixelMetric(QStyle::PM_LayoutTopMargin) + 
			2 * style()->pixelMetric(QStyle::PM_LayoutBottomMargin)
		);
		const int availableHeight(event->size().height() - uncompressibleHeight);
		const qreal scaleHeight(qreal(availableHeight)/qreal(idealContentHeight));
		
		// desired sizes for width
		const int idealContentWidth(1088+256*2);
		const int uncompressibleWidth(
			2 * style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) +
			style()->pixelMetric(QStyle::PM_LayoutLeftMargin) + 
			style()->pixelMetric(QStyle::PM_LayoutRightMargin) +
			#ifdef ANDROID
			40 + 
			#else // ANDROID
			style()->pixelMetric(QStyle::PM_ScrollBarSliderMin) +
			#endif // ANDROID
			2 * 20
		);
		const int availableWidth(event->size().width() - uncompressibleWidth);
		const qreal scaleWidth(qreal(availableWidth)/qreal(idealContentWidth));
		
		// compute and set scale
		const qreal scale(qMin(scaleHeight, scaleWidth));
		return scale;
	}
	
	void ThymioVisualProgramming::resizeEvent( QResizeEvent *event)
	{
		// compute size of elements for toolbar
		const float toolbarWidgetCount(12.25);
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
		
		// two pass of layout computation, should be a good-enough approximation
		qreal testScale(computeScale(event, desiredIconSize));
		desiredIconSize = qMin(desiredIconSize, int(256.*testScale));
		desiredIconSize = qMin(desiredIconSize, event->size().height()/14);
		const qreal scale(computeScale(event, desiredIconSize));
		
		// set toolbar
		const QSize tbIconSize(QSize(desiredIconSize, desiredIconSize));
		const QSize importantIconSize(tbIconSize * 1.41);
		#ifndef Q_OS_WIN
		regenerateRunButtonAnimation(importantIconSize);
		#endif // Q_OS_WIN
		newButton->setIconSize(tbIconSize);
		openButton->setIconSize(tbIconSize);
		saveButton->setIconSize(tbIconSize);
		saveAsButton->setIconSize(tbIconSize);
		undoButton->setIconSize(tbIconSize);
		redoButton->setIconSize(tbIconSize);
		runButton->setIconSize(importantIconSize);
		stopButton->setIconSize(importantIconSize);
		advancedButton->setIconSize(tbIconSize);
		helpButton->setIconSize(tbIconSize);
		snapshotButton->setIconSize(tbIconSize);
		spacer1->changeSize(desiredIconSize/4, desiredIconSize);
		spacer2->changeSize(desiredIconSize/4, desiredIconSize);
		spacer3->changeSize(desiredIconSize, desiredIconSize);
		spacerRunStop->changeSize(desiredIconSize/4, desiredIconSize);
		spacer4->changeSize(desiredIconSize, desiredIconSize);
		spacer5->changeSize(desiredIconSize/4, desiredIconSize);
		spacer6->changeSize(desiredIconSize/4, desiredIconSize);
		toolLayout->invalidate();
		
		// set view and cards on sides
		#ifdef Q_WS_MACX // we have to work around bugs in size reporting in OS X
		const QSize iconSize(243*scale, 243*scale);
		#else // Q_WS_MACX
		const QSize iconSize(256*scale, 256*scale);
		#endif // Q_WS_MACX
		for(QList<BlockButton*>::iterator itr = eventButtons.begin();
			itr != eventButtons.end(); ++itr)
			(*itr)->setIconSize(iconSize);
		for(QList<BlockButton*>::iterator itr = actionButtons.begin();
			itr != actionButtons.end(); ++itr)
			(*itr)->setIconSize(iconSize);
	}
	
	void ThymioVisualProgramming::timerEvent ( QTimerEvent * event )
	{
		#ifdef Q_OS_WIN
		// for unknown reason, animation does not work some times on Windows, so setting image directly from QIcon	
		if (runAnimFrame >= 0)
			runButton->setIcon(QIcon(":/images/icons/play.svgz"));
		else
			runButton->setIcon(QIcon(":/images/icons/play-green.svgz"));
		#else // Q_OS_WIN
		if (runAnimFrame >= 0)
			runButton->setIcon(runAnimFrames[runAnimFrame]);
		else
			runButton->setIcon(runAnimFrames[-runAnimFrame]);
		#endif // Q_OS_WIN
		runAnimFrame++;
		if (runAnimFrame == runAnimFrames.size())
			runAnimFrame = -runAnimFrames.size()+1;
	}
} } // namespace ThymioVPL / namespace Aseba
