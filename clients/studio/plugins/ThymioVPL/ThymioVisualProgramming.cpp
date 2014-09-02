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

#include "../../TargetModels.h"
#include "../../../../common/utils/utils.h"

// for backtrace
//#include <execinfo.h>

using namespace std;

namespace Aseba { namespace ThymioVPL
{
	// Visual Programming
	ThymioVisualProgramming::ThymioVisualProgramming(DevelopmentEnvironmentInterface *_de, bool showCloseButton, bool debugLog):
		debugLog(debugLog),
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
		
		//toolBar = new QToolBar();
		//mainLayout->addWidget(toolBar);
		toolLayout = new  QGridLayout();
		mainLayout->addLayout(toolLayout);

		newButton = new QPushButton();
		newButton->setIcon(QIcon(":/images/filenew.svgz"));
		newButton->setToolTip(tr("New"));
		newButton->setFlat(true);
		toolLayout->addWidget(newButton,0,0);
		
		openButton = new QPushButton();
		openButton->setIcon(QIcon(":/images/fileopen.svgz"));
		openButton->setToolTip(tr("Open"));
		openButton->setFlat(true);
		toolLayout->addWidget(openButton,0,1);
		
		saveButton = new QPushButton();
		saveButton->setIcon(QIcon(":/images/save.svgz"));
		saveButton->setToolTip(tr("Save"));
		saveButton->setFlat(true);
		toolLayout->addWidget(saveButton,1,0);
		
		saveAsButton = new QPushButton();
		saveAsButton->setIcon(QIcon(":/images/saveas.svgz"));
		saveAsButton->setToolTip(tr("Save as"));
		saveAsButton->setFlat(true);
		toolLayout->addWidget(saveAsButton,1,1);
		
		undoButton = new QPushButton();
		undoButton->setIcon(QIcon(":/images/edit-undo.svgz"));
		undoButton->setToolTip(tr("Undo"));
		undoButton->setFlat(true);
		undoButton->setEnabled(false);
		undoButton->setShortcut(QKeySequence::Undo);
		toolLayout->addWidget(undoButton,0,2);
		
		redoButton = new QPushButton();
		redoButton->setIcon(QIcon(":/images/edit-redo.svgz"));
		redoButton->setToolTip(tr("Redo"));
		redoButton->setFlat(true);
		redoButton->setEnabled(false);
		redoButton->setShortcut(QKeySequence::Redo);
		toolLayout->addWidget(redoButton,1,2);
		//toolLayout->addSeparator();

		//runButton = new QPushButton();
		runButton = new QPushButton();
		runButton->setIcon(QIcon(":/images/play.svgz"));
		runButton->setToolTip(tr("Load & Run"));
		runButton->setFlat(true);
		toolLayout->addWidget(runButton,0,3,2,2);

		stopButton = new QPushButton();
		stopButton->setIcon(QIcon(":/images/stop1.svgz"));
		stopButton->setToolTip(tr("Stop"));
		stopButton->setFlat(true);
		toolLayout->addWidget(stopButton,0,5,2,2);
		//toolLayout->addSeparator();
		
		advancedButton = new QPushButton();
		advancedButton->setIcon(QIcon(":/images/vpl_advanced_mode.svgz"));
		advancedButton->setToolTip(tr("Advanced mode"));
		advancedButton->setFlat(true);
		toolLayout->addWidget(advancedButton,0,7,2,2);
		//toolLayout->addSeparator();
		
		/*colorComboButton = new QComboBox();
		colorComboButton->setToolTip(tr("Color scheme"));
		setColors(colorComboButton);
		toolLayout->addWidget(colorComboButton,1,9,1,2);
		*/
		//toolLayout->addSeparator();

		helpButton = new QPushButton();
		helpButton->setIcon(QIcon(":/images/info.svgz"));
		helpButton->setToolTip(tr("Help"));
		helpButton->setFlat(true);
		toolLayout->addWidget(helpButton,0,9);
		
		snapshotButton = new QPushButton();
		snapshotButton->setIcon(QIcon(":/images/ksnapshot.svgz"));
		snapshotButton->setToolTip(tr("Snapshot"));
		snapshotButton->setFlat(true);
		toolLayout->addWidget(snapshotButton,1,9);

		if (showCloseButton)
		{
			//toolLayout->addSeparator();
			quitButton = new QPushButton();
			quitButton->setIcon(QIcon(":/images/exit.svgz"));
			quitButton->setToolTip(tr("Quit"));
			quitButton->setFlat(true);
			toolLayout->addWidget(quitButton,0,10);
			connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
			quitSpotSpacer = 0;
		}
		else
		{
			quitSpotSpacer = new QSpacerItem(1,1);
			toolLayout->addItem(quitSpotSpacer, 0, 10);
			quitButton = 0;
		}
		
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
		
		//connect(colorComboButton, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorScheme(int)));
		
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
		
