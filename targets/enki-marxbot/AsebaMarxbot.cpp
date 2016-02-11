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

#include "AsebaMarxbot.h"
#include "../../common/consts.h"
#include "../../vm/natives.h"
#include "../../transport/buffer/vm-buffer.h"
#include <set>
#include <map>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <QString>
#include <QApplication>
#include <QMessageBox>
#include <QDebug>

/*!	\file Marxbot.cpp
	\brief Implementation of the aseba-enabled marXbot robot
*/

// Implementation of aseba glue code

// map for aseba glue code
typedef std::map<AsebaVMState*, Enki::AsebaMarxbot*>  VmSocketMap;
static VmSocketMap asebaSocketMaps;

static AsebaNativeFunctionPointer nativeFunctions[] =
{
	ASEBA_NATIVES_STD_FUNCTIONS,
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
{
	ASEBA_NATIVES_STD_DESCRIPTIONS,
	0
};

extern "C" void AsebaPutVmToSleep(AsebaVMState *vm)
{
}

extern "C" void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	Enki::AsebaMarxbot& marxBot = *asebaSocketMaps[vm];
	Dashel::Stream* stream = marxBot.stream;
	if (!stream)
		return;
	
	// send to stream
	try
	{
		uint16 temp;
		temp = bswap16(length - 2);
		stream->write(&temp, 2);
		temp = bswap16(vm->nodeId);
		stream->write(&temp, 2);
		stream->write(data, length);
		stream->flush();

		// push to other nodes
		for (size_t i = 0; i < marxBot.modules.size(); ++i)
		{
			Enki::AsebaMarxbot::Module& module = *(marxBot.modules[i]);
			if (&(module.vm) != vm) 
			{
				module.events.push_back(Enki::AsebaMarxbot::Event(vm->nodeId, data, length));
				AsebaProcessIncomingEvents(&(module.vm));
			}
		}
	}
	catch (Dashel::DashelException e)
	{
		std::cerr << "Cannot write to socket: " << stream->getFailReason() << std::endl;
	}
}

extern "C" uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	// TODO: improve this, it is rather ugly
	Enki::AsebaMarxbot& marxBot = *asebaSocketMaps[vm];
	for (size_t i = 0; i < marxBot.modules.size(); ++i)
	{
		Enki::AsebaMarxbot::Module& module = *(marxBot.modules[i]);
		if (&(module.vm) == vm)
		{
			if (module.events.empty())
				return 0;
				
			// I do not put const here to work around a bug in MSVC 2005 implementation of std::valarray
			Enki::AsebaMarxbot::Event& event = module.events.front();
			*source = event.source;
			size_t length = event.data.size();
			length = std::min<size_t>(length, maxLength);
			memcpy(data, &(event.data[0]), length);
			module.events.pop_front();
			return length;
		}
	}
	return 0;
}

extern "C" AsebaVMDescription vmLeftMotorDescription;
extern "C" AsebaVMDescription vmRightMotorDescription;
extern "C" AsebaVMDescription vmProximitySensorsDescription;
extern "C" AsebaVMDescription vmDistanceSensorsDescription;

extern "C" const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	switch (vm->nodeId)
	{
		case 1: return &vmLeftMotorDescription;
		case 2: return &vmRightMotorDescription;
		case 3: return &vmProximitySensorsDescription;
		case 4: return &vmDistanceSensorsDescription;
		default: break;
	}
	assert(false);
	return 0;
}

extern "C" const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	return nativeFunctionsDescriptions;
}

static const AsebaLocalEventDescription localEvents[] = { { "timer", "periodic timer at 50 Hz" }, { NULL, NULL } };

extern "C" const AsebaLocalEventDescription * AsebaGetLocalEventsDescriptions(AsebaVMState *vm)
{
	return localEvents;
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	nativeFunctions[id](vm);
}

extern "C" void AsebaWriteBytecode(AsebaVMState *vm)
{
}

extern "C" void AsebaResetIntoBootloader(AsebaVMState *vm)
{
}

