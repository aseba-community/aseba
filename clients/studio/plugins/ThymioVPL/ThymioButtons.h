#ifndef THYMIO_BUTTONS_H
#define THYMIO_BUTTONS_H

#include <QGraphicsItem>
#include <QPushButton>

#include "ThymioIntermediateRepresentation.h"

class QSlider;
class QTimeLine;
class QMimeData;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class Card;
	
	class ThymioClickableButton : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		enum ButtonType 
		{
			CIRCULAR_BUTTON = 0,
			TRIANGULAR_BUTTON,
			RECTANGULAR_BUTTON,
			QUARTER_CIRCLE_BUTTON
		};
		
		//! Create a button with initially one state
		ThymioClickableButton (const QRectF rect, const ButtonType type, QGraphicsItem *parent=0, const QColor& initBrushColor = Qt::white, const QColor& initPenColor = Qt::black);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return boundingRectangle; }

		int getValue() const { return curState; }
		void setValue(int state) { curState = state; update(); }
		void setToggleState(bool state) { toggleState = state; update(); }

		void addState(const QColor& brushColor = Qt::white, const QColor& penColor = Qt::black);

		void addSibling(ThymioClickableButton *s) { siblings.push_back(s); }
	
	signals:
		void stateChanged();
	
	protected:
		const ButtonType buttonType;
		const QRectF boundingRectangle;
		
		int curState;
		bool toggleState;
		
		//! a (brush,pen) color pair
		typedef QPair<QColor, QColor> ColorPair;
		QList<ColorPair> colors;

		QList<ThymioClickableButton*> siblings;

		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
	};
	
	class ThymioPushButton : public QPushButton
	{
		Q_OBJECT
		
	public:
		ThymioPushButton(const QString& name, QWidget *parent=0);
		~ThymioPushButton();
		
		void changeButtonColor(const QColor& color);
		
	protected:
		virtual void mouseMoveEvent( QMouseEvent *event );
		virtual void dragEnterEvent( QDragEnterEvent *event );
		virtual void dropEvent( QDropEvent *event );

	private:
		Card *thymioButton;
	};
		
	/*@}*/
}; // Aseba

#endif
