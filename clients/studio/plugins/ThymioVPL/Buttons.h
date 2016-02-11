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

#ifndef VPL_BUTTONS_H
#define VPL_BUTTONS_H

#include <QGraphicsItem>
#include <QPushButton>

class QSlider;
class QTimeLine;
class QMimeData;

// FIXME: split this file into two

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class Block;
	class ThymioVisualProgramming;
	
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
		void setValue(int state);
		unsigned valuesCount() const;
		void setStateCountLimit(int limit);
		
		void setToggleState(bool state) { toggleState = state; update(); }

		void addState(const QColor& brushColor = Qt::white, const QColor& penColor = Qt::black);

		void addSibling(GeometryShapeButton *s) { siblings.push_back(s); }
	
	signals:
		void stateChanged();
	
	protected:
		const ButtonType buttonType;
		const QRectF boundingRectangle;
		
		int stateCountLimit; //!< limit of the value of curState, -1 if disabled
		int curState;
		bool toggleState;
		
		//! a (brush,pen) color pair
		typedef QPair<QColor, QColor> ColorPair;
		QList<ColorPair> colors;

		QList<GeometryShapeButton*> siblings;

		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
	};
	
	class AddRemoveButton : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		AddRemoveButton(bool add, QGraphicsItem *parent=0);
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const;
	
	signals:
		void clicked();
		
	protected:
		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
		
	protected:
		bool add;
	};
	
	class RemoveBlockButton : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		RemoveBlockButton(QGraphicsItem *parent=0);
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const;
	
	signals:
		void clicked();
		
	protected:
		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
	};
	
	class BlockButton : public QPushButton
	{
		Q_OBJECT
		
	signals:
		void contentChanged();
		void undoCheckpoint();
		
	public:
		BlockButton(const QString& name, ThymioVisualProgramming* vpl, QWidget *parent=0);
		~BlockButton();
		
		QString getName() const;
		
		void updateBlockImage(bool advanced);
		
	protected:
		virtual void mouseMoveEvent( QMouseEvent *event );
		virtual void dragEnterEvent( QDragEnterEvent *event );
		virtual void dropEvent( QDropEvent *event );

	protected:
		Block *block;
		ThymioVisualProgramming* vpl;
	};
		
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif // VPL_BUTTONS_H
