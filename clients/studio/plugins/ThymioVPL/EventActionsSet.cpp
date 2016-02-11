/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

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
#include <QMessageBox>
#include <QTimer>
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
#include "Style.h"
#include "../../../../common/utils/utils.h"
#include "UsageLogger.h"

namespace Aseba { namespace ThymioVPL
{
	//! Construct an new event-action set
	EventActionsSet::EventActionsSet(int row, bool advanced, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		event(0),
		stateFilter(0),
		blinkGraphicsItem(new QGraphicsSvgItem(":/images/vpl/blink.svg", this)),
		deleteButton(new AddRemoveButton(false, this)),
		addButton(new AddRemoveButton(true, this)),
		deleteBlockButton(new RemoveBlockButton(this)),
		highlightMode(HIGHLIGHT_NONE),
		removeBlockIndex(-1),
		dropAreaXPos(0),
		dropIndex(-1),
		currentWidth(0),
		basicWidth(0),
		totalWidth(0),
		columnPos(0),
		row(row),
		errorType(Compiler::NO_ERROR),
		beingDragged(false),
		wasDroppedTarget(false)
	{
		setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
		setAcceptHoverEvents(true);
		setAcceptedMouseButtons(Qt::LeftButton);
		setAcceptDrops(true);
		
		setAdvanced(advanced);
		
		blinkGraphicsItem->setVisible(false);
		blinkGraphicsItem->setPos(-90, (innerBoundingRect().height()-160)/2);
		clearBlinkTimer = new QTimer(this);
		connect(clearBlinkTimer, SIGNAL(timeout()), SLOT(clearBlink()));
		
		deleteBlockButton->setZValue(1);
		deleteBlockButton->setVisible(false);
		
		connect(deleteButton, SIGNAL(clicked()), SLOT(removeClicked()));
		connect(addButton, SIGNAL(clicked()), SLOT(addClicked()));
		connect(deleteBlockButton, SIGNAL(clicked()), SLOT(removeBlockClicked()));
		connect(this, SIGNAL(contentChanged()), SLOT(setSoleSelection()));
	}
	
	QRectF EventActionsSet::boundingRect() const
	{
		const qreal xmargin(qMax(Style::blockSpacing/2,4));
		// we consider the space for the error marker
		return QRectF(-xmargin, -4, totalWidth+2*xmargin, Style::blockHeight+2*Style::blockSpacing+50+4);
		
	}
	
	//! Bounding rect of visible area
	QRectF EventActionsSet::innerBoundingRect() const
	{
		return QRectF(0, 0, currentWidth, Style::blockHeight+2*Style::blockSpacing);
	}
	
	void EventActionsSet::setRow(int row) 
	{ 
		// set row and pos
		this->row = row;
		setPos(0, row*Style::eventActionsSetRowStep);
		
		// update scene rect
		Scene* vplScene(dynamic_cast<Scene*>(scene()));
		if (vplScene)
			vplScene->recomputeSceneRect();
	}
	
	//! Add an event block, call the event holder
	void EventActionsSet::addEventBlock(Block *block) 
	{
		setBlock(event, block);
		emit contentChanged();
		USAGE_LOG(logAddEventBlock(this->row,block));
		emit undoCheckpoint();
	}
	
	//! Add an action block, optionally at a given position (which must be valid, i.e. >= 0)
	void EventActionsSet::addActionBlock(Block *block, int number) 
	{
		addActionBlockNoEmit(block, number);
		emit contentChanged();
		USAGE_LOG(logAddActionBlock(this->row, block, number));
		emit undoCheckpoint();
	}
	
	//! Return whether the event holder has a block 
	const bool EventActionsSet::hasEventBlock() const
	{
		return event != 0;
	}
	
	//! Return the block in the event holder
	const Block *EventActionsSet::getEventBlock() const
	{
		return event;
	}
	
	//! Return the block in the state filter holder if it exists, 0 otherwise
	const Block *EventActionsSet::getStateFilterBlock() const
	{
		return stateFilter;
	}
	
	//! Return a string representing the event and the state filter, suitable for checking code duplications
	QString EventActionsSet::getEventAndStateFilterHash() const
	{
		// create document
		QDomDocument document("sethash");
		QDomElement element(document.createElement("sethash"));
		
		// add event
		if (event)
			element.appendChild(event->serialize(document));
		
		// add state
		if (stateFilter)
			element.appendChild(stateFilter->serialize(document));
		
		// return document as a string
		document.appendChild(element);
		return document.toString();
	}
	
	//! Return whether the action holders have any block
	bool EventActionsSet::hasAnyActionBlock() const
	{
		return actions.size() >= 1;
	}
	
	//! Return whether a given block name is already present in this set
	bool EventActionsSet::hasActionBlock(const QString& blockName) const
	{
		foreach (Block* action, actions)
		{
			if (action->name == blockName)
				return true;
		}
		return false;
	}
	
	//! Return the index of an action block given its name, -1 if not present
	int EventActionsSet::getActionBlockIndex(const QString& blockName) const
	{
		for (int i=0; i<actions.size(); ++i)
			if (actions[i]->name == blockName)
				return i;
		return -1;
	}
	
	//! Return an action block given its name, 0 if not present, const version
	const Block *EventActionsSet::getActionBlock(const QString& blockName) const
	{
		foreach (const Block* action, actions)
		{
			if (action->name == blockName)
				return action;
		}
		return 0;
	}
	
	//! Return an action block given its name, 0 if not present
	Block *EventActionsSet::getActionBlock(const QString& blockName)
	{
		foreach (Block* action, actions)
		{
			if (action->name == blockName)
				return action;
		}
		return 0;
	}
	
	//! Return the block in one of the action holders
	const Block *EventActionsSet::getActionBlock(int index) const
	{
		return actions.at(index);
	}
	
	//! Return how many action blocks this set has (with possible empty ones)
	int EventActionsSet::actionBlocksCount() const
	{
		return actions.size();
	}
	
	//! Does this set uses any feature from the advanced mode?
	bool EventActionsSet::isAnyAdvancedFeature() const
	{
		// is the event using an advanced feature
		if (event && (event->isAnyAdvancedFeature()))
			return true;
		
		// is the state filter set?
		if (stateFilter && stateFilter->isAnyValueSet())
			return true;
	
		// is any action using an advanced feature
		foreach (Block* action, actions)
		{
			if (action->isAnyAdvancedFeature())
				return true;
		}
		
		return false;
	}
	
	//! Does the action list contains a set state?
	bool EventActionsSet::hasSetStateAction() const
	{
		foreach (Block* action, actions)
		{
			if (action->getName() == "setstate")
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
		// Note: we do not emit contentChanged on purpose, as we know that the caller will recompile
		
		// switch event and actions to given mode
		if (event)
		{
			if (event->isAdvancedBlock())
				setBlock(event, 0);
			else
				event->setAdvanced(advanced);
		}
		// delete action if not valid any more
		int i = 0;
		while (i<actions.size())
		{
			if (!advanced && actions[i]->isAdvancedBlock())
			{
				Block* action(actions[i]);
				actions.removeAt(i);
				action->deleteLater();
			}
			else
			{
				actions[i]->setAdvanced(advanced);
				++i;
			}
		}
		
		// create and delete state filter
		if (advanced && !stateFilter)
		{
			// switching to advanced mode, create state filter
			setBlock(stateFilter, new StateFilterCheckBlock());
		}
		else if (!advanced && stateFilter)
		{
			// switching from advanced mode, delete state filter
			setBlock(stateFilter, 0);
		}
		
		// we might have to reposition some elements, because we might have removed some actions
		repositionElements();
	}
	
	//! Return whether we are in advanced mode by testing whether stateFilter is not null
	bool EventActionsSet::isAdvanced() const
	{
		return stateFilter != 0;
	}
	
	//! Set whether or not this set has an error
	void EventActionsSet::setErrorType(Compiler::ErrorType errorType)
	{
		if (this->errorType != errorType)
		{
			this->errorType = errorType;
			update();
		}
	}
	
	//! Blink the set
	void EventActionsSet::blink()
	{
		blinkGraphicsItem->setVisible(true);
		update();
		clearBlinkTimer->start(300);
	}
	
	//! Return the content compressed into a uint16 vector, to be used as debug events
	QVector<quint16> EventActionsSet::getContentCompressed() const
	{
		// invalid sets have no content
		if ((!event) || actions.empty())
			return QVector<quint16>();
		
		// the first word of the content is the number of the set
		QVector<quint16> content(1, row);
		// the second word is the number of blocks in this set
		content.push_back(1 + (stateFilter ? 1 : 0) + actions.size());
		
		// the next words are the types of the blocks, as uint4, compressed 4-by-4 into uint16s, big-endian
		quint16 curWord(event->getNameAsUInt4());
		unsigned bitUsed(4);
		if (stateFilter)
		{
			curWord |= (stateFilter->getNameAsUInt4() << bitUsed);
			bitUsed += 4;
		}
		foreach (Block* action, actions)
		{
			if (bitUsed == 16)
			{
				content += curWord;
				curWord = 0;
				bitUsed = 4;
			}
			curWord |= (action->getNameAsUInt4() << bitUsed);
			bitUsed += 4;
		}
		content += curWord;
		
		// then comes the data from the blocks
		content += event->getValuesCompressed();
		if (stateFilter)
			content += stateFilter->getValuesCompressed();
		foreach (Block* action, actions)
			content += action->getValuesCompressed();
		
		return content;
	}
	
	//! Clear set to its default status
	void EventActionsSet::resetSet()
	{
		// disconnect the selection setting mechanism, as we do not want a reset to select this set
		disconnect(this, SIGNAL(contentChanged()), this, SLOT(setSoleSelection()));
		
		// delete event
		setBlock(event, 0);
		
		// clear filter
		if (stateFilter)
			stateFilter->resetValues();
		
		// delete all actions
		foreach (Block* action, actions)
			action->deleteLater();
		actions.clear();
		
		repositionElements();
		
		connect(this, SIGNAL(contentChanged()), SLOT(setSoleSelection()));
	}
	
	//! Recompute the positions of the different elements for this set
	void EventActionsSet::repositionElements()
	{
		// constants and initialization
		const qreal ypos = Style::blockSpacing;
		qreal xpos = Style::blockSpacing;
		
		// add event block holder
		if (event)
			event->setPos(xpos, ypos);
		xpos += Style::blockWidth + Style::blockSpacing;
		
		// if state filter holder, add it
		if (stateFilter)
		{
			stateFilter->setPos(xpos, ypos);
			xpos += Style::blockWidth + Style::blockSpacing;
		}
		
		// add column
		columnPos = xpos;
		xpos += Style::eventActionsSetColumnWidth + Style::blockSpacing;
		dropAreaXPos = xpos;
		
		// add every action holder
		foreach (Block* action, actions)
		{
			action->setPos(xpos, ypos);
			xpos += Style::blockWidth + Style::blockSpacing;
		}
		
		// if we have no action, we leave space for one
		xpos += (actions.empty() ? Style::blockWidth + Style::blockSpacing : 0);
		
		// leave space for delete button
		xpos += deleteButton->boundingRect().width() + Style::blockSpacing;
		deleteButton->setPos(xpos-deleteButton->boundingRect().width()-Style::blockSpacing, Style::blockSpacing);
		
		// add button, under column
		addButton->setPos(columnPos+(Style::eventActionsSetColumnWidth-64)/2, innerBoundingRect().height() + Style::blockSpacing/2);
		
		// store width
		basicWidth = xpos;
		currentWidth = basicWidth;
		// handle case when we cannot add more
		if (actions.empty())
			totalWidth = basicWidth;
		else if (actions.size() == 6)
			totalWidth = basicWidth;
		else
			totalWidth = basicWidth + Style::blockWidth + Style::blockSpacing;
		//totalWidth = (actions.empty() ? basicWidth : basicWidth + Style::blockWidth + Style::blockSpacing);
		
		// clear highlight
		highlightMode = HIGHLIGHT_NONE;
		
		// compute global position
		setPos(0, row*Style::eventActionsSetRowStep);
		
		// notify scene of changes
		prepareGeometryChange();
		
		// update scene rect
		Scene* vplScene(dynamic_cast<Scene*>(scene()));
		if (vplScene)
			vplScene->recomputeSceneRect();
	}
	
	//! Recompute the positions of the actions for this set
	void EventActionsSet::updateActionPositions(qreal dropXPos)
	{
		const qreal ypos = Style::blockSpacing;
		qreal xpos = columnPos + Style::eventActionsSetColumnWidth + Style::blockSpacing;
		
		dropAreaXPos = xpos + actions.size() * (Style::blockWidth + Style::blockSpacing);
		for (int i=0; i<actions.size(); ++i)
		{
			if (dropXPos > xpos && dropXPos < xpos + Style::blockWidth + Style::blockSpacing)
			{
				dropAreaXPos = xpos;
				xpos += Style::blockSpacing + Style::blockWidth;
			}
			actions[i]->setPos(xpos, ypos);
			xpos += Style::blockSpacing + Style::blockWidth;
		}
	}
	
	//! Recompute the drop index for a given position
	void EventActionsSet::updateDropIndex(qreal dropXPos)
	{
		const int oldIndex(dropIndex);
		
		// start of actions
		dropXPos -= columnPos + Style::eventActionsSetColumnWidth + Style::blockSpacing;
		if (dropXPos < 0)
			dropIndex = -1;
		else
			dropIndex = dropXPos / (Style::blockSpacing + Style::blockWidth);
		
		// if action already exists, restrict index
		if (highlightMode == HIGHLIGHT_EXISTING_ACTION)
		{
			dropIndex = qMax(0, qMin(dropIndex, actions.size()-1));
			if (dropIndex != oldIndex)
				update();
		}
	}
	
	//! Select only this set and nothing else
	void EventActionsSet::setSoleSelection()
	{
		if (scene())
		{
			scene()->clearSelection();
			setSelected(true);
		}
	}
	
	void EventActionsSet::removeClicked()
	{
		polymorphic_downcast<Scene*>(scene())->removeSet(row);
	}
	
	void EventActionsSet::addClicked()
	{
		polymorphic_downcast<Scene*>(scene())->insertSet(row+1);
	}
	
	void EventActionsSet::removeBlockClicked()
	{
		if (removeBlockIndex == -2)
		{
			removeBlock(event);
			emit contentChanged();
			emit undoCheckpoint();
		}
		else if (removeBlockIndex >= 0)
		{
			removeBlock(actions.at(removeBlockIndex));
			emit contentChanged();
			emit undoCheckpoint();
		}
	}
	
	void EventActionsSet::clearBlink()
	{
		blinkGraphicsItem->setVisible(false);
		update();
	}
	
	QMimeData* EventActionsSet::mimeData() const
	{
		// create a DOM document and serialize the content of this block in it
		QDomDocument document("set");
		document.appendChild(serialize(document));
		
		// create a MIME data for this block
		QMimeData *mime = new QMimeData;
		mime->setData("EventActionsSet", document.toByteArray());
		
		return mime;
	}
	
	QDomElement EventActionsSet::serialize(QDomDocument& document) const
	{
		// create element
		QDomElement element = document.createElement("set");
		
		// event
		if (event)
			element.appendChild(event->serialize(document));
		// state
		if (stateFilter)
			element.appendChild(stateFilter->serialize(document));
		// actions
		foreach (Block* action, actions)
			element.appendChild(action->serialize(document));
		
		return element;
	}
	
	void EventActionsSet::deserialize(const QDomElement& element)
	{
		// this function assumes an empty set
		const bool advanced(stateFilter != 0);
		assert(isEmpty());
		
		// iterate on all stored block
		QDomElement blockElement(element.firstChildElement("block"));
		while (!blockElement.isNull())
		{
			// following type ...
			const QString& type(blockElement.attribute("type"));
			const QString& name(blockElement.attribute("name"));
			
			// deserialize block
			Block* block(Block::deserialize(blockElement, advanced));
			if (!block)
			{
				QMessageBox::warning(0,tr("Loading"),
					tr("Error in XML source file at %0:%1: cannot create block %2").arg(blockElement.lineNumber()).arg(blockElement.columnNumber()).arg(name));
				return;
			}
			
			if (type == "event")
			{
				setBlock(event, block);
			}
			else if (type == "state")
			{
				assert(advanced);
				setBlock(stateFilter, block);
			}
			else if (type == "action")
			{
				addActionBlockNoEmit(block);
			}
			else
			{
				QMessageBox::warning(0,tr("Loading"),
					tr("Error in XML source file at %0:%1: unknown block type %2").arg(blockElement.lineNumber()).arg(blockElement.columnNumber()).arg(type));
				return;
			}
			
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
	
	//! Remove this block for this set if present, does not select the set
	void EventActionsSet::removeBlock(Block* block)
	{
		// disconnect the selection setting mechanism, as we do not want this removal to select our set
		disconnect(this, SIGNAL(contentChanged()), this, SLOT(setSoleSelection()));
		
		if (block == event)
		{
			setBlock(event, 0);
		}
		else
		{
			int i = 0;
			while (i<actions.size())
			{
				if (actions[i] == block)
				{
					actions.removeAt(i);
					block->deleteLater();
				}
				else
					++i;
			}
		}
		
		repositionElements();
		
		connect(this, SIGNAL(contentChanged()), this, SLOT(setSoleSelection()));
	}
	
	//! Add an action block, optionally at a given position (which must be valid, i.e. >= 0), do not emit changes or undo checkpoint
	void EventActionsSet::addActionBlockNoEmit(Block *block, int number) 
	{
		// add block
		if (number == -1)
		{
			actions.push_back(0);
			setBlock(actions.last(), block);
		}
		else
		{
			number = qMin(number, actions.size());
			actions.insert(number, 0);
			setBlock(actions[number], block);
		}
	}
	
	//! Replace an existing block of this set with a new one
	void EventActionsSet::setBlock(Block*& blockPointer, Block* newBlock)
	{
		// remove existing block
		if (blockPointer)
		{
			disconnect(blockPointer, SIGNAL(contentChanged()), this,  SIGNAL(contentChanged()));
			
			// remove and delete block
			blockPointer->deleteLater();
		}
		
		// add new one
		blockPointer = newBlock;
		
		// if non zero, add it to the scene
		if (blockPointer)
		{
			blockPointer->setParentItem(this);
			blockPointer->setPos(0,0);
			
			connect(blockPointer, SIGNAL(contentChanged()), SIGNAL(contentChanged()));
			connect(blockPointer, SIGNAL(undoCheckpoint()), SIGNAL(undoCheckpoint()));
		}
		
		// change element positions
		repositionElements();
	}
	
	//! Update the delete block button position and visibility while hovering
	void EventActionsSet::hoverMoveEvent(QGraphicsSceneHoverEvent * hoverEvent)
	{
		const int x(hoverEvent->pos().x());
		const int y(hoverEvent->pos().y());
		const int dw(Style::addRemoveButtonWidth/2);
		const int dh(Style::addRemoveButtonHeight/2);
		const int dl(0);
		const int dr(Style::blockWidth+dw);
		const int bw(dr-dl);
		const int bstride(Style::blockWidth+Style::blockSpacing);
		
		// if out of bound on y, hide and return
		if ((y < Style::blockSpacing - dh) || (y >= Style::blockSpacing + Style::blockHeight))
		{
			deleteBlockButton->hide();
			removeBlockIndex = -1;
			return;
		}
		
		const qreal actionsXPos(columnPos + Style::eventActionsSetColumnWidth + Style::blockSpacing);
		const int actionsStartPos(actionsXPos + dl);
		const int actionsStopPos(actionsXPos + actions.size() * bstride + dr);
		
		// delete button
		if (
			event && 
			(x >= event->pos().x() + dl) &&
			(x < event->pos().x() + dr)
		)
		{
			deleteBlockButton->setPos(event->pos().x() + Style::blockWidth - dw, Style::blockSpacing - dh);
			deleteBlockButton->show();
			removeBlockIndex = -2;
		}
		else if (
			(x >= actionsStartPos) &&
			(x < actionsStopPos) &&
			(fmod(x-dl-actionsXPos, bstride) < bw) &&
			((x-dl-actionsXPos) / bstride < actions.size())
		)
		{
			removeBlockIndex = (x-dl-actionsXPos) / bstride;
			deleteBlockButton->setPos(actionsXPos + removeBlockIndex * bstride + Style::blockWidth - dw, Style::blockSpacing - dh);
			deleteBlockButton->show();
		}
		else
		{
			deleteBlockButton->hide();
			removeBlockIndex = -1;
		}
	}
	
	//! Disable delete block button
	void EventActionsSet::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
	{
		deleteBlockButton->hide();
		removeBlockIndex = -1;
	}
	
	void EventActionsSet::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
	{
		// drag and drop start
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
		QMimeData* myData(mimeData());
		drag->setHotSpot(hotspot);
		drag->setPixmap(pixmap);
		
		USAGE_LOG(logActionSetDrag(this->row,event, drag));
		
		beingDragged = true;
		Qt::DropAction dragResult(drag->exec(isCopy ? Qt::CopyAction : Qt::MoveAction));
		if (dragResult != Qt::IgnoreAction)
		{
			// find on which set the drag was droppes
			EventActionsSet* target(0);
			Scene* vplScene(polymorphic_downcast<Scene*>(scene()));
			for (Scene::SetItr it(vplScene->setsBegin()); it != vplScene->setsEnd(); ++it)
			{
				if ((*it)->wasDroppedTarget)
				{
					target = *it;
					target->wasDroppedTarget = false;
				}
			}
			// if it is this one, there is a bug
			assert(target != this);
			if (!isCopy)
			{
				// copy target content to this
				resetSet();
				deserialize(target->mimeData()->data("EventActionsSet"));
				repositionElements();
			}
			// copy this content to target
			target->resetSet();
			target->deserialize(myData->data("EventActionsSet"));
			target->repositionElements();
			target->setSoleSelection();
			
			// make sure that we have an empty set at the end
			polymorphic_downcast<Scene*>(scene())->ensureOneEmptySetAtEnd();
			
			// disconnect the selection setting mechanism, emit, and then re-enable
			disconnect(this, SIGNAL(contentChanged()), this, SLOT(setSoleSelection()));
			emit contentChanged();
			emit undoCheckpoint();
			connect(this, SIGNAL(contentChanged()), SLOT(setSoleSelection()));
		}
		delete myData;
		beingDragged = false;
		#endif // ANDROID
	}
	
	void EventActionsSet::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
	{
		if (isDnDValid(event))
		{
			event->accept();
			setVisualFromEvent(event);
		}
		else
			event->ignore();
	}
	
	void EventActionsSet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (isDnDValid(event))
		{
			event->accept();
			clearVisualFromEvent(event);
			updateActionPositions(-1);
		} 
		else
			event->ignore();
	}
	
	void EventActionsSet::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
	{
		// we assume the event has been accepted
		assert(isDnDValid(event));
		
		if (isDnDAction(event))
		{
			if (isDnDNewAction(event))
				updateActionPositions(event->pos().x());
			updateDropIndex(event->pos().x());
		}
	}

	void EventActionsSet::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (isDnDValid(event))
		{
			const bool advanced(polymorphic_downcast<Scene*>(scene())->getAdvanced());
			
			USAGE_LOG(logEventActionSetDrop(this->row, event));
			
			// It is a set
			if (event->mimeData()->hasFormat("EventActionsSet"))
			{
				wasDroppedTarget = true;
				// we just take note that this set was the target of the drop, as the drop
				// initiator will take care of changing our content
			}
			// It is a block
			else if (event->mimeData()->hasFormat("Block"))
			{
				// create block from mime data
				Block *newBlock(Block::deserialize(event->mimeData()->data("Block"), advanced));
				assert(newBlock);
				
				if (newBlock->type == "event")
				{
					setBlock(this->event, newBlock);
				}
				else if (newBlock->type == "state")
				{
					setBlock(this->stateFilter, newBlock);
				}
				else if (newBlock->type == "action")
				{
					Block* actionBlock(getActionBlock(newBlock->name));
					if (actionBlock)
					{
						assert(actionBlock->beingDragged);
						
						// tell the block not to delete itself
						actionBlock->keepAfterDrop = true;
						
						// exchange the blocks
						const int thatIndex(getActionBlockIndex(newBlock->name));
						assert(dropIndex < actions.size());
						assert(thatIndex < actions.size());
						qSwap(actions[dropIndex], actions[thatIndex]);
						repositionElements();
						
						// delete the newly created one
						delete newBlock;
					}
					else
					{
						addActionBlockNoEmit(newBlock, dropIndex);
					}
				}
				else
					abort();
				setSoleSelection();
			}
			else
				abort();
			
			if (scene())
				polymorphic_downcast<Scene*>(scene())->ensureOneEmptySetAtEnd();
		}
	}
	
	//! Set the visual elements of this set given the stuff being dragged in
	void EventActionsSet::setVisualFromEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (event->mimeData()->hasFormat("EventActionsSet"))
		{
			highlightMode = HIGHLIGHT_SET;
		}
		else if (isDnDAction(event))
		{
			if (isDnDNewAction(event))
			{
				currentWidth = totalWidth;
				deleteButton->setPos(currentWidth-deleteButton->boundingRect().width()-Style::blockSpacing, Style::blockSpacing);
				highlightMode = HIGHLIGHT_NEW_ACTION;
			}
			else
			{
				highlightMode = HIGHLIGHT_EXISTING_ACTION;
			}
			updateDropIndex(event->pos().x());
		}
		else if (Block::deserializeType(event->mimeData()->data("Block")) == "event")
		{
			highlightMode = HIGHLIGHT_EVENT;
		}
		update();
	}
	
	//! Remove the visual elements that were linked to the stuff being dragged in
	void EventActionsSet::clearVisualFromEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (isDnDNewAction(event))
		{
			currentWidth = basicWidth;
			deleteButton->setPos(currentWidth-deleteButton->boundingRect().width()-Style::blockSpacing, Style::blockSpacing);
		}
		highlightMode = HIGHLIGHT_NONE;
		update();
	}
	
