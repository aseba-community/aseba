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

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QSlider>
#include <QTimeLine>
#include <QGraphicsProxyWidget>
#include <QGraphicsSvgItem>
#include <QtCore/qmath.h>
#include <QDebug>

#include <cassert>

#include "ActionBlocks.h"
#include "Buttons.h"
#include "UsageLogger.h"

namespace Aseba { namespace ThymioVPL
{
	// Move Action
	MoveActionBlock::MoveActionBlock( QGraphicsItem *parent ) :
		Block("action", "move", parent)
	{
		new QGraphicsSvgItem (":/images/vpl_background_motor.svgz", this);

		for(int i=0; i<2; i++)
		{
			QSlider *s = new QSlider(Qt::Vertical);
			s->setRange(-10,10);
			s->setStyleSheet(
				"QSlider { border: 0px; padding: 0px; background: transparent; }"
				"QSlider::groove:vertical { "
					"border: none; "
					"background: transparent; }"
				"QSlider::handle:vertical { "
					"background: transparent; "
					"border: 4px solid black; height: 40px; margin: 0px; }"
			);
			s->setSliderPosition(0);
			
			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setAcceptDrops(false);
			w->setWidget(s);
			w->resize(48, 226);
			w->setPos(10+i*188, 15);
			
			sliders.push_back(s);
			
			connect(s, SIGNAL(valueChanged(int)), SLOT(valueChangeDetected()));
			connect(s, SIGNAL(valueChanged(int)), SIGNAL(contentChanged()));
			connect(s, SIGNAL(sliderPressed()), SLOT(clearChangedFlag()));
			connect(s, SIGNAL(sliderMoved(int)), SLOT(setChangedFlag()));
			connect(s, SIGNAL(sliderReleased()), SLOT(emitUndoCheckpointAndClearIfChanged()));
			USAGE_LOG(logSignal(s,SIGNAL(sliderReleased()),i,this));
		}
		
		thymioBody = new ThymioBody(this, -70);
		thymioBody->setUp(false);
		thymioBody->setPos(128,128);
		thymioBody->setScale(0.2);
		
		timer = new QTimeLine(2000, this);
		timer->setFrameRange(0, 150);
		timer->setCurveShape(QTimeLine::LinearCurve);
		timer->setLoopCount(1);
		connect(timer, SIGNAL(frameChanged(int)), SLOT(frameChanged(int)));
	}

	MoveActionBlock::~MoveActionBlock()
	{
	}
	
	void MoveActionBlock::frameChanged(int frame)
	{
		qreal pt[2];
		for(int i=0; i<2; i++) 
			pt[i] = (sliders[i]->value())*0.06*50; // [-30,30]
		
		const qreal step = frame/200.0;
		const qreal angle = (pt[0]-pt[1])*3*step;
		
		if (fabs(pt[1]-pt[0]) < 10e-4)
		{
			thymioBody->setPos(QPointF(128,128-(pt[1]+pt[0])*1.2*step));
		}
		else
		{
			const qreal center = -23.5*(pt[1]+pt[0])/(pt[1]-pt[0]);
			const qreal radNegAngle = -angle*3.14/180;
			thymioBody->setPos(QPointF(center*(1-cos(radNegAngle))+128,center*sin(radNegAngle)+128));
		}
		thymioBody->setRotation(angle);
	}

	void MoveActionBlock::valueChangeDetected()
	{
		timer->stop();
		timer->start();
	}
	
	int MoveActionBlock::getValue(unsigned i) const
	{ 
		if (int(i)<sliders.size()) 
			return sliders[i]->value()*50;
		return -1;
	}

	void MoveActionBlock::setValue(unsigned i, int value) 
	{ 
		if(int(i)<sliders.size()) 
			sliders[i]->setSliderPosition(value/50);
	}
	
	QVector<quint16> MoveActionBlock::getValuesCompressed() const
	{
		return QVector<quint16>(1, 
			((sliders[0]->value()+10) << 8) |
			 (sliders[1]->value()+10)
		);
	}
	
