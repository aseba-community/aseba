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

// Aseba
#include "../transport/buffer/vm-buffer.h"
#include "../vm/vm.h"
#include "../vm/natives.h"
#include "../common/consts.h"

// C++
#include <iostream>

static bool executionError(false);

extern "C" bool AsebaExecutionErrorOccurred()
{
	return executionError;
}

extern "C" void AsebaSendMessage(AsebaVMState *vm, uint16 type, const void *data, uint16 size)
{
	switch (type)
	{
		case ASEBA_MESSAGE_DIVISION_BY_ZERO:
		std::cerr << "Division by zero" << std::endl;
		executionError = true;
		break;
		
		case ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS:
		std::cerr << "Array access out of bounds" << std::endl;
		executionError = true;
		break;
		
		default:
		std::cerr << "AsebaSendMessage of type " << type << ", size " << size << std::endl;
		break;
	}
}

#ifdef __BIG_ENDIAN__
extern "C" void AsebaSendMessageWords(AsebaVMState *vm, uint16 type, const uint16* data, uint16 count)
{
	AsebaSendMessage(vm, type, data, count*2);
}
#endif

extern "C" void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length)
{
	std::cerr << "AsebaSendVariables at pos " << start << ", length " << length << std::endl;
}

extern "C" void AsebaSendDescription(AsebaVMState *vm)
{
	std::cerr << "AsebaSendDescription" << std::endl;
}

extern "C" void AsebaPutVmToSleep(AsebaVMState *vm)
{
	std::cerr << "AsebaPutVmToSleep" << std::endl;
}

static AsebaNativeFunctionPointer nativeFunctions[] =
{
	ASEBA_NATIVES_STD_FUNCTIONS,
};

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	nativeFunctions[id](vm);
}

extern "C" void AsebaWriteBytecode(AsebaVMState *vm)
{
	std::cerr << "AsebaWriteBytecode" << std::endl;
}

extern "C" void AsebaResetIntoBootloader(AsebaVMState *vm)
{
	std::cerr << "AsebaResetIntoBootloader" << std::endl;
}

extern "C" void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	std::cerr << "\nFatal error, internal VM exception: ";
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
	executionError = true;
	AsebaVMInit(vm);
}