	//! Return whether the proposed drag&drop is valid (i.e. acceptable)
	bool EventActionsSet::isDnDValid(QGraphicsSceneDragDropEvent *event) const
	{
		if (beingDragged)
			return false;
		if (event->mimeData()->hasFormat("EventActionsSet"))
			return true;
		if (event->mimeData()->hasFormat("Block"))
		{
			if (Block::deserializeType(event->mimeData()->data("Block")) == "action")
			{
				// only allow to d&d existing actions from within the same set
				const QString& name(Block::deserializeName(event->mimeData()->data("Block")));
				if (hasActionBlock(name) && !getActionBlock(name)->beingDragged)
					return false;
				else
					return true;
			}
			else if (Block::deserializeType(event->mimeData()->data("Block")) == "event")
			{
				if (this->event && this->event->beingDragged)
					return false;
				else
					return true;
			}
			else if (Block::deserializeType(event->mimeData()->data("Block")) == "state")
			{
				if (this->stateFilter && this->stateFilter->beingDragged)
					return false;
				else
					return true;
			}
			else
				return true;
		}
		return false;
	}
	
	//! Return whether the drag&drop is about an action
	bool EventActionsSet::isDnDAction(QGraphicsSceneDragDropEvent *event) const
	{
		if (!event->mimeData()->hasFormat("Block"))
			return false;
		if (Block::deserializeType(event->mimeData()->data("Block")) != "action")
			return false;
		return true;
	}
	