		eventsLabel = new QLabel(tr("<b>Events</b>"));
		eventsLabel ->setStyleSheet("QLabel { font-size: 10pt; }");
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
		
		horizontalLayout->addLayout(eventsLayout);
		
		sceneLayout = new QVBoxLayout();

		// compilation
		compilationResultImage = new QLabel();
		compilationResult = new QLabel(tr("Compilation success."));
		compilationResult->setStyleSheet("QLabel { font-size: 10pt; }");
		
		compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
		compilationResult->setWordWrap(true);
		compilationResult->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
		
		showCompilationError = new QPushButton(tr("show line"));
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
		
		if (debugLog)
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
		
		actionsLabel = new QLabel(tr("<b>Actions</b>"));
		actionsLabel ->setStyleSheet("QLabel { font-size: 10pt; }");
		actionsLayout->setAlignment(Qt::AlignTop);
		#ifdef Q_WS_MACX
		actionsLayout->setSpacing(20);
		#else // Q_WS_MACX
		actionsLayout->setSpacing(10);
		#endif // Q_WS_MACX
		actionsLayout->addWidget(actionsLabel);
		foreach (button, actionButtons)
		{
			actionsLayout->addWidget(button);
			connect(button, SIGNAL(clicked()), SLOT(addAction()));
			connect(button, SIGNAL(contentChanged()), scene, SLOT(recompile()));
			connect(button, SIGNAL(undoCheckpoint()), SLOT(pushUndo()));
		}
		
		actionButtons.last()->hide(); // memory

		horizontalLayout->addLayout(actionsLayout);
		
		// window properties
		setWindowModality(Qt::ApplicationModal);
		
		// save initial state
		pushUndo();
	}
	
	ThymioVisualProgramming::~ThymioVisualProgramming()
	{
		saveGeometryIfVisible();
	}
	
	void ThymioVisualProgramming::openHelp() const
	{
		QDesktopServices::openUrl(QUrl(tr("http://aseba.wikidot.com/en:thymiovpl")));
	}
	
	void ThymioVisualProgramming::saveSnapshot() const
	{
		QString initialFile;
		if (!de->openedFileName().isEmpty())
		{
			const QFileInfo pf(de->openedFileName());
			initialFile = (pf.absolutePath() + QDir::separator() + pf.baseName() + ".svg");
		}
		QString fileName(QFileDialog::getSaveFileName(0,
			tr("Export program as SVG"), initialFile, "Scalable Vector Graphics (*.svg)"));
		
		if (fileName.isEmpty())
			return;
		
		if (fileName.lastIndexOf(".") < 0)
			fileName += ".svg";
		
		QSvgGenerator generator;
		generator.setFileName(fileName);
		generator.setSize(scene->sceneRect().size().toSize());
		generator.setViewBox(scene->sceneRect());
		generator.setTitle(tr("VPL program %0").arg(de->openedFileName()));
		generator.setDescription(tr("This image was generated with Thymio VPL from Aseba, get it at http://thymio.org"));
		
		QPainter painter(&generator);
		scene->render(&painter);
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
		QPushButton *vplButton = new QPushButton(QIcon(":/images/thymiovpl.png"), tr("Launch VPL"));
		vplButton->setIconSize(QSize(86,32));
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
		view->recomputeScale();
	}

	void ThymioVisualProgramming::showVPLModal()
	{
		if (de->newFile())
		{
			toggleAdvancedMode(false, true);
			scene->reset();
			clearUndo();
			showAtSavedPosition();
			processCompilationResult();
		}
	}
	
	void ThymioVisualProgramming::newFile()
	{
		if (de->newFile())
		{
			toggleAdvancedMode(false, true);
			scene->reset();
			clearUndo();
			processCompilationResult();
		}
	}

	void ThymioVisualProgramming::openFile()
	{
		de->openFile();
	}
	
	bool ThymioVisualProgramming::save()
	{
		return de->saveFile(false);
	}
	
	bool ThymioVisualProgramming::saveAs()
	{
		return de->saveFile(true);
	}

	bool ThymioVisualProgramming::closeFile()
	{
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
		if(runButton->isEnabled())
		{
			de->loadAndRun();
			if (runAnimTimer)
			{
				killTimer(runAnimTimer);
				runAnimTimer = 0;
				runButton->setIcon(runAnimFrames[0]);
			}
		}
	}

