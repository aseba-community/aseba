#include <QGraphicsSceneMouseEvent>

#include "ThymioButtons.h"

#include "ThymioScene.h"
#include "ThymioScene.moc"

using namespace std;

namespace Aseba
{		

	ThymioScene::ThymioScene(QObject *parent) : 
		QGraphicsScene(parent),
		prevNewActionButton(false),
		prevNewEventButton(false),
		lastFocus(-1),
		eventButtonColor(QColor(0,191,255)),
		actionButtonColor(QColor(218,112,214)),
		sceneModified(false),
		scaleFactor(0.5),
		thymioCompiler(),
		newRow(false)
	{
		ThymioButtonSet *button = new ThymioButtonSet(0);
		button->setScale(scaleFactor);
		button->setPos(20, 20);
		buttonSets.push_back(button);
		thymioCompiler.AddButtonSet(button->getIRButtonSet());		
		
		buttonSetHeight = button->boundingRect().height();
		
		addItem(button);
		connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
	}
	
	ThymioScene::~ThymioScene()
	{	
		clear();
	}

	QGraphicsItem *ThymioScene::addAction(ThymioButton *item) 
	{
		ThymioButtonSet *button = 0; 
		prevNewActionButton = false;
		
		if( newRow )
		{
			button = new ThymioButtonSet(buttonSets.size());
			button->setColorScheme(eventButtonColor, actionButtonColor);
			button->setScale(scaleFactor);
			buttonSets.push_back(button);
		
			addItem(button);
			prevNewEventButton = true;
			
			connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
			thymioCompiler.AddButtonSet(button->getIRButtonSet());				
		}
		else if( prevNewEventButton )
		{
			button = buttonSets.last();
		}
		else 
		{
			if( lastFocus >= 0 )
				button = buttonSets.at(lastFocus);
			else
				for(int i=0; i<buttonSets.size(); ++i)
					if( buttonSets.at(i)->hasFocus() )
					{
						button = buttonSets.at(i);
						break;
					}
			
			if( !button )
			{
				if(!buttonSets.isEmpty() && !(buttonSets.last()->actionExists()) )
					button = buttonSets.last();
				else
				{
					button = new ThymioButtonSet(buttonSets.size());
					button->setColorScheme(eventButtonColor, actionButtonColor);
					button->setScale(scaleFactor);					
					buttonSets.push_back(button);
				
					addItem(button);
					prevNewActionButton = true;
					
					connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
					thymioCompiler.AddButtonSet(button->getIRButtonSet());						
				}
			}

				
		}
		
		if( item ) button->addActionButton(item);
		prevNewEventButton = false;
		newRow = false;
	
		if( button->eventExists() ) 
		{
			lastFocus = -1;
			setFocusItem(0);
		}

		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));

		sceneModified = true;
		return button;
	}

	QGraphicsItem *ThymioScene::addEvent(ThymioButton *item) 
	{ 
		ThymioButtonSet *button = 0; 
		prevNewEventButton = false;

		if( newRow )
		{
			button = new ThymioButtonSet(buttonSets.size());
			button->setColorScheme(eventButtonColor, actionButtonColor);
			button->setScale(scaleFactor);
			buttonSets.push_back(button);
		
			addItem(button);
			prevNewEventButton = true;
			
			connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
			thymioCompiler.AddButtonSet(button->getIRButtonSet());				
		}
		else if( prevNewActionButton )
		{
			button = buttonSets.last();
		}
		else 
		{
			if( lastFocus >= 0 )
				button = buttonSets.at(lastFocus);
			else
				for(int i=0; i<buttonSets.size(); ++i)
					if( buttonSets.at(i)->hasFocus() )
					{
						button = buttonSets.at(i);
						break;
					}
			
			if( !button )
			{
				if(!buttonSets.isEmpty() && !(buttonSets.last()->eventExists()) )
					button = buttonSets.last();
				else
				{
					button = new ThymioButtonSet(buttonSets.size());
					button->setColorScheme(eventButtonColor, actionButtonColor);
					button->setScale(scaleFactor);
					buttonSets.push_back(button);
				
					addItem(button);
					prevNewEventButton = true;
					
					connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
					thymioCompiler.AddButtonSet(button->getIRButtonSet());								
				}
			}

		}
		
		if( item ) button->addEventButton(item);
		prevNewActionButton = false;
		newRow = false;
				
		if( button->actionExists() ) 
		{
			lastFocus = -1;
			setFocusItem(0);
		}
		
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));		
		
		sceneModified = true;		
		return button;
	}

	bool ThymioScene::isEmpty()
	{
		if( buttonSets.isEmpty() )
			return true;
			
		if( buttonSets.size() == 1 &&
			buttonSets[0]->actionExists() &&
			buttonSets[0]->eventExists() )
			return true;

		return false;
	}

	void ThymioScene::reset()
	{
		clear();

		ThymioButtonSet *button = new ThymioButtonSet(0);
		button->setColorScheme(eventButtonColor, actionButtonColor);		
		button->setScale(scaleFactor);
		button->setPos(20, 20);
		buttonSets.push_back(button);
	
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));
	
		addItem(button);
		thymioCompiler.AddButtonSet(button->getIRButtonSet());
		connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
	}
	
	void ThymioScene::clear()
	{
		for(int i=0; i<buttonSets.size(); i++)
		{
			disconnect(buttonSets.at(i), SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
			removeItem(buttonSets.at(i));
			delete(buttonSets.at(i));
		}
		buttonSets.clear();
		thymioCompiler.clear();
		
		sceneModified = false;
		
		lastFocus = -1;
		prevNewActionButton = false;		
		prevNewEventButton = false;
	}
	
	void ThymioScene::setColorScheme(QColor eventColor, QColor actionColor)
	{
		eventButtonColor = eventColor;
		actionButtonColor = actionColor;

		for(ButtonSetItr itr = buttonsBegin(); itr != buttonsEnd(); ++itr)
			(*itr)->setColorScheme(eventColor, actionColor);					

		update();
	
		sceneModified = true;
	}

	void ThymioScene::removeButton(int row)
	{
		
		Q_ASSERT( row < buttonSets.size() );
			
		ThymioButtonSet *button = buttonSets[row];
		disconnect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
		buttonSets.removeAt(row);		
		thymioCompiler.RemoveButtonSet(row);
				
		removeItem(button);
		delete(button);
		
		rearrangeButtons(row);

		if( buttonSets.isEmpty() ) 
		{
			ThymioButtonSet *button = new ThymioButtonSet(0);
			button->setColorScheme(eventButtonColor, actionButtonColor);
			button->setScale(scaleFactor);
			button->setPos(20, 20);
			buttonSets.push_back(button);
		
			addItem(button);
			connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));			
		}

		prevNewEventButton = false;
		prevNewActionButton = false;
		lastFocus = -1;
	
		sceneModified = true;
		
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));
		
		buttonUpdateDetected();
	}
	
	void ThymioScene::insertButton(int row) 
	{			
		Q_ASSERT( row <= buttonSets.size() );

		ThymioButtonSet *button = new ThymioButtonSet(row);
		button->setColorScheme(eventButtonColor, actionButtonColor);
		button->setScale(scaleFactor);
		buttonSets.insert(row, button);
		addItem(button);
		
		thymioCompiler.InsertButtonSet(row, button->getIRButtonSet());
		
		connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
			
		setFocusItem(button);
		lastFocus = row;
			
		rearrangeButtons(row+1);

		prevNewEventButton = false;
		prevNewActionButton = false;
	
		sceneModified = true;
		
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));		
	}

	void ThymioScene::rearrangeButtons(int row)
	{
		for(int i=row; i<buttonSets.size(); ++i) 
			buttonSets.at(i)->setRow(i);
	}
	
	void ThymioScene::setScale(qreal scale) 
	{ 
		scaleFactor = scale*0.5;
		for( ButtonSetItr itr = buttonsBegin(); itr != buttonsEnd(); ++itr )
			(*itr)->setScale(scaleFactor);
			
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));			
	}	
	
	QString ThymioScene::getErrorMessage() 
	{ 
		switch(thymioCompiler.getErrorCode()) {
		case THYMIO_MISSING_EVENT:
			return tr("Missing event button");
			break;
		case THYMIO_MISSING_ACTION:
			return tr("Missing action button");
			break;
		case THYMIO_EVENT_NOT_SET:
			return tr("Event button is not set.");
			break;
		case THYMIO_EVENT_MULTISET:
			return tr("Redeclaration of event button.");
			break;
		case THYMIO_INVALID_CODE:
			return tr("Unknown button type.");
			break;
		case THYMIO_NO_ERROR:
			return tr("Compilation success.");
		default:
			break;
		}
		
		return "";
	}	
	
	QList<QString> ThymioScene::getCode()
	{
		QList<QString> out;
		
		for(std::vector<std::wstring>::const_iterator itr = thymioCompiler.beginCode();
			itr != thymioCompiler.endCode(); ++itr)
			out.push_back(QString::fromStdWString(*itr) + "\n");
	
		return out;
	}
	
	void ThymioScene::buttonUpdateDetected()
	{
		thymioCompiler.compile();
		thymioCompiler.generateCode();
		emit stateChanged();
	}

	void ThymioScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if( event->mimeData()->hasFormat("thymiobutton") && 
			((event->scenePos().y()-20)/(buttonSetHeight*scaleFactor)) >= buttonSets.size() )
		{
			setFocusItem(0);
			event->accept();		
		}
		else
			QGraphicsScene::dragMoveEvent(event);
	}
	
	void ThymioScene::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("thymiobuttonset") )
		{
			QByteArray buttonData = event->mimeData()->data("thymiobuttonset");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);

			int prevRow, currentRow;
			dataStream >> prevRow;
			
			ThymioButtonSet *button = buttonSets.at(prevRow);
			buttonSets.removeAt(prevRow);	

			thymioCompiler.RemoveButtonSet(prevRow);
			
			qreal currentYPos = event->scenePos().y();	
			currentRow = (int)((currentYPos-20)/(buttonSetHeight*scaleFactor));
			currentRow = (currentRow < 0 ? 0 :
			               (currentRow < buttonSets.size() ? currentRow : buttonSets.size()) );
			buttonSets.insert(currentRow, button);
			thymioCompiler.InsertButtonSet(currentRow, button->getIRButtonSet());
			
			rearrangeButtons( prevRow < currentRow ? prevRow : currentRow );
			
			event->setDropAction(Qt::MoveAction);
			event->accept();

			setFocusItem(button);
			lastFocus = currentRow;
	
			sceneModified = true;
		}
		else if( event->mimeData()->hasFormat("thymiobutton") && 
				((event->scenePos().y()-20)/(buttonSetHeight*scaleFactor)) >= buttonSets.size() )
		{
			QByteArray buttonData = event->mimeData()->data("thymiobutton");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);
			
			int parentID;		
			dataStream >> parentID;

			QString buttonName;
			int numButtons;
			dataStream >> buttonName >> numButtons;
			
			ThymioButton *button = 0;
			bool eventButtonFlag = true;

			if ( buttonName == "button" )
				button = new ThymioButtonsEvent();
			else if ( buttonName == "prox" )
				button = new ThymioProxEvent();
			else if ( buttonName == "proxground" )
				button = new ThymioProxGroundEvent();
			else if ( buttonName == "tap" )
			{
				button = new ThymioTapEvent();
				QByteArray rendererData = event->mimeData()->data("thymiorenderer");
				QDataStream rendererStream(&rendererData, QIODevice::ReadOnly);
				quint64 location;
				rendererStream >> location;
				QSvgRenderer *tapSvg;
				tapSvg = (QSvgRenderer*)location;
				button->setSharedRenderer(tapSvg);
			}
			else if ( buttonName == "clap" )
			{
				button = new ThymioClapEvent();
				QByteArray rendererData = event->mimeData()->data("thymiorenderer");
				QDataStream rendererStream(&rendererData, QIODevice::ReadOnly);
				quint64 location;
				rendererStream >> location;
				QSvgRenderer *clapSvg;
				clapSvg = (QSvgRenderer*)location;
				button->setSharedRenderer(clapSvg);
			}
			else if ( buttonName == "move" )
				button = new ThymioMoveAction();
			else if ( buttonName == "color" )
				button = new ThymioColorAction();
			else if ( buttonName == "circle" )
				button = new ThymioCircleAction();
			else if ( buttonName == "sound" )			
				button = new ThymioSoundAction();
							
			event->setDropAction(Qt::MoveAction);
			event->accept();
			
			lastFocus = -1;
			prevNewActionButton = false;
			prevNewEventButton = false;
			newRow = true;
			
			if( event->mimeData()->data("thymiotype") == QString("event").toLatin1() )
				addEvent(button);
			else if( event->mimeData()->data("thymiotype") == QString("action").toLatin1() )
				addAction(button);
		}
		else
			QGraphicsScene::dropEvent(event);
	}

	void ThymioScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
	{		
		QGraphicsScene::mousePressEvent(event);
	}
	
	void ThymioScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
	{
		QGraphicsScene::mouseReleaseEvent(event);
		
		if( event->button() == Qt::LeftButton ) 
		{
			QGraphicsItem *item = focusItem();
			if( item )
			{
				if( item->data(0) == "remove" ) 				
					removeButton((item->parentItem()->data(1)).toInt());
				else if( item->data(0) == "add" )
					insertButton((item->parentItem()->data(1)).toInt()+1);					
			}
		}
		
		update();
	}

};
