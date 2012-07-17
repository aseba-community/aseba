#include <QPainter>

#include <QGraphicsSceneMouseEvent>

#include "ThymioButtons.h"

namespace Aseba
{
	// Move Action
	ThymioMoveAction::ThymioMoveAction( QGraphicsItem *parent ) :
		ThymioButton(false, 0.4, false, parent)
	{
		setData(0, "action");
		setData(1, "move");
		
		for(int w=0; w<2; ++w) 
		{
			for(int i=-2; i<3; ++i) 
			{
				ThymioClickableButton *button;
				int num = qAbs(i);
		
				if( i==0 ) 
				{
					button = new ThymioClickableButton(QRectF(-25,-10,50,20), THYMIO_RECTANGULAR_BUTTON, this);
					button->setClicked(true);
				}
				else
					button = new ThymioClickableButton(QRectF(-25,-10-num*7.5,50,20+num*15), THYMIO_TRIANGULAR_BUTTON, this);
							
				button->setButtonColor(QColor(num<2 ? 255:0, num>0? 255:0, 0));

				button->setPos(30+w*196, 128+i*(num*10+30));
				button->setRotation(i<0 ? 0 : 180);

				button->setData(0, w);
				button->setData(1, i+2);
				
				button->setToggleState(false);
					
				thymioButtons.push_back(button);
				connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
			}
			
			for(int i=w*5; i<w*5+5; i++)
			{
				for(int j=w*5; j<w*5+5; j++) 
				{
					if( i!=j )
						thymioButtons.at(i)->addSibling(thymioButtons.at(j));
				}
			}
		}
	}

