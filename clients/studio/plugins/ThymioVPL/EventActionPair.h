#ifndef VPL_EVENT_ACTION_PAIR_H
#define VPL_EVENT_ACTION_PAIR_H

#include <QGraphicsItem>

#include "ThymioIntermediateRepresentation.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class Card;
	
	class EventActionPair : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		class ThymioRemoveButton : public QGraphicsItem
		{
		public:
			ThymioRemoveButton(QGraphicsItem *parent=0);
			virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(-32, -32, 64, 64); }
		};

		class ThymioAddButton : public QGraphicsItem
		{
		public:
			ThymioAddButton(QGraphicsItem *parent=0);
			virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(-32, -32, 64, 64); }
		};
		
		EventActionPair(int row, bool advanced, QGraphicsItem *parent=0);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return advancedMode? QRectF(-2, -2, 1028+2, 410) : QRectF(-2, -2, 900+2, 410); }
		QRectF innerBoundingRect() const { return advancedMode? QRectF(0, 0, 1028, 336) : QRectF(0, 0, 900, 336); }

		void addEventButton(Card *event);
		void addActionButton(Card *action);
		void setRow(int row);
		int getRow() const { return data(1).toInt(); }
		Card *getEventButton() { return eventButton; }
		Card *getActionButton() { return actionButton; }
		
		bool eventExists() const { return eventButton == 0 ? false : true; }
		bool actionExists() const { return actionButton == 0 ? false : true; }
		bool isAnyStateFilter() const;
		
		void setColorScheme(QColor eventColor, QColor actionColor);
		
		void setAdvanced(bool advanced);
		
		ThymioIRButtonSet *getIRButtonSet() { return &buttonSetIR; }

		void setErrorStatus(bool flag) { errorFlag = flag; }
		
	signals:
		void buttonUpdated();
	
	public slots:
		void stateChanged();
		
	protected:
		void repositionElements();
	
	private:
		Card *eventButton;
		Card *actionButton;
		ThymioRemoveButton *deleteButton;
		ThymioAddButton *addButton;
		ThymioIRButtonSet buttonSetIR;
		
		QColor eventButtonColor;
		QColor actionButtonColor;
		
		bool highlightEventButton;
		bool highlightActionButton;
		bool errorFlag;
		bool advancedMode;
		qreal trans;
		qreal xpos;
		
		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
		
		virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
	};
	
	/*@}*/
}; // Aseba

#endif