extern "C" void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	std::cerr << "\nFatal error: ";
	switch (vm->nodeId)
	{
		case 1: std::cerr << "left motor module"; break;
		case 2:	std::cerr << "right motor module"; break;
		case 3: std::cerr << "proximity sensors module"; break;
		case 4: std::cerr << "distance sensors module"; break;
		default: std::cerr << "unknown module"; break;
	}
	std::cerr << " has produced exception: ";
	switch (reason)
	{
		case ASEBA_ASSERT_UNKNOWN: std::cerr << "undefined"; break;
		case ASEBA_ASSERT_UNKNOWN_UNARY_OPERATOR: std::cerr << "unknown unary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode"; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow"; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow"; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds"; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds"; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run"; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds"; break;
		case ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG: std::cerr << "tried to emit a buffer too long"; break;
		default: std::cerr << "unknown exception"; break;
	}
	std::cerr << ".\npc = " << vm->pc << ", sp = " << vm->sp;
	std::cerr << "\nResetting VM" << std::endl;
	assert(false);
	AsebaVMInit(vm);
}

namespace Enki
{
	using namespace Dashel;
	
	int AsebaMarxbot::marxbotNumber = 0;
	
	AsebaMarxbot::Module::Module()
	{
		bytecode.resize(512);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(64);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
	}
	
	AsebaMarxbot::AsebaMarxbot() :
		stream(0)
	{
		// setup modules specific data
		leftMotor.vm.nodeId = 1;
		leftMotorVariables.id = 1;
		leftMotor.vm.variables = reinterpret_cast<sint16 *>(&leftMotorVariables);
		leftMotor.vm.variablesSize = sizeof(leftMotorVariables) / sizeof(sint16);
		modules.push_back(&leftMotor);
		
		rightMotor.vm.nodeId = 2;
		rightMotorVariables.id = 2;
		rightMotor.vm.variables = reinterpret_cast<sint16 *>(&rightMotorVariables);
		rightMotor.vm.variablesSize = sizeof(rightMotorVariables) / sizeof(sint16);
		modules.push_back(&rightMotor);
		
		proximitySensors.vm.nodeId = 3;
		proximitySensorVariables.id = 3;
		proximitySensors.vm.variables = reinterpret_cast<sint16 *>(&proximitySensorVariables);
		proximitySensors.vm.variablesSize = sizeof(proximitySensorVariables) / sizeof(sint16);
		modules.push_back(&proximitySensors);
		
		distanceSensors.vm.nodeId = 4;
		distanceSensorVariables.id = 4;
		distanceSensors.vm.variables = reinterpret_cast<sint16 *>(&distanceSensorVariables);
		distanceSensors.vm.variablesSize = sizeof(distanceSensorVariables) / sizeof(sint16);
		modules.push_back(&distanceSensors);
		
		// fill map
		asebaSocketMaps[&leftMotor.vm] = this;
		asebaSocketMaps[&rightMotor.vm] = this;
		asebaSocketMaps[&proximitySensors.vm] = this;
		asebaSocketMaps[&distanceSensors.vm] = this;
		
		// connect to target
		int port = ASEBA_DEFAULT_PORT + marxbotNumber;
		try
		{
			Dashel::Hub::connect(QString("tcpin:port=%1").arg(port).toStdString());
		}
		catch (Dashel::DashelException e)
		{
			QMessageBox::critical(0, QApplication::tr("Aseba Marxbot"), QApplication::tr("Cannot create listening port %0: %1").arg(port).arg(e.what()));
			abort();
		}
		marxbotNumber++;
		
		// init VM
		AsebaVMInit(&leftMotor.vm);
		AsebaVMInit(&rightMotor.vm);
		AsebaVMInit(&proximitySensors.vm);
		AsebaVMInit(&distanceSensors.vm);
	}
	
	AsebaMarxbot::~AsebaMarxbot()
	{
		// clean map
		asebaSocketMaps.erase(&leftMotor.vm);
		asebaSocketMaps.erase(&rightMotor.vm);
		asebaSocketMaps.erase(&proximitySensors.vm);
		asebaSocketMaps.erase(&distanceSensors.vm);
	}
	
