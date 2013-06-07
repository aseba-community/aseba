#ifndef VPL_EVENT_ACTION_PAIR_H
#define VPL_EVENT_ACTION_PAIR_H

#include <QGraphicsItem>

#include "Compiler.h"

namespace Aseba { namespace ThymioVPL
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
		
		void setRow(int row);
		int getRow() const { return data(1).toInt(); }
		
		void addEventCard(Card *event);
		void addActionCard(Card *action);
		const bool hasEventCard() const { return eventCard != 0; }
		const Card *getEventCard() const { return eventCard; }
		const bool hasActionCard() const { return actionCard != 0; }
		const Card *getActionCard() const { return actionCard; }
		
		bool isAnyStateFilter() const;
		
		void setColorScheme(QColor eventColor, QColor actionColor);
		
		void setAdvanced(bool advanced);
	
		void setErrorStatus(bool flag) { errorFlag = flag; }
		
	signals:
		void contentChanged();
		
	protected:
		void repositionElements();
	
	private:
		Card *eventCard;
		Card *actionCard;
		ThymioRemoveButton *deleteButton;
		ThymioAddButton *addButton;
		
		QColor eventCardColor;
		QColor actionCardColor;
		
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
} } // namespace ThymioVPL / namespace Aseba

#endif
