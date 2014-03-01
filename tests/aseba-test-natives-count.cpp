#include "../vm/natives.h"

// C++
#include <iostream>

// this prevents a link problem when compiling in debug
void AsebaSendMessage(AsebaVMState *vm, uint16 id, const void *data, uint16 size)
{
}

static AsebaNativeFunctionPointer nativeFunctions[] =
{
	ASEBA_NATIVES_STD_FUNCTIONS,
};

static bool executionError(false);

int main(int argc, char*argv[])
{
	size_t nativesCount = (sizeof(nativeFunctions)/sizeof(AsebaNativeFunctionPointer));
	if (nativesCount != ASEBA_NATIVES_STD_COUNT)
		return 1;
	else
		return 0;
}

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

