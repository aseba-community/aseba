/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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
	* AsebaProcessIncomingEvents()

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
void AsebaProcessIncomingEvents(AsebaVMState *vm);

// functions this helper needs

extern void AsebaSendBuffer(AsebaVMState *vm, const uint8_t* data, uint16_t length);

extern uint16_t AsebaGetBuffer(AsebaVMState *vm, uint8_t* data, uint16_t maxLength, uint16_t* source);

extern const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm);

extern const AsebaLocalEventDescription * AsebaGetLocalEventsDescriptions(AsebaVMState *vm);

extern const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
