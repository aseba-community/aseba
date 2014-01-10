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

#include "EventActionsSet.h"
#include "Buttons.h"
#include "Block.h"
#include "ActionBlocks.h"
#include "Scene.h"
#include "../../../../common/utils/utils.h"

namespace Aseba { namespace ThymioVPL
{
	EventActionsSet::EventActionsSet(int row, bool advanced, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		eventBlock(0),
		actionBlock(0),
		eventBlockColor(QColor(0,191,255)),
		actionBlockColor(QColor(218,112,214)),
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
	
	void EventActionsSet::removeClicked()
	{
		polymorphic_downcast<Scene*>(scene())->removePair(data(1).toInt());
	}
	
	void EventActionsSet::addClicked()
	{
		polymorphic_downcast<Scene*>(scene())->insertPair(data(1).toInt()+1);
	}
	
	void EventActionsSet::setSoleSelection()
	{
		scene()->clearSelection();
		setSelected(true);
	}
	
	void EventActionsSet::repositionElements()
	{
		Scene* vplScene(dynamic_cast<Scene*>(scene()));
		
		if (advancedMode)
		{
			trans = 128;
			xpos = 0;
		} 
		else
		{
			trans = 0;
			xpos = 0+64;
		}
		
		const unsigned rowPerCol(vplScene ? ceilf(float(vplScene->pairsCount())/float(vplScene->getZoomLevel())) : 0);
		const unsigned col(vplScene ? getRow()/rowPerCol : 0);
		const unsigned row(vplScene ? getRow()%rowPerCol : getRow());
		
		setPos(col*1088+xpos, row*420);
		deleteButton->setPos(830+trans, 70);
		addButton->setPos(450+trans/2, 378);
		
		if (actionBlock)
			actionBlock->setPos(500+trans, 40);
		
		prepareGeometryChange();
	}
	
	void EventActionsSet::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
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
		painter->drawText(QRect(830+trans-64, 240, 128, 64), Qt::AlignCenter, QString("%0").arg(getRow()));

		// card place and highlight
		painter->setBrush(Qt::NoBrush);
		
		if (eventBlock)
		{
			if (highlightEventButton)
			{
				eventBlockColor.setAlpha(130);
				painter->setPen(QPen(eventBlockColor, 20));
				painter->drawRoundedRect(30, 30, 276, 276, 5, 5);
			}
		}
		else
		{
			if (!highlightEventButton)
				eventBlockColor.setAlpha(100);
			painter->setPen(QPen(eventBlockColor, 10, Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->drawRoundedRect(45, 45, 246, 246, 5, 5);
		}
		eventBlockColor.setAlpha(255);
		
		if (actionBlock)
		{
			if (highlightActionButton)
			{
				actionBlockColor.setAlpha(130);
				painter->setPen(QPen(actionBlockColor, 20));
				painter->drawRoundedRect(490+trans, 30, 276, 276, 5, 5);
			}
		}
		else
		{
			if (!highlightActionButton)
				actionBlockColor.setAlpha(100);
			painter->setPen(QPen(actionBlockColor, 10,	Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->drawRoundedRect(505+trans, 45, 246, 246, 5, 5);
		}
		actionBlockColor.setAlpha(255);
	}
	
	bool EventActionsSet::isAnyAdvancedFeature() const
	{
		if (eventBlock && eventBlock->isAnyAdvancedFeature())
			return true;
		// FIXME: this is ugly, there should be something similar to isAnyAdvancedFeature and event should have an intermediate card for advanced feature
		if (actionBlock && (actionBlock->getName() == "statefilter"))
			return true;
		return false;
	}
	
	bool EventActionsSet::isEmpty() const
	{
		return (!hasEventBlock() && !hasActionBlock());
	}

	void EventActionsSet::setColorScheme(QColor eventColor, QColor actionColor)
	{
		eventBlockColor = eventColor;
		actionBlockColor = actionColor;
		
		if( eventBlock )
			eventBlock->setBackgroundColor(eventColor);
		if( actionBlock )
			actionBlock->setBackgroundColor(actionColor);
	}

	void EventActionsSet::setRow(int row) 
	{ 
		setData(1,row); 
		if( eventBlock )
			eventBlock->setParentID(row);
		if( actionBlock )
			actionBlock->setParentID(row);
		repositionElements();
	}
	
	void EventActionsSet::removeEventBlock() 
	{
		if( eventBlock ) 
		{
			disconnect(eventBlock, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem( eventBlock );
			eventBlock->deleteLater();
			eventBlock = 0;
			polymorphic_downcast<Scene*>(scene())->recompile();
		}
	}

	void EventActionsSet::addEventBlock(Block *event) 
	{ 
		removeEventBlock();
		event->setBackgroundColor(eventBlockColor);
		event->setPos(40, 40);
		event->setEnabled(true);
		event->setParentItem(this);
		event->setParentID(data(1).toInt());
		eventBlock = event;
		
		emit contentChanged();
		connect(eventBlock, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
	}
	
	void EventActionsSet::removeActionBlock()
	{
		if( actionBlock )
		{
			disconnect(actionBlock, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
			scene()->removeItem( actionBlock );
			actionBlock->deleteLater();
			actionBlock = 0;
			polymorphic_downcast<Scene*>(scene())->recompile();
		}
	}

	void EventActionsSet::addActionBlock(Block *action) 
	{ 
		removeActionBlock();
		action->setBackgroundColor(actionBlockColor);
		action->setPos(500+trans, 40);
		action->setEnabled(true);
		action->setParentItem(this);
		action->setParentID(data(1).toInt());
		actionBlock = action;
		
		emit contentChanged();
		connect(actionBlock, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
	}
	
	void EventActionsSet::setAdvanced(bool advanced)
	{
		advancedMode = advanced;
		if (eventBlock)
			eventBlock->setAdvanced(advanced);
		// remove any "state setter" action card
		if (!advanced && actionBlock && typeid(*actionBlock) == typeid(StateFilterActionBlock))
		{
			scene()->removeItem(actionBlock);
			delete actionBlock;
			actionBlock = 0;
		}
		
		repositionElements();
	}
	
	void EventActionsSet::setErrorStatus(bool flag)
	{
		if (flag != errorFlag)
		{
			errorFlag = flag;
			update();
		}
	}
	
	void EventActionsSet::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
	{
		if ( event->mimeData()->hasFormat("EventActionsSet") || 
			 event->mimeData()->hasFormat("Block") )
		{
			if( event->mimeData()->hasFormat("BlockType") )
			{
				if( event->mimeData()->data("BlockType") == QString("event").toLatin1() )
					highlightEventButton = true;
				else if( event->mimeData()->data("BlockType") == QString("action").toLatin1() )
					highlightActionButton = true;
			}

			event->setDropAction(Qt::MoveAction);
			event->accept();
			update();
		}
		else
			event->ignore();
	}
	
	void EventActionsSet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("Block") || 
			 event->mimeData()->hasFormat("EventActionsSet") )
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

	void EventActionsSet::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("Block") )
		{
			QByteArray cardData(event->mimeData()->data("Block"));
			QDataStream dataStream(&cardData, QIODevice::ReadOnly);
			
			int parentID;
			dataStream >> parentID;
			
			if (parentID != data(1).toInt())
			{
				QString cardName;
				int state;
				dataStream >> cardName >> state;
				
				Block *card(Block::createBlock(cardName, advancedMode));
				if( card ) 
				{
					event->setDropAction(Qt::MoveAction);
					event->accept();
					
					if( event->mimeData()->data("BlockType") == QString("event").toLatin1() )
					{
						if (parentID >= 0)
							polymorphic_downcast<Scene*>(scene())->getPairRow(parentID)->removeEventBlock();
						if (advancedMode)
						{
							if (parentID == -1)
							{
								if (eventBlock)
									card->setStateFilter(eventBlock->getStateFilter());
								else
									card->setStateFilter(0);
							}
							else
								card->setStateFilter(state);
						}
						addEventBlock(card);
						polymorphic_downcast<Scene*>(scene())->ensureOneEmptyPairAtEnd();
					}
					else if( event->mimeData()->data("BlockType") == QString("action").toLatin1() )
					{
						if (parentID >= 0)
							polymorphic_downcast<Scene*>(scene())->getPairRow(parentID)->removeActionBlock();
						addActionBlock(card);
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

	void EventActionsSet::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		#ifndef ANDROID
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;
		
		QByteArray data;
		QDataStream dataStream(&data, QIODevice::WriteOnly);
		
		dataStream << getRow();

		QMimeData *mime = new QMimeData;
		mime->setData("EventActionsSet", data);
		
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