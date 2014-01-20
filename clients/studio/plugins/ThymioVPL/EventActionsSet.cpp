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
#include "StateBlocks.h"
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
		stateFilterHolder(0),
		deleteButton(new AddRemoveButton(false, this)),
		addButton(new AddRemoveButton(true, this)),
		spacing(40),
		columnWidth(40),
		width(0),
		columnPos(0),
		row(row),
		errorFlag(false),
		beingDragged(false)
	{
		setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
		setAcceptedMouseButtons(Qt::LeftButton);
		setAcceptDrops(true);
		
		setAdvanced(advanced);
		
		connect(eventHolder, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()));
		connect(deleteButton, SIGNAL(clicked()), SLOT(removeClicked()));
		connect(addButton, SIGNAL(clicked()), SLOT(addClicked()));
		connect(this, SIGNAL(contentChanged()), SLOT(setSoleSelection()));
	}
	
	void EventActionsSet::setRow(int row) 
	{ 
		this->row = row;
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
		{
			assert(actionHolders.size() > 0);
			actionHolders.last()->addBlock(block);
		}
		else
		{
			actionHolders.at(number)->addBlock(block);
		}
		
		// make sure that there is a free one at the end and cleanup layout
		cleanupActionSlots();
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
		if (stateFilterHolder && stateFilterHolder->isAnyValueSet())
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
		if (stateFilterHolder && stateFilterHolder->isAnyValueSet())
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
		if (advanced && !stateFilterHolder)
		{
			// switching to advanced mode, create state filter
			stateFilterHolder = new BlockHolder("state", this, new StateFilterCheckBlock());
			connect(stateFilterHolder, SIGNAL(contentChanged()), SIGNAL(contentChanged()));
		}
		else if (!advanced && stateFilterHolder)
		{
			// switching from advanced mode, delete state filter
			delete stateFilterHolder;
			stateFilterHolder = 0;
		}
		
		// these operations might have left holes in the action blocks
		cleanupActionSlots();
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
	
	//! Make sure that there is no hole in action slots, and that there is a free slot at the end; reposition elements afterwards
	void EventActionsSet::cleanupActionSlots()
	{
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
		
		ensureOneEmptyActionHolderAtEnd();
		
		repositionElements();
	}
	
	//! Clear set to its default status
	void EventActionsSet::resetSet()
	{
		// disconnect the selection setting mechanism, as we do not want a reset to select this set
		disconnect(this, SIGNAL(contentChanged()), this, SLOT(setSoleSelection()));
		
		// delete event
		eventHolder->removeBlock();
		
		// clear filter
		if (stateFilterHolder)
			stateFilterHolder->getBlock()->resetValues();
		
		// delete all actions
		for (int i=0; i<actionHolders.size(); ++i)
			actionHolders[i]->deleteLater();
		actionHolders.clear();
		
		ensureOneEmptyActionHolderAtEnd();
		
		repositionElements();
		
		connect(this, SIGNAL(contentChanged()), SLOT(setSoleSelection()));
	}
	
	//! If action holders are not saturated, ensure at least one empty one
	void EventActionsSet::ensureOneEmptyActionHolderAtEnd()
	{
		const int maxCount(stateFilterHolder ? 6 : 5);
		
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
		if (stateFilterHolder)
		{
			stateFilterHolder->setPos(xpos, ypos);
			xpos += stateFilterHolder->boundingRect().width() + spacing;
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
		const unsigned rowPerCol(vplScene ? ceilf(float(vplScene->setsCount())/float(vplScene->getZoomLevel())) : 0);
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
		polymorphic_downcast<Scene*>(scene())->removeSet(row);
	}
	
	void EventActionsSet::addClicked()
	{
		polymorphic_downcast<Scene*>(scene())->insertSet(row+1);
	}
	
	QMimeData* EventActionsSet::mimeData() const
	{
		// create a DOM document and serialize the content of this block in it
		QDomDocument document("set");
		document.appendChild(serialize(document));
		
		// create a MIME data for this block
		QMimeData *mime = new QMimeData;
		mime->setData("EventActionsSet", document.toByteArray());
		qDebug() << document.toString();
		
		return mime;
	}
	
	QDomElement EventActionsSet::serialize(QDomDocument& document) const
	{
		// create element
		QDomElement element = document.createElement("set");
		
		// event
		if (!eventHolder->isEmpty())
			element.appendChild(eventHolder->getBlock()->serialize(document));
		// state
		if (stateFilterHolder && !stateFilterHolder->isEmpty())
			element.appendChild(stateFilterHolder->getBlock()->serialize(document));
		// actions
		for (int i=0; i<actionHolders.size(); ++i)
			if (!actionHolders[i]->isEmpty())
				element.appendChild(actionHolders[i]->getBlock()->serialize(document));
		
		return element;
	}
	
	void EventActionsSet::deserialize(const QDomElement& element)
	{
		// this function assumes an empty set
		const bool advanced(stateFilterHolder != 0);
		assert(isEmpty());
		
		// iterate on all stored block
		QDomElement blockElement(element.firstChildElement("block"));
		while (!blockElement.isNull())
		{
			// following type ...
			const QString& type(blockElement.attribute("type"));
			
			if (type == "event")
			{
				eventHolder->addBlock(Block::deserialize(blockElement, advanced));
			}
			else if (type == "state")
			{
				assert(advanced);
				stateFilterHolder->addBlock(Block::deserialize(blockElement, advanced));
			}
			else if (type == "action")
			{
				addActionBlock(Block::deserialize(blockElement, advanced));
			}
			else
				// should never happen
				abort();
			
			// get next element
			blockElement = blockElement.nextSiblingElement("block");
		}
	}
	
	void EventActionsSet::deserialize(const QByteArray& data)
	{
		QDomDocument document;
		document.setContent(data);
		return deserialize(document.documentElement());
	}
	
	void EventActionsSet::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		#ifndef ANDROID
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;
		
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
		
		const bool isCopy(event->modifiers() & Qt::ControlModifier);
		QDrag *drag = new QDrag(event->widget());
		drag->setMimeData(mimeData());
		drag->setHotSpot(hotspot);
		drag->setPixmap(pixmap);
		
		beingDragged = true;
		Qt::DropAction dragResult(drag->exec(isCopy ? Qt::CopyAction : Qt::MoveAction));
		if (!isCopy && (dragResult != Qt::IgnoreAction))
			resetSet();
		beingDragged = false;
		#endif // ANDROID
	}
	
	void EventActionsSet::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
	{
		if (isDnDValid(event))
		{
			event->accept();
			//update();
		}
		else
			event->ignore();
	}
	
	void EventActionsSet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (isDnDValid(event))
		{
			event->accept();
			//update();
		} 
		else
			event->ignore();
	}

	void EventActionsSet::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (isDnDValid(event))
		{
			resetSet();
			deserialize(event->mimeData()->data("EventActionsSet"));
			setSoleSelection();
			update();
		}
	}
	
	bool EventActionsSet::isDnDValid(QGraphicsSceneDragDropEvent *event) const
	{
		if (!event->mimeData()->hasFormat("EventActionsSet"))
			return false;
		if (beingDragged)
			return false;
		return true;
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
} } // namespace ThymioVPL / namespace Aseba