	void AsebaMarxbot::controlStep(double dt)
	{
		//stepCounter++;
		//std::cerr << stepCounter << std::endl;
		/*
			Values mapping
			
			motor:
				estimated 3000 == 30 cm/s
			
			encoders:
				16 tick per motor turn
				134 reduction
				6 cm wheel diameter
		*/
		
		// set physical variables
		leftSpeed = static_cast<double>(leftMotorVariables.speed) / 100;
		rightSpeed = static_cast<double>(rightMotorVariables.speed) / 100;
		
		// do motion
		DifferentialWheeled::controlStep(dt);
		
		// get physical variables
		int odoLeft = static_cast<int>((leftOdometry * 16  * 134) / (2 * M_PI));
		leftMotorVariables.odo[0] = odoLeft & 0xffff;
		leftMotorVariables.odo[1] = odoLeft >> 16;
		
		int odoRight = static_cast<int>((rightOdometry * 16  * 134) / (2 * M_PI));
		rightMotorVariables.odo[0] = odoRight & 0xffff;
		rightMotorVariables.odo[1] = odoRight >> 16;
		
		for (size_t i = 0; i < 24; i++)
			proximitySensorVariables.bumpers[i] = static_cast<sint16>(getVirtualBumper(i));
		std::fill(proximitySensorVariables.ground, proximitySensorVariables.ground + 12, 0);
		
		for (size_t i = 0; i < 180; i++)
		{
			if (rotatingDistanceSensor.zbuffer[i] > 32767)
				distanceSensorVariables.distances[i] = 32767;
			else
				distanceSensorVariables.distances[i] = static_cast<sint16>(rotatingDistanceSensor.zbuffer[i]);
		}
		
		// do a network step
		Hub::step();
		
		// disconnect old streams
		for (size_t i = 0; i < toDisconnect.size(); ++i)
		{
			qDebug() << this << " : Disconnected old client.";
			closeStream(toDisconnect[i]);
		}
		toDisconnect.clear();
		
		// run each module
		for (size_t i = 0; i < modules.size(); i++)
		{
			AsebaVMState* vm = &(modules[i]->vm);
			
			/* no queue for now
			// process all incoming events as long as 
			while (!AsebaVMIsExecutingThread(vm) && !events.empty())
				AsebaProcessIncomingEvents(&(modules[i]->vm));
			*/
			
			// run VM
			AsebaVMRun(vm, 65535);
			
			// reschedule a periodic event if we are not in step by step
			if (AsebaMaskIsClear(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK) || AsebaMaskIsClear(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK))
				AsebaVMSetupEvent(vm, ASEBA_EVENT_LOCAL_EVENTS_START);
		}
	}
	
	void AsebaMarxbot::connectionCreated(Dashel::Stream *stream)
	{
		std::string targetName = stream->getTargetName();
		if (targetName.substr(0, targetName.find_first_of(':')) == "tcp")
		{
			// schedule current stream for disconnection
			if (this->stream)
				toDisconnect.push_back(this->stream);
			
			// set new stream as current stream
			this->stream = stream;
			qDebug() << this << " : New client connected.";
		}
	}
	
	void AsebaMarxbot::incomingData(Stream *stream)
	{
		Event event(stream);
		
		// push to other nodes
		for (size_t i = 0; i < modules.size(); ++i)
		{
			Module& module = *(modules[i]);
			module.events.push_back(event);
			AsebaProcessIncomingEvents(&(module.vm));
		}
	}
	
	void AsebaMarxbot::connectionClosed(Dashel::Stream *stream, bool abnormal)
	{
		if (stream == this->stream)
		{
			this->stream = 0;
			// clear breakpoints
			for (size_t i = 0; i < modules.size(); i++)
				(modules[i]->vm).breakpointsCount = 0;
		}
		if (abnormal)
			qDebug() << this << " : Client has disconnected unexpectedly.";
		else
			qDebug() << this << " : Client has disconnected properly.";
	}
}

