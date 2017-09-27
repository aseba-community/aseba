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
#include "../../Parameters.h"
#include "../../EnkiGlue.h"
#include "../../../../common/productids.h"
#include "../../../../common/utils/utils.h"

namespace Enki
{
	using namespace std;
	using namespace Aseba;
	
	AsebaThymio2::AsebaThymio2(std::string robotName, int16_t nodeId):
		SingleVMNodeGlue(std::move(robotName), nodeId),
		sdCardFileNumber(-1),
		timer0(bind(&AsebaThymio2::timer0Timeout, this), 0),
		timer1(bind(&AsebaThymio2::timer1Timeout, this), 0),
		timer100Hz(bind(&AsebaThymio2::timer100HzTimeout, this), 0.01),
		counter100Hz(0),
		lastStepCollided(false),
		thisStepCollided(false)
	{
		oldTimerPeriod[0] = 0;
		oldTimerPeriod[1] = 0;
		
		bytecode.resize(766+768);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(32);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
		
		vm.variables = reinterpret_cast<int16_t *>(&variables);
		vm.variablesSize = sizeof(variables) / sizeof(int16_t);
		
		AsebaVMInit(&vm);
		
		variables.id = vm.nodeId;
		variables.fwversion[0] = 10; // this simulated Thymio complies with firmware 10 public API
		variables.fwversion[1] = 0;
		variables.productId = ASEBA_PID_THYMIO2;
		
		variables.temperature = 220;
	}
	
	void AsebaThymio2::collisionEvent(PhysicalObject *o)
	{
		thisStepCollided = true;
	}

	inline double distance(double x1, double y1, double z1, double x2, double y2, double z2)
	{
		return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
	}

	void AsebaThymio2::mousePressEvent(unsigned button, double pointX, double pointY, double pointZ)
	{
		if (button == MOUSE_BUTTON_LEFT)
		{
			if (distance(pointX, pointY, pointZ, 4.8, 0, 5.3) < 0.55)
			{
				variables.buttonCenter = 1;
				execLocalEvent(EVENT_B_CENTER);
			}
			else if (distance(pointX, pointY, pointZ, 6.3, 0, 5.3) < 0.65)
			{
				variables.buttonForward = 1;
				execLocalEvent(EVENT_B_FORWARD);
			}
			else if (distance(pointX, pointY, pointZ, 3.3, 0, 5.3) < 0.65)
			{
				variables.buttonBackward = 1;
				execLocalEvent(EVENT_B_BACKWARD);
			}
			else if (distance(pointX, pointY, pointZ, 4.8, -1.5, 5.3) < 0.65)
			{
				variables.buttonRight = 1;
				execLocalEvent(EVENT_B_RIGHT);
			}
			else if (distance(pointX, pointY, pointZ, 4.8, 1.5, 5.3) < 0.65)
			{
				variables.buttonLeft = 1;
				execLocalEvent(EVENT_B_LEFT);
			}
			else
			{
				// not a putton, tap event
				execLocalEvent(EVENT_TAP);
			}
		}
	}
	
	void AsebaThymio2::mouseReleaseEvent(unsigned button)
	{
		if (button == MOUSE_BUTTON_LEFT)
		{
			if (variables.buttonCenter == 1)
			{
				variables.buttonCenter = 0;
				execLocalEvent(EVENT_B_CENTER);
			}
			if (variables.buttonForward == 1)
			{
				variables.buttonForward = 0;
				execLocalEvent(EVENT_B_FORWARD);
			}
			if (variables.buttonBackward == 1)
			{
				variables.buttonBackward = 0;
				execLocalEvent(EVENT_B_BACKWARD);
			}
			if (variables.buttonRight == 1)
			{
				variables.buttonRight = 0;
				execLocalEvent(EVENT_B_RIGHT);
			}
			if (variables.buttonLeft == 1)
			{
				variables.buttonLeft = 0;
				execLocalEvent(EVENT_B_LEFT);
			}
		}
	}
	
