#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSvgItem>
#include <QSlider>
#include <QGraphicsProxyWidget>
#include <QtCore/qmath.h>
#include <QDebug>

#include "EventBlocks.h"
#include "Buttons.h"

#define deg2rad(x) ((x) * M_PI / 180.)

namespace Aseba { namespace ThymioVPL
{
	// Buttons Event
	ArrowButtonsEventBlock::ArrowButtonsEventBlock(QGraphicsItem *parent) : 
		BlockWithButtons("event", "button", true, parent)
	{
		const QColor color(Qt::red);
		
		// top, left, bottom, right
		for(int i=0; i<4; i++) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-25, -21.5, 50, 43), GeometryShapeButton::TRIANGULAR_BUTTON, this, Qt::lightGray, Qt::darkGray);

			qreal offset = (qreal)i;
			button->setRotation(-90*offset);
			button->setPos(128 - 70*qSin(1.57079633*offset), 
						   128 - 70*qCos(1.57079633*offset));
			button->addState(color);
			buttons.push_back(button);

			connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
		}

		GeometryShapeButton *button = new GeometryShapeButton(QRectF(-25, -25, 50, 50), GeometryShapeButton::CIRCULAR_BUTTON, this, Qt::lightGray, Qt::darkGray);
		button->setPos(QPointF(128, 128));
		button->addState(color);
		buttons.push_back(button);
		connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
	}
	
	
	// Prox Event
	ProxEventBlock::ProxEventBlock(bool advanced, QGraphicsItem *parent) : 
		BlockWithButtonsAndRange("event", "prox", true, 700, 4000, 1000, 2000, advanced, parent)
	{
		// front sensors
		for(int i=0; i<5; ++i) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-16,-16,32,32), GeometryShapeButton::RECTANGULAR_BUTTON, this, Qt::lightGray, Qt::darkGray);
			
			const qreal offset = (qreal)2-i;
			button->setRotation(-20*offset);
			button->setPos(128 - 150*qSin(0.34906585*offset) , 
						   175 - 150*qCos(0.34906585*offset) );
			//button->addState(QColor(110,255,110));
			//button->addState(QColor(230,0,0));
			button->addState(Qt::black);
			button->addState(Qt::red);

			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
		}
		// back sensors
		for(int i=0; i<2; ++i) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-16,-16,32,32), GeometryShapeButton::RECTANGULAR_BUTTON, this, Qt::lightGray, Qt::darkGray);

			button->setPos(QPointF(64 + i*128, 234));
			//button->addState(QColor(110,255,110));
			//button->addState(QColor(230,0,0));
			button->addState(Qt::black);
			button->addState(Qt::red);
			
			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
		}
	}
	
	
	// Prox Ground Event
	ProxGroundEventBlock::ProxGroundEventBlock(bool advanced, QGraphicsItem *parent) : 
		BlockWithButtonsAndRange("event", "proxground", false, 0, 1023, 150, 300, advanced, parent)
	{
		// sensors
		for(int i=0; i<2; ++i) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-16,-16,32,32), GeometryShapeButton::RECTANGULAR_BUTTON, this, Qt::lightGray, Qt::darkGray);

			button->setPos(QPointF(98 + i*60, 40));
			//button->addState(QColor(110,255,110));
			//button->addState(QColor(230,0,0));
			button->addState(Qt::black);
			button->addState(Qt::white);
			
			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
		}
		//connect(slider, SIGNAL(valueChanged(int)), this, SIGNAL(contentChanged()));
	}
	
	
	// Acc Event
	const QRectF AccEventBlock::buttonPoses[3] = {
		QRectF(32+(40+36)*0, 200, 40, 40),
		QRectF(32+(40+36)*1, 200, 40, 40),
		QRectF(32+(40+36)*2, 200, 40, 40)
	};
	const int AccEventBlock::resolution = 6;
	
	AccEventBlock::AccEventBlock( QGraphicsItem *parent) :
		Block("event", "acc", parent),
		mode(MODE_TAP),
		orientation(0),
		dragging(false),
		tapSvg(new QGraphicsSvgItem (":/images/thymiotap.svgz", this)),
		quadrantSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_quadrant.svgz", this)),
		pitchSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_pitch.svgz", this)),
		rollSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_roll.svgz", this))
	{
		quadrantSvg->setVisible(false);
		pitchSvg->setVisible(false);
		pitchSvg->setTransformOriginPoint(128, 128);
		rollSvg->setVisible(false);
		rollSvg->setTransformOriginPoint(128, 128);
	}
	
	void AccEventBlock::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		// paint parent
		Block::paint(painter, option, widget);
		
		// prepare line
		painter->setPen(QPen(Qt::black, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		
		// draw tab-style line
		/*painter->drawLine(20, 190, 32+(40+36)*mode, 190);
		painter->drawLine(32+(40+36)*mode+40, 190, 256-20, 190);
		painter->drawLine(32+(40+36)*mode, 190, 32+(40+36)*mode, 200);
		painter->drawLine(32+(40+36)*mode+40, 190, 32+(40+36)*mode+40, 200);*/
		
		// draw buttons
		for (unsigned i=0; i<3; ++i)
		{
			if (mode == i)
				painter->setBrush(QColor(255,128,0));
			else
				painter->setBrush(Qt::white);
			painter->drawRect(buttonPoses[i]);
		}
	}
	
	int AccEventBlock::getValue(unsigned i) const
	{
		if (i == 0)
			return mode;
		else
			return orientation;
	}
	
	void AccEventBlock::setValue(unsigned i, int value)
	{
		if (i == 0)
			setMode(value);
		else
			orientation = value;
	}
	
	void AccEventBlock::mousePressEvent(QGraphicsSceneMouseEvent * event)
	{
		if (event->button() == Qt::LeftButton)
		{
			for (unsigned i=0; i<3; ++i)
			{
				if (buttonPoses[i].contains(event->pos()))
				{
					setMode(i);
					return;
				}
			}
		}
		
		if (mode != MODE_TAP)
		{
			bool ok;
			const int orientation(orientationFromPos(event->pos(), &ok));
			if (ok)
			{
				setOrientation(orientation);
				dragging = true;
				return;
			}
		}
		
		Block::mousePressEvent(event);
	}
	
	void AccEventBlock::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
	{
		if (dragging)
		{
			bool ok;
			const int orientation(orientationFromPos(event->pos(), &ok));
			if (ok)
				setOrientation(orientation);
		}
		else
			Block::mouseMoveEvent(event);
	}
	
	void AccEventBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
	{
		if (dragging)
			dragging = false;
		else
			Block::mouseReleaseEvent(event);
	}
	
	int AccEventBlock::orientationFromPos(const QPointF& pos, bool* ok) const
	{
		const QPointF localPos(pos-QPointF(128,128));
		if (sqrt(localPos.x()*localPos.x()+localPos.y()*localPos.y()) <= 124)
		{
			const qreal angle(atan2(-localPos.y(), localPos.x()) - M_PI/2.);
			if (angle > deg2rad(-97.5) && angle < deg2rad(97.5))
			{
				if (ok) *ok = true;
				return int(round((angle * 2 * resolution) / M_PI));
			}
		}
		if (ok) *ok = false;
		return 0;
	}
	
	void AccEventBlock::setMode(unsigned mode)
	{
		if (mode != this->mode)
		{
			this->mode = mode;
			
			if (mode == MODE_TAP)
			{
				pitchSvg->setRotation(0);
				rollSvg->setRotation(0);
				orientation = 0;
			}
			tapSvg->setVisible(mode == MODE_TAP);
			quadrantSvg->setVisible(mode != MODE_TAP);
			pitchSvg->setVisible(mode == MODE_PITCH);
			rollSvg->setVisible(mode == MODE_ROLL);
			
			update();
			emit contentChanged();
		}
	}
	
	void AccEventBlock::setOrientation(int orientation)
	{
		if (orientation != this->orientation)
		{
			this->orientation = orientation;
			
			pitchSvg->setRotation(-orientation * 90 / resolution);
			rollSvg->setRotation(-orientation * 90 / resolution);
			
			update();
			emit contentChanged();
		}
	}
	
	// Clap Event
	ClapEventBlock::ClapEventBlock( QGraphicsItem *parent ) :
		BlockWithNoValues("event", "clap", parent)
	{
		new QGraphicsSvgItem (":/images/thymioclap.svgz", this);
	}
	
	
	// TimeoutEventBlock
	TimeoutEventBlock::TimeoutEventBlock(QGraphicsItem *parent):
		BlockWithNoValues("event", "timeout", parent)
	{
		new QGraphicsSvgItem (":/images/timeout.svgz", this);
	}

} } // namespace ThymioVPL / namespace Aseba