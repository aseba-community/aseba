/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
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

#include "../common/consts.h"
#include "../common/types.h"
#include "vm.h"
#include <string.h>

/**
	\file vm.c
	Implementation of Aseba Virtual Machine
*/

/** \addtogroup vm */
/*@{*/

// TODO: potentially replace by more efficient native asm instruction
//! Return true if bit b of v is 1
#define GET_BIT(v, b) (((v) >> (b)) & 0x1)
//! Set bit b of v to 1
#define BIT_SET(v, b) ((v) |= (1 << (b)))
//! Set bit b of v to 0
#define BIT_CLR(v, b) ((v) &= (~(1 << (b))))

void AsebaVMSendExecutionStateChanged(AsebaVMState *vm);

void AsebaVMInit(AsebaVMState *vm, uint16 nodeId)
{
	vm->nodeId = nodeId;
	vm->pc = 0;
	vm->flags = 0;
	vm->breakpointsCount = 0;
	
	// fill with no event
	vm->bytecode[0] = 0;
	memset(vm->variables, 0, vm->variablesSize*sizeof(sint16));
}

uint16 AsebaVMGetEventAddress(AsebaVMState *vm, uint16 event)
{
	uint16 eventVectorSize = vm->bytecode[0];
	uint16 i;

	// look into event vectors and if event match execute corresponding bytecode
	for (i = 1; i < eventVectorSize; i += 2)
		if (vm->bytecode[i] == event)
			return vm->bytecode[i + 1];
	return 0;
}


void AsebaVMSetupEvent(AsebaVMState *vm, uint16 event)
{
	uint16 address = AsebaVMGetEventAddress(vm, event);
	if (!address)
		return;
	
	vm->pc = address;
	vm->sp = -1;
	AsebaMaskSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK);
	
	// if we are in step by step, notify
	if (AsebaMaskIsSet(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK))
		AsebaVMSendExecutionStateChanged(vm);
}

/*! Execute one bytecode of the current VM thread.
	VM must be ready for run otherwise trashes may occur. */
