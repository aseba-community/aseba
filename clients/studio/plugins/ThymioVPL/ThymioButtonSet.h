#ifndef THYMIO_BUTTON_SET_H
#define THYMIO_BUTTON_SET_H

#include <QGraphicsItem>

#include "ThymioIntermediateRepresentation.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class ThymioButton;
	
	class ThymioButtonSet : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		class ThymioRemoveButton : public QGraphicsItem
		{
		public:
			ThymioRemoveButton(QGraphicsItem *parent=0);
			virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(-64, -64, 128, 128); }
		};

		class ThymioAddButton : public QGraphicsItem
		{
		public:
			ThymioAddButton(QGraphicsItem *parent=0);
			virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(-32, -32, 64, 64); }
		};
		
		ThymioButtonSet(int row, bool advanced, QGraphicsItem *parent=0);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return advancedMode? QRectF(-2, -2, 1064+2, 400) : QRectF(-2, -2, 1000+2, 400); }
		QRectF innerBoundingRect() const { return advancedMode? QRectF(0, 0, 1064, 336) : QRectF(0, 0, 1000, 336); }

		void addEventButton(ThymioButton *event);
		void addActionButton(ThymioButton *action);
		void setRow(int row);
		int getRow() { return data(1).toInt(); }
		ThymioButton *getEventButton() { return eventButton; }
		ThymioButton *getActionButton() { return actionButton; }
		
		bool eventExists() { return eventButton == 0 ? false : true; }
		bool actionExists() { return actionButton == 0 ? false : true; }
		void setColorScheme(QColor eventColor, QColor actionColor);
		
		void setAdvanced(bool advanced);
		
		ThymioIRButtonSet *getIRButtonSet() { return &buttonSetIR; }

		void setErrorStatus(bool flag) { errorFlag = flag; }
		
	signals:
		void buttonUpdated();
	
	public slots:
		void stateChanged();
	
	private:
		ThymioButton *eventButton;
		ThymioButton *actionButton;
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
