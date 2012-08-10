#include <QPainter>
#include <QGraphicsSceneMouseEvent>

#include "ThymioButtons.h"


namespace Aseba
{
	// Move Action
	ThymioMoveAction::ThymioMoveAction( QGraphicsItem *parent ) :
		ThymioButton(false, 0.2, false, false, parent)
	{
		setData(0, "action");
		setData(1, "move");

		QTransform transMatrix(2.0,0.0,0.0,0.0,2.3,0.0,0.0,0.0,1.0);
				
		for(int i=0; i<2; i++)
		{
			QSlider *s = new QSlider(Qt::Vertical);
			s->setRange(-500,500);
			s->setStyleSheet("QSlider::groove:vertical { width: 14px; border: 2px solid #000000; "
							  "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00FF00, stop:0.25 #FFFF00, stop:0.5 #FF0000, stop:0.75 #FFFF00, stop:1 #00FF00 ); }"
						      "QSlider::handle:vertical { "
						      "background: #FFFFFF; "
						      "border: 2px solid #000000; height: 10px; width: 20px; margin: 0px 0; }");
			s->setTickInterval(50);

			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setWidget(s);
			w->setPos(10+i*200, 15);
			w->setTransform(transMatrix);
			
			sliders.push_back(s);
			widgets.push_back(w);
			
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChangeDetected()));
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(updateIRButton()));
		}		

		timer = new QTimeLine(2000);
		timer->setFrameRange(0, 100);
		timer->setCurveShape(QTimeLine::LinearCurve);
		animation = new QGraphicsItemAnimation(this);
		animation->setItem(thymioBody);
		animation->setTimeLine(timer);				
		thymioBody->setTransformOriginPoint(0,-14);//(pt[1]+pt[0]) == 0 ? 0 : (abs(pt[1])-abs(pt[0]))/(abs(pt[1])+abs(pt[0]))*22.2,-25);
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
			pt[i] = (sliders[i]->value())*0.06; // [-30,30]

		float rate = abs(pt[0])+abs(pt[1]);
		timer->stop();
		timer->setLoopCount(1);//+(int)(rate*0.005));

#if 0
		double angle=0;
		double fspeed=(pt[0]+pt[1])*0.5; // (lspeed + rspeed)/2
		double aspeed=atan((pt[1]-pt[0])/47.6); // (rspeed - lspeed)/dist
		QPointF pos(0.0, 0.0);
		
		for (int i = 0; i < 200; ++i)
		{
			qreal step = i/200.0;
			animation->setPosAt(step, pos+QPointF(128,128));
			animation->setRotationAt(step, angle*180/3.14);
						
			pos += QPointF(fspeed*cos(angle+aspeed/200.0*0.5), fspeed*sin(angle+aspeed/200.0*0.5));
			angle += aspeed/200.0;
		}
#else			

		double angle=0;
		double center = -23.5*(pt[1]+pt[0])/(pt[1] == pt[0] ? 0.03 : (pt[1]-pt[0]));		
		
		for (int i = 0; i < 200; ++i)
		{
			qreal step = i/200.0;
			
			angle = (pt[0]-pt[1]-0.04)*3*step;
			animation->setPosAt(step, QPointF(center*(1-cos(-angle*3.14/180))+128,center*sin(-angle*3.14/180)+128));	
			animation->setRotationAt(step, angle);
		}


#endif

		timer->start();
	}

	QPixmap ThymioMoveAction::image(bool on)
	{
		QPixmap pixmap(256, 256);
		pixmap.fill(buttonColor);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);

		painter.translate(QPointF(128,128));
		painter.scale(0.4,0.4);
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

		QTransform transMatrix(1.0,0.0,0.0,0.0,2.0,0.0,0.0,0.0,1.0);
		QString sliderColor("FF0000");
		
		for(int i=0; i<3; i++)
		{
			if( i == 1 ) sliderColor="00FF00";
			else if( i == 2 ) sliderColor="0000FF";

			QSlider *s = new QSlider(Qt::Horizontal);
			s->setRange(0,32);
			s->setStyleSheet(QString("QSlider::groove:horizontal { height: 14px; border: 2px solid #000000; "
							  "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #000000, stop:1 #%0); }"
						      "QSlider::handle:horizontal { "
						      "background: #FFFFFF; "
						      "border: 5px solid #000000; width: 18px; margin: -2px 0; }").arg(sliderColor));

			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setWidget(s);
			w->setPos(27, 70+i*60);
			w->setTransform(transMatrix);
			
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

};

