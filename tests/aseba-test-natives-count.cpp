#include "../vm/natives.h"

// C++
#include <iostream>

static AsebaNativeFunctionPointer nativeFunctions[] =
{
	ASEBA_NATIVES_STD_FUNCTIONS,
};

int main(int argc, char*argv[])
{
	size_t nativesCount = (sizeof(nativeFunctions)/sizeof(AsebaNativeFunctionPointer));
	if (nativesCount != ASEBA_NATIVES_STD_COUNT)
		return 1;
	else
		return 0;
}


