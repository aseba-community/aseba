/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ASEBA_MCU_H
#define __ASEBA_MCU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../common/types.h"

/**
	\file vm.h
	Definition of Aseba Virtual Machine
*/

enum
{
	ASEBA_MAX_BREAKPOINTS = 16
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

/*
	Glue logic must implement:

	if (debug command queue is not empty)
		execute debug command
	else if (executing a thread)
		run VM
	else if (event queue is not empty)
		fetch event
		run VM
	else
		sleep until something happens
*/

/*! Init some variables that are not initialized otherwise.
	This is not sufficient to have a working VM.
	bytecode, variables and stack must be set outside this function.
	Furthermore, bytecode array must be allocated before any
	call to this function. */
void AsebaVMInit(AsebaVMState *vm, uint16 nodeId);

/*! Setup VM to execute an thread.
	If event is not managed, VM is not ready for run. */
void AsebaVMSetupEvent(AsebaVMState *vm, uint16 event);

/*! Run the VM depending on the current execution mode.
	Either run or step, depending of the current mode.
	Return 1 if anything was executed, 0 otherwise. */
uint16 AsebaVMRun(AsebaVMState *vm);

/*! Return 1 if VM is currently executing a thread */
uint16 AsebaVMIsExecutingThread(AsebaVMState *vm);

/*! Execute a debug action from a debug message. 
	dataLength is given in number of uint16 */
void AsebaVMDebugMessage(AsebaVMState *vm, uint16 id, uint16 *data, uint16 dataLength);

// Functions implemented outside by the io layer

/*! Called by AsebaStep if there is a message (not an user event) to send.
	size is given in number of bytes. */
void AsebaSendMessage(AsebaVMState *vm, uint16 id, void *data, uint16 size);

/*! Called by AsebaVMDebugMessage when some variables must be sent efficiently */
void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length);

/*! Called by AsebaVMDebugMessage when VM must send its description on the network. */
void AsebaSendDescription(AsebaVMState *vm);

/*! Called by AsebaStep to perform a native function call. */
void AsebaNativeFunction(AsebaVMState *vm, uint16 id);


// Function optionally implemented

#ifdef ASEBA_ASSERT

/*! Possible causes of AsebaAssert */
typedef enum
{
	ASEBA_ASSERT_UNKNOWN = 0,
	ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR,
	ASEBA_ASSERT_UNKNOWN_BYTECODE,
	ASEBA_ASSERT_STACK_OVERFLOW,
	ASEBA_ASSERT_STACK_UNDERFLOW,
	ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS,
	ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS,
	ASEBA_ASSERT_STEP_OUT_OF_RUN,
	ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS,
} AsebaAssertReason;

/*! If ASEBA_ASSERT is defined, this function is called when an error arise */
void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason);

#endif /* ASEBA_ASSERT */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
