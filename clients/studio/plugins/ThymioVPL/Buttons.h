#ifndef THYMIO_BUTTONS_H
#define THYMIO_BUTTONS_H

#include <QGraphicsItem>
#include <QPushButton>

#include "ThymioIntermediateRepresentation.h"

class QSlider;
class QTimeLine;
class QMimeData;

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class Card;
	
	class GeometryShapeButton : public QGraphicsObject
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
		GeometryShapeButton (const QRectF rect, const ButtonType type, QGraphicsItem *parent=0, const QColor& initBrushColor = Qt::white, const QColor& initPenColor = Qt::black);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return boundingRectangle; }

		int getValue() const { return curState; }
		void setValue(int state) { curState = state; update(); }
		void setToggleState(bool state) { toggleState = state; update(); }

		void addState(const QColor& brushColor = Qt::white, const QColor& penColor = Qt::black);

		void addSibling(GeometryShapeButton *s) { siblings.push_back(s); }
	
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

		QList<GeometryShapeButton*> siblings;

		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
	};
	
	class CardButton : public QPushButton
	{
		Q_OBJECT
		
	public:
		CardButton(const QString& name, QWidget *parent=0);
		~CardButton();
		
		void changeButtonColor(const QColor& color);
		
	protected:
		virtual void mouseMoveEvent( QMouseEvent *event );
		virtual void dragEnterEvent( QDragEnterEvent *event );
		virtual void dropEvent( QDropEvent *event );

	private:
		Card *thymioButton;
	};
		
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
