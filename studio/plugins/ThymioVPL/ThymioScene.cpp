#include <QGraphicsSceneMouseEvent>
#include <QDebug>

#include "ThymioButtons.h"

#include "ThymioScene.h"
#include "ThymioScene.moc"

using namespace std;

namespace Aseba
{		

	ThymioScene::ThymioScene(QObject *parent) : 
		QGraphicsScene(parent),
		prevNewEventButton(false),
		prevNewActionButton(false),
		lastFocus(-1),
		thymioCompiler(),
		eventButtonColor(QColor(0,191,255)),
		actionButtonColor(QColor(218,112,214)),
		sceneModified(false),
		scaleFactor(0.5),
		thymioCompiler(),
		newRow(false),
		advancedMode(false)
	{
		ThymioButtonSet *button = new ThymioButtonSet(0, advancedMode);
		button->setScale(scaleFactor);
		button->setPos(20, 20);
		buttonSets.push_back(button);
		thymioCompiler.addButtonSet(button->getIRButtonSet());		
		
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
		//qDebug() << "  ThymioScene -- add action : focus " << lastFocus;
				
		ThymioButtonSet *button = 0; 
		prevNewActionButton = false;
		
		if( newRow )
			button = createNewButtonSet();
		else if( prevNewEventButton )
			button = buttonSets.last();
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
					button = createNewButtonSet();
					prevNewActionButton = true;
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

		//qDebug() << "    == TS -- add action : done -- focus " << lastFocus;
		
		return button;
	}

	QGraphicsItem *ThymioScene::addEvent(ThymioButton *item) 
	{ 
		//qDebug() << "  ThymioScene -- add event : focus " << lastFocus;
				
		ThymioButtonSet *button = 0; 
		prevNewEventButton = false;

		if( newRow )
			button = createNewButtonSet();		
		else if( prevNewActionButton )		
			button = buttonSets.last();
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
					button = createNewButtonSet();
					prevNewEventButton = true;
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
		
		//qDebug() << "    == TS -- add event : done -- focus " << lastFocus;
		
		return button;
	}

	ThymioButtonSet *ThymioScene::createNewButtonSet()
	{
		//qDebug() << "ThymioScene -- create new button set " << buttonSets.size();
		
		ThymioButtonSet *button = new ThymioButtonSet(buttonSets.size(), advancedMode);
		button->setColorScheme(eventButtonColor, actionButtonColor);
		button->setScale(scaleFactor);
		buttonSets.push_back(button);
		
		addItem(button);

		connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
		thymioCompiler.addButtonSet(button->getIRButtonSet());

		//qDebug() << "  == TS -- create new button set : done";

		return button;		
	}

	bool ThymioScene::isEmpty() const
	{
		if( buttonSets.isEmpty() )
			return true;
			
		if( buttonSets.size() > 1 ||
			buttonSets[0]->actionExists() ||
			buttonSets[0]->eventExists() )
			return false;

		return true;
	}

