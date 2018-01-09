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

#ifndef __PLAYGROUND_THYMIO2_H
#define __PLAYGROUND_THYMIO2_H

#include "../../AsebaGlue.h"
#include "../../../../common/utils/utils.h"
#include <enki/PhysicalEngine.h>
#include <enki/robots/thymio2/Thymio2.h>
#include <fstream>
#include <utility>

namespace Enki
{
	class AsebaThymio2 : public Thymio2, public Aseba::SingleVMNodeGlue
	{
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
		struct Variables
		{
			int16_t id;
			int16_t source;
			int16_t args[32];
			int16_t productId;
			int16_t fwversion[2];
			
			int16_t buttonBackward;
			int16_t buttonLeft;
			int16_t buttonCenter;
			int16_t buttonForward;
			int16_t buttonRight;
			
			int16_t proxHorizontal[7];
			
			int16_t proxCommRx;
			int16_t proxCommTx;
			
			int16_t proxGroundAmbiant[2];
			int16_t proxGroundReflected[2];
			int16_t proxGroundDelta[2];
			
			int16_t motorLeftTarget;
			int16_t motorRightTarget;
			int16_t motorLeftSpeed;
			int16_t motorRightSpeed;
			int16_t motorLeftPwm;
			int16_t motorRightPwm;
			
			int16_t acc[3];
			
			int16_t temperature;
			
			int16_t rc5adress;
			int16_t rc5command;
			
			int16_t micIntensity;
			int16_t micThreshold;
			
			int16_t timerPeriod[2];
			
			int16_t sdPresent;
			
			int16_t freeSpace[512];
		} variables;
		
	public:
		std::fstream sdCardFile;
		int sdCardFileNumber;
		
		// Logging of Thymio native function calls
		//! A log entry consists of the number of the Thymio native function and the values of the arguments
		using NativeCallLogEntry = std::pair<unsigned, std::vector<int16_t>>;
		//! The log is a vector of entries
		using NativeCallLog = std::vector<NativeCallLogEntry>;
		//! The log of native calls, filled if logThymioNativeCalls is true.
		//! The code which set it is responsible to clear the log from time to time
		NativeCallLog thymioNativeCallLog;
		//! Whether thymioNativeCallLog should be filled each time a Thymio native function is called
		bool logThymioNativeCalls { false } ;
		
	protected:
		Aseba::SoftTimer timer0;
		Aseba::SoftTimer timer1;
		int16_t oldTimerPeriod[2];
		Aseba::SoftTimer timer100Hz;
		unsigned counter100Hz;
		
		bool lastStepCollided;
		bool thisStepCollided;
		
	public:
		AsebaThymio2(std::string robotName, int16_t nodeId);
		
		// from PhysicalObject
		
		virtual void collisionEvent(PhysicalObject *o);

		// from Robot

		virtual void mousePressEvent(unsigned button, double pointX, double pointY, double pointZ);
		virtual void mouseReleaseEvent(unsigned button);
		
		// from Thymio2
		
		virtual void controlStep(double dt);
		
		// from AbstractNodeGlue
		
		virtual const AsebaVMDescription* getDescription() const;
		virtual const AsebaLocalEventDescription * getLocalEventsDescriptions() const;
		virtual const AsebaNativeFunctionDescription * const * getNativeFunctionsDescriptions() const;
		virtual void callNativeFunction(uint16_t id);
		
		// for Thymio2-natives.cpp
		
		bool openSDCardFile(int number);
		
		// for debug purposes
		void execLocalEvent(uint16_t number);

	protected:
		
		void timer0Timeout();
		void timer1Timeout();
		void timer100HzTimeout();
		int16_t getSaturatedProxHorizontal(unsigned i) const;
	};
	
} // Enki

#endif // __PLAYGROUND_THYMIO2_H
