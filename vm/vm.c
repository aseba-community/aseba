/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

void AsebaVMInit(AsebaVMState *vm)
{
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


uint16 AsebaVMSetupEvent(AsebaVMState *vm, uint16 event)
{
	uint16 address = AsebaVMGetEventAddress(vm, event);
	if (address)
	{
		// if currently executing a thread, notify kill
		if (AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			AsebaSendMessageWords(vm, ASEBA_MESSAGE_EVENT_EXECUTION_KILLED, &vm->pc, 1);
		}
		
		vm->pc = address;
		vm->sp = -1;
		AsebaMaskSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK);
		
		// if we are in step by step, notify
		if (AsebaMaskIsSet(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK))
			AsebaVMSendExecutionStateChanged(vm);
	}
	return address;
}

static sint16 AsebaVMDoBinaryOperation(AsebaVMState *vm, sint16 valueOne, sint16 valueTwo, uint16 op)
{
	switch (op)
	{
		case ASEBA_OP_SHIFT_LEFT: return valueOne << valueTwo;
		case ASEBA_OP_SHIFT_RIGHT: return valueOne >> valueTwo;
		case ASEBA_OP_ADD: return valueOne + valueTwo;
		case ASEBA_OP_SUB: return valueOne - valueTwo;
		case ASEBA_OP_MULT: return valueOne * valueTwo;
		case ASEBA_OP_DIV:
			// check division by zero
			if (valueTwo == 0)
			{
				if(AsebaVMErrorCB)
					AsebaVMErrorCB(vm,NULL);
				vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
				AsebaSendMessageWords(vm, ASEBA_MESSAGE_DIVISION_BY_ZERO, &vm->pc, 1);
				return 0;
			}
			else
			{
				return valueOne / valueTwo;
			}
		case ASEBA_OP_MOD: return valueOne % valueTwo;
		
		case ASEBA_OP_BIT_OR: return valueOne | valueTwo;
		case ASEBA_OP_BIT_XOR: return valueOne ^ valueTwo;
		case ASEBA_OP_BIT_AND: return valueOne & valueTwo;
		
		case ASEBA_OP_EQUAL: return valueOne == valueTwo;
		case ASEBA_OP_NOT_EQUAL: return valueOne != valueTwo;
		case ASEBA_OP_BIGGER_THAN: return valueOne > valueTwo;
		case ASEBA_OP_BIGGER_EQUAL_THAN: return valueOne >= valueTwo;
		case ASEBA_OP_SMALLER_THAN: return valueOne < valueTwo;
		case ASEBA_OP_SMALLER_EQUAL_THAN: return valueOne <= valueTwo;
		
		case ASEBA_OP_OR: return valueOne || valueTwo;
		case ASEBA_OP_AND: return valueOne && valueTwo;
		
		default:
		#ifdef ASEBA_ASSERT
		AsebaAssert(vm, ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR);
		#endif
		return 0;
	}
}