	// Color Action
	ColorActionBlock::ColorActionBlock( QGraphicsItem *parent, bool top) :
		BlockWithBody("action", top ? "colortop" : "colorbottom", top, parent)
	{
		const char *sliderColors[] = { "FF0000", "00FF00", "0000FF" };
		
		for(unsigned i=0; i<3; i++)
		{
			QSlider *s = new QSlider(Qt::Horizontal);
			s->setRange(0,32);
			s->setStyleSheet(QString(
				"QSlider { border: 0px; padding: 0px; }"
				"QSlider::groove:horizontal { "
					"border: transparent;"
					"background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #000000, stop:1 #%0); "
				"}"
				"QSlider::handle:horizontal { "
					"background: white; "
					"border: 4px solid black; width: 40px; margin: 0px;"
				"}"
				";").arg(sliderColors[i]));
			s->setSliderPosition(0);

			QGraphicsProxyWidget *w = new QGraphicsProxyWidget(this);
			w->setAcceptDrops(false);
			w->setWidget(s);
			w->resize(202, 48);
			w->setPos(27, 60+i*64);
			
			sliders.push_back(s);
			
			connect(s, SIGNAL(valueChanged(int)), SLOT(valueChangeDetected()));
			connect(s, SIGNAL(valueChanged(int)), SIGNAL(contentChanged()));
			connect(s, SIGNAL(sliderPressed()), SLOT(clearChangedFlag()));
			connect(s, SIGNAL(sliderMoved(int)), SLOT(setChangedFlag()));
			connect(s, SIGNAL(sliderReleased()), SLOT(emitUndoCheckpointAndClearIfChanged()));
			USAGE_LOG(logSignal(s,SIGNAL(sliderReleased()),i,this));
		}
	}

	int ColorActionBlock::getValue(unsigned i) const
	{ 
		if (int(i)<sliders.size()) 
			return sliders[i]->value(); 
		return -1;
	}
	
	void ColorActionBlock::setValue(unsigned i, int value) 
	{ 
		if (int(i)<sliders.size()) 
			sliders[i]->setSliderPosition(value);
	}
	
	QVector<quint16> ColorActionBlock::getValuesCompressed() const
	{
		unsigned value(0);
		for (int i=0; i<sliders.size(); ++i)
		{
			value *= 33;
			value += sliders[i]->value();
		}
		assert(value <= 65535);
		return QVector<quint16>(1, value);
	}
	
	void ColorActionBlock::valueChangeDetected()
	{
		update();
	}
	
	void ColorActionBlock::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		bodyColor = QColor( sliders[0]->value()*5.46875+80, 
							sliders[1]->value()*5.46875+80, 
							sliders[2]->value()*5.46875+80);
		BlockWithBody::paint(painter, option, widget);
		if (!up)
		{
			painter->setBrush(Qt::white);
			const QRect myRect(128-15, 25, 30, 30);
			QRadialGradient gradient(128, 40, 15);
			gradient.setColorAt(0, Qt::white);
			gradient.setColorAt(1, Qt::transparent);
			painter->fillRect(myRect, gradient);
		}
	}
	
	TopColorActionBlock::TopColorActionBlock(QGraphicsItem *parent):
		ColorActionBlock(parent, true)
	{}
	
	BottomColorActionBlock::BottomColorActionBlock(QGraphicsItem *parent):
		ColorActionBlock(parent, false)
	{}
	
	// Sound Action
	SoundActionBlock::SoundActionBlock(QGraphicsItem *parent) :
		Block("action", "sound", parent),
		dragging(false)
	{
		notes[0] = 3; durations[0] = 1;
		notes[1] = 4; durations[1] = 1;
		notes[2] = 3; durations[2] = 2;
		notes[3] = 2; durations[3] = 1;
		notes[4] = 1; durations[4] = 1;
		notes[5] = 2; durations[5] = 2;
		
		new QGraphicsSvgItem (":/images/notes.svgz", this);
	}
	
	void SoundActionBlock::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Block::paint(painter, option, widget);
		
		painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
		for (unsigned row = 0; row < 5; ++row)
		{
			
			//painter->fillRect(17, 60 + row*37, 222, 37, QColor::fromHsv((5-row)*51, 100, 255));
			painter->fillRect(17, 60 + row*37 + 2, 222, 33, Qt::lightGray);
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
	
	void SoundActionBlock::mousePressEvent(QGraphicsSceneMouseEvent * event)
	{
		if (event->button() == Qt::LeftButton)
		{
			unsigned noteIdx, noteVal;
			bool ok;
			idxAndValFromPos(event->pos(), &ok, noteIdx, noteVal);
			if (ok)
			{
				const unsigned& note(notes[noteIdx]);
				if (note == 5)
				{
					setDuration(noteIdx, 1);
					setNote(noteIdx, noteVal);
				}
				else if (note == noteVal)
				{
					if (durations[noteIdx] == 2)
					{
						setDuration(noteIdx, 2);
						setNote(noteIdx, 5);
					}
					else
						setDuration(noteIdx, (durations[noteIdx] % 2) + 1);
				}
				else
					setNote(noteIdx, noteVal);
				
				dragging = true;
				setChangedFlag();
				return;
			}
		}
		
		Block::mousePressEvent(event);
	}
	
	void SoundActionBlock::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
	{
		if (dragging)
		{
			unsigned noteIdx, noteVal;
			bool ok;
			idxAndValFromPos(event->pos(), &ok, noteIdx, noteVal);
			if (ok)
				setNote(noteIdx, noteVal);
		}
		else
			Block::mouseMoveEvent(event);
	}
	
	void SoundActionBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
	{
		if (dragging)
		{
			dragging = false;
			emitUndoCheckpointAndClearIfChanged();
		}
		else
			Block::mouseReleaseEvent(event);
	}
	
	void SoundActionBlock::idxAndValFromPos(const QPointF& pos, bool* ok, unsigned& noteIdx, unsigned& noteVal)
	{
		if (QRectF(17,60,220,185).contains(pos))
		{
			noteIdx = (int(pos.x())-17)/37;
			noteVal = 4-(int(pos.y())-60)/37;
			if (ok) *ok = true;
		}
		else
		{
			if (ok) *ok = false;
		}
	}
	
	void SoundActionBlock::setNote(unsigned noteIdx, unsigned noteVal)
	{
		unsigned& note(notes[noteIdx]);
		if (note != noteVal)
		{
			note = noteVal;
			update();
			USAGE_LOG(logBlockAction(SET_NOTE, getName(), getType(), UsageLogger::getRow(this), noteIdx, NULL, &noteVal, NULL,NULL));
			emit contentChanged();
		}
	}
	
	void SoundActionBlock::setDuration(unsigned noteIdx, unsigned durationVal)
	{
		unsigned& duration(durations[noteIdx]);
		if (duration != durationVal)
		{
			duration = durationVal;
			update();
			USAGE_LOG(logBlockAction(SET_DURATION, getName(), getType(), UsageLogger::getRow(this), noteIdx, NULL, &durationVal, NULL,NULL));
			emit contentChanged();
		}
	}
	
	int SoundActionBlock::getValue(unsigned i) const
	{ 
		if (i<6) 
			return notes[i] | (durations[i] << 8); 
		return -1;
	}
	
	void SoundActionBlock::setValue(unsigned i, int value) 
	{ 
		if (i<6)
		{
			notes[i] = value & 0xff;
			durations[i] = (value >> 8) & 0xff;
			update();
			emit contentChanged();
		}
	}
	
	QVector<quint16> SoundActionBlock::getValuesCompressed() const
	{
		unsigned compressedNotes = 0;
		unsigned compressedDurations = 0;
		for (unsigned i = 0; i < 6; ++i)
		{
			compressedNotes *= 6;
			compressedNotes += notes[i];
			compressedDurations *= 3;
			compressedDurations += durations[i];
		}
		return (QVector<quint16>() << compressedNotes << compressedDurations);
	}
	
	// TimerActionBlock
	TimerActionBlock::TimerActionBlock(QGraphicsItem *parent) :
		Block("action", "timer", parent),
		dragging(false),
		duration(1.0)
	{
		new QGraphicsSvgItem (":/images/timer.svgz", this);
		
		timer = new QTimeLine(duration, this);
		timer->setFrameRange(0, duration/40);
		timer->setCurveShape(QTimeLine::LinearCurve);
		timer->setLoopCount(1);
		connect(timer, SIGNAL(frameChanged(int)), SLOT(frameChanged(int)));
	}
	
	void TimerActionBlock::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		Block::paint(painter, option, widget);
		
		painter->setBrush(Qt::white);
		painter->drawEllipse(128-72, 136-72, 72*2, 72*2);
		
		const float angle(float(duration) * 2.f * M_PI / 4000.f);
		painter->setBrush(Qt::darkBlue);
		painter->drawPie(QRectF(128-50, 136-50, 100, 100), 16*90, -angle*16*360/(2*M_PI));
		
		const float leftDuration(duration-float(timer->currentFrame())*40.f);
		if (timer->state() == QTimeLine::Running)
		{
			const float leftAngle(leftDuration * 2.f * M_PI / (4000.f));
			painter->setBrush(Qt::red);
			painter->drawPie(QRectF(128-50, 136-50, 100, 100), 16*90, -leftAngle*16*360/(2*M_PI));
		}
		
		painter->setPen(QPen(Qt::black, 10, Qt::SolidLine, Qt::RoundCap));
		
		// drawing handle
		{
			static const QPointF points[5] = {
				QPointF(0, 5),
				QPointF(35, 12),
				QPointF(60, 0),
				QPointF(35, -12),
				QPointF(0, -5)
			};
			painter->save();
			painter->translate(128,136);
			painter->rotate((angle-M_PI/2) * 180 / M_PI);
			painter->setBrush(Qt::white);
			painter->setPen(QPen(Qt::black, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
			painter->drawConvexPolygon(points, 5);
			painter->setBrush(Qt::lightGray);
			painter->drawEllipse(-10,-10,20,20);
			painter->restore();
		}
		//painter->drawLine(128, 136, 128+sinf(angle)*50, 136-cosf(angle)*50);
		
		
	}
	
	void TimerActionBlock::frameChanged(int frame)
	{
		update();
	}
	
	void TimerActionBlock::mousePressEvent(QGraphicsSceneMouseEvent * event)
	{
		bool ok;
		const unsigned duration(durationFromPos(event->pos(), &ok));
		if (event->button() == Qt::LeftButton && ok)
		{
			clearChangedFlag();
			setDuration(duration);
			dragging = true;
		}
		else
			Block::mousePressEvent(event);
	}
	
	void TimerActionBlock::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
	{
		if (dragging)
		{
			bool ok;
			const unsigned duration(durationFromPos(event->pos(), &ok));
			if (ok)
				setDuration(duration);
		}
		else
			Block::mouseMoveEvent(event);
	}
	
	void TimerActionBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
	{
		if (dragging)
		{
			dragging = false;
			emitUndoCheckpointAndClearIfChanged();
			USAGE_LOG(logBlockAction(TIMER, getName(), getType(), UsageLogger::getRow(this), 0, NULL, NULL, &this->duration, NULL));
		}
		else
			Block::mouseReleaseEvent(event);
	}
	
	unsigned TimerActionBlock::durationFromPos(const QPointF& pos, bool* ok) const
	{
		const double TIMER_RESOLUTION = 4;
		const QPointF localPos(pos-QPointF(128,136));
		if (sqrt(localPos.x()*localPos.x()+localPos.y()*localPos.y()) <= 70)
		{
			if (ok) *ok = true;
			double duration = (atan2(-localPos.x(), localPos.y()) + M_PI) * 4. / (2*M_PI);
			return round(duration * TIMER_RESOLUTION) * 1000 / TIMER_RESOLUTION;
		}
		if (ok) *ok = false;
		return 0;
	}
	
	int TimerActionBlock::getValue(unsigned i) const
	{ 
		return duration;
	}
	
	void TimerActionBlock::setValue(unsigned i, int value) 
	{ 
		setDuration(value);
	}
	
	QVector<quint16> TimerActionBlock::getValuesCompressed() const
	{
		return QVector<quint16>(1, duration);
	}
	
	void TimerActionBlock::setDuration(unsigned duration)
	{
		if (duration != this->duration)
		{
			this->duration = duration;
			
			update();
			emit contentChanged();
			setChangedFlag();
			
			timer->stop();
			timer->setDuration(duration);
			timer->setFrameRange(0, duration/40);
			timer->start();
		}
	}

	// State Filter Action
	StateFilterActionBlock::StateFilterActionBlock(QGraphicsItem *parent) : 
		StateFilterBlock("action", "setstate", parent)
	{
	}
	
} } // namespace ThymioVPL / namespace Aseba