void AsebaVMStep(AsebaVMState *vm)
{
	uint16 bytecode = vm->bytecode[vm->pc];
	
	#ifdef ASEBA_ASSERT
	if (AsebaMaskIsClear(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		AsebaAssert(vm, ASEBA_ASSERT_STEP_OUT_OF_RUN);
	#endif
	
	switch (bytecode >> 12)
	{
		// Bytecode: Stop
		case ASEBA_BYTECODE_STOP:
		{
			AsebaMaskClear(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK);
		}
		break;
		
		// Bytecode: Small Immediate
		case ASEBA_BYTECODE_SMALL_IMMEDIATE:
		{
			sint16 value = ((sint16)(bytecode << 4)) >> 4;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp + 1 >= vm->stackSize)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_OVERFLOW);
			#endif
			
			// push value in stack
			vm->stack[++vm->sp] = value;
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Large Immediate
		case ASEBA_BYTECODE_LARGE_IMMEDIATE:
		{
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp + 1 >= vm->stackSize)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_OVERFLOW);
			#endif
			
			// push value in stack
			vm->stack[++vm->sp] = vm->bytecode[vm->pc + 1];
			
			// increment PC
			vm->pc += 2;
		}
		break;
		
		// Bytecode: Load
		case ASEBA_BYTECODE_LOAD:
		{
			uint16 variableIndex = bytecode & 0x0fff;
			
			// check sp and variable index
			#ifdef ASEBA_ASSERT
			if (vm->sp + 1 >= vm->stackSize)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_OVERFLOW);
			if (variableIndex >= vm->variablesSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS);
			#endif
			
			// push value in stack
			vm->stack[++vm->sp] = vm->variables[variableIndex];
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Store
		case ASEBA_BYTECODE_STORE:
		{
			uint16 variableIndex = bytecode & 0x0fff;
			
			// check sp and variable index
			#ifdef ASEBA_ASSERT
			if (vm->sp < 0)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			if (variableIndex >= vm->variablesSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS);
			#endif
			
			// pop value from stack
			vm->variables[variableIndex] = vm->stack[vm->sp--];
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Load Indirect
		case ASEBA_BYTECODE_LOAD_INDIRECT:
		{
			uint16 variableIndex;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 0)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			#endif
			
			// get variable index
			variableIndex =  vm->stack[vm->sp];
			
			// check variable index
			#ifndef ASEBA_NO_ARRAY_CHECK
			if (variableIndex >= vm->variablesSize)
			{
				uint16 buffer[2];
				AsebaMaskClear(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK);
				buffer[0] = vm->pc;
				buffer[1] = variableIndex;
				AsebaSendMessage(vm, ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS, buffer, sizeof(buffer));
				break;
			}
			#endif
			
			// load variable
			vm->stack[vm->sp] = vm->variables[variableIndex];
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Store Indirect
		case ASEBA_BYTECODE_STORE_INDIRECT:
		{
			uint16 variableIndex;
			sint16 variableValue;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 1)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			#endif
			
			// get variable value and index
			variableValue =  vm->stack[vm->sp - 1];
			variableIndex =  (uint16)vm->stack[vm->sp];
			
			// check variable index
			#ifndef ASEBA_NO_ARRAY_CHECK
			if (variableIndex >= vm->variablesSize)
			{
				uint16 buffer[2];
				buffer[0] = vm->pc;
				buffer[1] = variableIndex;
				vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
				AsebaSendMessage(vm, ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS, buffer, sizeof(buffer));
				break;
			}
			#endif
			
			// store variable and change sp
			vm->variables[variableIndex] = variableValue;
			vm->sp -= 2;
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Unary Arithmetic
		case ASEBA_BYTECODE_UNARY_ARITHMETIC:
		{
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 0)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			#endif
			
			// Only unary negation is supported for now
			vm->stack[vm->sp] = -vm->stack[vm->sp];
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Binary Arithmetic
		case ASEBA_BYTECODE_BINARY_ARITHMETIC:
		{
			sint16 valueOne, valueTwo, opResult;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 1)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			#endif
			
			// get operands
			valueOne = vm->stack[vm->sp - 1];
			valueTwo = vm->stack[vm->sp];
			
			// check division by zero
			if (((bytecode & 0x00ff) == ASEBA_OP_DIV) && (valueTwo == 0))
			{
				vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
				AsebaSendMessage(vm, ASEBA_MESSAGE_DIVISION_BY_ZERO, &(vm->pc), sizeof(vm->pc));
				break;
			}
			
			// do operations
			switch (bytecode & 0x00ff)
			{
				case ASEBA_OP_SHIFT_LEFT: opResult = valueOne << valueTwo; break;
				case ASEBA_OP_SHIFT_RIGHT: opResult = valueOne >> valueTwo; break;
				case ASEBA_OP_ADD: opResult = valueOne + valueTwo; break;
				case ASEBA_OP_SUB: opResult = valueOne - valueTwo; break;
				case ASEBA_OP_MULT: opResult = valueOne * valueTwo; break;
				case ASEBA_OP_DIV: opResult = valueOne / valueTwo; break;
				case ASEBA_OP_MOD: opResult = valueOne % valueTwo; break;
				default:
				#ifdef ASEBA_ASSERT
				AsebaAssert(vm, ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR);
				#endif
				break;
			};
			
			// write result
			vm->sp--;
			vm->stack[vm->sp] = opResult;
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Jump
		case ASEBA_BYTECODE_JUMP:
		{
			sint16 disp = ((sint16)(bytecode << 4)) >> 4;
			
			// check pc
			#ifdef ASEBA_ASSERT
			if ((vm->pc + disp < 0) || (vm->pc + disp >=  vm->bytecodeSize))
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS);
			#endif
			
			// do jump
			vm->pc += disp;
		}
		break;
		
		// Bytecode: Conditional Branch
		case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
		{
			sint16 conditionResult;
			sint16 valueOne, valueTwo;
			sint16 disp;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 1)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_OVERFLOW);
			#endif
			
			// evaluate condition
			valueOne = vm->stack[vm->sp - 1];
			valueTwo = vm->stack[vm->sp];
			vm->sp -= 2;
			switch (bytecode & 0x7)
			{
				case ASEBA_CMP_EQUAL: conditionResult = valueOne == valueTwo; break;
				case ASEBA_CMP_NOT_EQUAL: conditionResult = valueOne != valueTwo; break;
				case ASEBA_CMP_BIGGER_THAN: conditionResult = valueOne > valueTwo; break;
				case ASEBA_CMP_BIGGER_EQUAL_THAN: conditionResult = valueOne >= valueTwo; break;
				case ASEBA_CMP_SMALLER_THAN: conditionResult = valueOne < valueTwo; break;
				case ASEBA_CMP_SMALLER_EQUAL_THAN: conditionResult = valueOne <= valueTwo; break;
			}
			
			// is the condition really true ?
			if (conditionResult && !(GET_BIT(bytecode, 3) && GET_BIT(bytecode, 4)))
			{
				// if true disp
				disp = (sint16)vm->bytecode[vm->pc + 1];
			}
			else
			{
				// if false disp
				disp = (sint16)vm->bytecode[vm->pc + 2];
			}
			
			// write back condition result
			if (conditionResult)
				BIT_SET(vm->bytecode[vm->pc], 4);
			else
				BIT_CLR(vm->bytecode[vm->pc], 4);
			
			// check pc
			#ifdef ASEBA_ASSERT
			if ((vm->pc + disp < 0) || (vm->pc + disp >=  vm->bytecodeSize))
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS);
			#endif
			
			// do branch
			vm->pc += disp;
		}
		break;
		
		// Bytecode: Emit
		case ASEBA_BYTECODE_EMIT:
		{
			// emit event
			uint16 start = vm->bytecode[vm->pc + 1];
			uint16 length = vm->bytecode[vm->pc + 2] * 2;
			AsebaSendMessage(vm, bytecode & 0x0fff, vm->variables + start, length);
			
			// increment PC
			vm->pc += 3;
		}
		break;
		
		// Bytecode: Call
		case ASEBA_BYTECODE_CALL:
		{
			// call native function
			AsebaNativeFunction(vm, bytecode & 0x0fff);
			vm->sp = -1;
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		default:
		#ifdef ASEBA_ASSERT
		AsebaAssert(vm, ASEBA_ASSERT_UNKNOWN_BYTECODE);
		#endif
		break;
	} // switch bytecode...
}

