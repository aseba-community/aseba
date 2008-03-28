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
#include <set>
#include <map>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>

/*!	\file Marxbot.cpp
	\brief Implementation of the aseba-enabled marXbot robot
*/

// Implementation of aseba glue code

// map for aseba glue code
typedef std::map<AsebaVMState*, Enki::AsebaMarxbot*>  VmSocketMap;
static VmSocketMap asebaSocketMaps;

static const unsigned nativeFunctionsCount = 2;
static AsebaNativeFunctionPointer nativeFunctions[nativeFunctionsCount] =
{
	AsebaNative_vecdot,
	AsebaNative_vecstat
};
static AsebaNativeFunctionDescription* nativeFunctionsDescriptions[nativeFunctionsCount] =
{
	&AsebaNativeDescription_vecdot,
	&AsebaNativeDescription_vecstat
};

//int stepCounter = 0;

extern "C" void AsebaSendMessage(AsebaVMState *vm, uint16 id, void *data, uint16 size)
{
	Dashel::Stream* stream = asebaSocketMaps[vm]->stream;
	assert(stream);
	
	// write message
	stream->write(&size, 2);
	stream->write(&vm->nodeId, 2);
	stream->write(&id, 2);
	stream->write(data, size);
	stream->flush();
	std::cout << "event " << id << std::endl;
}

extern "C" void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length)
{
	Dashel::Stream* stream = asebaSocketMaps[vm]->stream;
	assert(stream);
	
	// write message
	uint16 size = length * 2 + 2;
	uint16 id = ASEBA_MESSAGE_VARIABLES;
	stream->write(&size, 2);
	stream->write(&vm->nodeId, 2);
	stream->write(&id, 2);
	stream->write(&start, 2);
	stream->write(vm->variables + start, length * 2);
	stream->flush();
}

void AsebaWriteString(Dashel::Stream* stream, const char *s)
{
	size_t len = strlen(s);
	uint8 lenUint8 = static_cast<uint8>(strlen(s));
	stream->write(&lenUint8, 1);
	stream->write(s, len);
}

void AsebaWriteNativeFunctionDescription(Dashel::Stream* stream, const AsebaNativeFunctionDescription* description)
{
	AsebaWriteString(stream, description->name);
	AsebaWriteString(stream, description->doc);
	stream->write(description->argumentCount);
	for (unsigned i = 0; i < description->argumentCount; i++)
	{
		stream->write(description->arguments[i].size);
		AsebaWriteString(stream, description->arguments[i].name);
	}
}