static sint16 AsebaVMDoUnaryOperation(AsebaVMState *vm, sint16 value, uint16 op)
{
	switch (op)
	{
		case ASEBA_UNARY_OP_SUB: return -value;
		case ASEBA_UNARY_OP_ABS: return value >= 0 ? value : -value;
		case ASEBA_UNARY_OP_BIT_NOT: return ~value;
		
		default:
		#ifdef ASEBA_ASSERT
		AsebaAssert(vm, ASEBA_ASSERT_UNKNOWN_UNARY_OPERATOR);
		#endif
		return 0;
	}
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
			uint16 arrayIndex;
			uint16 arraySize;
			uint16 variableIndex;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 0)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			#endif
			
			// get indexes
			arrayIndex = bytecode & 0x0fff;
			arraySize = vm->bytecode[vm->pc + 1];
			variableIndex = vm->stack[vm->sp];
			
			// check variable index
			if (variableIndex >= arraySize)
			{
				uint16 buffer[3];
				buffer[0] = vm->pc;
				buffer[1] = arraySize;
				buffer[2] = variableIndex;
				vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
				AsebaSendMessage(vm, ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS, buffer, 3);
				if(AsebaVMErrorCB)
					AsebaVMErrorCB(vm,NULL);
				break;
			}
			
			// load variable
			vm->stack[vm->sp] = vm->variables[arrayIndex + variableIndex];
			
			// increment PC
			vm->pc += 2;
		}
		break;
		
		// Bytecode: Store Indirect
		case ASEBA_BYTECODE_STORE_INDIRECT:
		{
			uint16 arrayIndex;
			uint16 arraySize;
			uint16 variableIndex;
			sint16 variableValue;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 1)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			#endif
			
			// get value and indexes
			arrayIndex = bytecode & 0x0fff;
			arraySize = vm->bytecode[vm->pc + 1];
			variableValue = vm->stack[vm->sp - 1];
			variableIndex = (uint16)vm->stack[vm->sp];
			
			// check variable index
			if (variableIndex >= arraySize)
			{
				uint16 buffer[3];
				buffer[0] = vm->pc;
				buffer[1] = arraySize;
				buffer[2] = variableIndex;
				vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
				AsebaSendMessageWords(vm, ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS, buffer, 3);
				if(AsebaVMErrorCB)
					AsebaVMErrorCB(vm,NULL);
				break;
			}
			
			// store variable and change sp
			vm->variables[arrayIndex + variableIndex] = variableValue;
			vm->sp -= 2;
			
			// increment PC
			vm->pc += 2;
		}
		break;
		
		// Bytecode: Unary Arithmetic
		case ASEBA_BYTECODE_UNARY_ARITHMETIC:
		{
			sint16 value, opResult;
			
			// check sp
			#ifdef ASEBA_ASSERT
			if (vm->sp < 0)
				AsebaAssert(vm, ASEBA_ASSERT_STACK_UNDERFLOW);
			#endif
			
			// get operand
			value = vm->stack[vm->sp];
			
			// do operation
			opResult = AsebaVMDoUnaryOperation(vm, value, bytecode & ASEBA_UNARY_OPERATOR_MASK);
			
			// write result
			vm->stack[vm->sp] = opResult;
			
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
			
			// do operation
			opResult = AsebaVMDoBinaryOperation(vm, valueOne, valueTwo, bytecode & ASEBA_BINARY_OPERATOR_MASK);
			
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
			conditionResult = AsebaVMDoBinaryOperation(vm, valueOne, valueTwo, bytecode & ASEBA_BINARY_OPERATOR_MASK);
			vm->sp -= 2;
			
			// is the condition really true ?
			if (conditionResult && !(GET_BIT(bytecode, ASEBA_IF_IS_WHEN_BIT) && GET_BIT(bytecode, ASEBA_IF_WAS_TRUE_BIT)))
			{
				// if true disp
				disp = 2;
			}
			else
			{
				// if false disp
				disp = (sint16)vm->bytecode[vm->pc + 1];
			}
			
			// write back condition result
			if (conditionResult)
				BIT_SET(vm->bytecode[vm->pc], ASEBA_IF_WAS_TRUE_BIT);
			else
				BIT_CLR(vm->bytecode[vm->pc], ASEBA_IF_WAS_TRUE_BIT);
			
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
			uint16 length = vm->bytecode[vm->pc + 2];
			
			#ifdef ASEBA_ASSERT
			if (length > ASEBA_MAX_PACKET_SIZE - 6)
				AsebaAssert(vm, ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG);
			#endif
			AsebaSendMessageWords(vm, bytecode & 0x0fff, vm->variables + start, length);
			
			// increment PC
			vm->pc += 3;
		}
		break;
		
		// Bytecode: Call
		case ASEBA_BYTECODE_NATIVE_CALL:
		{
			// call native function
			AsebaNativeFunction(vm, bytecode & 0x0fff);
			
			// increment PC
			vm->pc ++;
		}
		break;
		
		// Bytecode: Subroutine call
		case ASEBA_BYTECODE_SUB_CALL:
		{
			uint16 dest = bytecode & 0x0fff;
			
			// store return value on stack
			vm->stack[++vm->sp] = vm->pc + 1;
			
			// jump
			vm->pc = dest;
		}
		break;
		
		// Bytecode: Subroutine return
		case ASEBA_BYTECODE_SUB_RET:
		{
			// do return
			vm->pc = vm->stack[vm->sp--];
		}
		break;
		
		default:
		#ifdef ASEBA_ASSERT
		AsebaAssert(vm, ASEBA_ASSERT_UNKNOWN_BYTECODE);
		#endif
		break;
	} // switch bytecode...
}

