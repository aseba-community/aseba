#include "../buffer/vm-buffer.h"

void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	while (AsebaCanSend(data, length) == 0)
		AsebaIdle();
}

uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	return AsebaCanRecv(data, maxLength, source);
}