	void AsebaThymio2::controlStep(double dt)
	{
		// get physical variables
		variables.proxHorizontal[0] = static_cast<int16_t>(infraredSensor0.getValue());
		variables.proxHorizontal[1] = static_cast<int16_t>(infraredSensor1.getValue());
		variables.proxHorizontal[2] = static_cast<int16_t>(infraredSensor2.getValue());
		variables.proxHorizontal[3] = static_cast<int16_t>(infraredSensor3.getValue());
		variables.proxHorizontal[4] = static_cast<int16_t>(infraredSensor4.getValue());
		variables.proxHorizontal[5] = static_cast<int16_t>(infraredSensor5.getValue());
		variables.proxHorizontal[6] = static_cast<int16_t>(infraredSensor6.getValue());
		variables.proxGroundReflected[0] = static_cast<int16_t>(groundSensor0.getValue());
		variables.proxGroundReflected[1] = static_cast<int16_t>(groundSensor1.getValue());
		variables.proxGroundDelta[0] = static_cast<int16_t>(groundSensor0.getValue());
		variables.proxGroundDelta[1] = static_cast<int16_t>(groundSensor1.getValue());
		variables.motorLeftSpeed = leftSpeed * 500. / 16.6;
		variables.motorRightSpeed = rightSpeed * 500. / 16.6;
		
		// run timers
		timer0.step(dt);
		timer1.step(dt);
		timer100Hz.step(dt);
		
		// process external inputs (incoming event from network or environment, etc.)
		externalInputStep(dt);
		
		// set physical variables
		leftSpeed = double(variables.motorLeftTarget) * 16.6 / 500.;
		rightSpeed = double(variables.motorRightTarget) * 16.6 / 500.;
		
		// reset a timer if its period changed
		if (variables.timerPeriod[0] != oldTimerPeriod[0])
		{
			oldTimerPeriod[0] = variables.timerPeriod[0];
			timer0.setPeriod(variables.timerPeriod[0] / 1000.);
		}
		if (variables.timerPeriod[1] != oldTimerPeriod[1])
		{
			oldTimerPeriod[1] = variables.timerPeriod[1];
			timer1.setPeriod(variables.timerPeriod[1] / 1000.);
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
		{ nullptr, nullptr }
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
	
	void AsebaThymio2::callNativeFunction(uint16_t id)
	{
		nativeFunctions[id](&vm);
	}
	
	//! Open the virtual SD card file number, if -1, close current one
	bool AsebaThymio2::openSDCardFile(int number)
	{
		// close current file, ignore errors
		if (sdCardFile.is_open())
		{
			sdCardFile.close();
			sdCardFileNumber = -1;
		}
		
		// if we have to open another file
		if (number >= 0)
		{
			// path for the file
			if (!Enki::simulatorEnvironment)
				return false;
			const string fileName(Enki::simulatorEnvironment->getSDFilePath(robotName, unsigned(number)));
			// try to open file
			sdCardFile.open(fileName.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			if (sdCardFile.fail())
			{
				// failed... maybe the file does not exist, try with trunc
				sdCardFile.open(fileName.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
			}
			if (sdCardFile.fail())
			{
				// still failed, then it is an error
				return false;
			}
			else
			{
				sdCardFileNumber = number;
			}
		}
		return true;
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
	
	//! Execute a local event, killing the execution of the current one if not in step-by-step mode
	void AsebaThymio2::execLocalEvent(uint16_t number)
	{
		// in step-by-step, only setup an event if none is being executed currently
		if (AsebaMaskIsSet(vm.flags, ASEBA_VM_STEP_BY_STEP_MASK) && AsebaMaskIsSet(vm.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
			return;
		
		variables.source = vm.nodeId;
		AsebaVMSetupEvent(&vm, ASEBA_EVENT_LOCAL_EVENTS_START-number);
		AsebaVMRun(&vm, 1000);
	}
	
} // Enki
