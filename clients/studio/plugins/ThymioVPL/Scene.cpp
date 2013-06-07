#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QDebug>

#include "Scene.h"
#include "Card.h"

using namespace std;

namespace Aseba { namespace ThymioVPL
{

	Scene::Scene(QObject *parent) : 
		QGraphicsScene(parent),
		prevNewEventButton(false),
		prevNewActionButton(false),
		lastFocus(-1),
		compiler(*this),
		eventCardColor(QColor(0,191,255)),
		actionCardColor(QColor(218,112,214)),
		sceneModified(false),
		newRow(false),
		advancedMode(false)
	{
		// create initial button
		EventActionPair *button = createNewEventActionPair();
		buttonSetHeight = button->boundingRect().height();
		
		connect(this, SIGNAL(selectionChanged()), this, SLOT(recompile()));
	}
	
	Scene::~Scene()
	{
		clear();
	}

	QGraphicsItem *Scene::addAction(Card *item) 
	{
		// FIXME: redo this
		EventActionPair *button = 0; 
		prevNewActionButton = false;
		
		if( newRow )
			button = createNewEventActionPair();
		else if( prevNewEventButton )
			button = eventActionPairs.last();
		else 
		{
			if( lastFocus >= 0 )
				button = eventActionPairs.at(lastFocus);
			else
				for(int i=0; i<eventActionPairs.size(); ++i)
					if( eventActionPairs.at(i)->hasFocus() )
					{
						button = eventActionPairs.at(i);
						break;
					}
			
			if( !button )
			{
				if(!eventActionPairs.isEmpty() && !(eventActionPairs.last()->hasActionCard()) )
					button = eventActionPairs.last();
				else
				{
					button = createNewEventActionPair();
					prevNewActionButton = true;
				}
			}
		}
		
		if( item ) button->addActionCard(item);
		prevNewEventButton = false;
		newRow = false;
	
		if( button->hasEventCard() ) 
		{
			lastFocus = -1;
			setFocusItem(0);
		}

		return button;
	}

	QGraphicsItem *Scene::addEvent(Card *item) 
	{
		// FIXME: redo this
		EventActionPair *button = 0; 
		prevNewEventButton = false;

		if( newRow )
			button = createNewEventActionPair();
		else if( prevNewActionButton )
			button = eventActionPairs.last();
		else 
		{
			if( lastFocus >= 0 )
				button = eventActionPairs.at(lastFocus);
			else
				for(int i=0; i<eventActionPairs.size(); ++i)
					if( eventActionPairs.at(i)->hasFocus() )
					{
						button = eventActionPairs.at(i);
						break;
					}
			
			if( !button )
			{
				if(!eventActionPairs.isEmpty() && !(eventActionPairs.last()->hasEventCard()) )
					button = eventActionPairs.last();
				else
				{
					button = createNewEventActionPair();
					prevNewEventButton = true;
				}
			}

		}
		
		if( item )
			button->addEventCard(item);
		prevNewActionButton = false;
		newRow = false;
		
		if( button->hasActionCard() ) 
		{
			lastFocus = -1;
			setFocusItem(0);
		}
		
		return button;
	}

	void Scene::addEventActionPair(Card *event, Card *action)
	{
		EventActionPair *p(createNewEventActionPair());
		if(event)
			p->addEventCard(event);
		if(action)
			p->addActionCard(action);
		lastFocus = -1;
		setFocusItem(0);

		prevNewEventButton = false;
		prevNewActionButton = false;
		newRow = false;
	}

	EventActionPair *Scene::createNewEventActionPair()
	{
		EventActionPair *p(new EventActionPair(eventActionPairs.size(), advancedMode));
		p->setColorScheme(eventCardColor, actionCardColor);
		eventActionPairs.push_back(p);
		
		addItem(p);

		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		
		return p;
	}

	bool Scene::isEmpty() const
	{
		if( eventActionPairs.isEmpty() )
			return true;
			
		if( eventActionPairs.size() > 1 ||
			eventActionPairs[0]->hasActionCard() ||
			eventActionPairs[0]->hasEventCard() )
			return false;

		return true;
	}

	void Scene::reset()
	{
		clear();
		createNewEventActionPair();
		recompile();
	}
	
	void Scene::clear()
	{
		for(int i=0; i<eventActionPairs.size(); i++)
		{
			disconnect(eventActionPairs.at(i), SIGNAL(contentChanged()), this, SLOT(recompile()));
			removeItem(eventActionPairs.at(i));
			delete(eventActionPairs.at(i));
		}
		eventActionPairs.clear();
		compiler.clear();
		
		sceneModified = false;
		
		lastFocus = -1;
		prevNewActionButton = false;
		prevNewEventButton = false;
		advancedMode = false;
	}
	
	void Scene::haveAtLeastAnEmptyCard()
	{
		if (eventActionPairs.empty())
			createNewEventActionPair();
	}
	