extern "C" void AsebaSendDescription(AsebaVMState *vm)
{
	Dashel::Stream* stream = asebaSocketMaps[vm]->stream;
	assert(stream);
	
	// compute the size of all native functions inside description
	unsigned nativeFunctionSizes = 2;
	for (unsigned i = 0; i < nativeFunctionsCount; i++)
	{
		nativeFunctionSizes += AsebaNativeFunctionGetDescriptionSize(nativeFunctionsDescriptions[i]);
		std::cout << i << " : " << AsebaNativeFunctionGetDescriptionSize(nativeFunctionsDescriptions[i]) << "\n";
	}
	
	// write sizes (basic + nodeName + variables)
	uint16 size;
	sint16 ssize;
	switch (vm->nodeId)
	{
		case 1: size = 8 + 11 + (8 + 15) + nativeFunctionSizes; break;
		case 2: size = 8 + 12 + (8 + 15) + nativeFunctionSizes; break;
		case 3: size = 8 + 18 + (8 + 20) + nativeFunctionSizes; break;
		case 4: size = 8 + 17 + (6 + 15) + nativeFunctionSizes; break;
		default: assert(false); break;
	}
	stream->write(&size, 2);
	stream->write(&vm->nodeId, 2);
	uint16 id = ASEBA_MESSAGE_DESCRIPTION;
	stream->write(&id, 2);
	
	// write node name
	switch (vm->nodeId)
	{
		case 1: AsebaWriteString(stream, "left motor"); break;
		case 2: AsebaWriteString(stream, "right motor"); break;
		case 3: AsebaWriteString(stream, "proximity sensors"); break;
		case 4: AsebaWriteString(stream, "distance sensors"); break;
		default: assert(false); break;
	}
	
	// write sizes
	stream->write(&vm->bytecodeSize, 2);
	stream->write(&vm->stackSize, 2);
	stream->write(&vm->variablesSize, 2);
	
	// write list of variables
	switch (vm->nodeId)
	{
		case 1:
		case 2:
		{
			// motors
			size = 3;
			stream->write(&size, 2);
			
			size = 32;
			stream->write(&size, 2);
			AsebaWriteString(stream, "args");
			
			size = 1;
			stream->write(&size, 2);
			AsebaWriteString(stream, "speed");
			
			size = 2;
			stream->write(&size, 2);
			AsebaWriteString(stream, "odo");
		}
		break;
		
		case 3:
		{
			// proximity sensors
			size = 3;
			stream->write(&size, 2);
			
			size = 32;
			stream->write(&size, 2);
			AsebaWriteString(stream, "args");
			
			size = 24;
			stream->write(&size, 2);
			AsebaWriteString(stream, "bumpers");
			
			size = 12;
			stream->write(&size, 2);
			AsebaWriteString(stream, "ground");
		}
		break;
		
		case 4:
		{
			// distance sensors
			size = 2;
			stream->write(&size, 2);
			
			size = 32;
			stream->write(&size, 2);
			AsebaWriteString(stream, "args");
			
			size = 180;
			stream->write(&size, 2);
			AsebaWriteString(stream, "distances");
		}
		break;
		
		default: assert(false);	break;
	}
	
	// write native functions
	size = nativeFunctionsCount;
	stream->write(&size, 2);
	
	// write all native functions descriptions
	for (unsigned i = 0; i < nativeFunctionsCount; i++)
		AsebaWriteNativeFunctionDescription(stream, nativeFunctionsDescriptions[i]);
	
	stream->flush();
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	switch (id)
	{
		case 0:
		{
			// variable pos
			int src1 = vm->stack[0];
			int src2 = vm->stack[1];
			int dest = vm->stack[2];
			int shift = vm->variables[vm->stack[3]];
			
			// variable size
			int length = vm->stack[4];
			
			long long int res = 0;
			
			for (int i = 0; i < length; i++)
			{
				res += (int)vm->variables[src1++] * (int)vm->variables[src2++];
			}
			res >>= shift;
			vm->variables[dest] = (sint16)res;
		}
		break;
		
		default: assert(false); break;
	}
	// only one native functions for now
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
	AsebaVMInit(vm, vm->nodeId);
}

namespace Enki
{
	using namespace Dashel;
	
	AsebaMarxbot::Module::Module()
	{
		bytecode.resize(512);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(64);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
		
		amountOfTimerEventInQueue = 0;
	}
	
