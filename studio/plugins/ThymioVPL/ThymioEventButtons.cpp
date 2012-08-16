#include <QPainter>

#include <QGraphicsSceneMouseEvent>

#include "ThymioButtons.h"

namespace Aseba
{
	// Buttons Event
	ThymioButtonsEvent::ThymioButtonsEvent(QGraphicsItem *parent, bool advanced) : 
		ThymioButton(true, 1.0, true, advanced, parent)
	{	
		setData(0, "event");
		setData(1, "button");
		
		// top, left, bottom, right		
		for(int i=0; i<4; i++) 
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-25, -21.5, 50, 43), THYMIO_TRIANGULAR_BUTTON, 2, this);

			qreal offset = (qreal)i;
			button->setRotation(-90*offset);
			button->setPos(128 - 70*qSin(1.57079633*offset), 
						   128 - 70*qCos(1.57079633*offset));
			button->setButtonColor(Qt::green);
			
			thymioButtons.push_back(button);

			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
		}

		ThymioClickableButton *button = new ThymioClickableButton(QRectF(-25, -25, 50, 50), THYMIO_CIRCULAR_BUTTON, 2, this);
		button->setPos(QPointF(128, 128));
		button->setButtonColor(Qt::green);
		thymioButtons.push_back(button);
		connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
	}
			
	// Prox Event
	ThymioProxEvent::ThymioProxEvent(QGraphicsItem *parent, bool advanced) : 
		ThymioButton(true, 1.0, true, advanced, parent)
	{
		setData(0, "event");
		setData(1, "prox");
		
		for(int i=0; i<5; ++i) 
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-16,-16,32,32), THYMIO_RECTANGULAR_BUTTON, 3, this);	
			
			qreal offset = (qreal)2-i;
			button->setRotation(-20*offset);
			button->setPos(128 - 150*qSin(0.34906585*offset) , 
						   175 - 150*qCos(0.34906585*offset) );
			button->setBeginButtonColor(QColor(110,255,110));
			button->setButtonColor(QColor(230,0,0));

			thymioButtons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));			
		}
		
		for(int i=0; i<2; ++i) 
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-16,-16,32,32), THYMIO_RECTANGULAR_BUTTON, 3, this);	

			button->setPos(QPointF(64 + i*128, 234));
			button->setBeginButtonColor(QColor(110,255,110));
			button->setButtonColor(QColor(230,0,0));
			
			thymioButtons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
		}

	}
	
	// Prox Ground Event
	ThymioProxGroundEvent::ThymioProxGroundEvent( QGraphicsItem *parent, bool advanced ) : 
		ThymioButton(true, 1.0, false, advanced, parent)
	{
		setData(0, "event");
		setData(1, "proxground");
		
		for(int i=0; i<2; ++i) 
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-16,-16,32,32), THYMIO_RECTANGULAR_BUTTON, 3, this);	

			button->setPos(QPointF(98 + i*60, 40));
			button->setBeginButtonColor(QColor(110,255,110));
			button->setButtonColor(QColor(230,0,0));
			
			thymioButtons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
		}
	}	
			
	// Tap Event
	ThymioTapEvent::ThymioTapEvent( QGraphicsItem *parent, bool advanced ) :
		ThymioButton(true, 1.0, true, advanced, parent)
	{
		setData(0, "event");
		setData(1, "tap");
		
		delete(thymioBody);
		thymioBody=0;
	}
	
	
	// Clap Event
	ThymioClapEvent::ThymioClapEvent( QGraphicsItem *parent, bool advanced ) :
		ThymioButton(true, 1.0, true, advanced, parent)
	{		
		setData(0, "event");		
		setData(1, "clap");
		
		delete(thymioBody);
		thymioBody=0;
	}
	

};

