#include "../vm/natives.h"

int main(int argc, char*argv[])
{
	AsebaNativeFunctionPointer defaultNatives[] ={ ASEBA_NATIVES_STD_FUNCTIONS };
	size_t nativesCount = (sizeof(defaultNatives)/sizeof(AsebaNativeFunctionPointer));
	if (nativesCount != ASEBA_NATIVES_STD_COUNT)
		return 1;
	else
		return 0;
}