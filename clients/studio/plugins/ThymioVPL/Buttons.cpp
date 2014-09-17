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
#include "Style.h"
#include "../../../../common/utils/utils.h"
#include "UsageLogger.h"

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
	
	//! Return the number of states
	unsigned GeometryShapeButton::valuesCount() const
	{
		return colors.size();
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

		//qreal alpha = pressed ? 255 : 150;
		
		// background
		painter->setPen(Qt::NoPen);
		painter->setBrush(Style::addRemoveButtonBackgroundColor);
		painter->drawRoundedRect(boundingRect(), Style::addRemoveButtonCornerSize, Style::addRemoveButtonCornerSize);

		const int hx(boundingRect().width()/2);
		const int hy(boundingRect().height()/2);
		painter->setPen(QPen(Qt::white,5,Qt::SolidLine,Qt::RoundCap));
		if (add)
		{
			painter->drawLine(hx+-16,hy+0,hx+16,hy+0);
			painter->drawLine(hx+0,hy+-16,hx+0,hy+16);
		}
		else
		{
			painter->drawLine(hx+-11,hy+-11,hx+11,hy+11);
			painter->drawLine(hx+-11,hy+11,hx+11,hy+-11);
		}
	}
	
	QRectF AddRemoveButton::boundingRect() const
	{
		return QRectF(0, 0, Style::addRemoveButtonWidth, Style::addRemoveButtonHeight);
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
		
		updateBlockImage(false);
		
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
	void BlockButton::updateBlockImage(bool advanced) 
	{
		block->setAdvanced(advanced);
		setIcon(QPixmap::fromImage(block->image(1.)));
	}

	void BlockButton::mouseMoveEvent( QMouseEvent *event )
	{
		#ifndef ANDROID
		assert(block);
		
		QDrag *drag = new QDrag(this);
		drag->setMimeData(block->mimeData());
		drag->setPixmap(QPixmap::fromImage(block->translucidImage(vpl->getViewScale())));
		const qreal thisScale = width() / 256.;
		drag->setHotSpot(event->pos()*vpl->getViewScale() / thisScale);
		Qt::DropAction dragResult(drag->exec(Qt::CopyAction));
		if (dragResult != Qt::IgnoreAction)
		{
			emit contentChanged();
			USAGE_LOG(logButtonDrag(block->name, block->type, event, drag));
			emit undoCheckpoint();
		}
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
		){
			event->accept();
			USAGE_LOG(logDropButton(this, event));
		}
		else
			event->ignore();
	}
} } // namespace ThymioVPL / namespace Aseba