	void ThymioVisualProgramming::stop()
	{
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
			advancedButton->setIcon(QIcon(":/images/vpl_simple_mode.svgz"));
			actionButtons.last()->show(); // state button
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
				advancedButton->setIcon(QIcon(":/images/vpl_advanced_mode.svgz"));
				actionButtons.last()->hide(); // state button
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

		QDomElement settings = document.createElement("settings");
		//settings.setAttribute("color-scheme", QString::number(colorComboButton->currentIndex()));
		settings.setAttribute("color-scheme", 0);
		vplroot.appendChild(settings);
		
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
		loading = true;
		
		// first load settings
		const QDomElement settingsElement(document.documentElement().firstChildElement("settings"));
		if (!settingsElement.isNull())
		{
			//colorComboButton->setCurrentIndex(settingsElement.attribute("color-scheme").toInt());
		}
		
		// then load program
		const QDomElement programElement(document.documentElement().firstChildElement("program"));
		if (!programElement.isNull())
		{
			scene->deserialize(programElement);
			toggleAdvancedMode(scene->getAdvanced(), true);
		}
		
		scene->setModified(!fromFile);
		scene->recompileWithoutSetModified();
		
		if (!scene->isEmpty())
			showAtSavedPosition();
		
		loading = false;
		clearUndo();
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
		compilationResult->setText(scene->compilationResult().getMessage(scene->getAdvanced()));
		if (scene->compilationResult().isSuccessful())
		{
			compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
			de->displayCode(scene->getCode(), scene->getSelectedSetCodeId());
			runButton->setEnabled(true);
			showCompilationError->hide();
			emit compilationOutcome(true);
		}
		else
		{
			compilationResultImage->setPixmap(QPixmap(QString(":/images/warning.png")));
			runButton->setEnabled(false);
			showCompilationError->show();
			emit compilationOutcome(false);
		}
		// content changed but not uploaded
		if (!runAnimTimer)
			runAnimTimer = startTimer(50);
	}
	
	void ThymioVisualProgramming::processHighlightChange()
	{
		if (scene->compilationResult().isSuccessful())
			de->displayCode(scene->getCode(), scene->getSelectedSetCodeId());
	}
	
	void ThymioVisualProgramming::userEvent(unsigned id, const VariablesDataVector& data)
	{
		// NOTE: here we can add react to incoming events
		if (id != 0)
			return;
		
		// TODO: highlight pair
		cerr << "event " << data.at(0) << endl;
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
	
	void ThymioVisualProgramming::regenerateRunButtonAnimation(const QSize& iconSize)
	{
		// load the play animation
		// first image
		QImageReader playReader(":/images/play.svgz");
		playReader.setScaledSize(iconSize);
		const QImage playImage(playReader.read());
		// last image
		QImageReader playRedReader(":/images/play-green.svgz");
		playRedReader.setScaledSize(iconSize);
		const QImage playRedImage(playRedReader.read());
		
		const int count(runAnimFrames.size());
		for (int i = 0; i<count; ++i)
		{
			qreal alpha((1.+cos(qreal(i)*M_PI/qreal(count)))*.5);
			runAnimFrames[i] = QPixmap::fromImage(blend(playImage, playRedImage, alpha));
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
		const int toolbarWidgetCount(11);
		//const int toolbarSepCount(quitButton ? 5 : 4);
		// get width of combox box element (not content)
		QStyleOptionComboBox opt;
		QSize tmp(0, 0);
		tmp = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, tmp);
		int desiredIconSize((
			event->size().width() -
			(
				(toolbarWidgetCount-1) * style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) +
				(toolbarWidgetCount-1) * 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth) + 
				2 * style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth) +
				toolbarWidgetCount * 2 * style()->pixelMetric(QStyle::PM_ButtonMargin) + 
				style()->pixelMetric(QStyle::PM_LayoutLeftMargin) +
				style()->pixelMetric(QStyle::PM_LayoutRightMargin) +
				tmp.width()
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
		const QSize importantIconSize(tbIconSize * 2);
		regenerateRunButtonAnimation(importantIconSize);
		newButton->setIconSize(tbIconSize);
		openButton->setIconSize(tbIconSize);
		saveButton->setIconSize(tbIconSize);
		saveAsButton->setIconSize(tbIconSize);
		undoButton->setIconSize(tbIconSize);
		redoButton->setIconSize(tbIconSize);
		runButton->setIconSize(importantIconSize);
		stopButton->setIconSize(importantIconSize);
		advancedButton->setIconSize(importantIconSize);
		//colorComboButton->setIconSize(QSize((desiredIconSize*3)/2,desiredIconSize));
		helpButton->setIconSize(tbIconSize);
		snapshotButton->setIconSize(tbIconSize);
		if (quitButton)
			quitButton->setIconSize(tbIconSize);
		if (quitSpotSpacer)
		{
			quitSpotSpacer->changeSize(desiredIconSize, desiredIconSize);
			quitSpotSpacer->invalidate();
		}
		//toolBar->setIconSize(tbIconSize);
		
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
		if (runAnimFrame >= 0)
			runButton->setIcon(runAnimFrames[runAnimFrame]);
		else
			runButton->setIcon(runAnimFrames[-runAnimFrame]);
		runAnimFrame++;
		if (runAnimFrame == runAnimFrames.size())
			runAnimFrame = -runAnimFrames.size()+1;
	}
} } // namespace ThymioVPL / namespace Aseba
