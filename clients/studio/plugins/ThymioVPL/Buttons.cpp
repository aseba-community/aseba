#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QDrag>
#include <QMimeData>
#include <QCursor>
#include <QApplication>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsView>
#include <QSlider>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDebug>
#include <cassert>

#include "Buttons.h"
#include "Block.h"
#include "EventBlocks.h"
#include "ActionBlocks.h"
#include "Scene.h"
#include "ThymioVisualProgramming.h"
#include "../../../../common/utils/utils.h"

namespace Aseba { namespace ThymioVPL
{
	// Clickable button
	GeometryShapeButton::GeometryShapeButton (const QRectF rect, const ButtonType  type, QGraphicsItem *parent, const QColor& initBrushColor, const QColor& initPenColor) :
		QGraphicsObject(parent),
		buttonType(type),
		boundingRectangle(rect),
		curState(0),
		toggleState(true)
	{
		colors.push_back(qMakePair(initBrushColor, initPenColor));
	}
	
	void GeometryShapeButton::addState(const QColor& brushColor, const QColor& penColor)
	{
		colors.push_back(qMakePair(brushColor, penColor));
	}
	
	void GeometryShapeButton::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
	{
		painter->setBrush(colors[curState].first);
		painter->setPen(QPen(colors[curState].second, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); // outline
		
		switch (buttonType)
		{
			case CIRCULAR_BUTTON:
				painter->drawEllipse(boundingRectangle);
			break;
			case TRIANGULAR_BUTTON:
			{
				QPointF points[3];
				points[0] = (boundingRectangle.topLeft() + boundingRectangle.topRight())*0.5;
				points[1] = boundingRectangle.bottomLeft();
				points[2] = boundingRectangle.bottomRight();
				painter->drawPolygon(points, 3);
			}
			break;
			case QUARTER_CIRCLE_BUTTON:
			{
				const int x(boundingRectangle.x());
				const int y(boundingRectangle.y());
				const int w(boundingRectangle.width());
				const int h(boundingRectangle.height());
				painter->setPen(QPen(colors[curState].second, 35, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				painter->drawArc(x,y,2*w,2*h,(90+15)*16, (90-30)*16);
				painter->setPen(QPen(colors[curState].first, 25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				painter->drawArc(x,y,2*w,2*h,(90+15)*16, (90-30)*16);
			}
			break;
			default:
				painter->drawRect(boundingRectangle);
			break;
		}
	}

	void GeometryShapeButton::mousePressEvent ( QGraphicsSceneMouseEvent * event )
	{
		if( event->button() == Qt::LeftButton ) 
		{
			if( toggleState )
			{
				curState = (curState + 1) % colors.size();
			}
			else 
			{
				curState = 1;
				for( QList<GeometryShapeButton*>::iterator itr = siblings.begin();
					 itr != siblings.end(); ++itr )
					(*itr)->setValue(0);
			}
			update();
			emit stateChanged();
		}
	}
	
	
	AddRemoveButton::AddRemoveButton(bool add, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		add(add),
		pressed(false)
	{
		setData(0, "add"); 
		setData(0, "remove");
		setAcceptedMouseButtons(Qt::LeftButton);
	}
	
	void AddRemoveButton::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);

 		qreal alpha = pressed ? 255 : 150;
		
		QLinearGradient linearGrad(QPointF(-32, -32), QPointF(32, 32));
		linearGrad.setColorAt(0, QColor(209, 196, 180, alpha));
		linearGrad.setColorAt(1, QColor(167, 151, 128, alpha));
     
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(linearGrad);
		painter->drawEllipse(-32,-32,64,64);
		
		painter->setPen(QPen(QColor(147, 134, 115),5,Qt::SolidLine,Qt::RoundCap));
		if (add)
		{
			painter->drawLine(-16,0,16,0);
			painter->drawLine(0,-16,0,16);
		}
		else
		{
			painter->drawLine(-11,-11,11,11);
			painter->drawLine(-11,11,11,-11);
		}
	}
	
	void AddRemoveButton::mousePressEvent ( QGraphicsSceneMouseEvent * event )
	{
		pressed = true;
		update();
	}
	
	void AddRemoveButton::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event ) 
	{
		if (boundingRect().contains(event->pos()))
			emit clicked();
		pressed = false;
		update();
	}
	
	
	BlockButton::BlockButton(const QString& name, ThymioVisualProgramming* vpl, QWidget *parent) : 
		QPushButton(parent), 
		block(Block::createBlock(name)),
		vpl(vpl)
	{
		setToolTip(QString("%0 %1").arg(block->getName()).arg(block->getType()));
		
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		
		setStyleSheet("QPushButton {border: none; }");
		
		updateBlockImage();
		
		setAcceptDrops(true);
	}
	
	BlockButton::~BlockButton()
	{
		assert(block);
		delete(block); 
	}
	
	QString BlockButton::getName() const
	{
		assert(block);
		return block->getName();
	}
	
	//! Re-render the block image
	void BlockButton::updateBlockImage() 
	{ 
		const qreal factor = width() / 256.;
		setIcon(block->image(factor));
	}	

	void BlockButton::mouseMoveEvent( QMouseEvent *event )
	{
		#ifndef ANDROID
		assert(block);
		
		QDrag *drag = new QDrag(this);
		drag->setMimeData(block->mimeData());
		drag->setPixmap(block->image(vpl->getViewScale()));
		const qreal thisScale = width() / 256.;
		drag->setHotSpot(event->pos()*vpl->getViewScale() / thisScale);
		drag->exec(Qt::CopyAction);
		#endif // ANDROID
	}

	void BlockButton::dragEnterEvent( QDragEnterEvent *event )
	{
		if (event->mimeData()->hasFormat("Block") &&
			(Block::deserializeType(event->mimeData()->data("Block")) == block->getType())
		)
			event->accept();
		else
			event->ignore();
	}

	void BlockButton::dropEvent( QDropEvent *event )
	{
		if (event->mimeData()->hasFormat("Block") &&
			(Block::deserializeType(event->mimeData()->data("Block")) == block->getType())
		)
			event->accept();
		else
			event->ignore();
	}
} } // namespace ThymioVPL / namespace Aseba
