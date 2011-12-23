/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

#ifndef __ASEBA_VM_H
#define __ASEBA_VM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../common/types.h"

/**
	\file vm.h
	Definition of Aseba Virtual Machine
*/

/**
	\defgroup vm Virtual Machine
	
Glue logic must implement:

\verbatim
if (debug command queue is not empty)
	execute debug command
else if (executing a thread)
	run VM
else if (event queue is not empty)
	fetch event
	run VM
else
	sleep until something happens
\endverbatim
*/
/*@{*/

enum
{
	ASEBA_MAX_BREAKPOINTS = 16		//!< maximum number of simultaneous breakpoints the target supports
};

/*! This structure contains the state of the Aseba VM.
	This is the required and the sufficient data for the VM to run.
	This is not sufficient for the compiler to build bytecode, as there is
	no description of variable names and sizes nor description of native
	function. For this, a description corresponding to the actual target
	must be provided to the compiler.
	ALL fields of this structure have to be initialized correctly for
	aseba to work. An initial call to AsebaVMInitStep must be done prior
	to any call to AsebaVMPeriodicStep or AsebaVMEventStep.
*/
typedef struct
{
	// node id
	uint16 nodeId;
	
	// bytecode
	uint16 bytecodeSize; /*!< total amount of bytecode space */
	uint16 * bytecode; /*!< bytecode space of size bytecodeSize */
	
	// variables
	uint16 variablesSize; /*!< total amount of variables space */
	sint16 * variables; /*!< variables of size variableCount */
	
	// execution stack
	uint16 stackSize; /*!< depth of execution stack */
	sint16 * stack; /*!< execution stack of size stackSize */
	
	// execution
	uint16 flags;
	uint16 pc;
	sint16 sp;
	
	// breakpoint
	uint16 breakpoints[ASEBA_MAX_BREAKPOINTS];
	uint16 breakpointsCount;
} AsebaVMState;

// Macros to work with masks

//! Set the part masked by m of v to 1
#define AsebaMaskSet(v, m) ((v) |= (m))
//! Set the part masked by m of v to 0
#define AsebaMaskClear(v, m) ((v) &= (~(m)))
//! Returns true if the part masked by m of v is 1
#define AsebaMaskIsSet(v, m) (((v) & (m)) != 0)
//! Returns true if the part masked by m of v is 0
#define AsebaMaskIsClear(v, m) (((v) & (m)) == 0)


// Functions provided by aseba-core


/*! Setup the execution status of the VM.
	This is not sufficient to have a working VM.
	nodeId and bytecode, variables, and stack along with their sizes must be set outside this function.
	The content of the variable array is zeroed by this function.
*/
void AsebaVMInit(AsebaVMState *vm);

/*!	Return the starting address of an event, or 0 if the event is not handled. */
uint16 AsebaVMGetEventAddress(AsebaVMState *vm, uint16 event);

/*! Setup VM to execute an event.
	If event is not handled, VM is not ready for run.
	Return the starting address of the event, or 0 if the event is not handled. */
uint16 AsebaVMSetupEvent(AsebaVMState *vm, uint16 event);

/*! Run the VM depending on the current execution mode.
	Either run or step, depending of the current mode.
	If stepsLimit > 0, execute at maximim stepsLimit
	Return 1 if anything was executed, 0 otherwise. */
uint16 AsebaVMRun(AsebaVMState *vm, uint16 stepsLimit);

/*! Execute a debug action from a debug message. 
	dataLength is given in number of uint16. */
void AsebaVMDebugMessage(AsebaVMState *vm, uint16 id, uint16 *data, uint16 dataLength);

/*! Can be called by glue code (including native functions), to stop vm and emit a node specific error */
void AsebaVMEmitNodeSpecificError(AsebaVMState *vm, const char* message);

/*! Return non-zero if VM will ignore the packet, 0 otherwise */
uint16 AsebaVMShouldDropPacket(AsebaVMState *vm, uint16 source, const uint8* data);

// Functions implemented outside by the glue/transport layer

/*! Called by AsebaStep if there is a message (not an user event) to send.
	size is given in number of bytes. */
void AsebaSendMessage(AsebaVMState *vm, uint16 id, const void *data, uint16 size);

#ifdef __BIG_ENDIAN__
/*! Called by AsebaStep if there is a message (not an user event) to send.
	count is given in number of words. */
void AsebaSendMessageWords(AsebaVMState *vm, uint16 type, const uint16* data, uint16 count);
#else
	#define AsebaSendMessageWords(vm,type,data,size) AsebaSendMessage(vm,type,data,(size)*2)
#endif

/*! Called by AsebaVMDebugMessage when some variables must be sent efficiently */
void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length);

/*! Called by AsebaVMDebugMessage when VM must send its description on the network. */
void AsebaSendDescription(AsebaVMState *vm);

/*! Called by AsebaStep to perform a native function call. */
void AsebaNativeFunction(AsebaVMState *vm, uint16 id);

/*! Called by AsebaVMDebugMessage when VM must write its bytecode to flash, write an empty function to leave feature unsupported */
void AsebaWriteBytecode(AsebaVMState *vm);

/*! Called by AsebaVMDebugMessage when VM must restart the node and enter to the bootloader, write an empty function to leave feature unsupported */
void AsebaResetIntoBootloader(AsebaVMState *vm);

/*! Called by AsebaVMDebugMessage when VM must put to node in deep sleep. Write an empty function to leave feature unsupported */
void AsebaPutVmToSleep(AsebaVMState *vm);

/*! Called by AsebaVMDebugMessage when VM is going to be run */
void __attribute__((weak)) AsebaVMRunCB(AsebaVMState *vm);

/*! Called by AsebaVMEmitNodeSpecificError to be notified when VM hit an execution error
	Is also called for wrong array access or division by 0 with message == NULL */
void __attribute__((weak)) AsebaVMErrorCB(AsebaVMState *vm, const char* message);

// Function optionally implemented

#ifdef ASEBA_ASSERT

/*! Possible causes of AsebaAssert */
typedef enum
{
	ASEBA_ASSERT_UNKNOWN = 0,
	ASEBA_ASSERT_UNKNOWN_UNARY_OPERATOR,
	ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR,
	ASEBA_ASSERT_UNKNOWN_BYTECODE,
	ASEBA_ASSERT_STACK_OVERFLOW,
	ASEBA_ASSERT_STACK_UNDERFLOW,
	ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS,
	ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS,
	ASEBA_ASSERT_STEP_OUT_OF_RUN,
	ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS,
	ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG,
} AsebaAssertReason;

/*! If ASEBA_ASSERT is defined, this function is called when an error arise */
void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason);

#endif /* ASEBA_ASSERT */

/*@}*/

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
