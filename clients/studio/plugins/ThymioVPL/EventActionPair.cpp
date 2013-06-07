#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QDrag>
#include <QMimeData>
#include <QCursor>
#include <QApplication>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsView>
#include <QDebug>
#include <cassert>
#include <typeinfo>

#include "EventActionPair.h"
#include "Buttons.h"
#include "Card.h"
#include "ActionCards.h"

namespace Aseba { namespace ThymioVPL
{
	EventActionPair::ThymioRemoveButton::ThymioRemoveButton(QGraphicsItem *parent) : 
		QGraphicsItem(parent) 
	{
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setData(0, "remove"); 
		
		setAcceptedMouseButtons(Qt::LeftButton);
	}
	
	void EventActionPair::ThymioRemoveButton::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);

		qreal alpha = hasFocus() ? 255 : 150;
		
		QLinearGradient linearGrad(QPointF(-32, -32), QPointF(32, 32));
		linearGrad.setColorAt(0, QColor(209, 196, 180, alpha));
		linearGrad.setColorAt(1, QColor(167, 151, 128, alpha));
     
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(linearGrad);
		painter->drawEllipse(-32,-32,64,64);
		
		painter->setPen(QPen(QColor(147, 134, 115),5,Qt::SolidLine,Qt::RoundCap));
		painter->drawLine(-11,-11,11,11);
		painter->drawLine(-11,11,11,-11);
	}

	EventActionPair::ThymioAddButton::ThymioAddButton(QGraphicsItem *parent) : 
		QGraphicsItem(parent) 
	{ 
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setData(0, "add"); 

		setAcceptedMouseButtons(Qt::LeftButton);
	}
	
	void EventActionPair::ThymioAddButton::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		qreal alpha = hasFocus() ? 255 : 150;
		
		QLinearGradient linearGrad(QPointF(-32, -32), QPointF(32, 32));
		linearGrad.setColorAt(0, QColor(209, 196, 180, alpha));
		linearGrad.setColorAt(1, QColor(167, 151, 128, alpha));
     
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(linearGrad);
		painter->drawEllipse(-32,-32,64,64);
		
		painter->setPen(QPen(QColor(147, 134, 115),5,Qt::SolidLine,Qt::RoundCap));
		painter->drawLine(-16,0,16,0);
		painter->drawLine(0,-16,0,16);
	}

	EventActionPair::EventActionPair(int row, bool advanced, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		eventCard(0),
		actionCard(0),
		eventCardColor(QColor(0,191,255)),
		actionCardColor(QColor(218,112,214)),
		highlightEventButton(false),
		highlightActionButton(false),
		errorFlag(false),
		advancedMode(advanced),
		trans(0),
		xpos(0)
	{ 
		setData(0, "eventactionpair"); 
		setData(1, row);
		
		deleteButton = new ThymioRemoveButton(this);
		addButton = new ThymioAddButton(this);
		
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setAcceptDrops(true);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		repositionElements();
	}
	
	void EventActionPair::repositionElements()
	{
		if (advancedMode)
		{
			trans = 128;
			xpos = 5;
		} 
		else
		{
			trans = 0;
			xpos = 5+64;
		}
		
		setPos(xpos, (getRow()*420+20));
		deleteButton->setPos(830+trans, 70);
		addButton->setPos(450+trans/2, 378);
		
		if (actionCard)
			actionCard->setPos(500+trans, 40);
		
		prepareGeometryChange();
	}
	
	void EventActionPair::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		// first check whether some buttons have been removed
		if( eventCard && eventCard->getParentID() < 0 ) 
		{
			disconnect(eventCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem(eventCard);
			delete eventCard;
			eventCard = 0;
		
			emit contentChanged();
		}
		if( actionCard && actionCard->getParentID() < 0 )
		{
			disconnect(actionCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem(actionCard);
			delete actionCard;
			actionCard = 0;

			emit contentChanged();
		}
		
		// then proceed with painting
		
		// select colors (error, selected, normal)
		const int bgColors[][3] = {
			{255, 180, 180},
			{255, 220, 211},
			{255, 237, 233},
		};
		const int fgColors[][3] = {
			{237, 120, 120},
			{237, 172, 140},
			{237, 208, 194},
		};
		const int i = (errorFlag ? 0 : hasFocus() ? 1 : 2);
		
		// background
		if (errorFlag)
			painter->setPen(QPen(Qt::red, 4));
		else
			painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(bgColors[i][0], bgColors[i][1], bgColors[i][2]));
		painter->drawRoundedRect(0, 0, 900+trans, 336, 5, 5);

		// arrow
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(fgColors[i][0], fgColors[i][1], fgColors[i][2]));
		painter->drawRect(350+trans, 143, 55, 55);
		QPointF pts[3];
		pts[0] = QPointF(404.5+trans, 118);
		pts[1] = QPointF(404.5+trans, 218);
		pts[2] = QPointF(456+trans, 168);
		painter->drawPolygon(pts, 3);
		
		if( eventCard == 0 )
		{
			if( !highlightEventButton )
				eventCardColor.setAlpha(100);
			painter->setPen(QPen(eventCardColor, 10, Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->setBrush(Qt::NoBrush);
			painter->drawRoundedRect(45, 45, 246, 246, 5, 5);
			eventCardColor.setAlpha(255);
		}

		if( actionCard == 0 )
		{
			if( !highlightActionButton )
				actionCardColor.setAlpha(100);
			painter->setPen(QPen(actionCardColor, 10,	Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->setBrush(Qt::NoBrush);
			painter->drawRoundedRect(505+trans, 45, 246, 246, 5, 5);
			actionCardColor.setAlpha(255);
		}
	}
	
	bool EventActionPair::isAnyStateFilter() const
	{
		if (eventCard && eventCard->isAnyStateFilter())
			return true;
		if (actionCard && (actionCard->getName() == "statefilter"))
			return true;
		return false;
	}

	void EventActionPair::setColorScheme(QColor eventColor, QColor actionColor)
	{
		eventCardColor = eventColor;
		actionCardColor = actionColor;
		
		if( eventCard )
			eventCard->setBackgroundColor(eventColor);
		if( actionCard )
			actionCard->setBackgroundColor(actionColor);
	}

	void EventActionPair::setRow(int row) 
	{ 
		setData(1,row); 
		if( eventCard )
			eventCard->setParentID(row);
		if( actionCard )
			actionCard->setParentID(row);
		setPos(xpos, (row*420+20)*scale());
	}

	void EventActionPair::addEventCard(Card *event) 
	{ 
		if( eventCard ) 
		{
			disconnect(eventCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem( eventCard );
			eventCard->deleteLater();
		}
		
		event->setBackgroundColor(eventCardColor);
		event->setPos(40, 40);
		event->setEnabled(true);
		event->setParentItem(this);
		event->setParentID(data(1).toInt());
		eventCard = event;
		
		emit contentChanged();
		connect(eventCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
	}

	void EventActionPair::addActionCard(Card *action) 
	{ 
		if( actionCard )
		{
			disconnect(actionCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem( actionCard );
			actionCard->deleteLater();
		}
		
		action->setBackgroundColor(actionCardColor);
		action->setPos(500+trans, 40);
		action->setEnabled(true);
		action->setParentItem(this);
		action->setParentID(data(1).toInt());
		actionCard = action;
		
		emit contentChanged();
		connect(actionCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
	}
	
	void EventActionPair::setAdvanced(bool advanced)
	{
		advancedMode = advanced;
		if (eventCard)
			eventCard->setAdvanced(advanced);
		// remove any "state setter" action card
		if (!advanced && actionCard && typeid(*actionCard) == typeid(StateFilterActionCard))
		{
			scene()->removeItem(actionCard);
			delete actionCard;
			actionCard = 0;
		}
		
		repositionElements();
	}
	
	void EventActionPair::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
	{
		if ( event->mimeData()->hasFormat("EventActionPair") || 
			 event->mimeData()->hasFormat("Card") )
		{
			if( event->mimeData()->hasFormat("CardType") )
			{
				if( event->mimeData()->data("CardType") == QString("event").toLatin1() )
					highlightEventButton = true;
				else if( event->mimeData()->data("CardType") == QString("action").toLatin1() )
					highlightActionButton = true;
			}

			event->setDropAction(Qt::MoveAction);
			event->accept();
			scene()->setFocusItem(this);
			update();
		}
		else
			event->ignore();
	}

	void EventActionPair::dragMoveEvent( QGraphicsSceneDragDropEvent *event )
	{
		/*if ( event->mimeData()->hasFormat("EventActionPair") ||
			 event->mimeData()->hasFormat("Card") )
		{
			if( event->mimeData()->hasFormat("CardType") )
			{
				if( event->mimeData()->data("CardType") == QString("event").toLatin1() )
					highlightEventButton = true;
				else if( event->mimeData()->data("CardType") == QString("action").toLatin1() )
					highlightActionButton = true;
			}
			
			event->setDropAction(Qt::MoveAction);
			event->accept();
			scene()->setFocusItem(this);
			update();
		}
		else
			event->ignore();*/
		QGraphicsObject::dragMoveEvent(event);
	}
	
	void EventActionPair::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("Card") || 
			 event->mimeData()->hasFormat("EventActionPair") )
		{
			highlightEventButton = false;
			highlightActionButton = false;
			event->setDropAction(Qt::MoveAction);
			event->accept();
			update();
		} 
		else
			event->ignore();
	}

	void EventActionPair::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		scene()->setFocusItem(0);
	
		if ( event->mimeData()->hasFormat("Card") )
		{
			QByteArray buttonData = event->mimeData()->data("Card");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);
			
			int parentID;
			dataStream >> parentID;
			
			if( parentID != data(1).toInt() )
			{
				QString buttonName;
				int state;
				dataStream >> buttonName >> state;
				
				Card *button(Card::createCard(buttonName, advancedMode));
				if( button ) 
				{
					event->setDropAction(Qt::MoveAction);
					event->accept();
					
					if( event->mimeData()->data("CardType") == QString("event").toLatin1() )
					{
						if (advancedMode)
						{
							if (parentID == -1)
							{
								if (eventCard)
									button->setStateFilter(eventCard->getStateFilter());
								else
									button->setStateFilter(0);
							}
							else
								button->setStateFilter(state);
						}
						addEventCard(button);
						highlightEventButton = false;
					}
					else if( event->mimeData()->data("CardType") == QString("action").toLatin1() )
					{
						addActionCard(button);
						highlightActionButton = false;
					}
					
					int numButtons;
					dataStream >> numButtons;
					for( int i=0; i<numButtons; ++i ) 
					{
						int status;
						dataStream >> status;
						button->setValue(i, status);
					}
				}
			}
			
			update();
		}
		else
			event->ignore();
	}

	void EventActionPair::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		#ifndef ANDROID
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;
		
		QByteArray data;
		QDataStream dataStream(&data, QIODevice::WriteOnly);
		
		dataStream << getRow();

		QMimeData *mime = new QMimeData;
		mime->setData("EventActionPair", data);
		
		Q_ASSERT(scene()->views().size() > 0);
		QGraphicsView* view(scene()->views()[0]);
		const QRectF sceneRect(mapRectToScene(innerBoundingRect()));
		const QRect viewRect(view->mapFromScene(sceneRect).boundingRect());
		const QPoint hotspot(view->mapFromScene(event->scenePos()) - viewRect.topLeft());
		QPixmap pixmap(viewRect.size());
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		scene()->render(&painter, QRectF(), sceneRect);
		painter.end();
		
		QDrag *drag = new QDrag(event->widget());
		drag->setMimeData(mime);
		drag->setHotSpot(hotspot);
		drag->setPixmap(pixmap);
		drag->exec(Qt::MoveAction);
		#endif // ANDROID
	}
} } // namespace ThymioVPL / namespace Aseba