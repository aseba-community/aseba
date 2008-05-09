#ifndef ASEBA_VM_BUFFER
#define ASEBA_VM_BUFFER

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/types.h"
#include "../../vm/vm.h"
#include "../../vm/natives.h"

/**
	\defgroup transport-buffer Helper for transport layers using buffers
	
	This helper provides to the VM:
	* AsebaSendMessage()
	* AsebaSendVariables()
	* AsebaSendDescription()
	
	This helper provides to the glue code:
	* AsebaDebugHandleCommands()
	
	This helper requires from the lower level transport layer:
	* AsebaSendBuffer()
	* AsebaGetBuffer()
	
	This helper requires from the glue code:
	* AsebaGetVMDescription()
	* AsebaGetNativeFunctionsDescriptions()
	
	To have a working implementation, the glue code must still implement:
	* AsebaNativeFunction()
	* AsebaAssert(), if ASEBA_ASSERT is defined
*/
/*@{*/

// functions this helper provides

/*! Read messages and process messages from transport layer, if any */
void AsebaDebugHandleCommands(AsebaVMState *vm);

// functions this helper needs

extern void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length);

extern uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source);

extern const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm);

extern const AsebaNativeFunctionDescription** AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