	AsebaMarxbot::AsebaMarxbot(const std::string &target)
	{
		// setup modules specific data
		leftMotor.vm.variables = reinterpret_cast<sint16 *>(&leftMotorVariables);
		leftMotor.vm.variablesSize = sizeof(leftMotorVariables) / sizeof(sint16);
		modules.push_back(&leftMotor);
		
		rightMotor.vm.variables = reinterpret_cast<sint16 *>(&rightMotorVariables);
		rightMotor.vm.variablesSize = sizeof(rightMotorVariables) / sizeof(sint16);
		modules.push_back(&rightMotor);
		
		proximitySensors.vm.variables = reinterpret_cast<sint16 *>(&proximitySensorVariables);
		proximitySensors.vm.variablesSize = sizeof(proximitySensorVariables) / sizeof(sint16);
		modules.push_back(&proximitySensors);
		
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
		
		// init VM
		AsebaVMInit(&leftMotor.vm, 1);
		AsebaVMInit(&rightMotor.vm, 2);
		AsebaVMInit(&proximitySensors.vm, 3);
		AsebaVMInit(&distanceSensors.vm, 4);
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
		
		// push on timer events
		Event onTimer;
		onTimer.id = ASEBA_EVENT_PERIODIC;
		for (size_t i = 0; i < modules.size(); i++)
		{
			if (modules[i]->amountOfTimerEventInQueue == 0)
			{
				modules[i]->eventsQueue.push_back(onTimer);
				modules[i]->amountOfTimerEventInQueue++;
			}
		}
		
		// process all events
		while (true)
		{
			bool wasActivity = false;
			
			Hub::step();
			
			for (size_t i = 0; i < modules.size(); i++)
			{
				// if events in queue and not blocked in a thread, execute new thread
				if (!modules[i]->eventsQueue.empty() && !AsebaVMIsExecutingThread(&modules[i]->vm))
				{
					// copy event to vm
					BaseVariables *vars = reinterpret_cast<BaseVariables*>(modules[i]->vm.variables);
					const Event &event = modules[i]->eventsQueue.front();
					size_t amount = std::min(event.data.size(), sizeof(vars->args) / sizeof(sint16));
					std::copy(event.data.begin(), event.data.begin() + amount, vars->args);
					AsebaVMSetupEvent(&modules[i]->vm, event.id);
					
					// pop event
					if (event.id == ASEBA_EVENT_PERIODIC)
						modules[i]->amountOfTimerEventInQueue--;
					modules[i]->eventsQueue.pop_front();
				}
				
				// try to run, notice if anything was run
				if (AsebaVMRun(&modules[i]->vm, 65535))
					wasActivity = true;
			}
			
			if (!wasActivity)
				break;
		}
	}
	
	void AsebaMarxbot::incomingConnection(Stream *stream)
	{
		this->stream = stream;
	}
	
	void AsebaMarxbot::incomingData(Stream *stream)
	{
		unsigned short len;
		unsigned short type;
		unsigned short source;
		stream->read(&len, 2);
		stream->read(&source, 2);
		stream->read(&type, 2);
		std::valarray<unsigned char> buffer(static_cast<size_t>(len));
		stream->read(&buffer[0], buffer.size());
		
		signed short *dataPtr = reinterpret_cast<signed short *>(&buffer[0]);
		
		if (type < 0x8000)
		{
			// user type queue
			assert(buffer.size() % 2 == 0);
			
			// create event
			Event event;
			event.id = type;
			for (size_t i = 0; i < buffer.size(); i+=2)
				event.data.push_back(*dataPtr++);
			
			// for each module
			for (size_t i = 0; i < modules.size(); i++)
			{
				unsigned short address = AsebaVMGetEventAddress(&modules[i]->vm, type);
				// push event in queue if there is code to handle it
				if (address)
					modules[i]->eventsQueue.push_back(event);
			}
		}
		else
		{
			// debug message
			if (type >= 0xA000)
			{
				// not bootloader
				if (buffer.size() % 2 != 0)
				{
					std::cerr << std::hex << std::showbase;
					std::cerr << "AsebaMarxbot::incomingData() : fatal error: received event: " << type;
					std::cerr << std::dec << std::noshowbase;
					std::cerr << " of size " << buffer.size();
					std::cerr << ", which is not a multiple of two." << std::endl;
					assert(false);
				}
				for (size_t i = 0; i < modules.size(); i++)
					AsebaVMDebugMessage(&modules[i]->vm, type, reinterpret_cast<uint16 *>(dataPtr), buffer.size() / 2);
			}
		}
	}
	
	void AsebaMarxbot::connectionClosed(Stream *stream, bool abnormal)
	{
		// do nothing in addition to what is done by Client
	}
}

