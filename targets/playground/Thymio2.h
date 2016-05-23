/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2013:
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

#ifndef __CHALLENGE_THYMIO2_H
#define __CHALLENGE_THYMIO2_H

#include "AsebaGlue.h"
#include <enki/PhysicalEngine.h>
#include <enki/robots/thymio2/Thymio2.h>
#include <QTimer>

namespace Enki
{
	class AsebaThymio2 : public QObject, public Thymio2, public Aseba::AbstractNodeGlue, public Aseba::SimpleDashelConnection
	{
		Q_OBJECT
		
	public:
		enum Thymio2Events
		{
			EVENT_B_BACKWARD = 0,
			EVENT_B_LEFT,
			EVENT_B_CENTER,
			EVENT_B_FORWARD,
			EVENT_B_RIGHT,
			EVENT_BUTTONS,
			EVENT_PROX,
			EVENT_PROX_COMM,
			EVENT_TAP,
			EVENT_ACC,
			EVENT_MIC,
			EVENT_SOUND_FINISHED,
			EVENT_TEMPERATURE,
			EVENT_RC5,
			EVENT_MOTOR,
			EVENT_TIMER0,
			EVENT_TIMER1,
			EVENT_COUNT // number of events
		};
		
	public:
		AsebaVMState vm;
		std::valarray<unsigned short> bytecode;
		std::valarray<signed short> stack;
		struct Variables
		{
			sint16 id;
			sint16 source;
			sint16 args[32];
			sint16 productId;
			sint16 fwversion[2];
			
			sint16 buttonBackward;
			sint16 buttonLeft;
			sint16 buttonCenter;
			sint16 buttonForward;
			sint16 buttonRight;
			
			sint16 proxHorizontal[7];
			
			sint16 proxCommRx;
			sint16 proxCommTx;
			
			sint16 proxGroundAmbiant[2];
			sint16 proxGroundReflected[2];
			sint16 proxGroundDelta[2];
			
			sint16 motorLeftTarget;
			sint16 motorRightTarget;
			sint16 motorLeftSpeed;
			sint16 motorRightSpeed;
			sint16 motorLeftPwm;
			sint16 motorRightPwm;
			
			sint16 acc[3];
			
			sint16 temperature;
			
			sint16 rc5adress;
			sint16 rc5command;
			
			sint16 micIntensity;
			sint16 micThreshold;
			
			sint16 timerPeriod[2];
			
			sint16 freeSpace[512];
		} variables;
		
	protected:
		QTimer* timer0;
		QTimer* timer1;
		sint16 oldTimerPeriod[2];
		QTimer* timer100Hz;
		unsigned counter100Hz;
		
	public:
		AsebaThymio2(unsigned port);
		virtual ~AsebaThymio2();
		
		// from Thymio2
		
		virtual void controlStep(double dt);
		
		// from AbstractNodeGlue
		
		virtual const AsebaVMDescription* getDescription() const;
		virtual const AsebaLocalEventDescription * getLocalEventsDescriptions() const;
		virtual const AsebaNativeFunctionDescription * const * getNativeFunctionsDescriptions() const;
		virtual void callNativeFunction(uint16 id);
		
	protected slots:
		void timer0Timeout();
		void timer1Timeout();
		void timer100HzTimeout();
		
	protected:
		friend class Thymio2Interface;
		void execLocalEvent(uint16 number);
	};
} // Enki

#endif // __CHALLENGE_EPUCK_H
