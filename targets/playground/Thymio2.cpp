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

#include "Thymio2.h"
#include "Thymio2-natives.h"
#include "Parameters.h"
#include "PlaygroundViewer.h"
#include "../../common/productids.h"
#include "../../common/utils/utils.h"

namespace Enki
{
	using namespace Aseba;
	
	AsebaThymio2::AsebaThymio2(unsigned port):
		SimpleDashelConnection(port),
		timer0(new QTimer(this)),
		timer1(new QTimer(this)),
		timer100Hz(new QTimer(this)),
		counter100Hz(0),
		lastStepCollided(false),
		thisStepCollided(false)
	{
		oldTimerPeriod[0] = 0;
		oldTimerPeriod[1] = 0;
		
		vm.nodeId = 1;
		
		bytecode.resize(766+768);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(32);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
		
		vm.variables = reinterpret_cast<sint16 *>(&variables);
		vm.variablesSize = sizeof(variables) / sizeof(sint16);
		
		AsebaVMInit(&vm);
		
		variables.id = vm.nodeId;
		variables.fwversion[0] = 10; // this simulated Thymio complies with firmware 10 public API
		variables.fwversion[1] = 0;
		variables.productId = ASEBA_PID_THYMIO2;
		
		variables.temperature = 220;
		
		vmStateToEnvironment[&vm] = qMakePair((Aseba::AbstractNodeGlue*)this, (Aseba::AbstractNodeConnection *)this);
		
		QObject::connect(timer0, SIGNAL(timeout()), SLOT(timer0Timeout()));
		QObject::connect(timer1, SIGNAL(timeout()), SLOT(timer1Timeout()));
		QObject::connect(timer100Hz, SIGNAL(timeout()), SLOT(timer100HzTimeout()));
		timer100Hz->start(10);
	}
	
	AsebaThymio2::~AsebaThymio2()
	{
		vmStateToEnvironment.remove(&vm);
	}
	
	void AsebaThymio2::collisionEvent(PhysicalObject *o)
	{
		thisStepCollided = true;
	}

	inline double distance(double x1, double y1, double z1, double x2, double y2, double z2)
	{
		return std::sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
	}

	void AsebaThymio2::clickedInteraction(bool pressed, unsigned int buttonCode, double pointX, double pointY, double pointZ)
	{
		pointX -= pos.x;
		pointY -= pos.y;
		double relativeX =  cos(angle)*pointX + sin(angle)*pointY;
		double relativeY = -sin(angle)*pointX + cos(angle)*pointY;

		if (pressed && buttonCode == LEFT_MOUSE_BUTTON)
		{
			if (distance(relativeX,relativeY,pointZ,2.5,0,5.3) < 0.55)
			{
				variables.buttonCenter = 1;
				variables.buttonBackward = 0;
				variables.buttonForward = 0;
				variables.buttonLeft = 0;
				variables.buttonRight = 0;
			}
			else if (distance(relativeX,relativeY,pointZ,4.0,0,5.3) < 0.65)
			{
				variables.buttonCenter = 0;
				variables.buttonBackward = 0;
				variables.buttonForward = 1;
				variables.buttonLeft = 0;
				variables.buttonRight = 0;
			}
			else if (distance(relativeX,relativeY,pointZ,1.0,0,5.3) < 0.65)
			{
				variables.buttonCenter = 0;
				variables.buttonBackward = 1;
				variables.buttonForward = 0;
				variables.buttonLeft = 0;
				variables.buttonRight = 0;
			}
			else if (distance(relativeX,relativeY,pointZ,2.5,-1.5,5.3) < 0.65)
			{
				variables.buttonCenter = 0;
				variables.buttonBackward = 0;
				variables.buttonForward = 0;
				variables.buttonLeft = 0;
				variables.buttonRight = 1;
			}
			else if (distance(relativeX,relativeY,pointZ,2.5,1.5,5.3) < 0.65)
			{
				variables.buttonCenter = 0;
				variables.buttonBackward = 0;
				variables.buttonForward = 0;
				variables.buttonLeft = 1;
				variables.buttonRight = 0;
			}
			else
			{
				variables.buttonCenter = 0;
				variables.buttonBackward = 0;
				variables.buttonForward = 0;
				variables.buttonLeft = 0;
				variables.buttonRight = 0;
				
				// not a putton, tap event
				execLocalEvent(EVENT_TAP);
			}
		}
		else
		{
			variables.buttonCenter = 0;
			variables.buttonBackward = 0;
			variables.buttonForward = 0;
			variables.buttonLeft = 0;
			variables.buttonRight = 0;
		}
	}
	