	void Scene::setColorScheme(QColor eventColor, QColor actionColor)
	{
		eventCardColor = eventColor;
		actionCardColor = actionColor;

		for(PairItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
			(*itr)->setColorScheme(eventColor, actionColor);

		update();
	
		sceneModified = true;
	}

	void Scene::setAdvanced(bool advanced)
	{
		advancedMode = advanced;
		for(PairItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
		{
			EventActionPair* p(*itr);
			p->disconnect(SIGNAL(contentChanged()), this, SLOT(recompile()));
			p->setAdvanced(advanced);
			connect(p, SIGNAL(contentChanged()), SLOT(recompile()));
		}
		
		recompile();
	}
	
	bool Scene::isAnyStateFilter() const
	{
		for (PairConstItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
			if ((*itr)->isAnyStateFilter())
				return true;
		return false;
	}

	void Scene::removeButton(int row)
	{
		Q_ASSERT( row < eventActionPairs.size() );
		
		EventActionPair *p(eventActionPairs[row]);
		disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		eventActionPairs.removeAt(row);
		
		removeItem(p);
		delete(p);
		
		rearrangeButtons(row);

		if (eventActionPairs.isEmpty()) 
			createNewEventActionPair();

		prevNewEventButton = false;
		prevNewActionButton = false;
		lastFocus = -1;
	
		sceneModified = true;
		
		recompile();
	}
	
	void Scene::insertButton(int row) 
	{
		Q_ASSERT( row <= eventActionPairs.size() );

		EventActionPair *p(new EventActionPair(row, advancedMode));
		p->setColorScheme(eventCardColor, actionCardColor);
		eventActionPairs.insert(row, p);
		addItem(p);
		
		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		
		setFocusItem(p);
		lastFocus = row;
		
		rearrangeButtons(row+1);

		prevNewEventButton = false;
		prevNewActionButton = false;
	
		sceneModified = true;
		
		recompile();
	}

	void Scene::rearrangeButtons(int row)
	{
		for(int i=row; i<eventActionPairs.size(); ++i) 
			eventActionPairs.at(i)->setRow(i);
	}
	
	QString Scene::getErrorMessage() const
	{ 
		switch(compiler.getErrorCode())
		{
		case Compiler::MISSING_EVENT:
			return tr("Line %0: Missing event").arg(compiler.getErrorLine());
			break;
		case Compiler::MISSING_ACTION:
			return tr("Line %0: Missing action").arg(compiler.getErrorLine());
			break;
		case Compiler::EVENT_REPEATED:
			return tr("Line %0: Redeclaration of event").arg(compiler.getErrorLine());
			break;
		case Compiler::INVALID_CODE:
			return tr("Line %0: Unknown event/action type").arg(compiler.getErrorLine());
			break;
		case Compiler::NO_ERROR:
			return tr("Compilation success");
		default:
			return tr("Unknown VPL error");
		}
	}	
	
	QList<QString> Scene::getCode() const
	{
		QList<QString> out;
		
		for(std::vector<std::wstring>::const_iterator itr = compiler.beginCode();
			itr != compiler.endCode(); ++itr)
			out.push_back(QString::fromStdWString(*itr));
	
		return out;
	}

	int Scene::getFocusItemId() const 
	{ 
		QGraphicsItem *item = focusItem();
		
		while( item != 0 )
		{
			if( item->data(0).toString() == "eventactionpair" )
				return compiler.buttonToCode(item->data(1).toInt());
			else
				item = item->parentItem();
		}

		return -1;
	}
	
	void Scene::recompile()
	{
		sceneModified = true;
		
		compiler.compile();
		compiler.generateCode();
		
		for(int i=0; i<eventActionPairs.size(); i++)
			eventActionPairs[i]->setErrorStatus(false);
		
		if( !compiler.isSuccessful() )
			eventActionPairs[compiler.getErrorLine()]->setErrorStatus(true);
		
		emit contentRecompiled();
	}

	void Scene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if( event->mimeData()->hasFormat("Card") && 
			((event->scenePos().y()-20)/(buttonSetHeight)) >= eventActionPairs.size() ) 
		{
			setFocusItem(0);
			event->accept();
		} 
		else
			QGraphicsScene::dragMoveEvent(event);
	}
	
	void Scene::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("EventActionPair") )
		{
			QByteArray buttonData = event->mimeData()->data("EventActionPair");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);

			int prevRow, currentRow;
			dataStream >> prevRow;
			
			EventActionPair *p(eventActionPairs.at(prevRow));
			eventActionPairs.removeAt(prevRow);

			qreal currentYPos = event->scenePos().y();
			currentRow = (int)((currentYPos-20)/(buttonSetHeight));
			currentRow = (currentRow < 0 ? 0 : currentRow > eventActionPairs.size() ? eventActionPairs.size() : currentRow);
			
			eventActionPairs.insert(currentRow, p);

			rearrangeButtons( prevRow < currentRow ? prevRow : currentRow );
			
			event->setDropAction(Qt::MoveAction);
			event->accept();

			setFocusItem(p);
			lastFocus = currentRow;
			
			recompile();
		}
		else if( event->mimeData()->hasFormat("Card") && 
				((event->scenePos().y()-20)/(buttonSetHeight)) >= eventActionPairs.size() )
		{
			QByteArray buttonData = event->mimeData()->data("Card");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);
			
			int parentID;
			dataStream >> parentID;

			QString cardName;
			int numButtons;
			dataStream >> cardName >> numButtons;
			
			Card *button(Card::createCard(cardName, advancedMode));
			if( button ) 
			{
				event->setDropAction(Qt::MoveAction);
				event->accept();
				
				lastFocus = -1;
				prevNewActionButton = false;
				prevNewEventButton = false;
				newRow = true;
				
				if( event->mimeData()->data("CardType") == QString("event").toLatin1() )
					addEvent(button);
				else if( event->mimeData()->data("CardType") == QString("action").toLatin1() )
					addAction(button);

				for( int i=0; i<numButtons; ++i ) 
				{
					int status;
					dataStream >> status;
					button->setValue(i, status);
				}
			}
		}
		else
			QGraphicsScene::dropEvent(event);
	}

	void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
	{
		QGraphicsScene::mousePressEvent(event);
	}
	
	void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
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
} } // namespace ThymioVPL / namespace Aseba
