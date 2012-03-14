#include "../vm/natives.h"

// this prevents a link problem when compiling in debug
void AsebaSendMessage(AsebaVMState *vm, uint16 id, const void *data, uint16 size)
{
}

int main(int argc, char*argv[])
{
	AsebaNativeFunctionPointer defaultNatives[] ={ ASEBA_NATIVES_STD_FUNCTIONS };
	size_t nativesCount = (sizeof(defaultNatives)/sizeof(AsebaNativeFunctionPointer));
	if (nativesCount != ASEBA_NATIVES_STD_COUNT)
		return 1;
	else
		return 0;
}