#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QSlider>
#include <QTimeLine>
#include <QGraphicsProxyWidget>
#include <QGraphicsSvgItem>
#include <QtCore/qmath.h>

#include "ActionCards.h"
#include "Buttons.h"

namespace Aseba { namespace ThymioVPL
{
	// Move Action
	MoveActionCard::MoveActionCard( QGraphicsItem *parent ) :
		Card(false, false, parent)
	{
		setData(0, "action");
		setData(1, "move");

		for(int i=0; i<2; i++)
		{
			QSlider *s = new QSlider(Qt::Vertical);
			s->setRange(-10,10);
			s->setStyleSheet(
				"QSlider { border: 0px; padding: 0px; }"
				"QSlider::groove:vertical { "
					"border: 4px solid black; "
					"background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00FF00, stop:0.25 #FFFF00, stop:0.5 #FF0000, stop:0.75 #FFFF00, stop:1 #00FF00 ); }"
				"QSlider::handle:vertical { "
					"background: white; "
					"border: 4px solid black; height: 44px; margin: -4px; }"
			);
			s->setSliderPosition(0);

			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setAcceptDrops(false);
			w->setWidget(s);
			w->resize(48, 226);
			w->setPos(10+i*188, 15);
			
			sliders.push_back(s);
			
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChangeDetected()));
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(updateIRButtonAndNotify()));
		}
		
		thymioBody = new ThymioBody(this, -70);
		thymioBody->setUp(false);
		thymioBody->setPos(128,128+14);
		thymioBody->setScale(0.2);
		
		timer = new QTimeLine(2000, this);
		timer->setFrameRange(0, 150);
		timer->setCurveShape(QTimeLine::LinearCurve);
		timer->setLoopCount(1);
		connect(timer, SIGNAL(frameChanged(int)), SLOT(frameChanged(int)));
	}

	MoveActionCard::~MoveActionCard()
	{
	}
	
	void MoveActionCard::frameChanged(int frame)
	{
		qreal pt[2];
		for(int i=0; i<2; i++) 
			pt[i] = (sliders[i]->value())*0.06*50; // [-30,30]
		
		const qreal step = frame/200.0;
		const qreal angle = (pt[0]-pt[1]-0.04)*3*step;
		const qreal center = -23.5*(pt[1]+pt[0])/(pt[1] == pt[0] ? 0.03 : (pt[1]-pt[0]));
		
		thymioBody->setPos(QPointF(center*(1-cos(-angle*3.14/180))+128,center*sin(-angle*3.14/180)+128+14));
		thymioBody->setRotation(angle);
	}

	void MoveActionCard::valueChangeDetected()
	{
		timer->stop();
		timer->start();
	}
	
	int MoveActionCard::getValue(int i) const
	{ 
		if (i<sliders.size()) 
			return sliders[i]->value()*50;
		return -1;
	}

	void MoveActionCard::setValue(int i, int value) 
	{ 
		if(i<sliders.size()) 
			sliders[i]->setSliderPosition(value/50);
	}
	
	// Color Action
	ColorActionCard::ColorActionCard( QGraphicsItem *parent, bool top) :
		CardWithBody(false, top, false, parent)
	{
		setData(0, "action");
		if (top)
			setData(1, "colortop");
		else
			setData(1, "colorbottom");

		const char *sliderColors[] = { "FF0000", "00FF00", "0000FF" };
		
		for(int i=0; i<3; i++)
		{
			QSlider *s = new QSlider(Qt::Horizontal);
			s->setRange(0,32);
			s->setStyleSheet(QString(
				"QSlider { border: 0px; padding: 0px; }"
				"QSlider::groove:horizontal { "
					"border: 4px solid black;"
					"background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #000000, stop:1 #%0); "
				"}"
				"QSlider::handle:horizontal { "
					"background: white; "
					"border: 4px solid black; width: 40px; margin: -4px;"
				"}"
				";").arg(sliderColors[i]));
			s->setSliderPosition(16);

			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setAcceptDrops(false);
			w->setWidget(s);
			w->resize(202, 48);
			w->setPos(27, 60+i*64);
			
			sliders.push_back(s);
			
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChangeDetected()));
			connect(s, SIGNAL(valueChanged(int)), this, SLOT(updateIRButtonAndNotify()));
		}
	}

	int ColorActionCard::getValue(int i) const
	{ 
		if(i<sliders.size()) 
			return sliders[i]->value(); 
		return -1;
	}
	
	void ColorActionCard::setValue(int i, int status) 
	{ 
		if(i<sliders.size()) 
			sliders[i]->setSliderPosition(status);
	}
	
	void ColorActionCard::valueChangeDetected()
	{
		update();
	}
	
	void ColorActionCard::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		bodyColor = QColor( sliders[0]->value()*5.46875+80, 
							sliders[1]->value()*5.46875+80, 
							sliders[2]->value()*5.46875+80);
		CardWithBody::paint(painter, option, widget);
	}
	
	TopColorActionCard::TopColorActionCard(QGraphicsItem *parent):
		ColorActionCard(parent, true)
	{}
	
	BottomColorActionCard::BottomColorActionCard(QGraphicsItem *parent):
		ColorActionCard(parent, false)
	{}
	
	// Sound Action
	SoundActionCard::SoundActionCard(QGraphicsItem *parent) :
		CardWithBody(false, true, false, parent)
	{
		setData(0, "action");
		setData(1, "sound");
		
		notes[0] = 3; durations[0] = 1;
		notes[1] = 4; durations[1] = 1;
		notes[2] = 3; durations[2] = 2;
		notes[3] = 2; durations[3] = 1;
		notes[4] = 1; durations[4] = 1;
		notes[5] = 2; durations[5] = 2;
		
		new QGraphicsSvgItem (":/images/notes.svgz", this);
	}
	
	void SoundActionCard::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		CardWithBody::paint(painter, option, widget);
		
		painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
		for (unsigned row = 0; row < 5; ++row)
		{
			
			painter->fillRect(17, 60 + row*37, 222, 37, QColor::fromHsv((5-row)*51, 100, 255));
			for (unsigned col = 0; col < 6; ++col)
			{
				if (notes[col] == 4-row)
				{
					if (durations[col] == 2)
					{
						painter->setPen(QPen(Qt::black, 4, Qt::SolidLine));
						painter->setBrush(Qt::white);
						painter->drawEllipse(17 + col*37 + 3, 60 + row*37 + 3, 31, 31);
						
					}
					else
					{
						painter->setPen(Qt::NoPen);
						painter->setBrush(Qt::black);
						painter->drawEllipse(17 + col*37 + 2, 60 + row*37 + 2, 33, 33);
					}
				}
			}
		}
	}
	
	void SoundActionCard::mousePressEvent(QGraphicsSceneMouseEvent * event)
	{
		if (event->button() == Qt::LeftButton)
		{
			const QPointF& pos(event->pos());
			if (QRectF(17,60,220,185).contains(pos))
			{
				const size_t noteIdx((int(pos.x())-17)/37);
				const size_t noteVal(4-(int(pos.y())-60)/37);
				unsigned& note(notes[noteIdx]);
				
				if (note == noteVal)
					durations[noteIdx] = (durations[noteIdx] % 2) + 1;
				else
					note = noteVal;
			}
			update();
			updateIRButtonAndNotify();
		}
	}
	
	int SoundActionCard::getValue(int i) const
	{ 
		if (i<6) 
			return notes[i] | (durations[i] << 8); 
		return -1;
	}
	
	// TODO: FIXME: use size_t for index
	void SoundActionCard::setValue(int i, int value) 
	{ 
		if (i<6) 
		{
			notes[i] = value;
			update();
			updateIRButtonAndNotify();
		}
	}

	// State Filter Action
	StateFilterActionCard::StateFilterActionCard(QGraphicsItem *parent) : 
		CardWithButtons(false, true, false, parent)
	{
		setData(0, "action");
		setData(1, "memory");
		
		const int angles[4] = {0,90,270,180};
		for(uint i=0; i<4; i++)
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-20,-20,40,40), GeometryShapeButton::QUARTER_CIRCLE_BUTTON, this, Qt::lightGray, Qt::darkGray);
			button->setPos(98 + (i%2)*60, 98 + (i/2)*60);
			button->setRotation(angles[i]);
			button->addState(QColor(255,128,0));
			button->addState(Qt::white);

			buttons.push_back(button);
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButtonAndNotify()));
		}
	}	
} } // namespace ThymioVPL / namespace Aseba
