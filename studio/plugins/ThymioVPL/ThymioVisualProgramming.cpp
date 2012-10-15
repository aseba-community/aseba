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

//#include "../../utils/HexFile.h"

#include "ThymioVisualProgramming.h"
#include "ThymioVisualProgramming.moc"
#include "../../TargetModels.h"

using namespace std;

namespace Aseba
{		
	// Visual Programming
	ThymioVisualProgramming::ThymioVisualProgramming(NodeTab* nodeTab):
		InvasivePlugin(nodeTab),
		windowWidth(900),
		windowHeight(800)
	{
		// Create the gui ...
		setWindowTitle(tr("Thymio Visual Programming Language"));

		resize(windowWidth, windowHeight);
		setMinimumSize(QSize(450,400));
		
		mainLayout = new QVBoxLayout(this);
		
		toolBar = new QToolBar();
		toolBar->setMaximumHeight(36);
		toolBar->setIconSize(QSize(24,24));
		toolBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		mainLayout->addWidget(toolBar);

		newButton = new QToolButton();
		openButton = new QToolButton();
		saveButton = new QToolButton();
		saveAsButton = new QToolButton();
		runButton = new QToolButton();
		stopButton = new QToolButton();	
		colorComboButton = new QComboBox();
		advancedButton = new QToolButton();
		quitButton = new QToolButton();		

		newButton->setIcon(QIcon(":/images/filenew.svgz"));
		newButton->setToolTip(tr("New"));
		
		openButton->setIcon(QIcon(":/images/fileopen.svgz"));
		openButton->setToolTip(tr("Open"));		
		
		saveButton->setIcon(QIcon(":/images/save.svgz"));
		saveButton->setToolTip(tr("Save"));

		saveAsButton->setIcon(QIcon(":/images/saveas.svgz"));
		saveAsButton->setToolTip(tr("Save as"));

		runButton->setIcon(QIcon(":/images/play.svgz"));
		runButton->setToolTip(tr("Load & Run"));

		stopButton->setIcon(QIcon(":/images/stop1.png"));
		stopButton->setToolTip(tr("Stop"));
	
		colorComboButton->setToolTip(tr("Color scheme"));
		setColors(colorComboButton);
		//colorComboButton->setIconSize(QSize(64,32));

		advancedButton->setIcon(QIcon(":/images/run.png"));
		advancedButton->setToolTip(tr("Advanced mode"));

		quitButton->setIcon(QIcon(":/images/exit.svgz"));
		quitButton->setToolTip(tr("Quit"));
			
		toolBar->addWidget(newButton);
		toolBar->addWidget(openButton);
		toolBar->addWidget(saveButton);
		toolBar->addWidget(saveAsButton);
		toolBar->addSeparator();
		toolBar->addWidget(runButton);
		toolBar->addWidget(stopButton);
		toolBar->addSeparator();
		toolBar->addWidget(colorComboButton);
		toolBar->addSeparator();
		toolBar->addWidget(advancedButton);
		toolBar->addSeparator();
		toolBar->addWidget(quitButton);
		
		connect(newButton, SIGNAL(clicked()), this, SLOT(newFile()));
		connect(openButton, SIGNAL(clicked()), this, SLOT(openFile()));
		connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
		connect(saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));
		connect(colorComboButton, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorScheme(int)));
		connect(quitButton, SIGNAL(clicked()), this, SLOT(closeFile()));
		connect(runButton, SIGNAL(clicked()), this, SLOT(run()));
		connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
		connect(advancedButton, SIGNAL(clicked()), this, SLOT(advancedMode()));
		
		horizontalLayout = new QHBoxLayout();
		mainLayout->addLayout(horizontalLayout);
		
		// events
		eventsLayout = new QVBoxLayout();

		tapSvg = new QSvgRenderer(QString(":/images/thymiotap.svg"));
		clapSvg = new QSvgRenderer(QString(":/images/thymioclap.svg"));

		ThymioPushButton *buttonsButton = new ThymioPushButton("button");
		ThymioPushButton *proxButton = new ThymioPushButton("prox");
		ThymioPushButton *proxGroundButton = new ThymioPushButton("proxground");
		ThymioPushButton *tapButton = new ThymioPushButton("tap", tapSvg);
		ThymioPushButton *clapButton = new ThymioPushButton("clap", clapSvg);

		eventButtons.push_back(buttonsButton);
		eventButtons.push_back(proxButton);
		eventButtons.push_back(proxGroundButton);		
		eventButtons.push_back(tapButton);
		eventButtons.push_back(clapButton);
		
		eventsLabel = new QLabel(tr("<b>Events</b>"));
		eventsLayout->setAlignment(Qt::AlignTop);
		eventsLayout->setSpacing(10);
		eventsLayout->addWidget(eventsLabel);
		eventsLayout->addWidget(buttonsButton);
		eventsLayout->addWidget(proxButton);
		eventsLayout->addWidget(proxGroundButton);
		eventsLayout->addWidget(tapButton);
		eventsLayout->addWidget(clapButton);
		
		horizontalLayout->addLayout(eventsLayout);
		
		sceneLayout = new QVBoxLayout();

		// compilation
		compilationResultImage = new QLabel();
		compilationResult = new QLabel(tr("Compilation success."));
				
		compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
		compilationResult->setWordWrap(true);
		compilationResult->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

		compilationResultLayout = new QHBoxLayout;
		compilationResultLayout->addWidget(compilationResultImage);  
		compilationResultLayout->addWidget(compilationResult,10000);
		sceneLayout->addLayout(compilationResultLayout);

		// scene		
		scene = new ThymioScene(this);
		view = new QGraphicsView(scene);
		view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);		
		view->setRenderHint(QPainter::Antialiasing);
		view->setAcceptDrops(true);
		view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		view->centerOn(200,0);
		sceneLayout->addWidget(view);

		connect(scene, SIGNAL(stateChanged()), this, SLOT(recompileButtonSet()));

		horizontalLayout->addLayout(sceneLayout);
     
		// actions
		actionsLayout = new QVBoxLayout();

		ThymioPushButton *moveButton = new ThymioPushButton("move");
		ThymioPushButton *colorButton = new ThymioPushButton("color");
		ThymioPushButton *circleButton = new ThymioPushButton("circle");
		ThymioPushButton *soundButton = new ThymioPushButton("sound");
		ThymioPushButton *memoryButton = new ThymioPushButton("memory");
		actionsLabel = new QLabel(tr("<b>Actions</b>"));
								
		actionButtons.push_back(moveButton);
		actionButtons.push_back(colorButton);
		actionButtons.push_back(circleButton);
		actionButtons.push_back(soundButton);
		actionButtons.push_back(memoryButton);
		
		actionsLayout->setAlignment(Qt::AlignTop);
		actionsLayout->setSpacing(10);
		actionsLayout->addWidget(actionsLabel);
		actionsLayout->addWidget(moveButton);
		actionsLayout->addWidget(colorButton);
		actionsLayout->addWidget(circleButton);
		actionsLayout->addWidget(soundButton);
		actionsLayout->addWidget(memoryButton);
		
		memoryButton->hide(); // memory

		horizontalLayout->addLayout(actionsLayout);
		
		// make connections	
		connect(buttonsButton, SIGNAL(clicked()),this,SLOT(addButtonsEvent()));
		connect(proxButton, SIGNAL(clicked()), this, SLOT(addProxEvent()));
		connect(proxGroundButton, SIGNAL(clicked()), this, SLOT(addProxGroundEvent()));
		connect(tapButton, SIGNAL(clicked()), this, SLOT(addTapEvent()));
		connect(clapButton, SIGNAL(clicked()), this, SLOT(addClapEvent()));

		connect(moveButton, SIGNAL(clicked()), this, SLOT(addMoveAction()));
		connect(colorButton, SIGNAL(clicked()), this, SLOT(addColorAction()));	
		connect(circleButton, SIGNAL(clicked()), this, SLOT(addCircleAction()));
		connect(soundButton, SIGNAL(clicked()), this, SLOT(addSoundAction()));
		connect(memoryButton, SIGNAL(clicked()), this, SLOT(addMemoryAction()));
		
		QRect clientRect =  QApplication::desktop()->availableGeometry();

		if( clientRect.width() < (windowWidth+100) || clientRect.height() < (windowHeight+100) )		
			resize(clientRect.width()-100, clientRect.height()-100);
	}
	
	ThymioVisualProgramming::~ThymioVisualProgramming()
	{
		delete(tapSvg);
		delete(clapSvg);
	}
	
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
		QPushButton *vplButton = new QPushButton(tr("Launch VPL"));
		connect(vplButton, SIGNAL(clicked()), SLOT(showVPL()));
		return vplButton;
	}
	
	void ThymioVisualProgramming::closeAsSoonAsPossible()
	{
		advancedButton->setEnabled(true);
		actionButtons.last()->hide(); // state button
//		scene->clear();
		scene->reset();
		close();
	}

	void ThymioVisualProgramming::showVPL()
	{
		if (InvasivePlugin::newFile())
			exec();
	}
	
	void ThymioVisualProgramming::newFile()
	{
		if( !scene->isEmpty() && warningDialog() ) 
		{
			bool advanced = scene->getAdvanced();
			scene->reset();
			thymioFilename.clear();
			scene->setAdvanced(advanced);		
		}
	}

	void ThymioVisualProgramming::openFile()
	{
		if( scene->isEmpty() || warningDialog() ) 
			InvasivePlugin::openFile();
	}
	
	bool ThymioVisualProgramming::save()
	{
		return InvasivePlugin::saveFile(false);
	}
	
	bool ThymioVisualProgramming::saveAs()
	{
		return InvasivePlugin::saveFile(true);
	}

	void ThymioVisualProgramming::closeFile()
	{
		if( scene->isEmpty() || warningDialog() ) 
		{
			advancedButton->setEnabled(true);
			actionButtons.last()->hide(); // state button
			scene->reset();
			close();
		}
	}
	
	void ThymioVisualProgramming::setColorScheme(int index)
	{
		scene->setColorScheme(eventColors.at(index), actionColors.at(index));
		
		for(QList<ThymioPushButton*>::iterator itr = eventButtons.begin();
			itr != eventButtons.end(); ++itr)
			(*itr)->changeButtonColor(eventColors.at(index));

		for(QList<ThymioPushButton*>::iterator itr = actionButtons.begin();
			itr != actionButtons.end(); ++itr)
			(*itr)->changeButtonColor(actionColors.at(index));
	}

	void ThymioVisualProgramming::run()
	{
		if(runButton->isEnabled())
			loadNrun();
	}

	void ThymioVisualProgramming::stop()
	{
		InvasivePlugin::stop();
		const unsigned leftSpeedVarPos = getVariablesModel()->getVariablePos("motor.left.target");
		setVariableValues(leftSpeedVarPos, VariablesDataVector(1, 0));
		const unsigned rightSpeedVarPos = getVariablesModel()->getVariablePos("motor.right.target");
		setVariableValues(rightSpeedVarPos, VariablesDataVector(1, 0));
	}
	
	void ThymioVisualProgramming::advancedMode()
	{
		advancedButton->setEnabled(false);
		actionButtons.last()->show(); // state button
		scene->setAdvanced(true);
	}
	
	void ThymioVisualProgramming::closeEvent ( QCloseEvent * event )
	{
		if ( scene->isEmpty() || warningDialog() )
		{
			advancedButton->setEnabled(true);
			actionButtons.last()->hide(); // state button
//			scene->clear();
			scene->reset();
			close();
		}
		else
			event->ignore();
	}
	
	bool ThymioVisualProgramming::warningDialog()
	{
		if(!scene->isModified())
			return true;
		
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Warning"));
		msgBox.setText(tr("The VPL document has been modified."));
		msgBox.setInformativeText(tr("Do you want to save the changes?"));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Save);
		
		int ret = msgBox.exec();
		switch (ret)
		{
			case QMessageBox::Save:
				if (save())
					return true;
				else
					return false;
			case QMessageBox::Discard:
				return true;
			case QMessageBox::Cancel:
			default:
				return false;
		}

		return false;
	}

	QDomDocument ThymioVisualProgramming::saveToDom() const 
	{ 
		QDomDocument document("tool-plugin-data");
		
		QDomElement vplroot = document.createElement("vplroot");
		document.appendChild(vplroot);

		QDomElement settings = document.createElement("settings");
		settings.setAttribute("advanced-mode", scene->getAdvanced() ? "true" : "false");
		settings.setAttribute("color-scheme", QString::number(colorComboButton->currentIndex()));
		vplroot.appendChild(settings);
		vplroot.appendChild(document.createTextNode("\n\n"));		
		
		for (ThymioScene::ButtonSetItr itr = scene->buttonsBegin(); 
			  itr != scene->buttonsEnd(); ++itr )
		{
			QDomElement element = document.createElement("buttonset");			
			
			if( (*itr)->eventExists() ) 
			{
				ThymioButton *button = (*itr)->getEventButton();
				element.setAttribute("event-name", button->getName() );
			
				for(int i=0; i<button->getNumButtons(); ++i)
					element.setAttribute( QString("eb%0").arg(i), button->isClicked(i));					
				element.setAttribute("state", button->getState());
			}
			
			if( (*itr)->actionExists() ) 
			{
				ThymioButton *button = (*itr)->getActionButton();
				element.setAttribute("action-name", button->getName() );
				for(int i=0; i<button->getNumButtons(); ++i)					
					element.setAttribute(QString("ab%0").arg(i), button->isClicked(i));
			}
			
			vplroot.appendChild(element);
			vplroot.appendChild(document.createTextNode("\n\n"));
		}

		scene->setModified(false);

		return document;
	}

	void ThymioVisualProgramming::loadFromDom(const QDomDocument& document, bool fromFile) 
	{
		scene->clear();

		QDomNode domNode = document.documentElement().firstChild();

		while (!domNode.isNull())
		{
			if (domNode.isElement())
			{
				QDomElement element = domNode.toElement();
				if (element.tagName() == "settings") 
				{
					if( element.attribute("advanced-mode") == "true" )
						advancedMode();
					else
					{
						advancedButton->setEnabled(true);
						actionButtons.last()->hide(); // state button
						scene->setAdvanced(false);
					}
					
					colorComboButton->setCurrentIndex(element.attribute("color-scheme").toInt());
				}
				else if(element.tagName() == "buttonset")
				{
					QString buttonName;
					ThymioButton *eventButton = 0;
					ThymioButton *actionButton = 0;
					
					if( !(buttonName = element.attribute("event-name")).isEmpty() )
					{
						
						if ( buttonName == "button" )
							eventButton = new ThymioButtonsEvent(0,scene->getAdvanced());
						else if ( buttonName == "prox" )
							eventButton = new ThymioProxEvent(0,scene->getAdvanced());
						else if ( buttonName == "proxground" )
							eventButton = new ThymioProxGroundEvent(0,scene->getAdvanced());
						else if ( buttonName == "tap" )
						{					
							eventButton = new ThymioTapEvent(0,scene->getAdvanced());
							eventButton->setSharedRenderer(tapSvg);
						}
						else if ( buttonName == "clap" )
						{
							eventButton = new ThymioClapEvent(0,scene->getAdvanced());
							eventButton->setSharedRenderer(clapSvg);
						}
						else
						{
							QMessageBox::warning(this,tr("Loading"),
												 tr("Error in XML source file: %0 unknown event type").arg(buttonName));
							return;
						}

						for(int i=0; i<eventButton->getNumButtons(); ++i)
							eventButton->setClicked(i,element.attribute(QString("eb%0").arg(i)).toInt());

						eventButton->setState(element.attribute("state").toInt());
					}
					
					if( !(buttonName = element.attribute("action-name")).isEmpty() )
					{
						if ( buttonName == "move" )
							actionButton = new ThymioMoveAction();
						else if ( buttonName == "color" )
							actionButton = new ThymioColorAction();
						else if ( buttonName == "circle" )
							actionButton = new ThymioCircleAction();
						else if ( buttonName == "sound" )
							actionButton = new ThymioSoundAction();
						else if ( buttonName == "memory" )
							actionButton = new ThymioMemoryAction();
						else
						{
							QMessageBox::warning(this,tr("Loading"),
												 tr("Error in XML source file: %0 unknown event type").arg(buttonName));
							return;
						}

						for(int i=0; i<actionButton->getNumButtons(); ++i)
							actionButton->setClicked(i,element.attribute(QString("ab%0").arg(i)).toInt());
					}

					scene->addButtonSet(eventButton, actionButton);
				}
			}
			domNode = domNode.nextSibling();
		}
		
		scene->setModified(!fromFile);
		
		if (!scene->isEmpty())
			QTimer::singleShot(0, this, SLOT(exec()));
	}

	void ThymioVisualProgramming::recompileButtonSet()
	{
		compilationResult->setText(scene->getErrorMessage());

		if( scene->isSuccessful() ) 
		{
			compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
			displayCode(scene->getCode(), scene->getFocusItemId());
			runButton->setEnabled(true);
		}
		else
		{
			compilationResultImage->setPixmap(QPixmap(QString(":/images/warning.png")));
			runButton->setEnabled(false);
		}
	}

	void ThymioVisualProgramming::addButtonsEvent()
	{
		ThymioButtonsEvent *button = new ThymioButtonsEvent(0, scene->getAdvanced());
		scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}

	void ThymioVisualProgramming::addProxEvent()
	{
		ThymioProxEvent *button = new ThymioProxEvent(0, scene->getAdvanced());
		scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}	

	void ThymioVisualProgramming::addProxGroundEvent()
	{
		ThymioProxGroundEvent *button = new ThymioProxGroundEvent(0, scene->getAdvanced());
		scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}	
	
	void ThymioVisualProgramming::addTapEvent()
	{
		ThymioTapEvent *button = new ThymioTapEvent(0, scene->getAdvanced());
		button->setSharedRenderer(tapSvg);
		scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}
	
	void ThymioVisualProgramming::addClapEvent()
	{
		ThymioClapEvent *button = new ThymioClapEvent(0, scene->getAdvanced());
		button->setSharedRenderer(clapSvg);
		scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}
	
	void ThymioVisualProgramming::addMoveAction()
	{
		ThymioMoveAction *button = new ThymioMoveAction();
		scene->setFocus();
		view->centerOn(scene->addAction(button));
	}
	
	void ThymioVisualProgramming::addColorAction()
	{
		ThymioColorAction *button = new ThymioColorAction();
		scene->setFocus();
		view->centerOn(scene->addAction(button));
	}

	void ThymioVisualProgramming::addCircleAction()
	{
		ThymioCircleAction *button = new ThymioCircleAction();
		scene->setFocus();
		view->centerOn(scene->addAction(button));
	}

	void ThymioVisualProgramming::addSoundAction()
	{
		ThymioSoundAction *button = new ThymioSoundAction();
		scene->setFocus();
		view->centerOn(scene->addAction(button));
	}

	void ThymioVisualProgramming::addMemoryAction()
	{
		ThymioMemoryAction *button = new ThymioMemoryAction();
		scene->setFocus();
		view->centerOn(scene->addAction(button));
	}
		
	void ThymioVisualProgramming::resizeEvent( QResizeEvent *event)
	{
		QSize iconSize;
		QSize deltaSize = (event->size() - QSize(windowWidth, windowHeight));
		double scaleH = 1 + 1.2*(double)deltaSize.height()/windowHeight;
		double scaleV = 1 + 1.2*(double)deltaSize.width()/windowWidth;
		double scale = (scaleH < scaleV ? (scaleH > 1.95 ? 1.95 : scaleH) :
 										  (scaleV > 1.95 ? 1.95 : scaleV) );

		iconSize = QSize(128*scale, 128*scale);
		scene->setScale(scale);
		
		for(QList<ThymioPushButton*>::iterator itr = eventButtons.begin();
			itr != eventButtons.end(); ++itr)
			(*itr)->setIconSize(iconSize);

		for(QList<ThymioPushButton*>::iterator itr = actionButtons.begin();
			itr != actionButtons.end(); ++itr)
			(*itr)->setIconSize(iconSize);
	}

};