void AsebaVMEmitNodeSpecificError(AsebaVMState *vm, const char* message)
{
	uint16 msgLen = strlen(message);
#if defined(__GNUC__)
	uint8 buffer[msgLen+3];
#elif defined(_MSC_VER)
	uint8 * buffer = _alloca(msgLen+3);
#else
	#error "Please provide a stack memory allocator for your compiler"
#endif
	
	if (AsebaVMErrorCB)
		AsebaVMErrorCB(vm, message);
	
	vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
	
	((uint16*)buffer)[0] = bswap16(vm->pc);
	buffer[2] = (uint8)msgLen;
	memcpy(buffer+3, message, msgLen);
	
	AsebaSendMessage(vm, ASEBA_MESSAGE_NODE_SPECIFIC_ERROR, buffer, msgLen+3);
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
	AsebaSendMessageWords(vm, ASEBA_MESSAGE_EXECUTION_STATE_CHANGED, buffer, 2);
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
	if (bswap16(data[0]) != vm->nodeId)
		return;

	data++;
	dataLength--;
	
	switch (id)
	{
		case ASEBA_MESSAGE_SET_BYTECODE:
		{
			uint16 start = bswap16(data[0]);
			uint16 length = dataLength - 1;
			uint16 i;
			#ifdef ASEBA_ASSERT
			if (start + length > vm->bytecodeSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS);
			#endif
			for (i = 0; i < length; i++)
				vm->bytecode[start+i] = bswap16(data[i+1]);
		}
		// There is no break here because we want to do a reset after a set bytecode
		
		case ASEBA_MESSAGE_RESET:
		vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
		// try to setup event, if it fails, return the execution state anyway
		if (AsebaVMSetupEvent(vm, ASEBA_EVENT_INIT) == 0)
			AsebaVMSendExecutionStateChanged(vm);
		break;
		
		case ASEBA_MESSAGE_RUN:
		AsebaMaskClear(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK);
		AsebaVMSendExecutionStateChanged(vm);
		if(AsebaVMRunCB)
			AsebaVMRunCB(vm);
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
			buffer[0] = bswap16(data[0]);
			buffer[1] = AsebaVMSetBreakpoint(vm, buffer[0]);
			AsebaSendMessageWords(vm, ASEBA_MESSAGE_BREAKPOINT_SET_RESULT, buffer, 2);
		}
		break;
		
		case ASEBA_MESSAGE_BREAKPOINT_CLEAR:
		AsebaVMClearBreakpoint(vm, bswap16(data[0]));
		break;
		
		case ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL:
		AsebaVMClearBreakpoints(vm);
		break;
		
		case ASEBA_MESSAGE_GET_VARIABLES:
		{
			uint16 start = bswap16(data[0]);
			uint16 length = bswap16(data[1]);
			#ifdef ASEBA_ASSERT
			if (start + length > vm->variablesSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS);
			#endif
			AsebaSendVariables(vm, start, length);
		}
		break;
		
		case ASEBA_MESSAGE_SET_VARIABLES:
		{
			uint16 start = bswap16(data[0]);
			uint16 length = dataLength - 1;
			uint16 i;
			#ifdef ASEBA_ASSERT
			if (start + length > vm->variablesSize)
				AsebaAssert(vm, ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS);
			#endif
			for (i = 0; i < length; i++)
				vm->variables[start+i] = bswap16(data[i+1]);
		}
		break;
		
		case ASEBA_MESSAGE_WRITE_BYTECODE:
		AsebaWriteBytecode(vm);
		break;
		
		case ASEBA_MESSAGE_REBOOT:
		AsebaResetIntoBootloader(vm);
		break;
		
		case ASEBA_MESSAGE_SUSPEND_TO_RAM:
		AsebaPutVmToSleep(vm);
		break;
		
		default:
		break;
	}
}

uint16 AsebaVMShouldDropPacket(AsebaVMState *vm, uint16 source, const uint8* data)
{
	uint16 type = bswap16(((const uint16*)data)[0]);
	if (type < 0x8000)
	{
		// user message
		return !AsebaVMGetEventAddress(vm, type);
	}
	else if (type >= 0xA000)
	{
		// debug message
		uint16 dest = bswap16(((const uint16*)data)[1]);
		if (type == ASEBA_MESSAGE_GET_DESCRIPTION)
			return 0;
		
		// check it is for us
		return dest != vm->nodeId;
	}
	return 1;
}	

/*@}*/
