#ifndef VPL_EVENT_ACTIONS_SET_H
#define VPL_EVENT_ACTIONS_SET_H

#include <QGraphicsItem>
#include <QDomElement>

#include "Compiler.h"

class QMimeData;
class QDomDocument;

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
		
		// from QGraphicsObject
		virtual QRectF boundingRect() const { return QRectF(-2, -2, width+2, 410); }
		
		// specific
		QRectF innerBoundingRect() const { return QRectF(0, 0, width, 336); }
		
		void setRow(int row);
		int getRow() const { return row; }
		
		void addEventBlock(Block *block);
		void addActionBlock(Block *block, int number = -1);
		const bool hasEventBlock() const;
		const Block *getEventBlock() const;
		const Block *getStateFilterBlock() const;
		QString getEventAndStateFilterHash() const;
		
		bool hasAnyActionBlock() const;
		bool hasActionBlock(const QString& blockName) const;
		int actionBlocksCount() const;
		const Block *getActionBlock(int number) const;
		BlockHolder *getActionBlockHolder(const QString& name);
		int getBlockHolderIndex(BlockHolder *holder) const;
		
		bool isAnyAdvancedFeature() const;
		bool isEmpty() const; 
		
		void setAdvanced(bool advanced);
		bool isAdvanced() const;
	
		void setErrorStatus(bool flag);
		
		QDomElement serialize(QDomDocument& document) const;
		void deserialize(const QDomElement& element);
		void deserializeOldFormat_1_3(const QDomElement& element);
		void deserialize(const QByteArray& data);
		
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
		// from QGraphicsObject
		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
		
		virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		// specific
		bool isDnDValid(QGraphicsSceneDragDropEvent *event) const;
		QMimeData* mimeData() const;
		
		void resetSet();
		void ensureOneEmptyActionHolderAtEnd();
	
	protected:
		BlockHolder* eventHolder;
		BlockHolder* stateFilterHolder;
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
		bool beingDragged;
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
