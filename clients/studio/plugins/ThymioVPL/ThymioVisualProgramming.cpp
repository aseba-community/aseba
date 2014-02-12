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
#include <QtDebug>

#include "ThymioVisualProgramming.h"
#include "Block.h"
#include "EventBlocks.h"
#include "ActionBlocks.h"
#include "Scene.h"
#include "Buttons.h"

#include "../../TargetModels.h"
#include "../../../../common/utils/utils.h"

using namespace std;

namespace Aseba { namespace ThymioVPL
{
	ResizingView::ResizingView(QGraphicsScene * scene, QWidget * parent):
		QGraphicsView(scene, parent),
		computedScale(1)
	{
		assert(scene);
		connect(scene, SIGNAL(sceneSizeChanged()), SLOT(recomputeScale()));
	}
	
	void ResizingView::resizeEvent(QResizeEvent * event)
	{
		QGraphicsView::resizeEvent(event);
		recomputeScale();
	}
	
	void ResizingView::recomputeScale()
	{
		// set transform
		resetTransform();
		computedScale = 0.95*qreal(viewport()->width())/qreal(scene()->width());
		scale(computedScale, computedScale);
	}
	
	// Visual Programming
	ThymioVisualProgramming::ThymioVisualProgramming(DevelopmentEnvironmentInterface *_de, bool showCloseButton):
		de(_de),
		scene(new Scene(this)),
		loading(false),
		runAnimFrame(0),
		runAnimTimer(0)
	{
		runAnimFrames.resize(17);
		
		// Create the gui ...
		setWindowTitle(tr("Thymio Visual Programming Language"));
		setMinimumSize(QSize(400,400));
		
		mainLayout = new QVBoxLayout(this);
		
		toolBar = new QToolBar();
		mainLayout->addWidget(toolBar);

		newButton = new QToolButton();
		newButton->setIcon(QIcon(":/images/filenew.svgz"));
		newButton->setToolTip(tr("New"));
		toolBar->addWidget(newButton);
		
		openButton = new QToolButton();
		openButton->setIcon(QIcon(":/images/fileopen.svgz"));
		openButton->setToolTip(tr("Open"));
		toolBar->addWidget(openButton);
		
		saveButton = new QToolButton();
		saveButton->setIcon(QIcon(":/images/save.svgz"));
		saveButton->setToolTip(tr("Save"));
		toolBar->addWidget(saveButton);
		
		saveAsButton = new QToolButton();
		saveAsButton->setIcon(QIcon(":/images/saveas.svgz"));
		saveAsButton->setToolTip(tr("Save as"));
		toolBar->addWidget(saveAsButton);
		toolBar->addSeparator();

		runButton = new QToolButton();
		runButton->setIcon(QIcon(":/images/play.svgz"));
		runButton->setToolTip(tr("Load & Run"));
		toolBar->addWidget(runButton);

		stopButton = new QToolButton();
		stopButton->setIcon(QIcon(":/images/stop1.svgz"));
		stopButton->setToolTip(tr("Stop"));
		toolBar->addWidget(stopButton);
		toolBar->addSeparator();
	
		colorComboButton = new QComboBox();
		colorComboButton->setToolTip(tr("Color scheme"));
		setColors(colorComboButton);
		toolBar->addWidget(colorComboButton);
		toolBar->addSeparator();

		advancedButton = new QToolButton();
		advancedButton->setIcon(QIcon(":/images/vpl_advanced_mode.svgz"));
		advancedButton->setToolTip(tr("Advanced mode"));
		toolBar->addWidget(advancedButton);
		toolBar->addSeparator();
		
		helpButton = new QToolButton();
		QAction* action = new QAction(helpButton);
		action->setIcon(QIcon(":/images/info.svgz"));
		action->setToolTip(tr("Help"));
		action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymiovpl")));
		connect(action, SIGNAL(triggered()), this, SLOT(openToUrlFromAction()));
		helpButton->setDefaultAction(action);
		toolBar->addWidget(helpButton);

		if (showCloseButton)
		{
			toolBar->addSeparator();
			quitButton = new QToolButton();
			quitButton->setIcon(QIcon(":/images/exit.svgz"));
			quitButton->setToolTip(tr("Quit"));
			toolBar->addWidget(quitButton);
			connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
		}
		else
			quitButton = 0;
		
		connect(newButton, SIGNAL(clicked()), this, SLOT(newFile()));
		connect(openButton, SIGNAL(clicked()), this, SLOT(openFile()));
		connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
		connect(saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));
		connect(colorComboButton, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorScheme(int)));
		
