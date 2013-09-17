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
#include <cmath>
#include <typeinfo>

#include "EventActionPair.h"
#include "Buttons.h"
#include "Card.h"
#include "ActionCards.h"
#include "Scene.h"
#include "../../../../common/utils/utils.h"

namespace Aseba { namespace ThymioVPL
{
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
		
		setFlag(QGraphicsItem::ItemIsSelectable);
		setAcceptDrops(true);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		deleteButton = new AddRemoveButton(false, this);
		addButton = new AddRemoveButton(true, this);
		repositionElements();
		
		connect(deleteButton, SIGNAL(clicked()), SLOT(removeClicked()));
		connect(addButton, SIGNAL(clicked()), SLOT(addClicked()));
		connect(this, SIGNAL(contentChanged()), SLOT(setSoleSelection()));
	}
	
	void EventActionPair::removeClicked()
	{
		polymorphic_downcast<Scene*>(scene())->removePair(data(1).toInt());
	}
	
	void EventActionPair::addClicked()
	{
		polymorphic_downcast<Scene*>(scene())->insertPair(data(1).toInt()+1);
	}
	
	void EventActionPair::setSoleSelection()
	{
		scene()->clearSelection();
		setSelected(true);
	}
	
	void EventActionPair::repositionElements()
	{
		Scene* vplScene(dynamic_cast<Scene*>(scene()));
		
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
		
		const unsigned rowPerCol(vplScene ? ceilf(float(vplScene->pairsCount())/float(vplScene->getZoomLevel())) : 0);
		const unsigned col(vplScene ? getRow()/rowPerCol : 0);
		const unsigned row(vplScene ? getRow()%rowPerCol : getRow());
		
		setPos(xpos+col*(boundingRect().width()+60), row*420+20);
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
		
		// select colors (error, selected, normal)
		const int bgColors[][3] = {
			{255, 180, 180},
			{255, 220, 211},
			//{255, 237, 233},
			{241, 241, 241},
		};
		const int fgColors[][3] = {
			{237, 120, 120},
			{237, 172, 140},
			//{237, 208, 194},
			{213, 213, 213},
		};
		const int i = (isSelected() ? 1 : 2);
		
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
		
		// line number
		painter->setPen(QColor(fgColors[i][0], fgColors[i][1], fgColors[i][2]));
		painter->setFont(QFont("Arial", 50));
		painter->drawText(QRect(830+trans-32, 240, 64, 64), Qt::AlignCenter, QString("%0").arg(getRow()));

		// card place and highlight
		painter->setBrush(Qt::NoBrush);
		
		if (eventCard)
		{
			if (highlightEventButton)
			{
				eventCardColor.setAlpha(130);
				painter->setPen(QPen(eventCardColor, 20));
				painter->drawRoundedRect(30, 30, 276, 276, 5, 5);
			}
		}
		else
		{
			if (!highlightEventButton)
				eventCardColor.setAlpha(100);
			painter->setPen(QPen(eventCardColor, 10, Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->drawRoundedRect(45, 45, 246, 246, 5, 5);
		}
		eventCardColor.setAlpha(255);
		
		if (actionCard)
		{
			if (highlightActionButton)
			{
				actionCardColor.setAlpha(130);
				painter->setPen(QPen(actionCardColor, 20));
				painter->drawRoundedRect(490+trans, 30, 276, 276, 5, 5);
			}
		}
		else
		{
			if (!highlightActionButton)
				actionCardColor.setAlpha(100);
			painter->setPen(QPen(actionCardColor, 10,	Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->drawRoundedRect(505+trans, 45, 246, 246, 5, 5);
		}
		actionCardColor.setAlpha(255);
	}
	
	bool EventActionPair::isAnyStateFilter() const
	{
		if (eventCard && eventCard->isAnyStateFilter())
			return true;
		if (actionCard && (actionCard->getName() == "statefilter"))
			return true;
		return false;
	}
	
	bool EventActionPair::isEmpty() const
	{
		return (!hasEventCard() && !hasActionCard());
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
		repositionElements();
	}
	
	void EventActionPair::removeEventCard() 
	{
		if( eventCard ) 
		{
			disconnect(eventCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem( eventCard );
			eventCard->deleteLater();
			eventCard = 0;
			polymorphic_downcast<Scene*>(scene())->recompile();
		}
	}

	void EventActionPair::addEventCard(Card *event) 
	{ 
		removeEventCard();
		event->setBackgroundColor(eventCardColor);
		event->setPos(40, 40);
		event->setEnabled(true);
		event->setParentItem(this);
		event->setParentID(data(1).toInt());
		eventCard = event;
		
		emit contentChanged();
		connect(eventCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
	}
	
	void EventActionPair::removeActionCard()
	{
		if( actionCard )
		{
			disconnect(actionCard, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem( actionCard );
			actionCard->deleteLater();
			actionCard = 0;
			polymorphic_downcast<Scene*>(scene())->recompile();
		}
	}

	void EventActionPair::addActionCard(Card *action) 
	{ 
		removeActionCard();
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
	
	void EventActionPair::setErrorStatus(bool flag)
	{
		if (flag != errorFlag)
		{
			errorFlag = flag;
			update();
		}
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
			update();
		}
		else
			event->ignore();
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
		if ( event->mimeData()->hasFormat("Card") )
		{
			QByteArray cardData(event->mimeData()->data("Card"));
			QDataStream dataStream(&cardData, QIODevice::ReadOnly);
			
			int parentID;
			dataStream >> parentID;
			
			if (parentID != data(1).toInt())
			{
				QString cardName;
				int state;
				dataStream >> cardName >> state;
				
				Card *card(Card::createCard(cardName, advancedMode));
				if( card ) 
				{
					event->setDropAction(Qt::MoveAction);
					event->accept();
					
					if( event->mimeData()->data("CardType") == QString("event").toLatin1() )
					{
						if (parentID >= 0)
							polymorphic_downcast<Scene*>(scene())->getPairRow(parentID)->removeEventCard();
						if (advancedMode)
						{
							if (parentID == -1)
							{
								if (eventCard)
									card->setStateFilter(eventCard->getStateFilter());
								else
									card->setStateFilter(0);
							}
							else
								card->setStateFilter(state);
						}
						addEventCard(card);
						polymorphic_downcast<Scene*>(scene())->ensureOneEmptyPairAtEnd();
					}
					else if( event->mimeData()->data("CardType") == QString("action").toLatin1() )
					{
						if (parentID >= 0)
							polymorphic_downcast<Scene*>(scene())->getPairRow(parentID)->removeActionCard();
						addActionCard(card);
						polymorphic_downcast<Scene*>(scene())->ensureOneEmptyPairAtEnd();
					}
					
					int numButtons;
					dataStream >> numButtons;
					for( int i=0; i<numButtons; ++i ) 
					{
						int status;
						dataStream >> status;
						card->setValue(i, status);
					}
				}
			}
			highlightEventButton = false;
			highlightActionButton = false;
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