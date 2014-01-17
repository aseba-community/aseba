#ifndef VPL_EVENT_ACTIONS_SET_H
#define VPL_EVENT_ACTIONS_SET_H

#include <QGraphicsItem>

#include "Compiler.h"

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class Block;
	class BlockHolder;
	class AddRemoveButton;
	class StateFilterEventBlock;
	
	class EventActionsSet : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		EventActionsSet(int row, bool advanced, QGraphicsItem *parent=0);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return QRectF(-2, -2, width+2, 410); }
		QRectF innerBoundingRect() const { return QRectF(0, 0, width, 336); }
		
		void setRow(int row);
		int getRow() const { return data(1).toInt(); }
		
		void addEventBlock(Block *block);
		void addActionBlock(Block *block, int number = -1);
		const bool hasEventBlock() const;
		const Block *getEventBlock() const;
		const bool hasAnyActionBlock() const;
		const Block *getActionBlock(int number) const;
		BlockHolder *getActionBlockHolder(const QString& name);
		int getBlockHolderIndex(BlockHolder *holder) const;
		
		bool isAnyAdvancedFeature() const;
		bool isEmpty() const; 
		
		void setAdvanced(bool advanced);
	
		void setErrorStatus(bool flag);
		
		bool hasActionBlock(const QString& blockName) const;
		void cleanupActionSlots();
		void repositionElements();
		
	signals:
		void contentChanged();
		
	public slots:
		void setSoleSelection();
		
	protected slots:
		void removeClicked();
		void addClicked();
	
	protected:
		BlockHolder* eventHolder;
		StateFilterEventBlock* stateFilter;
		friend class BlockHolder;
		QList<BlockHolder*> actionHolders;
		
		AddRemoveButton *deleteButton;
		AddRemoveButton *addButton;
		
		const qreal spacing;
		const qreal columnWidth;
		qreal width;
		qreal columnPos;
		int row;
		
		bool errorFlag;
		
		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
		
		virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
