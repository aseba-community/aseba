/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AsebaMarxbot.h"
#include "../../common/consts.h"
#include "../../vm/natives.h"
#include <../../transport/buffer/vm-buffer.h>
#include <set>
#include <map>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <QString>

/*!	\file Marxbot.cpp
	\brief Implementation of the aseba-enabled marXbot robot
*/

// Implementation of aseba glue code

// map for aseba glue code
typedef std::map<AsebaVMState*, Enki::AsebaMarxbot*>  VmSocketMap;
static VmSocketMap asebaSocketMaps;

static AsebaNativeFunctionPointer nativeFunctions[] =
{
	AsebaNative_vecfill,
	AsebaNative_veccopy,
	AsebaNative_vecdot,
	AsebaNative_vecstat
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
{
	&AsebaNativeDescription_vecfill,
	&AsebaNativeDescription_veccopy,
	&AsebaNativeDescription_vecdot,
	&AsebaNativeDescription_vecstat,
	0
};

extern "C" void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	Dashel::Stream* stream = asebaSocketMaps[vm]->stream;
	assert(stream);
	
	uint16 len = length - 2;
	stream->write(&len, 2);
	stream->write(&vm->nodeId, 2);
	stream->write(data, length);
	stream->flush();
}

extern "C" uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	if (asebaSocketMaps[vm]->lastMessageData.size())
	{
		*source = asebaSocketMaps[vm]->lastMessageSource;
		memcpy(data, &asebaSocketMaps[vm]->lastMessageData[0], asebaSocketMaps[vm]->lastMessageData.size());
	}
	return asebaSocketMaps[vm]->lastMessageData.size();
}

extern "C" const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	switch (vm->nodeId)
	{
		case 1: // TODO
		case 2: // TODO
		case 3: // TODO
		case 4: // TODO
		break;
	}
	std::cerr << "simulation broken until we define the variables availables in the physical robot" << std::endl;
	assert(false);
	return 0; // FIXME
}

extern "C" const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	return nativeFunctionsDescriptions;
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	nativeFunctions[id](vm);
}

extern "C" void AsebaWriteBytecode()
{
}

extern "C" void AsebaResetIntoBootloader()
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
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode"; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow"; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow"; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds"; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds"; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run"; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds"; break;
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
	
	AsebaMarxbot::AsebaMarxbot(const std::string &target)
	{
		// setup modules specific data
		leftMotor.vm.nodeId = 1;
		leftMotor.vm.variables = reinterpret_cast<sint16 *>(&leftMotorVariables);
		leftMotor.vm.variablesSize = sizeof(leftMotorVariables) / sizeof(sint16);
		modules.push_back(&leftMotor);
		
		rightMotor.vm.nodeId = 2;
		rightMotor.vm.variables = reinterpret_cast<sint16 *>(&rightMotorVariables);
		rightMotor.vm.variablesSize = sizeof(rightMotorVariables) / sizeof(sint16);
		modules.push_back(&rightMotor);
		
		proximitySensors.vm.nodeId = 3;
		proximitySensors.vm.variables = reinterpret_cast<sint16 *>(&proximitySensorVariables);
		proximitySensors.vm.variablesSize = sizeof(proximitySensorVariables) / sizeof(sint16);
		modules.push_back(&proximitySensors);
		
		distanceSensors.vm.nodeId = 4;
		distanceSensors.vm.variables = reinterpret_cast<sint16 *>(&distanceSensorVariables);
		distanceSensors.vm.variablesSize = sizeof(distanceSensorVariables) / sizeof(sint16);
		modules.push_back(&distanceSensors);
		
		// fill map
		asebaSocketMaps[&leftMotor.vm] = this;
		asebaSocketMaps[&rightMotor.vm] = this;
		asebaSocketMaps[&proximitySensors.vm] = this;
		asebaSocketMaps[&distanceSensors.vm] = this;
		
		// connect to target
		stream = Hub::connect(target);
		
		int port = ASEBA_DEFAULT_PORT + marxbotNumber;
		Dashel::Hub::connect(QString("tcpin:port=%1").arg(port).toStdString());
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
	
	void AsebaMarxbot::step(double dt)
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
		DifferentialWheeled::step(dt);
		
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
		
		for (size_t i = 0; i < modules.size(); i++)
		{
			AsebaVMState* vm = &(modules[i]->vm);
			
			/* no queue for now
			// process all incoming events as long as 
			while (!AsebaVMIsExecutingThread(vm) && !events.empty())
				AsebaProcessIncomingEvents(&(modules[i]->vm));
			*/
			
			// run VM with a periodic event if nothing else is currently running
			if (!AsebaVMIsExecutingThread(vm))
				AsebaVMSetupEvent(vm, ASEBA_EVENT_PERIODIC);
			
			AsebaVMRun(vm, 65535);
		}
	}
	
	void AsebaMarxbot::incomingConnection(Stream *stream)
	{
		this->stream = stream;
	}
	
	void AsebaMarxbot::incomingData(Stream *stream)
	{
		uint16 len;
		stream->read(&len, 2);
		stream->read(&lastMessageSource, 2);
		lastMessageData.resize(len+2);
		stream->read(&lastMessageData[0], lastMessageData.size());
		
		for (size_t i = 0; i < modules.size(); i++)
			AsebaProcessIncomingEvents(&(modules[i]->vm));
	}
	
	void AsebaMarxbot::connectionClosed(Stream *stream, bool abnormal)
	{
		// do nothing in addition to what is done by Client
	}
}