		connect(runButton, SIGNAL(clicked()), this, SLOT(run()));
		connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
		connect(advancedButton, SIGNAL(clicked()), this, SLOT(toggleAdvancedMode()));
		
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
		eventsLayout->setAlignment(Qt::AlignTop);
		eventsLayout->setSpacing(0);
		BlockButton* button;
		eventsLayout->addWidget(eventsLabel);
		foreach (button, eventButtons)
		{
			eventsLayout->addItem(new QSpacerItem(0,10));
			eventsLayout->addWidget(button);
			connect(button, SIGNAL(clicked()), SLOT(addEvent()));
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
		connect(scene, SIGNAL(highlightChanged()), SLOT(processHighlightChange()));
		connect(scene, SIGNAL(modifiedStatusChanged(bool)), SIGNAL(modifiedStatusChanged(bool)));
		
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
		actionsLayout->setSpacing(0);
		actionsLayout->addWidget(actionsLabel);
		foreach (button, actionButtons)
		{
			actionsLayout->addItem(new QSpacerItem(0,10));
			actionsLayout->addWidget(button);
			connect(button, SIGNAL(clicked()), SLOT(addAction()));
		}
		
		actionButtons.last()->hide(); // memory

		horizontalLayout->addLayout(actionsLayout);
		
		// window properties
		setWindowModality(Qt::ApplicationModal);
	}
	
	ThymioVisualProgramming::~ThymioVisualProgramming()
	{
		saveGeometryIfVisible();
	}
	
	void ThymioVisualProgramming::openToUrlFromAction() const
	{
		const QAction *action(reinterpret_cast<QAction *>(sender()));
		QDesktopServices::openUrl(action->data().toUrl());
	}
	
	// TODO: add state to color scheme, use static data for color arrays
	
	static QColor currentEventColor = QColor(0,191,255);
	static QColor currentStateColor = QColor(122, 204, 0);
	static QColor currentActionColor = QColor(218,112,214);
	
	void ThymioVisualProgramming::setColors(QComboBox *button)
	{
		eventColors.push_back(QColor(0,191,255)); actionColors.push_back(QColor(218,112,214));
		eventColors.push_back(QColor(155,48,255)); actionColors.push_back(QColor(159,182,205));
		eventColors.push_back(QColor(67,205,128)); actionColors.push_back(QColor(0,197,205)); 
		eventColors.push_back(QColor(255,215,0)); actionColors.push_back(QColor(255,99,71));
		eventColors.push_back(QColor(255,97,3)); actionColors.push_back(QColor(142,56,142));
		eventColors.push_back(QColor(125,158,192)); actionColors.push_back(QColor(56,142,142)); 

		if( button )
		{
			for(int i=0; i<eventColors.size(); ++i)
				button->addItem(drawColorScheme(eventColors.at(i), actionColors.at(i)), "");
		}
	}
	
	QColor ThymioVisualProgramming::getBlockColor(const QString& type)
	{
		if (type == "event")
			return currentEventColor;
		else if (type == "state")
			return currentStateColor;
		else
			return currentActionColor;
	}
	