	//! Return whether the drag&drop is about an action not currently present in the set
	bool EventActionsSet::isDnDNewAction(QGraphicsSceneDragDropEvent *event) const
	{
		if (!isDnDAction(event))
			return false;
		if (hasActionBlock(Block::deserializeName(event->mimeData()->data("Block"))))
			return false;
		return true;
	}
	
	void EventActionsSet::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		const qreal borderWidth(Style::blockDropAreaBorderWidth);
		//const qreal borderWidth(Style::eventActionsSetCornerSize);
		int colorId = (isSelected() ? 1 : 0);
		
		// if we are the last one, do not show buttons
		Scene* vplScene(dynamic_cast<Scene*>(scene()));
		if (vplScene)
		{
			assert(vplScene->setsBegin() != vplScene->setsEnd());
			//const bool isFirst(*vplScene->setsBegin() == this);
			const bool isLast(*(vplScene->setsEnd()-1) == this);
			addButton->setVisible(!isLast);
			deleteButton->setVisible(!isLast);
			if (stateFilter)
				stateFilter->setVisible(!isEmpty());
			// if last and not drop target, draw dotted area
			//if (/*isLast && */(highlightMode == HIGHLIGHT_NONE))
			//if (!isFirst && isEmpty() && (highlightMode == HIGHLIGHT_NONE))
			/*if (false)
			{
				const qreal hb(borderWidth/2);
				painter->setBrush(Qt::transparent);
				painter->setPen(QPen(Style::eventActionsSetBackgroundColors[0], borderWidth, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
				//painter->drawRoundedRect(innerBoundingRect().adjusted(hb,hb,-hb,-hb), borderWidth, borderWidth);
				painter->drawRoundedRect(innerBoundingRect().adjusted(hb,hb,-hb,-hb), Style::eventActionsSetCornerSize, Style::eventActionsSetCornerSize);
				return;
			}*/
		}
		