	void AsebaThymio2::controlStep(double dt)
	{
		// get physical variables
		variables.proxHorizontal[0] = static_cast<sint16>(infraredSensor0.getValue());
		variables.proxHorizontal[1] = static_cast<sint16>(infraredSensor1.getValue());
		variables.proxHorizontal[2] = static_cast<sint16>(infraredSensor2.getValue());
		variables.proxHorizontal[3] = static_cast<sint16>(infraredSensor3.getValue());
		variables.proxHorizontal[4] = static_cast<sint16>(infraredSensor4.getValue());
		variables.proxHorizontal[5] = static_cast<sint16>(infraredSensor5.getValue());
		variables.proxHorizontal[6] = static_cast<sint16>(infraredSensor6.getValue());
		variables.proxGroundReflected[0] = static_cast<sint16>(groundSensor0.getValue());
		variables.proxGroundReflected[1] = static_cast<sint16>(groundSensor1.getValue());
		variables.proxGroundDelta[0] = static_cast<sint16>(groundSensor0.getValue());
		variables.proxGroundDelta[1] = static_cast<sint16>(groundSensor1.getValue());
		variables.motorLeftSpeed = leftSpeed * 500. / 16.6;
		variables.motorRightSpeed = rightSpeed * 500. / 16.6;
		
		// do a network step, if there are some events from the network, they will be executed
		Hub::step();
		
		// disconnect old streams
		closeOldStreams();
		
		// set physical variables
		leftSpeed = double(variables.motorLeftTarget) * 16.6 / 500.;
		rightSpeed = double(variables.motorRightTarget) * 16.6 / 500.;
		
		// reset a timer if its period changed
		if (variables.timerPeriod[0] != oldTimerPeriod[0])
		{
			oldTimerPeriod[0] = variables.timerPeriod[0];
			if (variables.timerPeriod[0])
				timer0->start(variables.timerPeriod[0]);
			else
				timer0->stop();
		}
		if (variables.timerPeriod[1] != oldTimerPeriod[1])
		{
			oldTimerPeriod[1] = variables.timerPeriod[1];
			if (variables.timerPeriod[1])
				timer1->start(variables.timerPeriod[1]);
			else
				timer1->stop();
		}
		
		// set motion
		Thymio2::controlStep(dt);
		
		// trigger tap event
		if (thisStepCollided && !lastStepCollided)
			execLocalEvent(EVENT_TAP);
		lastStepCollided = thisStepCollided;
		thisStepCollided = false;
	}
	
	// robot description
	
	extern "C" AsebaVMDescription PlaygroundThymio2VMDescription;
	
	const AsebaVMDescription* AsebaThymio2::getDescription() const
	{
		return &PlaygroundThymio2VMDescription;
	}
	
	// local events, static so only visible in this file
	
	static const AsebaLocalEventDescription localEvents[] = {
		{ "button.backward", "Backward button status changed"},
		{ "button.left", "Left button status changed"},
		{ "button.center", "Center button status changed"},
		{ "button.forward", "Forward button status changed"},
		{ "button.right", "Right button status changed"},
		{ "buttons", "Buttons values updated"},
		{ "prox", "Proximity values updated"},
		{ "prox.comm", "Data received on the proximity communication"},
		{ "tap", "A tap is detected"},
		{ "acc", "Accelerometer values updated"},
		{ "mic", "Fired when microphone intensity is above threshold"},
		{ "sound.finished", "Fired when the playback of a user initiated sound is finished"},
		{ "temperature", "Temperature value updated"},
		{ "rc5", "RC5 message received"},
		{ "motor", "Motor timer"},
		{ "timer0", "Timer 0"},
		{ "timer1", "Timer 1"},
		{ NULL, NULL }
	};
	
	const AsebaLocalEventDescription * AsebaThymio2::getLocalEventsDescriptions() const
	{
		return localEvents;
	}
	
	
	// array of descriptions of native functions, static so only visible in this file
	
	static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
	{
		ASEBA_NATIVES_STD_DESCRIPTIONS,
		PLAYGROUND_THYMIO2_NATIVES_DESCRIPTIONS,
		0
	};

	const AsebaNativeFunctionDescription * const * AsebaThymio2::getNativeFunctionsDescriptions() const
	{
		return nativeFunctionsDescriptions;
	}
	
	// array of native functions, static so only visible in this file
	
	static AsebaNativeFunctionPointer nativeFunctions[] =
	{
		ASEBA_NATIVES_STD_FUNCTIONS,
		PLAYGROUND_THYMIO2_NATIVES_FUNCTIONS
	};
	
	void AsebaThymio2::callNativeFunction(uint16 id)
	{
		nativeFunctions[id](&vm);
	}
	
	
	void AsebaThymio2::timer0Timeout()
	{
		execLocalEvent(EVENT_TIMER0);
	}
	
	void AsebaThymio2::timer1Timeout()
	{
		execLocalEvent(EVENT_TIMER1);
	}
		
	void AsebaThymio2::timer100HzTimeout()
	{
		++counter100Hz;
		
		execLocalEvent(EVENT_MOTOR);
		if (counter100Hz % 5 == 0)
			execLocalEvent(EVENT_BUTTONS);
		if (counter100Hz % 10 == 0)
			execLocalEvent(EVENT_PROX);
		if (counter100Hz % 6 == 0)
			execLocalEvent(EVENT_ACC);
		if (counter100Hz % 100 == 0)
			execLocalEvent(EVENT_TEMPERATURE);
	}
	
	void AsebaThymio2::execLocalEvent(uint16 number)
	{
		// in step-by-step, only setup an event if none is being executed currently
		if (AsebaMaskIsSet(vm.flags, ASEBA_VM_STEP_BY_STEP_MASK) && AsebaMaskIsSet(vm.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
			return;
		
		variables.source = vm.nodeId;
		AsebaVMSetupEvent(&vm, ASEBA_EVENT_LOCAL_EVENTS_START-number);
		AsebaVMRun(&vm, 1000);
	}
	
} // Enki