	QPixmap ThymioVisualProgramming::drawColorScheme(QColor color1, QColor color2)
	{
		QPixmap pixmap(128,58);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		
		painter.setBrush(color1);
		painter.drawRoundedRect(0,0,54,54,4,4);
		
		painter.setBrush(color2);
		painter.drawRoundedRect(66,0,54,54,4,4);
		
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
		scene->clear();
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
			scene->reset();
			toggleAdvancedMode(false);
			showAtSavedPosition();
			processCompilationResult();
		}
	}
	
	void ThymioVisualProgramming::newFile()
	{
		if (de->newFile())
		{
			scene->reset();
			toggleAdvancedMode(false);
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
			clearHighlighting(true);
			clearSceneWithoutRecompilation();
			return true;
		}
		else
			return false;
	}
	
	void ThymioVisualProgramming::showErrorLine()
	{
		if (!scene->compilationResult().isSuccessful())
			view->ensureVisible(scene->getSetRow(scene->compilationResult().errorLine));
	}
	
	void ThymioVisualProgramming::setColorScheme(int index)
	{
		currentEventColor = eventColors.at(index);
		currentActionColor = actionColors.at(index);
		
		scene->update();
		
		for(QList<BlockButton*>::iterator itr(eventButtons.begin());
			itr != eventButtons.end(); ++itr)
			(*itr)->updateBlockImage();

		for(QList<BlockButton*>::iterator itr(actionButtons.begin());
			itr != actionButtons.end(); ++itr)
			(*itr)->updateBlockImage();
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
	
	void ThymioVisualProgramming::toggleAdvancedMode(bool advanced, bool force)
	{
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
		settings.setAttribute("advanced-mode", scene->getAdvanced() ? "true" : "false");
		settings.setAttribute("color-scheme", QString::number(colorComboButton->currentIndex()));
		vplroot.appendChild(settings);
		vplroot.appendChild(document.createTextNode("\n\n"));
		
		for (Scene::SetConstItr itr(scene->setsBegin()); itr != scene->setsEnd(); ++itr)
		{
			const EventActionsSet* eventActionsSet(*itr);
			vplroot.appendChild(eventActionsSet->serialize(document));
			vplroot.appendChild(document.createTextNode("\n\n"));
		}

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
		scene->clear();
		
		QDomElement element(document.documentElement().firstChildElement());
		while (!element.isNull())
		{
			if (element.tagName() == "settings") 
			{
				toggleAdvancedMode(element.attribute("advanced-mode") == "true", true);
				colorComboButton->setCurrentIndex(element.attribute("color-scheme").toInt());
			}
			else if (element.tagName() == "set")
			{
				scene->addEventActionsSet(element);
			}
			else if(element.tagName() == "buttonset")
			{
				QMessageBox::warning(this, "Feature not implemented", "Loading of files from 1.3 is not implementd yet, it will be soon, please be patient. Thank you.");
				break;
				//scene->addEventActionsSetOldFormat_1_3(element);
			}
			
			element = element.nextSiblingElement();
		}
		
		scene->ensureOneEmptySetAtEnd();
		scene->recomputeSceneRect();
		scene->clearSelection();
		scene->setModified(!fromFile);
		scene->recompileWithoutSetModified();
		
		if (!scene->isEmpty())
			showAtSavedPosition();
		
		loading = false;
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
	
	void ThymioVisualProgramming::regenerateRunButtonAnimation(const QSize& iconSize)
	{
		// load the play animation
		// first image
		QImageReader playReader(":/images/play.svgz");
		playReader.setScaledSize(iconSize);
		const QPixmap playPixmap(QPixmap::fromImageReader(&playReader));
		// last image
		QImageReader playRedReader(":/images/play-red.svgz");
		playRedReader.setScaledSize(iconSize);
		const QPixmap playRedImage(QPixmap::fromImageReader(&playRedReader));
		
		const int count(runAnimFrames.size());
		for (int i = 0; i<count; ++i)
		{
			qreal alpha((1.+cos(qreal(i)*M_PI/qreal(count)))*.5);
			QPixmap &pixmap(runAnimFrames[i]);
			pixmap = QPixmap(iconSize);
			pixmap.fill(Qt::transparent);
			QPainter painter(&pixmap);
			painter.setCompositionMode(QPainter::CompositionMode_Plus);
			// copy first pixmap
			{
				QPixmap temp(iconSize);
				temp.fill(Qt::transparent);
				QPainter p(&temp);
				p.setCompositionMode(QPainter::CompositionMode_Source);
				p.drawPixmap(0, 0, playPixmap);
				p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
				p.fillRect(temp.rect(), QColor(0, 0, 0, alpha*255));
				p.end();
				painter.drawPixmap(0,0,temp);
			}
			// copy second pixmap
			{
				QPixmap temp(iconSize);
				temp.fill(Qt::transparent);
				QPainter p(&temp);
				p.setCompositionMode(QPainter::CompositionMode_Source);
				p.drawPixmap(0, 0, playRedImage);
				p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
				p.fillRect(temp.rect(), QColor(0, 0, 0, (1.-alpha)*255));
				p.end();
				painter.drawPixmap(0,0,temp);
			}
		}
	}
	
	float ThymioVisualProgramming::computeScale(QResizeEvent *event, int desiredToolbarIconSize)
	{
		// FIXME: scale computation should be updated 
		// desired sizes for height
		const int idealContentHeight(6*256);
		const int uncompressibleHeight(
			max(actionsLabel->height(), eventsLabel->height()) +
			desiredToolbarIconSize + 2 * style()->pixelMetric(QStyle::PM_ToolBarFrameWidth) +
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
		const int toolbarWidgetCount(quitButton ? 10 : 9);
		const int toolbarSepCount(quitButton ? 5 : 4);
		// get width of combox box element (not content)
		QStyleOptionComboBox opt;
		QSize tmp(0, 0);
		tmp = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, tmp);
		int desiredIconSize((
			event->size().width() -
			(
				(toolbarWidgetCount-1) * style()->pixelMetric(QStyle::PM_ToolBarItemSpacing) +
				(toolbarWidgetCount-1) * 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth) + 
				2 * style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth) +
				toolbarWidgetCount *  style()->pixelMetric(QStyle::PM_ButtonMargin) + 
				toolbarSepCount * style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent) +
				style()->pixelMetric(QStyle::PM_LayoutLeftMargin) +
				style()->pixelMetric(QStyle::PM_LayoutRightMargin) +
				2 * style()->pixelMetric(QStyle::PM_ToolBarItemMargin) +
				2 * style()->pixelMetric(QStyle::PM_ToolBarFrameWidth) +
				tmp.width() +
				#ifdef Q_WS_MAC
				55 // safety factor, as it seems that metrics do miss some space
				#else // Q_WS_MAC
				20
				#endif // Q_WS_MAC
				//20 // safety factor, as it seems that metrics do miss some space
			)
		) / (toolbarWidgetCount));
		
		// two pass of layout computation, should be a good-enough approximation
		qreal testScale(computeScale(event, desiredIconSize));
		desiredIconSize = qMin(desiredIconSize, int(256.*testScale));
		const qreal scale(computeScale(event, desiredIconSize));
		
		// set toolbar
		const QSize tbIconSize(QSize(desiredIconSize, desiredIconSize));
		regenerateRunButtonAnimation(tbIconSize);
		newButton->setIconSize(tbIconSize);
		openButton->setIconSize(tbIconSize);
		saveButton->setIconSize(tbIconSize);
		saveAsButton->setIconSize(tbIconSize);
		runButton->setIconSize(tbIconSize);
		runButton->setIconSize(tbIconSize);
		stopButton->setIconSize(tbIconSize);
		colorComboButton->setIconSize(tbIconSize);
		advancedButton->setIconSize(tbIconSize);
		helpButton->setIconSize(tbIconSize);
		if (quitButton)
			quitButton->setIconSize(tbIconSize);
		toolBar->setIconSize(tbIconSize);
		
		// set view and cards on sides
		const QSize iconSize(256*scale, 256*scale);
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
