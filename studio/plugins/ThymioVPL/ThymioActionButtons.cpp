#include <QPainter>

#include <QGraphicsSceneMouseEvent>

#include "ThymioButtons.h"

namespace Aseba
{
	// Move Action
	void ThymioMoveAction::MoveArrow::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) 
	{		
		Q_UNUSED(option);
		Q_UNUSED(widget);

//		double span = (-lwheel+rwheel)*2.88;
//		double width = abs(lwheel-rwheel) < 500 ? abs(lwheel-rwheel)*0.1 : 50; 
//		double height = (abs(lwheel)+abs(rwheel)) < 500 ? (abs(lwheel)+abs(rwheel))*0.1 : 50;
//		double w2 = width*0.5;
//		double h2 = height*0.5;
//		QPointF trans;
//
//		painter->setPen(QPen(Qt::black, 10.0, Qt::SolidLine, Qt::SquareCap));
//		painter->setBrush(Qt::black);
//
//		if( lwheel >= rwheel) 
//		{
////			double span2 = -(lwheel-rwheel)*0.003141593;
//			if( lwheel > 0 )
//			{
//				painter->drawArc(-w2, -h2, width, height, 2880, span-1440);		
////				trans = QPointF(w2*cos(span2 + 1.5707965), h2*sin(span2 + 1.5707965) - h2);
//			}
//			else
//			{
//				painter->drawArc(-w2, -h2, width, height, 2880, -span+1440);		
////				trans = QPointF(w2*cos(span2 + 1.5707965), h2*sin(span2 + 1.5707965) + h2);
//			}
//		}
//		else
//		{
////			double span2 = (lwheel+rwheel)*0.003141593;
//			if( rwheel > 0 )
//			{
//				painter->drawArc(-w2, -h2, width, height, 0, span+1440);
////				trans = QPointF(w2*cos(span2 + 1.5707965), h2*sin(span2 + 1.5707965) + h2 );
//			}
//			else
//			{
//				painter->drawArc(-w2, -h2, width, height, 0, -span-1440);
////				trans = QPointF(w2*cos(span2 + 1.5707965), h2*sin(span2 + 1.5707965) - h2 );
//			}
//		
////			span = (lwheel+rwheel)*0.003141593;
//
//		}
//
//		double span2 = (lwheel-rwheel)*0.003141593;
//		trans = QPointF(w2*cos(span2 - 1.5707965), h2*sin(span2 + 1.5707965) );
//			
//		painter->setPen(QPen(Qt::red, 1.0, Qt::SolidLine, Qt::SquareCap));
//		painter->setBrush(Qt::red);
//		painter->drawEllipse(trans, 5, 5);
		
//		if( (lwheel + rwheel) >= 0 )
//		{
//			span = -(lwheel+rwheel+1000)*0.0015707965; //0.003141593;
//			trans = QPointF(w2*cos(span + 1.5707965), h2*sin(span + 1.5707965) );
//			qDebug() << "l>= r -- trans: " << trans;		
//		}
//		else
//		{
//			span = -(lwheel+rwheel+1000)*0.0015707965; //0.003141593;
//			trans = QPointF(w2*cos(span - 4.7123895), h2*sin(span - 4.7123895) );
//			qDebug() << "l<r -- trans: " << trans;
//		}
//							
//		if( height != 0 )
//		{
//			painter->setPen(QPen(Qt::black, 1.0, Qt::SolidLine, Qt::SquareCap));		
//			QPointF points[3];
//			span = (lwheel+rwheel-1000)*0.0015707965;//0.003141593;
//			double cosA = cos(span);
//			double sinA = sin(span);
////			points[0] = QPointF(-10*sinA,10*cosA);
////			points[1] = QPointF(-10*cosA+5*sinA, (-10*sinA-5*cosA));
////			points[2] = QPointF(10*cosA+5*sinA, (10*sinA-5*cosA));
//			points[0] = QPointF(10*sinA,-10*cosA) + trans;
//			points[1] = QPointF(-10*cosA-5*sinA, (-10*sinA+5*cosA)) + trans;
//			points[2] = QPointF(10*cosA-5*sinA, (10*sinA+5*cosA)) + trans;
//
//			painter->drawPolygon(points, 3);
//		}
	}	
	
	ThymioMoveAction::ThymioMoveAction( QGraphicsItem *parent ) :
		ThymioButton(false, 0.4, false, false, parent)
	{
		setData(0, "action");
		setData(1, "move");	
		
//		for(int w=0; w<2; ++w) 
//		{
//			for(int i=-2; i<3; ++i) 
//			{
//				ThymioClickableButton *button;
//				int num = qAbs(i);
//		
//				if( i==0 ) 
//				{
//					button = new ThymioClickableButton(QRectF(-25,-10,50,20), THYMIO_RECTANGULAR_BUTTON, 2, this);
//					button->setClicked(1);
//				}
//				else
//					button = new ThymioClickableButton(QRectF(-25,-10-num*7.5,50,20+num*15), THYMIO_TRIANGULAR_BUTTON, 2, this);
//							
//				button->setButtonColor(QColor(num<2 ? 255:0, num>0? 255:0, 0));
//
//				button->setPos(30+w*196, 128+i*(num*10+30));
//				button->setRotation(i<0 ? 0 : 180);
//
//				button->setData(0, w);
//				button->setData(1, i+2);
//				
//				button->setToggleState(false);
//					
//				thymioButtons.push_back(button);
//				connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
//			}
//			
//			for(int i=w*5; i<w*5+5; i++)
//			{
//				for(int j=w*5; j<w*5+5; j++) 
//				{
//					if( i!=j )
//						thymioButtons.at(i)->addSibling(thymioButtons.at(j));
//				}
//			}
//		}

		QTransform trans;
		trans.scale(2.0,2.3);
				
		for(int i=0; i<2; i++)
		{
			QSlider *s = new QSlider(Qt::Vertical);
			s->setRange(-500,500);
			s->setStyleSheet("QSlider::groove:vertical { width: 14px; border: 2px solid #000000; "
							  "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00FF00, stop:0.25 #FFFF00, stop:0.5 #FF0000, stop:0.75 #FFFF00, stop:1 #00FF00 ); }"
						      "QSlider::handle:vertical { "
						      "background: #FFFFFF; "
						      "border: 2px solid #000000; height: 10px; width: 20px; margin: 0px 0; }");
			s->setTickInterval(10);

			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setWidget(s);
			w->setPos(10+i*200, 15);
			w->setTransform(trans);
			
			sliders.push_back(s);
			widgets.push_back(w);
			
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChangeDetected()));
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(updateIRButton()));
		}		

		moveArrow = new MoveArrow(this);
	}

	void ThymioMoveAction::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
	
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(buttonColor); // filling
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);			
	
		qreal pt[2];
		for(int i=0; i<2; i++) 
			pt[i] = sliders[i]->value();	

		thymioBody->setPos(128, -(pt[0]+pt[1])*0.06 + 128);
		thymioBody->setRotation((pt[0]-pt[1])*0.18);

		moveArrow->setWheels(pt[0], pt[1]);		
		moveArrow->setPos(thymioBody->pos());
	}

	QPixmap ThymioMoveAction::image(bool on)
	{
		QPixmap pixmap(256, 256);
		pixmap.fill(buttonColor);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);

		painter.translate(thymioBody->pos());
		painter.scale(thymioBody->scale(),thymioBody->scale());
		painter.rotate(thymioBody->rotation());
		thymioBody->paint(&painter, 0, 0);
		painter.resetTransform();

		for(int i=0; i<getNumButtons(); i++)
		{
			painter.translate(widgets[i]->pos());
			painter.scale(2.0, 2.3);
			sliders[i]->render(&painter);
			painter.resetTransform();
		}
	
		painter.translate(moveArrow->pos());
		moveArrow->paint(&painter, 0, 0);
		painter.resetTransform();
	
		return pixmap;
	}

	void ThymioMoveAction::valueChangeDetected()
	{
		update();
	}

	void ThymioMoveAction::setClicked(int i, int status) 
	{ 
		if(i<getNumButtons()) 
			sliders[i]->setSliderPosition(status); 		
	}

	int ThymioMoveAction::isClicked(int i) 
	{ 
		if(i<getNumButtons()) 
			return sliders[i]->value(); 		
		return -1;
	}
				
	// Color Action
	ThymioColorAction::ThymioColorAction( QGraphicsItem *parent ) :
		ThymioButton(false, 1.0, true, false, parent)
	{
		setData(0, "action");
		setData(1, "color");

		QTransform trans;
		trans.scale(1.0,2.0);
		
		for(int i=0; i<3; i++)
		{
			QSlider *s = new QSlider(Qt::Horizontal);
			s->setRange(0,32);
			s->setStyleSheet(QString("QSlider::groove:horizontal { height: 14px; border: 2px solid #000000; "
							  "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #000000, stop:1 #%0); }"
						      "QSlider::handle:horizontal { "
						      "background: #FFFFFF; "
						      "border: 5px solid #000000; width: 18px; margin: -2px 0; }").arg(i==0?"FF0000":i==1?"00FF00":"0000FF"));

			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setWidget(s);
			w->setPos(27, 70+i*60);
			w->setTransform(trans);
			
			sliders.push_back(s);
			widgets.push_back(w);
			
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChangeDetected()));
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(updateIRButton()));
		}

	}

	void ThymioColorAction::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(buttonColor); // filling
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);		