	void ThymioScene::reset()
	{
		//qDebug() << "ThymioScene -- reset " << buttonSets.size();		
				
		clear();

		ThymioButtonSet *button = createNewButtonSet();
		button->setPos(20, 20);
		
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));

		//qDebug() << "  == TS -- reset : done";
	}
	
	void ThymioScene::clear()
	{
		//qDebug() << "ThymioScene -- clear " << buttonSets.size();

		advancedMode = false;
		for(ButtonSetItr itr = buttonsBegin(); itr != buttonsEnd(); ++itr)
			(*itr)->setAdvanced(false);	
		
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

		//qDebug() << "  == TS -- clear : done";		
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

	void ThymioScene::setAdvanced(bool advanced)
	{
		advancedMode = advanced;
		for(ButtonSetItr itr = buttonsBegin(); itr != buttonsEnd(); ++itr)
			(*itr)->setAdvanced(advanced);
		
		buttonUpdateDetected();
	}

	void ThymioScene::removeButton(int row)
	{
		//qDebug() << "ThymioScene -- remove button " << row << ", " << buttonSets.size();
		
		Q_ASSERT( row < buttonSets.size() );
			
		ThymioButtonSet *button = buttonSets[row];
		disconnect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
		buttonSets.removeAt(row);		
		thymioCompiler.removeButtonSet(row);
				
		removeItem(button);
		delete(button);
		
		rearrangeButtons(row);

		if( buttonSets.isEmpty() ) 
		{
			ThymioButtonSet *button = createNewButtonSet();
			button->setPos(20,20);		
		}

		prevNewEventButton = false;
		prevNewActionButton = false;
		lastFocus = -1;
	
		sceneModified = true;
		
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));
		
		buttonUpdateDetected();
		
		//qDebug() << "  == TS -- remove button : done";
	}
	
	void ThymioScene::insertButton(int row) 
	{
		//qDebug() << "ThymioScene -- insert button " << row << ", " << buttonSets.size();		
		Q_ASSERT( row <= buttonSets.size() );

		ThymioButtonSet *button = new ThymioButtonSet(row, advancedMode);
		button->setColorScheme(eventButtonColor, actionButtonColor);
		button->setScale(scaleFactor);
		buttonSets.insert(row, button);
		addItem(button);
		
		thymioCompiler.insertButtonSet(row, button->getIRButtonSet());
		
		connect(button, SIGNAL(buttonUpdated()), this, SLOT(buttonUpdateDetected()));
			
		setFocusItem(button);
		lastFocus = row;
			
		rearrangeButtons(row+1);

		prevNewEventButton = false;
		prevNewActionButton = false;
	
		sceneModified = true;
		
		setSceneRect(QRectF(0, 0, 1000*scaleFactor+40, (buttonSets.size()+2)*400*scaleFactor));		

		buttonUpdateDetected();
		
		//qDebug() << "  == TS -- insert button : done";
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
	
	QString ThymioScene::getErrorMessage() const
	{ 
		switch(thymioCompiler.getErrorCode()) {
		case THYMIO_MISSING_EVENT:
			return tr("Line %0: Missing event").arg(thymioCompiler.getErrorLine());
			break;
		case THYMIO_MISSING_ACTION:
			return tr("Line %0: Missing action").arg(thymioCompiler.getErrorLine());
			break;
		case THYMIO_EVENT_MULTISET:
			return tr("Line %0: Redeclaration of event").arg(thymioCompiler.getErrorLine());
			break;
		case THYMIO_INVALID_CODE:
			return tr("Line %0: Unknown event/action type").arg(thymioCompiler.getErrorLine());
			break;
		case THYMIO_NO_ERROR:
			return tr("Compilation success");
		default:
			return tr("Unknown VPL error");
			break;
		}
	}	
	
	QList<QString> ThymioScene::getCode() const
	{
		QList<QString> out;
		
		for(std::vector<std::wstring>::const_iterator itr = thymioCompiler.beginCode();
			itr != thymioCompiler.endCode(); ++itr)
			out.push_back(QString::fromStdWString(*itr) + "\n");
	
		return out;
	}
	
	void ThymioScene::buttonUpdateDetected()
	{
		sceneModified = true;
		
		thymioCompiler.compile();
		thymioCompiler.generateCode();
		
		for(int i=0; i<buttonSets.size(); i++)
			buttonSets[i]->setErrorStatus(false);
						
		if( !thymioCompiler.isSuccessful() )
			buttonSets[thymioCompiler.getErrorLine()]->setErrorStatus(true);
				
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
			//qDebug() << "ThymioScene -- button set dropped";
			
			QByteArray buttonData = event->mimeData()->data("thymiobuttonset");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);

			int prevRow, currentRow;
			dataStream >> prevRow;
			
			ThymioButtonSet *button = buttonSets.at(prevRow);		
			buttonSets.removeAt(prevRow);
			thymioCompiler.removeButtonSet(prevRow);

			qreal currentYPos = event->scenePos().y();	
			currentRow = (int)((currentYPos-20)/(buttonSetHeight*scaleFactor));
			currentRow = (currentRow < 0 ? 0 : currentRow > buttonSets.size() ? buttonSets.size() : currentRow);

			buttonSets.insert(currentRow, button);
			thymioCompiler.insertButtonSet(currentRow, button->getIRButtonSet());			

			rearrangeButtons( prevRow < currentRow ? prevRow : currentRow );
			
			event->setDropAction(Qt::MoveAction);
			event->accept();

			setFocusItem(button);
			lastFocus = currentRow;
			
			buttonUpdateDetected();
			
			//qDebug() << "  == done";
		}
		else if( event->mimeData()->hasFormat("thymiobutton") && 
				((event->scenePos().y()-20)/(buttonSetHeight*scaleFactor)) >= buttonSets.size() )
		{
			//qDebug() << "ThymioScene -- button dropped : a new button set created";
						
			QByteArray buttonData = event->mimeData()->data("thymiobutton");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);
			
			int parentID;		
			dataStream >> parentID;

			QString buttonName;
			int numButtons;
			dataStream >> buttonName >> numButtons;
			
			ThymioButton *button = 0;

			if ( buttonName == "button" )
				button = new ThymioButtonsEvent(0, advancedMode);
			else if ( buttonName == "prox" )
				button = new ThymioProxEvent(0, advancedMode);
			else if ( buttonName == "proxground" )
				button = new ThymioProxGroundEvent(0, advancedMode);
			else if ( buttonName == "tap" )
			{
				button = new ThymioTapEvent(0, advancedMode);
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
				button = new ThymioClapEvent(0, advancedMode);
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
			else if ( buttonName == "memory" )
				button = new ThymioMemoryAction();

			if( button ) 
			{			
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

				for( int i=0; i<numButtons; ++i ) 
				{
					int status;
					dataStream >> status;	
					button->setClicked(i, status);
				}

			}
			//qDebug() << "  == done";
		}
		else
		{
			//qDebug() << "ThymioScene -- button dropped to an existing button set";
						
			QGraphicsScene::dropEvent(event);

			//qDebug() << "  == done";
		}
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
				else if( item->data(0) == "buttonset" )
					lastFocus = item->data(1).toInt();
			}
		}
		
		update();
	}

};
