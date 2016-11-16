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

#include <cassert>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSvgItem>
#include <QSlider>
#include <QGraphicsProxyWidget>
#include <QGraphicsEllipseItem>
#include <QtCore/qmath.h>
#include <QDebug>

#include "EventBlocks.h"
#include "Buttons.h"
#include "UsageLogger.h"
#include "Style.h"

#define deg2rad(x) ((x) * M_PI / 180.)

namespace Aseba { namespace ThymioVPL
{
	// Buttons Event
	const QRectF ArrowButtonsEventBlock::buttonPoses[3] = {
		QRectF(32+(40+36)*0, 204, 40, 40),
		QRectF(32+(40+36)*1, 204, 40, 40),
		QRectF(32+(40+36)*2, 204, 40, 40)
	};
	
	ArrowButtonsEventBlock::ArrowButtonsEventBlock(bool advanced, QGraphicsItem *parent) : 
		BlockWithButtons("event", "button", true, parent),
		advanced(advanced),
		mode(MODE_ARROW)
	{
		const QColor color(Qt::red);
		
		// robot arrows
		
		// top, left, bottom, right
		for(int i=0; i<4; i++) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-25, -21.5, 50, 43), GeometryShapeButton::TRIANGULAR_BUTTON, this, Style::unusedButtonFillColor,  Style::unusedButtonStrokeColor);

			qreal offset = (qreal)i;
			button->setRotation(-90*offset);
			button->addState(color);
			buttons.push_back(button);

			connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
			connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
			USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),buttons.size()-1,this));
		}
		// center
		GeometryShapeButton *button = new GeometryShapeButton(QRectF(-25, -25, 50, 50), GeometryShapeButton::CIRCULAR_BUTTON, this, Style::unusedButtonFillColor, Style::unusedButtonStrokeColor);
		
		button->addState(color);
		buttons.push_back(button);
		
		connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
		connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
		USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),buttons.size()-1,this));
		setButtonsPos(advanced);
		
		// rc arrows
		
		// top, left, bottom, right
		for(int i=0; i<4; i++) 
		{
			button = new GeometryShapeButton(QRectF(-25, -25, 50, 50), GeometryShapeButton::QUARTER_CIRCLE_BUTTON, this, Style::unusedButtonFillColor,  Style::unusedButtonStrokeColor);

			qreal offset = (qreal)i;
			button->setRotation(-90*offset+45);
			button->setPos(128 - 54*qSin(1.57079633*offset),
						   100 - 54*qCos(1.57079633*offset));
			button->addState(color);
			button->setVisible(false);
			buttons.push_back(button);

			connect(button, SIGNAL(stateChanged()), SLOT(ensureSingleRCArrowButtonSelected()));
			connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
			connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
			USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),buttons.size()-1,this));
		}
		// center
		button = new GeometryShapeButton(QRectF(-25, -25, 50, 50), GeometryShapeButton::CIRCULAR_BUTTON, this, Style::unusedButtonFillColor, Style::unusedButtonStrokeColor);
		
		button->addState(color);
		button->setPos(128, 100);
		button->setVisible(false);
		button->setValue(1);
		buttons.push_back(button);
		
		connect(button, SIGNAL(stateChanged()), SLOT(ensureSingleRCArrowButtonSelected()));
		connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
		connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
		USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),buttons.size()-1,this));
		
		// rc keypad
		
		for (int row=0; row<4; ++row)
			for (int col=0; col<3; ++col)
			{
				button = new GeometryShapeButton(QRectF(0, 0, 48, 30), GeometryShapeButton::RECTANGULAR_BUTTON, this, Style::unusedButtonFillColor, Style::unusedButtonStrokeColor);
				
				button->addState(color);
				button->setPos(40+col*64, 12+row*48);
				button->setVisible(false);
				if (row == 0 && col == 0)
					button->setValue(1);
				buttons.push_back(button);
				
				connect(button, SIGNAL(stateChanged()), SLOT(ensureSingleRCKeypadButtonSelected()));
				connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
				connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
				USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),buttons.size()-1,this));
			}
	}
	
	void ArrowButtonsEventBlock::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		// if simple mode, simply return
		if (!advanced)
		{
			// paint parent
			BlockWithButtons::paint(painter, option, widget);
			return;
		}
		
		// paint block
		Block::paint(painter, option, widget);
		
		painter->setPen(Qt::NoPen);
		painter->setBrush(Qt::white);
		if (mode == MODE_ARROW)
		{
			painter->drawChord(128-110, 128-122, 220, 130, 0, 2880);
			painter->drawRoundedRect(128-110, 128-63, 220, 127, 5, 5);
		}
		else
			painter->drawRoundedRect(128-110, 128-122, 220, 186, 5, 5);
		
		// draw buttons
		for (unsigned i=0; i<3; ++i)
		{
			if (mode == i)
			{
				painter->setPen(QPen(Qt::black, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				painter->setBrush(Qt::red);
			}
			else
			{
				painter->setPen(QPen(Style::unusedButtonStrokeColor, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				painter->setBrush(Style::unusedButtonFillColor);
			}
			painter->drawEllipse(buttonPoses[i]);
		}
	}
	
	int ArrowButtonsEventBlock::getValue(unsigned i) const
	{
		if (i < 5 && mode == MODE_ARROW)
			return BlockWithButtons::getValue(i);
		else if (i == 5)
			return mode;
		else if (i == 6 && mode == MODE_RC_ARROW)
			return getSelectedRCArrowButton();
		else if (i == 6 && mode == MODE_RC_KEYPAD)
			return getSelectedRCKeypadButton();
		else
			return 0;
	}
	
	void ArrowButtonsEventBlock::setValue(unsigned i, int value)
	{
		if (i < 5)
		{
			BlockWithButtons::setValue(i, value);
		}
		else if (i == 5)
		{
			setMode(value);
		}
		else if (i == 6 && mode == MODE_RC_ARROW)
		{
			assert(value >= 0);
			assert(value < 5);
			buttons.at(5+value)->setValue(1);
			ensureSingleRCArrowButtonSelected(value);
			emit contentChanged();
		}
		else if (i == 6 && mode == MODE_RC_KEYPAD)
		{
			assert(value >= 0);
			assert(value < 12);
			buttons.at(10+value)->setValue(1);
			ensureSingleRCKeypadButtonSelected(value);
			emit contentChanged();
		}
	}
	
	QVector<quint16> ArrowButtonsEventBlock::getValuesCompressed() const
	{
		QVector<quint16> result;
		// mode
		result.push_back(mode);
		if (mode == MODE_ARROW)
		{
			// robot buttons
			unsigned value(0);
			for (int i=0; i<5; ++i)
			{
				value *= buttons[i]->valuesCount();
				value += buttons[i]->getValue();
			}
			assert(value <= 65535);
			result.push_back(value);
		}
		else if (mode == MODE_RC_ARROW)
		{
			const int value(getSelectedRCArrowButton());
			assert(value >= 0);
			result.push_back(unsigned(value));
		}
		else if (mode == MODE_RC_KEYPAD)
		{
			const int value(getSelectedRCKeypadButton());
			assert(value >= 0);
			result.push_back(unsigned(value));
		}
		else
			assert(false);
		return result;
	}
	
	bool ArrowButtonsEventBlock::isAnyAdvancedFeature() const
	{
		return mode != MODE_ARROW;
	}
	
	void ArrowButtonsEventBlock::setAdvanced(bool advanced)
	{
		if (!advanced)
		{
			setMode(MODE_ARROW);
		}
		this->advanced = advanced;
		setButtonsPos(advanced);
		update();
	}
	
	void ArrowButtonsEventBlock::mousePressEvent(QGraphicsSceneMouseEvent * event)
	{
		if (event->button() == Qt::LeftButton)
		{
			for (unsigned i=0; i<3; ++i)
			{
				if (buttonPoses[i].contains(event->pos()))
				{
					setMode(i);
					USAGE_LOG(logEventBlockMode(this->name, this->type,i));
					emit undoCheckpoint();
					return;
				}
			}
		}
		
		Block::mousePressEvent(event);
	}
	
	void ArrowButtonsEventBlock::setMode(unsigned mode)
	{
		if (!advanced)
			return;
		
		if (mode != this->mode)
		{
			this->mode = mode;
			
			if (mode == MODE_ARROW)
			{
				for (size_t i = 0; i<5; ++i)
					buttons[i]->setVisible(true);
				for (size_t i = 5; i<10; ++i)
					buttons[i]->setVisible(false);
				for (size_t i = 10; i<22; ++i)
					buttons[i]->setVisible(false);
			}
			else if (mode == MODE_RC_ARROW)
			{
				for (size_t i = 0; i<5; ++i)
					buttons[i]->setVisible(false);
				for (size_t i = 5; i<10; ++i)
					buttons[i]->setVisible(true);
				for (size_t i = 10; i<22; ++i)
					buttons[i]->setVisible(false);
				// select one if none is selected
				if (getSelectedRCArrowButton() < 0)
					buttons[9]->setValue(1);
			}
			else if (mode == MODE_RC_KEYPAD)
			{
				for (size_t i = 0; i<5; ++i)
					buttons[i]->setVisible(false);
				for (size_t i = 5; i<10; ++i)
					buttons[i]->setVisible(false);
				for (size_t i = 10; i<22; ++i)
					buttons[i]->setVisible(true);
				// select one if none is selected
				if (getSelectedRCKeypadButton() < 0)
					buttons[10]->setValue(1);
			}
			else
				assert(false);
			
			update();
			emit contentChanged();
		}
	}
	
	void ArrowButtonsEventBlock::setButtonsPos(bool advanced)
	{
		if (advanced)
		{
			for (int i=0; i<4; i++)
			{
				qreal offset = (qreal)i;
				buttons[i]->setPos(128 - 64*qSin(1.57079633*offset), 
								   100 - 64*qCos(1.57079633*offset));
			}
			buttons[4]->setPos(128, 100);
		}
		else
		{
			for (int i=0; i<4; i++)
			{
				qreal offset = (qreal)i;
				buttons[i]->setPos(128 - 70*qSin(1.57079633*offset), 
								   128 - 70*qCos(1.57079633*offset));
			}
			buttons[4]->setPos(128, 128);
		}
	}
	
	//! Return the currently-selected RC arrow button, or -1 if none is selected
	int ArrowButtonsEventBlock::getSelectedRCArrowButton() const
	{
		for (size_t i=5; i<10; ++i)
			if (buttons[i]->getValue())
				return i-5;
		return -1;
	}
	
	//! Return the currently-selected RC keypad, or -1 if none is selected
	int ArrowButtonsEventBlock::getSelectedRCKeypadButton() const
	{
		for (size_t i=10; i<buttons.size(); ++i)
				if (buttons[i]->getValue())
					return i-10;
		return -1;
	}
	
	void ArrowButtonsEventBlock::ensureSingleRCArrowButtonSelected(int current)
	{
		for (size_t i=5; i<10; ++i)
			if ((current + 5 != i) && buttons[i] != sender())
				buttons[i]->setValue(0);
			else if (buttons[i] == sender())
				buttons[i]->setValue(1);
	}
	
	void ArrowButtonsEventBlock::ensureSingleRCKeypadButtonSelected(int current)
	{
		for (size_t i=10; i<buttons.size(); ++i)
			if ((current + 10 != i) && buttons[i] != sender())
				buttons[i]->setValue(0);
			else if (buttons[i] == sender())
				buttons[i]->setValue(1);
	}
	
	
	// Prox Event
	ProxEventBlock::ProxEventBlock(bool advanced, QGraphicsItem *parent) : 
		BlockWithButtonsAndRange("event", "prox", true, PIXEL_TO_VAL_SQUARE, 700, 4000, 1000, 2000, Qt::black, Qt::white, advanced, parent)
	{
		buttonsCountSimple = 3;
		
		// indication LEDs for front sensors
		indicationLEDs.push_back(createIndicationLED(15,78));
		indicationLEDs.push_back(createIndicationLED(54,43));
		QGraphicsItemGroup* frontIndicationLEDs(new QGraphicsItemGroup(this));
		frontIndicationLEDs->addToGroup(createIndicationLED(104,26));
		frontIndicationLEDs->addToGroup(createIndicationLED(152,26));
		indicationLEDs.push_back(frontIndicationLEDs);
		indicationLEDs.push_back(createIndicationLED(202,43));
		indicationLEDs.push_back(createIndicationLED(241,78));
		// indication LEDs for back sensors
		indicationLEDs.push_back(createIndicationLED(40,234));
		indicationLEDs.push_back(createIndicationLED(216,234));
		
		// front sensors
		for(int i=0; i<5; ++i) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-16,-16,32,32), GeometryShapeButton::RECTANGULAR_BUTTON, this, Style::unusedButtonFillColor,  Style::unusedButtonStrokeColor);
			
			const qreal offset = (qreal)2-i;
			button->setRotation(-20*offset);
			button->setPos(128 - 150*qSin(0.34906585*offset) , 
						   175 - 150*qCos(0.34906585*offset) );
			button->addState(Qt::white, Qt::red);
			button->addState(Qt::black, Qt::black);
			button->addState(Qt::darkGray, QColor(128,0,0));
			if (!advanced)
				button->setStateCountLimit(buttonsCountSimple);

			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
			connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
			connect(button, SIGNAL(stateChanged()), SLOT(updateIndicationLEDsOpacity()));
			USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),i,this));
		}
		// back sensors
		for(int i=0; i<2; ++i) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-16,-16,32,32), GeometryShapeButton::RECTANGULAR_BUTTON, this, Style::unusedButtonFillColor,  Style::unusedButtonStrokeColor);

			button->setPos(QPointF(64 + i*128, 234));
			button->addState(Qt::white, Qt::red);
			button->addState(Qt::black, Qt::black);
			button->addState(Qt::darkGray, QColor(128,0,0));
			if (!advanced)
				button->setStateCountLimit(buttonsCountSimple);
			
			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
			connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
			connect(button, SIGNAL(stateChanged()), SLOT(updateIndicationLEDsOpacity()));
			USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),i+5,this));
		}
		
		updateIndicationLEDsOpacity();
	}
	
	
	// Prox Ground Event
	ProxGroundEventBlock::ProxGroundEventBlock(bool advanced, QGraphicsItem *parent) : 
		BlockWithButtonsAndRange("event", "proxground", false, PIXEL_TO_VAL_LINEAR, 0, 1023, 400, 450, Qt::black, Qt::white, advanced, parent)
	{
		buttonsCountSimple = 3;
		
		// indication LEDs
		indicationLEDs.push_back(createIndicationLED(72,40));
		indicationLEDs.push_back(createIndicationLED(184,40));
		
		// sensors
		for(int i=0; i<2; ++i) 
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-16,-16,32,32), GeometryShapeButton::RECTANGULAR_BUTTON, this, Style::unusedButtonFillColor,  Style::unusedButtonStrokeColor);

			button->setPos(QPointF(98 + i*60, 40));
			button->addState(Qt::white, Qt::red);
			button->addState(Qt::black, Qt::black);
			button->addState(Qt::darkGray, QColor(128,0,0));
			if (!advanced)
				button->setStateCountLimit(buttonsCountSimple);
			
			buttons.push_back(button);
			
			connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
			connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
			connect(button, SIGNAL(stateChanged()), SLOT(updateIndicationLEDsOpacity()));
			USAGE_LOG(logSignal(button,SIGNAL(stateChanged()),i,this));
		}
		
		updateIndicationLEDsOpacity();
	}
	
	
	// Acc Event
	const QRectF AccEventBlock::buttonPoses[3] = {
		QRectF(32+(40+36)*0, 200, 40, 40),
		QRectF(32+(40+36)*1, 200, 40, 40),
		QRectF(32+(40+36)*2, 200, 40, 40)
	};
	const int AccEventBlock::resolution = 6;
	
	AccEventBlock::AccEventBlock(bool advanced, QGraphicsItem *parent) :
		Block("event", "acc", parent),
		mode(MODE_TAP),
		orientation(0),
		dragging(false),
		tapSimpleSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_tap_simple.svgz", this)),
		tapAdvancedSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_tap_advanced.svgz", this)),
		quadrantSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_quadrant.svgz", this)),
		pitchSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_pitch.svgz", this)),
		rollSvg(new QGraphicsSvgItem (":/images/vpl_block_acc_roll.svgz", this))
	{
		tapSimpleSvg->setVisible(!advanced);
		tapAdvancedSvg->setVisible(advanced);
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
		
		// if simple mode, simply return
		if (tapSimpleSvg->isVisible())
			return;
		
		// draw buttons
		for (unsigned i=0; i<3; ++i)
		{
			if (mode == i)
			{
				painter->setPen(QPen(Qt::black, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				painter->setBrush(Qt::red);
			}
			else
			{
				painter->setPen(QPen(Style::unusedButtonStrokeColor, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				painter->setBrush(Style::unusedButtonFillColor);
			}
			painter->drawEllipse(buttonPoses[i]);
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
			setOrientation(value);
	}
	
	QVector<quint16> AccEventBlock::getValuesCompressed() const
	{
		return QVector<quint16>(1, (orientation<<2) | mode);
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
					USAGE_LOG(logEventBlockMode(this->name, this->type,i));
					emit undoCheckpoint();
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
				clearChangedFlag();
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
		{
			dragging = false;
			emitUndoCheckpointAndClearIfChanged();
		}
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
		if (tapSimpleSvg->isVisible())
			return;
		if (mode != this->mode)
		{
			this->mode = mode;
			
			if (mode == MODE_TAP)
			{
				pitchSvg->setRotation(0);
				rollSvg->setRotation(0);
				orientation = 0;
			}
			tapAdvancedSvg->setVisible(mode == MODE_TAP);
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
			setChangedFlag();
		}
	}
	
	bool AccEventBlock::isAnyAdvancedFeature() const
	{
		return (!tapSimpleSvg->isVisible() && !tapAdvancedSvg->isVisible());
	}
	
	void AccEventBlock::setAdvanced(bool advanced)
	{
		if (!advanced)
		{
			setMode(MODE_TAP);
			tapAdvancedSvg->setVisible(false);
			tapSimpleSvg->setVisible(true);
		}
		else
		{
			tapAdvancedSvg->setVisible(true);
			tapSimpleSvg->setVisible(false);
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
