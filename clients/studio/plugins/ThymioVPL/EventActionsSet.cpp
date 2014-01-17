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
#include "EventBlocks.h"
#include "ActionBlocks.h"
#include "Scene.h"
#include "BlockHolder.h"
#include "../../../../common/utils/utils.h"

namespace Aseba { namespace ThymioVPL
{
	//! Construct an new event-action set
	EventActionsSet::EventActionsSet(int row, bool advanced, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		eventHolder(new BlockHolder("event", this)),
		stateFilter(0),
		deleteButton(new AddRemoveButton(false, this)),
		addButton(new AddRemoveButton(true, this)),
		spacing(40),
		columnWidth(40),
		width(0),
		columnPos(0),
		row(row),
		errorFlag(false)
	{
		// FIXME: correct that
		setData(0, "eventactionpair"); 
		setData(1, row);
		
		setFlag(QGraphicsItem::ItemIsSelectable);
		setAcceptDrops(true);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		setAdvanced(advanced);
		
		connect(eventHolder, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
		connect(deleteButton, SIGNAL(clicked()), SLOT(removeClicked()));
		connect(addButton, SIGNAL(clicked()), SLOT(addClicked()));
		connect(this, SIGNAL(contentChanged()), SLOT(setSoleSelection()));
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
		painter->drawRoundedRect(innerBoundingRect(), 5, 5);

		// column
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(fgColors[i][0], fgColors[i][1], fgColors[i][2]));
		const qreal ymiddle(innerBoundingRect().height()/2);
		painter->drawEllipse(columnPos, ymiddle-2*columnWidth, columnWidth, columnWidth);
		painter->drawEllipse(columnPos, ymiddle+columnWidth, columnWidth, columnWidth);
		
		/* Note: that was arrow code:
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(fgColors[i][0], fgColors[i][1], fgColors[i][2]));
		painter->drawRect(350+trans, 143, 55, 55);
		QPointF pts[3];
		pts[0] = QPointF(404.5+trans, 118);
		pts[1] = QPointF(404.5+trans, 218);
		pts[2] = QPointF(456+trans, 168);
		painter->drawPolygon(pts, 3);
		*/
		
		// line number
		painter->setPen(QColor(fgColors[i][0], fgColors[i][1], fgColors[i][2]));
		painter->setFont(QFont("Arial", 50));
		painter->drawText(QRect(width-spacing-128, 240, 128, 64), Qt::AlignRight, QString("%0").arg(getRow()));
	}
	
	// FIXME: check
	void EventActionsSet::setRow(int row) 
	{ 
		setData(1,row); 
		/*
		FIXME: does it makes sesntes
		if( eventBlock )
			eventBlock->setParentID(row);
		if( actionBlock )
			actionBlock->setParentID(row);
		*/
		repositionElements();
	}
	
	//! Add an event block, call the event holder
	void EventActionsSet::addEventBlock(Block *block) 
	{ 
		eventHolder->addBlock(block);
	}
	
	//! Add an action block, optionally at a given position (which must be valid)
	void EventActionsSet::addActionBlock(Block *block, int number) 
	{
		// add block
		if (number == -1)
			actionHolders.last()->addBlock(block);
		else
			actionHolders.at(number)->addBlock(block);
		
		// make sure that there is a free one at the end and cleanup layout
		cleanupActionSlots();
		repositionElements();
	}
	
	//! Return whether the event holder has a block 
	const bool EventActionsSet::hasEventBlock() const
	{
		return eventHolder->getBlock() != 0;
	}
	
	//! Return the block in the event holder
	const Block *EventActionsSet::getEventBlock() const
	{
		return eventHolder->getBlock();
	}
	
	//! Return whether the action holders have any block
	const bool EventActionsSet::hasAnyActionBlock() const
	{
		return actionHolders.size() > 1;
	}
	
	//! Return the block in one of the action holders
	const Block *EventActionsSet::getActionBlock(int number) const
	{
		return actionHolders.at(number)->getBlock();
	}
	
	//! Return an action block holder by block name, 0 if not found
	BlockHolder *EventActionsSet::getActionBlockHolder(const QString& name)
	{
		for (int i=0; i<actionHolders.size(); ++i)
		{
			const Block* actionBlock(actionHolders[i]->getBlock());
			if (actionBlock && actionBlock->name == name)
				return actionHolders[i];
		}
		return 0;
	}
	
	//! Return the index of a given block holder, -1 if not found
	int EventActionsSet::getBlockHolderIndex(BlockHolder *holder) const
	{
		for (int i=0; i<actionHolders.size(); ++i)
		{
			if (actionHolders[i] == holder)
				return i;
		}
		return -1;
	}
	
	//! Does this set uses any feature from the advanced mode?
	bool EventActionsSet::isAnyAdvancedFeature() const
	{
		// is the event using an advanced feature
		if (eventHolder->getBlock() && (eventHolder->getBlock()->isAnyAdvancedFeature()))
			return true;
		
		// is the state filter set?
		if (stateFilter && stateFilter->isAnyValueSet())
			return true;
	
		// is any action using an advanced feature
		for (int i=0; i<actionHolders.size(); ++i)
		{
			const Block* actionBlock(actionHolders[i]->getBlock());
			if (actionBlock && actionBlock->isAnyAdvancedFeature())
				return true;
		}
		
		return false;
	}
	
	//! Is this pair empty?
	bool EventActionsSet::isEmpty() const
	{
		if (hasEventBlock())
			return false;
		if (stateFilter && stateFilter->isAnyValueSet())
			return false;
		if (hasAnyActionBlock())
			return false;
		return true;
	}
	
	//! Switch to or from advanced mode
	void EventActionsSet::setAdvanced(bool advanced)
	{
		// Note: we do not emit contentChanged on purprose, as we know that the caller will recompile
		
		// switch event and actions to given mode
		eventHolder->setAdvanced(advanced);
		for (int i=0; i<actionHolders.size(); ++i)
			actionHolders[i]->setAdvanced(advanced);
		
		// create and delete state filter
		if (advanced && !stateFilter)
		{
			// switching to advanced mode, create state filter
			// TODO: switch state filter to third type
			stateFilter = new StateFilterEventBlock(this);
			connect(stateFilter, SIGNAL(contentChanged()), SIGNAL(contentChanged()));
		}
		else if (!advanced && stateFilter)
		{
			// switching from advanced mode, delete state filter
			delete stateFilter;
			stateFilter = 0;
		}
		
		// these operations might have left holes in the action blocks
		cleanupActionSlots();
		repositionElements();
	}
	
	//! Set whether or not this set has an error
	void EventActionsSet::setErrorStatus(bool flag)
	{
		if (flag != errorFlag)
		{
			errorFlag = flag;
			update();
		}
	}
	
	//! Return whether a given block name is already present in this set
	bool EventActionsSet::hasActionBlock(const QString& blockName) const
	{
		for (int i=0; i<actionHolders.size(); ++i)
			if (actionHolders[i]->getBlock() && 
				(actionHolders[i]->getBlock()->name == blockName))
				return true;
		return false;
	}
	
	//! Make sure that there is no hole in action slots, and that there is a free slot at the end
	void EventActionsSet::cleanupActionSlots()
	{
		const int maxCount(stateFilter ? 6 : 5);
		
		// remove empty holders except the last
		int i = 0;
		while (i<actionHolders.size())
		{
			if (actionHolders[i]->isEmpty())
			{
				BlockHolder* holder(actionHolders[i]);
				actionHolders.removeAt(i);
				holder->deleteLater();
			}
			else
				++i;
		}
		
		// if no holder at all or the last holder is not empty, add an empty holder
		if ((actionHolders.isEmpty()) ||
			(!actionHolders.last()->isEmpty() && (actionHolders.size() < maxCount))
		)
		{
			actionHolders.push_back(new BlockHolder("action", this));
			connect(actionHolders.last(), SIGNAL(contentChanged()), SIGNAL(contentChanged()));
		}
	}
	
	//! Recompute the positions of the different elements for this set
	void EventActionsSet::repositionElements()
	{
		// constants and initialization
		const qreal ypos = spacing;
		qreal xpos = spacing;
		
		// add event block holder
		eventHolder->setPos(xpos, ypos);
		xpos += eventHolder->boundingRect().width() + spacing;
		
		// if state filter holder, add it
		if (stateFilter)
		{
			stateFilter->setPos(xpos, ypos);
			xpos += stateFilter->boundingRect().width() + spacing;
		}
		
		// add column
		columnPos = xpos;
		xpos += columnWidth + spacing;
		
		// add every action holder
		for (int i=0; i<actionHolders.size(); ++i)
		{
			actionHolders[i]->setPos(xpos, ypos);
			xpos += actionHolders[i]->boundingRect().width() + spacing;
		}
		
		// delete button
		deleteButton->setPos(xpos+32, 70);
		xpos += deleteButton->boundingRect().width() + spacing;
		
		// add button, under column
		addButton->setPos(columnPos+20, 378);
		
		// store width
		width = xpos;
		
		// compute global position
		Scene* vplScene(dynamic_cast<Scene*>(scene()));
		const unsigned rowPerCol(vplScene ? ceilf(float(vplScene->pairsCount())/float(vplScene->getZoomLevel())) : 0);
		const unsigned col(vplScene ? getRow()/rowPerCol : 0);
		const unsigned row(vplScene ? getRow()%rowPerCol : getRow());
		setPos(col*1088, row*420);
		
		// notify scene of changes
		prepareGeometryChange();
		if (vplScene) vplScene->recomputeSceneRect();
	}
	
	//! Select only this set and nothing else
	void EventActionsSet::setSoleSelection()
	{
		scene()->clearSelection();
		setSelected(true);
	}
	
	void EventActionsSet::removeClicked()
	{
		// FIXME: cleanup
		polymorphic_downcast<Scene*>(scene())->removePair(data(1).toInt());
	}
	
	void EventActionsSet::addClicked()
	{
		// FIXME: cleanup
		polymorphic_downcast<Scene*>(scene())->insertPair(data(1).toInt()+1);
	}
	
	
	
	void EventActionsSet::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
	{
		if (event->mimeData()->hasFormat("EventActionsSet"))
		{
			// FIXME: why that?
			//event->setDropAction(Qt::MoveAction);
			event->accept();
			//update();
		}
		else
			event->ignore();
	}
	
	void EventActionsSet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (event->mimeData()->hasFormat("EventActionsSet"))
		{
			//event->setDropAction(Qt::MoveAction);
			event->accept();
			//update();
		} 
		else
			event->ignore();
	}

	void EventActionsSet::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (event->mimeData()->hasFormat("EventActionsSet"))
		{
			// FIXME: implement
			/*QByteArray cardData(event->mimeData()->data("Block"));
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
						// TODO; check number
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
			update();*/
		}
		else
			event->ignore();
	}

	void EventActionsSet::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		// FIXME: implement
		/*
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
		*/
	}
} } // namespace ThymioVPL / namespace Aseba