/*! Execute on bytecode of the current VM thread and check for potential breakpoints.
	Return 1 if breakpoint was seen, 0 otherwise.
	VM must be ready for run otherwise trashes may occur. */
uint16 AsebaVMCheckBreakpoint(AsebaVMState *vm)
{
	uint16 i;
	for (i = 0; i < vm->breakpointsCount; i++)
	{
		if (vm->breakpoints[i] == vm->pc)
		{
			AsebaMaskSet(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK);
			return 1;
		}
	}
	
	return 0;
}

/*! Run without support of breakpoints.
	Check ASEBA_VM_EVENT_RUNNING_MASK to exit on interrupts or stepsLimit if > 0. */
void AsebaDebugBareRun(AsebaVMState *vm, uint16 stepsLimit)
{
	AsebaMaskSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
	
	if (stepsLimit > 0)
	{
		// no breakpoint, still poll the mask and check stepsLimit
		while (AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK) &&
			AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK) &&
			stepsLimit
		)
		{
			AsebaVMStep(vm);
			stepsLimit--;
			// TODO : send exception event on step limits overflow
		}
	}
	else
	{
		// no breakpoint, only poll the mask
		while (AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK) &&
			AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK) 
		)
			AsebaVMStep(vm);
	}
	
	AsebaMaskClear(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
}

/*! Run with support of breakpoints.
	Also check ASEBA_VM_EVENT_RUNNING_MASK to exit on interrupts. */
void AsebaDebugBreakpointRun(AsebaVMState *vm, uint16 stepsLimit)
{
	AsebaMaskSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
	
	if (stepsLimit > 0)
	{
		// breakpoints, check before each step, poll the mask, and check stepsLimit
		while (AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK) &&
			AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK) &&
			stepsLimit
		)
		{
			if (AsebaVMCheckBreakpoint(vm) != 0)
			{
				AsebaMaskSet(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK);
				AsebaVMSendExecutionStateChanged(vm);
				return;
			}
			AsebaVMStep(vm);
			stepsLimit--;
			// TODO : send exception event on step limits overflow
		}
	}
	else
	{
		// breakpoints, check before each step and poll the mask
		while (AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK) &&
			AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK)
		)
		{
			if (AsebaVMCheckBreakpoint(vm) != 0)
			{
				AsebaMaskSet(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK);
				AsebaVMSendExecutionStateChanged(vm);
				return;
			}
			AsebaVMStep(vm);
		}
	}
	
	AsebaMaskClear(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
}

