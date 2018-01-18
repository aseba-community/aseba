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

#ifndef ASEBA_ASSERT
#define ASEBA_ASSERT
#endif

#include <string>
#include <typeinfo>
#include <algorithm>
#include <cassert>
#include <cstring>
#include "AsebaGlue.h"
#include "EnkiGlue.h"
#include "../../vm/vm.h"
#include "../../common/utils/FormatableString.h"

namespace Aseba
{
	// Mapping so that Aseba C callbacks can dispatch to the right objects
	VMStateToEnvironment vmStateToEnvironment;

	NamedRobot::NamedRobot(std::string robotName):
		robotName(std::move(robotName))
	{
	}

	// SingleVMNodeGlue

	SingleVMNodeGlue::SingleVMNodeGlue(std::string robotName, int16_t nodeId):
		NamedRobot(std::move(robotName))
	{
		vm.nodeId = nodeId;
	}

	// RecvBufferNodeConnection

	uint16_t RecvBufferNodeConnection::getBuffer(uint8_t* data, uint16_t maxLength, uint16_t* source)
	{
		if (lastMessageData.size())
		{
			*source = lastMessageSource;
			size_t len(std::min<size_t>(maxLength, lastMessageData.size()));
			memcpy(data, &lastMessageData[0], len);
			return len;
		}
		return 0;
	}

} // Aseba

// implementation of Aseba glue C functions

extern "C" void AsebaPutVmToSleep(AsebaVMState *vm)
{
	// not implemented in playground
}

extern "C" void AsebaSendBuffer(AsebaVMState *vm, const uint8_t* data, uint16_t length)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.find(vm)->second);
	Aseba::AbstractNodeConnection* connection(environment.second);
	assert(connection);
	connection->sendBuffer(vm->nodeId, data, length);
}

extern "C" uint16_t AsebaGetBuffer(AsebaVMState *vm, uint8_t* data, uint16_t maxLength, uint16_t* source)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.find(vm)->second);
	Aseba::AbstractNodeConnection* connection(environment.second);
	assert(connection);
	return connection->getBuffer(data, maxLength, source);
}

extern "C" const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.find(vm)->second);
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	return glue->getDescription();
}

extern "C" const AsebaLocalEventDescription * AsebaGetLocalEventsDescriptions(AsebaVMState *vm)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.find(vm)->second);
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	return glue->getLocalEventsDescriptions();
}

extern "C" const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.find(vm)->second);
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	return glue->getNativeFunctionsDescriptions();
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16_t id)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.find(vm)->second);
	Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	glue->callNativeFunction(id);
}

extern "C" void AsebaWriteBytecode(AsebaVMState *vm)
{
	// not implemented in playground
}

extern "C" void AsebaResetIntoBootloader(AsebaVMState *vm)
{
	// not implemented in playground
}

extern "C" void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.find(vm)->second);
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	std::cerr << Aseba::FormatableString("\nFatal error: glue %0 with node id %1 of type %2 at has produced exception: ").arg(glue).arg(vm->nodeId).arg(typeid(glue).name()) << std::endl;
	switch (reason)
	{
		case ASEBA_ASSERT_UNKNOWN: std::cerr << "undefined" << std::endl; break;
		case ASEBA_ASSERT_UNKNOWN_UNARY_OPERATOR: std::cerr << "unknown unary operator" << std::endl; break;
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator" << std::endl; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode" << std::endl; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow" << std::endl; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow" << std::endl; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds" << std::endl; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds" << std::endl; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run" << std::endl; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds" << std::endl; break;
		case ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG: std::cerr << "tried to emit a buffer too long" << std::endl; break;
		default: std::cerr << "unknown exception" << std::endl; break;
	}
	std::cerr << ".\npc = " << vm->pc << ", sp = " << vm->sp << std::endl;
	//std::cerr << "\nResetting VM" << std::endl;
	std::cerr << "\nAborting playground, please report bug to \nhttps://github.com/aseba-community/aseba/issues/new" << std::endl;
	abort();
	AsebaVMInit(vm);
}