//		for(int i=0; i<3; ++i)
//			if( thymioButtons.at(i)->isClicked() > 0 ) {
//				thymioBody->bodyColor.setRed(80*i+85);
//				break;
//			}
//		for(int i=3; i<6; ++i)
//			if( thymioButtons.at(i)->isClicked() > 0 ) {
//				thymioBody->bodyColor.setGreen(80*(i-3)+85);
//				break;
//			}
//		for(int i=6; i<9; ++i)
//			if( thymioButtons.at(i)->isClicked() > 0) {
//				thymioBody->bodyColor.setBlue(80*(i-6)+85);
//				break;
//			}

		thymioBody->bodyColor = QColor(sliders[0]->value()*5.46875+80, 
									   sliders[1]->value()*5.46875+80, 
									   sliders[2]->value()*5.46875+80);
	}

	QPixmap ThymioColorAction::image(bool on)
	{
		QPixmap pixmap(256, 256);
		pixmap.fill(buttonColor);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);

		painter.translate(thymioBody->pos());
		painter.scale(thymioBody->scale(),thymioBody->scale());
		painter.rotate(thymioBody->rotation());
		thymioBody->paint(&painter, 0, 0);
		painter.resetTransform();

		for(int i=0; i<getNumButtons(); i++)
		{
			painter.translate(widgets[i]->pos());
			painter.scale(1.0, 2.0);
			sliders[i]->render(&painter);
			painter.resetTransform();
		}
	
		return pixmap;
	}

	void ThymioColorAction::valueChangeDetected()
	{
		update();
	}

	void ThymioColorAction::setClicked(int i, int status) 
	{ 
		if(i<getNumButtons()) 
			sliders[i]->setSliderPosition(status); 		
	}

	int ThymioColorAction::isClicked(int i) 
	{ 
		if(i<getNumButtons()) 
			return sliders[i]->value(); 		
		return -1;
	}
	
	// Circle Action
	ThymioCircleAction::ThymioCircleAction(QGraphicsItem *parent) : 
		ThymioButton(false, 1.0, true, false, parent)
	{
		setData(0, "action");		
		setData(1, "circle");
		
		for(uint i=0; i<8; i++)
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-25,-15,50,30), THYMIO_RECTANGULAR_BUTTON, 5, this);

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
		ThymioButton(false, 1.0, true, false, parent)
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
		thymioButtons.at(0)->setClicked(1);
		
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
	
	// Memory Action
	ThymioMemoryAction::ThymioMemoryAction(QGraphicsItem *parent) : 
		ThymioButton(false, 1.0, true, false, parent)
	{
		setData(0, "action");		
		setData(1, "memory");
		
		for(uint i=0; i<4; i++)
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-20,-20,40,40), THYMIO_CIRCULAR_BUTTON, 2, this);

			button->setPos(128, i*60 + 40);
			button->setButtonColor(Qt::gray);

			thymioButtons.push_back(button);
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
		}
	}	

	// Reset Action