	void ThymioMoveAction::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
	
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(buttonColor); // filling
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);			

		qreal pt[2];
		for(int k=0; k<2; ++k)
		{
			for(int i=0; i<5; i++)
				if( thymioButtons.at(k*5+i)->isClicked() )
				{
					pt[k] = (i-2);
					break;
				}
		}
		
		thymioBody->setPos(128, (pt[0]+pt[1])*15 + 128);
		thymioBody->setRotation((-pt[0]+pt[1])*45);	
	}
	
	// Color Action
	ThymioColorAction::ThymioColorAction( QGraphicsItem *parent ) :
		ThymioButton(false, 1.0, true, parent)
	{
		setData(0, "action");
		setData(1, "color");
		
		for(int k=0; k<3; ++k) 
		{			
			for(int i=0; i<3; ++i) 
			{
				ThymioClickableButton *button = new ThymioClickableButton(QRectF(0,0,40,40), THYMIO_CIRCULAR_BUTTON, this);

				button->setData(0, k);   // color
				button->setData(1, i); 	 // value

				if( k==0 )		
					button->setButtonColor(QColor(i*127.5, 0, 0));
				else if( k==1 )
					button->setButtonColor(QColor(0, i*127.5, 0));
				else
					button->setButtonColor(QColor(0, 0, i*127.5));
		
				button->setPos(QPointF(48+i*60,60+k*60));				
				button->setToggleState(false);

				thymioButtons.push_back(button);
				connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
			}
			thymioButtons.back()->setClicked(true);
			
			for(int i=k*3; i<k*3+3; i++)
			{
				for(int j=k*3; j<k*3+3; j++) 
				{
					if( i!=j )
						thymioButtons.at(i)->addSibling(thymioButtons.at(j));
				}
			}			
		}
		
	}

	void ThymioColorAction::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(buttonColor); // filling
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);		

		for(int i=0; i<3; ++i)
			if( thymioButtons.at(i)->isClicked() ) {
				thymioBody->bodyColor.setRed(127.5*i);
				break;
			}
		for(int i=3; i<6; ++i)
			if( thymioButtons.at(i)->isClicked() ) {
				thymioBody->bodyColor.setGreen(127.5*(i-3));
				break;
			}
		for(int i=6; i<9; ++i)
			if( thymioButtons.at(i)->isClicked() ) {
				thymioBody->bodyColor.setBlue(127.5*(i-6));
				break;
			}	
	}
	
	// Circle Action
	ThymioCircleAction::ThymioCircleAction(QGraphicsItem *parent) : 
		ThymioButton(false, 1.0, true, parent)
	{
		setData(0, "action");		
		setData(1, "circle");
		
		for(uint i=0; i<8; i++)
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-25,-15,50,30), THYMIO_RECTANGULAR_BUTTON, this);

			qreal offset = (qreal)i;			
			button->setRotation(45*offset);
			button->setPos(128 - 90*qCos(0.785398163*offset+1.57079633), 
						   128 - 90*qSin(0.785398163*offset+1.57079633));

			button->setButtonColor(QColor(255, 90, 54));

			thymioButtons.push_back(button);
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
		}
	}
		
	// Sound Action
	void ThymioSoundAction::Speaker::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) 
	{		
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		painter->setPen(QPen(Qt::black, 3.0, Qt::SolidLine, Qt::RoundCap));
		painter->setBrush(Qt::black);
		painter->drawRect(25, 108, 30, 50);

		QPointF points[4];
		points[0] = QPointF(52, 158);
		points[1] = QPointF(52, 108);
		points[2] = QPointF(84, 78);
		points[3] = QPointF(84, 188);
		painter->drawPolygon(points, 4);
		painter->drawChord(64, 78, 40, 110, -1440, 2880);
		
		painter->drawArc(64, 68, 60, 130, -1440, 2880);
		painter->drawArc(64, 53, 80, 160, -1440, 2880);
	}	
	
	ThymioSoundAction::ThymioSoundAction(QGraphicsItem *parent) :
		ThymioButton(false, 1.0, true, parent)
	{
		setData(0, "action");		
		setData(1, "sound");

		for(int i=0; i<3; i++) 
		{		
			ThymioFaceButton *button = new ThymioFaceButton(QRectF(-30,-30,60,60), (ThymioSmileType)i, this);

			button->setPos(195,70+70*(qreal)i);
			button->setButtonColor(i == 0 ? Qt::green : (i == 1 ? Qt::yellow : Qt::red) );
			button->setData(0, i);	
			button->setToggleState(false);										
			
			thymioButtons.push_back(button);
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
		}
		thymioButtons.at(0)->setClicked(true);
		
		for(int i=0; i<3; i++)
		{
			for(int j=0; j<3; j++) 
			{
				if( i!=j )
					thymioButtons.at(i)->addSibling(thymioButtons.at(j));
			}
		}
		
		speaker = new Speaker(this);
	}

	QPixmap ThymioSoundAction::image(bool on)
	{
		QPixmap pixmap = ThymioButton::image(on);
		QPainter painter(&pixmap);

		painter.translate(speaker->pos());
		speaker->paint(&painter, 0, 0);
		painter.resetTransform();
		
		return pixmap;
	}	

	// Reset Action
	void ThymioResetAction::Reset::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) 
	{		
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		painter->setPen(QPen(Qt::black, 20.0, Qt::SolidLine, Qt::RoundCap));
		painter->setBrush(Qt::black);

		painter->drawArc(50, 50, 156, 156, 4320, -2240);
		painter->drawArc(50, 50, 156, 156, 1440, -2240);
		
		painter->setPen(QPen(Qt::black, 3.0, Qt::SolidLine, Qt::RoundCap));
		QPointF points[3];
		points[0] = QPointF(123, 186);
		points[1] = QPointF(123, 226);
		points[2] = QPointF(153, 206);
		painter->drawPolygon(points, 3);		

		points[0] = QPointF(133, 30);
		points[1] = QPointF(133, 70);
		points[2] = QPointF(103, 50);
		painter->drawPolygon(points, 3);
	}	
	
	ThymioResetAction::ThymioResetAction(QGraphicsItem *parent) : 
		ThymioButton(false, 1.0, true, parent)
	{
		setData(0, "action");		
		setData(1, "reset");
		
		logo = new Reset(this);
	}

	QPixmap ThymioResetAction::image(bool on)
	{
		QPixmap pixmap = ThymioButton::image(on);
		QPainter painter(&pixmap);

		painter.translate(logo->pos());
		logo->paint(&painter, 0, 0);
		painter.resetTransform();
		
		return pixmap;
	}	

};

