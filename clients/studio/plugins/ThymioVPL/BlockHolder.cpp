#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <typeinfo>

#include "BlockHolder.h"
#include "Block.h"
#include "ActionBlocks.h"
#include "Scene.h"
#include "EventActionsSet.h"
#include "ThymioVisualProgramming.h"
#include "../../../../common/utils/utils.h"

namespace Aseba { namespace ThymioVPL
{
	// A position for a block
	
	BlockHolder::BlockHolder(const QString& blockType, QGraphicsItem *parent, Block* block):
		QGraphicsObject(parent),
		blockType(blockType),
		highlight(false),
		block(0)
	{
		if (block)
			addBlock(block);
		
		assert(this->parentItem());
		setAcceptDrops(true);
	}
	
	QRectF BlockHolder::boundingRect() const
	{
		return QRectF(0,0,256,256);
	}
	
	void BlockHolder::addBlock(Block* newBlock)
	{
		// remove existing block if any
		removeBlock(false);
		
		// add the new block
		block = newBlock;
		block->setParentItem(this);
		block->setPos(0,0);
		
		// make sure that there is still a free action slot in parent and notify changes
		polymorphic_downcast<EventActionsSet*>(parentItem())->cleanupActionSlots();
		connect(block, SIGNAL(contentChanged()), SIGNAL(contentChanged()));
		emit contentChanged();
	}
	
	void BlockHolder::removeBlock(bool cleanupSet)
	{
		if (block)
		{
			disconnect(block, SIGNAL(contentChanged()), this,  SIGNAL(contentChanged()));
			
			// remove and delete block
			if (scene())
				scene()->removeItem(block);
			block->deleteLater();
			block = 0;
			
			// update the set and notify changes
			if (cleanupSet)
			{
				polymorphic_downcast<EventActionsSet*>(parentItem())->cleanupActionSlots();
				emit contentChanged();
			}
		}
	}
	
	void BlockHolder::setAdvanced(bool advanced)
	{
		// apply changes to block for advanced mode
		if (block)
			block->setAdvanced(block);
		
		// remove state filter actions if going away from advanced mode
		// FIXME: maybe we should use block name instead of typeid
		if (!advanced && block && (typeid(*block) == typeid(StateFilterActionBlock)))
			removeBlock();
	}
	
	bool BlockHolder::isAnyAdvancedFeature() const
	{
		if (block)
			return block->isAnyAdvancedFeature();
		return false;
	}
	
	bool BlockHolder::isAnyValueSet() const
	{
		if (block)
			return block->isAnyValueSet();
		return false;
	}
	
	bool BlockHolder::isEmpty() const
	{
		return block == 0;
	}
	
	void BlockHolder::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (isDnDValid(event))
		{
			highlight = true;
			event->accept();
			update();
		}
		else
			event->ignore();
	}
	
	void BlockHolder::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (isDnDValid(event))
		{
			highlight = false;
			event->accept();
			update();
		}
		else
			event->ignore();
	}
	
	void BlockHolder::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if (!isDnDValid(event))
			return;
		
		const bool advanced(polymorphic_downcast<Scene*>(scene())->getAdvanced());
		
		// create block from mime data
		Block *newBlock(Block::deserialize(event->mimeData()->data("Block"), advanced));
		assert(newBlock);
		
		// if we already have this block in the row
		EventActionsSet* eventActionsSet(polymorphic_downcast<EventActionsSet*>(parentItem()));
		if (eventActionsSet->hasActionBlock(newBlock->name))
		{
			// exchange holder places
			BlockHolder* that(eventActionsSet->getActionBlockHolder(newBlock->name));
			const int thatIndex(eventActionsSet->getBlockHolderIndex(that));
			const int thisIndex(eventActionsSet->getBlockHolderIndex(this));
			
			eventActionsSet->actionHolders[thisIndex] = that;
			eventActionsSet->actionHolders[thatIndex] = this;
			
			// add block to that holder
			that->addBlock(newBlock);
		}
		else
		{
			// add block to this holder
			addBlock(newBlock);
		}
		
		// accept and redraw
		highlight = false;
		eventActionsSet->setSoleSelection();
		if (scene())
			polymorphic_downcast<Scene*>(scene())->ensureOneEmptySetAtEnd();
		update();
	}
	
	void BlockHolder::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		const qreal borderLength(20);
		
		// get color
		QColor color(ThymioVisualProgramming::getBlockColor(blockType));
		if (!highlight)
		{
			color.setAlpha(130);
			painter->setPen(QPen(color, borderLength, Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
		}
		else
		{
			painter->setPen(QPen(color, borderLength));
		}
		
		// compute size and draw
		const QSizeF size(boundingRect().size());
		const qreal hbl(borderLength/2);
		painter->drawRoundedRect(hbl, hbl, size.width()-borderLength, size.height()-borderLength, borderLength, borderLength);
	}
	
	//! Return whether a given drag & drop event is valid for this block holder
	bool BlockHolder::isDnDValid(QGraphicsSceneDragDropEvent *event) const
	{
		if (!event->mimeData()->hasFormat("Block"))
			return false;
		if (Block::deserializeType(event->mimeData()->data("Block")) != blockType)
			return false;
		if (block && block->beingDragged)
			return false;
		return true;
	}
	
} } // namespace ThymioVPL / namespace Aseba