		// extension drop area
		if (!actions.empty())
		{
			const qreal hb(borderWidth/2);
			painter->setBrush(Qt::NoBrush);
			painter->setPen(QPen(Style::eventActionsSetBackgroundColors[0], borderWidth, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
			const QRectF rect(QRectF(totalWidth-Style::blockWidth-Style::blockSpacing,0,Style::blockWidth+Style::blockSpacing,innerBoundingRect().height()).adjusted(-hb,hb,-hb,-hb));
			painter->drawRoundedRect(rect, Style::eventActionsSetCornerSize, Style::eventActionsSetCornerSize);
		}
		
		// background
		//if (errorType != Compiler::NO_ERROR)
		if (errorType == Compiler::DUPLICATED_EVENT)
			painter->setPen(QPen(Qt::red, 8));
		else if (blinkGraphicsItem->isVisible())
			painter->setPen(QPen(QColor("#f5e800"), 8));
		else
			painter->setPen(Qt::NoPen);
		
		painter->setBrush(Style::eventActionsSetBackgroundColors[colorId]);
		if (highlightMode == HIGHLIGHT_SET)
			painter->drawRoundedRect(-Style::blockSpacing/2,0,currentWidth+Style::blockSpacing,Style::blockHeight+2*Style::blockSpacing,borderWidth,borderWidth);
		else
			painter->drawRoundedRect(innerBoundingRect(), Style::eventActionsSetCornerSize, Style::eventActionsSetCornerSize);
		
		// event drop area
		if (!event)
		{
			drawBlockArea(painter, "event", QPointF(Style::blockSpacing, Style::blockSpacing), highlightMode == HIGHLIGHT_EVENT);
		}
		else
		{
			// if event drag&drop
			if (highlightMode == HIGHLIGHT_EVENT)
			{
				painter->setBrush(Style::blockCurrentColor("event"));
				painter->setPen(Qt::NoPen);
				painter->drawRoundedRect(Style::blockSpacing, Style::blockSpacing/2, Style::blockWidth, Style::blockSpacing+Style::blockHeight, borderWidth, borderWidth);
			}
		}
		
		// column
		painter->setPen(Qt::NoPen);
		painter->setBrush(Style::eventActionsSetForegroundColors[colorId]);
		const qreal ymiddle(innerBoundingRect().height()/2);
		painter->drawEllipse(columnPos, ymiddle-2*Style::eventActionsSetColumnWidth, Style::eventActionsSetColumnWidth, Style::eventActionsSetColumnWidth);
		painter->drawEllipse(columnPos, ymiddle+Style::eventActionsSetColumnWidth, Style::eventActionsSetColumnWidth, Style::eventActionsSetColumnWidth);
		
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
		
		// action drop area
		if ((actions.empty() || currentWidth == totalWidth) && (actions.size() != 6))
			drawBlockArea(painter, "action", QPointF(dropAreaXPos, Style::blockSpacing), highlightMode == HIGHLIGHT_NEW_ACTION);
		
		// if inner drag&drop, show drop indicator
		if (highlightMode == HIGHLIGHT_EXISTING_ACTION)
		{
			qreal xpos(columnPos + Style::eventActionsSetColumnWidth + Style::blockSpacing);
			xpos += dropIndex * (Style::blockSpacing + Style::blockWidth);
			painter->setBrush(Style::blockCurrentColor("action"));
			painter->setPen(Qt::NoPen);
			painter->drawRoundedRect(xpos, Style::blockSpacing/2, Style::blockWidth, Style::blockSpacing+Style::blockHeight, borderWidth, borderWidth);
		}
		
		// line number
		painter->setPen(Style::eventActionsSetForegroundColors[colorId]);
		painter->setFont(QFont("Arial", 50));
		painter->drawText(QRect(currentWidth-Style::blockSpacing-128, Style::blockHeight+Style::blockSpacing-128, 128, 128), Qt::AlignRight|Qt::AlignBottom, QString("%0").arg(getRow()+1));
		
		// error marker
		painter->setPen(Qt::NoPen);
		//painter->setBrush(Qt::red);
		painter->setBrush(QColor(Qt::red).lighter());
		if (errorType == Compiler::MISSING_EVENT)
		{
			const qreal x(Style::blockSpacing + Style::blockWidth / 2);
			const qreal y(Style::blockSpacing + Style::blockHeight + Style::blockSpacing + 20);
			const QPointF points[3] = {
				QPointF(x - 40, y + 30),
				QPointF(x, y),
				QPointF(x + 40, y + 30),
			};
			painter->drawConvexPolygon(points, 3);
		}
		else if (errorType == Compiler::MISSING_ACTION)
		{
			const qreal x(dropAreaXPos + Style::blockWidth / 2);
			const qreal y(Style::blockSpacing + Style::blockHeight + Style::blockSpacing + 20);
			const QPointF points[3] = {
				QPointF(x - 40, y + 30),
				QPointF(x, y),
				QPointF(x + 40, y + 30),
			};
			painter->drawConvexPolygon(points, 3);
		}
	}
	
	//! Draw the drop area for block
	void EventActionsSet::drawBlockArea(QPainter * painter, const QString& type, const QPointF& pos, bool highlight) const
	{
		// get color
		const qreal borderWidth(Style::blockDropAreaBorderWidth);
		QColor color(Style::blockCurrentColor(type));
		if (!highlight)
		{
			qreal h,s,v,a;
			color.getHsvF(&h, &s, &v, &a);
			s *= Style::blockDropAreaSaturationFactor;
			v = std::min<double>(1.0, v*Style::blockDropAreaValueFactor);
			color.setHsvF(h,s,v,a);
			//color.setAlpha(130);
			painter->setPen(QPen(color, borderWidth, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
		}
		else
		{
			painter->setPen(QPen(color, borderWidth));
		}
		painter->setBrush(Qt::transparent);
		
		// compute size and draw
		const qreal halfBorderWidth(borderWidth/2);
		painter->drawRoundedRect(pos.x()+halfBorderWidth, pos.y()+halfBorderWidth, Style::blockWidth-borderWidth, Style::blockHeight-borderWidth, borderWidth, borderWidth);
	}
} } // namespace ThymioVPL / namespace Aseba
