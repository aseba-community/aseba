#ifndef VPL_EVENT_ACTION_PAIR_H
#define VPL_EVENT_ACTION_PAIR_H

#include <QGraphicsItem>

#include "Compiler.h"

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class Block;
	class AddRemoveButton;
	
	class EventActionsSet : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		EventActionsSet(int row, bool advanced, QGraphicsItem *parent=0);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return advancedMode? QRectF(-2, -2, 1028+2, 410) : QRectF(-2, -2, 900+2, 410); }
		QRectF innerBoundingRect() const { return advancedMode? QRectF(0, 0, 1028, 336) : QRectF(0, 0, 900, 336); }
		
		void setRow(int row);
		int getRow() const { return data(1).toInt(); }
		
		void removeEventBlock();
		void addEventBlock(Block *event);
		void removeActionBlock();
		void addActionBlock(Block *action);
		const bool hasEventBlock() const { return eventBlock != 0; }
		const Block *getEventBlock() const { return eventBlock; }
		const bool hasActionBlock() const { return actionBlock != 0; }
		const Block *getActionBlock() const { return actionBlock; }
		
		bool isAnyAdvancedFeature() const;
		bool isEmpty() const; 
		
		void setColorScheme(QColor eventColor, QColor actionColor);
		
		void setAdvanced(bool advanced);
	
		void setErrorStatus(bool flag);
		
		void repositionElements();
		
	signals:
		void contentChanged();
		
	public slots:
		void setSoleSelection();
		
	protected slots:
		void removeClicked();
		void addClicked();
	
	private:
		Block *eventBlock;
		Block *actionBlock;
		AddRemoveButton *deleteButton;
		AddRemoveButton *addButton;
		
		QColor eventBlockColor;
		QColor actionBlockColor;
		
		bool highlightEventButton;
		bool highlightActionButton;
		bool errorFlag;
		bool advancedMode;
		qreal trans;
		qreal xpos;
		
		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
		
		virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