//	void ThymioResetAction::Reset::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) 
//	{		
//		Q_UNUSED(option);
//		Q_UNUSED(widget);
//		
//		painter->setPen(QPen(Qt::black, 20.0, Qt::SolidLine, Qt::RoundCap));
//		painter->setBrush(Qt::black);
//
//		painter->drawArc(50, 50, 156, 156, 4320, -2240);
//		painter->drawArc(50, 50, 156, 156, 1440, -2240);
//		
//		painter->setPen(QPen(Qt::black, 3.0, Qt::SolidLine, Qt::RoundCap));
//		QPointF points[3];
//		points[0] = QPointF(123, 186);
//		points[1] = QPointF(123, 226);
//		points[2] = QPointF(153, 206);
//		painter->drawPolygon(points, 3);		
//
//		points[0] = QPointF(133, 30);
//		points[1] = QPointF(133, 70);
//		points[2] = QPointF(103, 50);
//		painter->drawPolygon(points, 3);
//	}	
//	
//	ThymioResetAction::ThymioResetAction(QGraphicsItem *parent) : 
//		ThymioButton(false, 1.0, true, parent)
//	{
//		setData(0, "action");		
//		setData(1, "reset");
//		
//		logo = new Reset(this);
//	}
//
//	QPixmap ThymioResetAction::image(bool on)
//	{
//		QPixmap pixmap = ThymioButton::image(on);
//		QPainter painter(&pixmap);
//
//		painter.translate(logo->pos());
//		logo->paint(&painter, 0, 0);
//		painter.resetTransform();
//		
//		return pixmap;
//	}	

};

