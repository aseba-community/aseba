#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSvgItem>
#include <QSlider>
#include <QGraphicsProxyWidget>
#include <QtCore/qmath.h>

#include "EventCards.h"
#include "Buttons.h"

namespace Aseba { namespace ThymioVPL
{
	// Buttons Event
	ArrowButtonsEventCard::ArrowButtonsEventCard(QGraphicsItem *parent, bool advanced) : 
		CardWithButtons(true, true, advanced, parent)
	{
		setData(0, "event");
		setData(1, "button");
		
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
	ProxEventCard::ProxEventCard(QGraphicsItem *parent, bool advanced) : 
		CardWithButtonsAndRange(true, true, 700, 4000, 1000, 2000, advanced, parent)
	{
		setData(0, "event");
		setData(1, "prox");
		
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
			button->addState(Qt::white);
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
			button->addState(Qt::white);
			button->addState(Qt::red);
			
			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
		}
	}
	
	// Prox Ground Event
	ProxGroundEventCard::ProxGroundEventCard( QGraphicsItem *parent, bool advanced ) : 
		CardWithButtonsAndRange(true, false, 0, 1023, 150, 300, advanced, parent)
	{
		setData(0, "event");
		setData(1, "proxground");
		
		// sensors
		for(int i=0; i<2; ++i) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-16,-16,32,32), GeometryShapeButton::RECTANGULAR_BUTTON, this, Qt::lightGray, Qt::darkGray);

			button->setPos(QPointF(98 + i*60, 40));
			//button->addState(QColor(110,255,110));
			//button->addState(QColor(230,0,0));
			button->addState(Qt::white);
			button->addState(Qt::red);
			
			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
		}
		//connect(slider, SIGNAL(valueChanged(int)), this, SIGNAL(contentChanged()));
	}
	
	// Tap Event
	TapEventCard::TapEventCard( QGraphicsItem *parent, bool advanced ) :
		CardWithNoValues(true, advanced, parent)
	{
		setData(0, "event");
		setData(1, "tap");
		
		new QGraphicsSvgItem (":/images/thymiotap.svgz", this);
	}
	
	
	// Clap Event
	ClapEventCard::ClapEventCard( QGraphicsItem *parent, bool advanced ) :
		CardWithNoValues(true, advanced, parent)
	{
		setData(0, "event");
		setData(1, "clap");
		
		new QGraphicsSvgItem (":/images/thymioclap.svgz", this);
	}
	
	// TimeoutEventCard
	TimeoutEventCard::TimeoutEventCard(QGraphicsItem *parent, bool advanced):
		CardWithNoValues(true, advanced, parent)
	{
		setData(0, "event");
		setData(1, "timeout");
	
		new QGraphicsSvgItem (":/images/timeout.svgz", this);
	}

} } // namespace ThymioVPL / namespace Aseba