uint16 AsebaVMRun(AsebaVMState *vm, uint16 stepsLimit)
{
	// if there is nothing to execute, just return
	if (AsebaMaskIsClear(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		return 0;
	
	// if we are running step by step, just return either
	if (AsebaMaskIsSet(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK))
		return 0;
	
	// run until something stops the vm
	if (vm->breakpointsCount)
		AsebaDebugBreakpointRun(vm, stepsLimit);
	else
		AsebaDebugBareRun(vm, stepsLimit);
	
	return 1;
}

uint16 AsebaVMIsExecutingThread(AsebaVMState *vm)
{
	return AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK);
}

/*! Set a breakpoint at a specific location. */
uint8 AsebaVMSetBreakpoint(AsebaVMState *vm, uint16 pc)
{
	#ifdef ASEBA_ASSERT
	if (pc >= vm->bytecodeSize)
		AsebaAssert(vm, ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS);
	#endif
	
	if (vm->breakpointsCount < ASEBA_MAX_BREAKPOINTS)
	{
		vm->breakpoints[vm->breakpointsCount++] = pc;
		return 1;
	}
	else
		return 0;
}

/*! Clear the breakpoint at a specific location. */
uint16 AsebaVMClearBreakpoint(AsebaVMState *vm, uint16 pc)
{
	uint16 i;
	for (i = 0; i < vm->breakpointsCount; i++)
	{
		if (vm->breakpoints[i] == pc)
		{
			uint16 j;
			// displace
			vm->breakpointsCount--;
			for (j = i; j < vm->breakpointsCount; j++)
				vm->breakpoints[j] = vm->breakpoints[j+1];
			return 1;
		}
	}
	return 0;
}

/*! Clear all breakpoints. */
void AsebaVMClearBreakpoints(AsebaVMState *vm)
{
	vm->breakpointsCount = 0;
}

/*! Send an execution state changed message */
void AsebaVMSendExecutionStateChanged(AsebaVMState *vm)
{
	uint16 buffer[2];
	buffer[0] = vm->pc;
	buffer[1] = vm->flags;
	AsebaSendMessage(vm, ASEBA_MESSAGE_EXECUTION_STATE_CHANGED, buffer, sizeof(buffer));
}

void AsebaVMDebugMessage(AsebaVMState *vm, uint16 id, uint16 *data, uint16 dataLength)
{
	// react to global presence
	if (id == ASEBA_MESSAGE_GET_DESCRIPTION)
	{
		AsebaSendDescription(vm);
		return;
	}
	
	// check if we are the destination, return otherwise
	if (data[0] != vm->nodeId)
		return;
	
	data++;
	dataLength--;
	
	switch (id)
	{
		case ASEBA_MESSAGE_SET_BYTECODE:
		{
			uint16 start = data[0];
			uint16 length = dataLength - 1;
			uint16 i;
			#ifdef ASEBA_ASSERT
			if (start + length > vm->bytecodeSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS);
			#endif
			for (i = 0; i < length; i++)
				vm->bytecode[start+i] = data[i+1];
		}
		// There is no break here because we want to do a reset after a set bytecode
		
		case ASEBA_MESSAGE_RESET:
		vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
		AsebaVMSetupEvent(vm, ASEBA_EVENT_INIT);
		break;
		
		case ASEBA_MESSAGE_RUN:
		AsebaMaskClear(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK);
		AsebaVMSendExecutionStateChanged(vm);
		break;
		
		case ASEBA_MESSAGE_PAUSE:
		AsebaMaskSet(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK);
		AsebaVMSendExecutionStateChanged(vm);
		break;
		
		case ASEBA_MESSAGE_STEP:
		if (AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			AsebaVMStep(vm);
			AsebaVMSendExecutionStateChanged(vm);
		}
		break;
		
		case ASEBA_MESSAGE_STOP:
		vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
		AsebaVMSendExecutionStateChanged(vm);
		break;
		
		case ASEBA_MESSAGE_GET_EXECUTION_STATE:
		AsebaVMSendExecutionStateChanged(vm);
		break;
		
		case ASEBA_MESSAGE_BREAKPOINT_SET:
		{
			uint16 buffer[2];
			buffer[0] = data[0];
			buffer[1] = AsebaVMSetBreakpoint(vm, data[0]);
			AsebaSendMessage(vm, ASEBA_MESSAGE_BREAKPOINT_SET_RESULT, buffer, sizeof(buffer));
		}
		break;
		
		case ASEBA_MESSAGE_BREAKPOINT_CLEAR:
		AsebaVMClearBreakpoint(vm, data[0]);
		break;
		
		case ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL:
		AsebaVMClearBreakpoints(vm);
		break;
		
		case ASEBA_MESSAGE_GET_VARIABLES:
		{
			uint16 start = data[0];
			uint16 length = data[1];
			#ifdef ASEBA_ASSERT
			if (start + length > vm->variablesSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS);
			#endif
			AsebaSendVariables(vm, start, length);
		}
		break;
		
		case ASEBA_MESSAGE_SET_VARIABLES:
		{
			uint16 start = data[0];
			uint16 length = dataLength - 1;
			uint16 i;
			#ifdef ASEBA_ASSERT
			if (start + length > vm->variablesSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS);
			#endif
			for (i = 0; i < length; i++)
				vm->variables[start+i] = data[i+1];
		}
		break;
		
		default:
		break;
	}
}

/*@}*/
