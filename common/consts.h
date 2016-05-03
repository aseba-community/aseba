/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
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

#ifndef __ASEBA_CONSTS_H
#define __ASEBA_CONSTS_H

/**
	\defgroup common Constants and types common to all subsystems
*/
/*@{*/

/*! version of Aseba as string */
#define ASEBA_VERSION "1.5.3"

/*! version of Aseba as an int */
#define ASEBA_VERSION_INT 10503

/*! version of aseba protocol, including bytecodes types and constants */
#define ASEBA_PROTOCOL_VERSION 5

/*! minimal accepted protocol version in targets */
#define ASEBA_MIN_TARGET_PROTOCOL_VERSION 4

/*! default listen target for aseba */
#define ASEBA_DEFAULT_LISTEN_TARGET "tcpin:33333"

/*! default target for aseba */
#define ASEBA_DEFAULT_TARGET "tcp:localhost;33333"

/*! default host for aseba */
#define ASEBA_DEFAULT_HOST "localhost"

/*! default port for aseba */
#define ASEBA_DEFAULT_PORT 33333


/*! List of bytecodes identifiers */
typedef enum
{
	ASEBA_BYTECODE_STOP = 0x0,
	ASEBA_BYTECODE_SMALL_IMMEDIATE = 0x1,
	ASEBA_BYTECODE_LARGE_IMMEDIATE = 0x2,
	ASEBA_BYTECODE_LOAD = 0x3,
	ASEBA_BYTECODE_STORE = 0x4,
	ASEBA_BYTECODE_LOAD_INDIRECT = 0x5,
	ASEBA_BYTECODE_STORE_INDIRECT = 0x6,
	ASEBA_BYTECODE_UNARY_ARITHMETIC = 0x7,
	ASEBA_BYTECODE_BINARY_ARITHMETIC = 0x8,
	ASEBA_BYTECODE_JUMP = 0x9,
	ASEBA_BYTECODE_CONDITIONAL_BRANCH = 0xA,
	ASEBA_BYTECODE_EMIT = 0xB,
	ASEBA_BYTECODE_NATIVE_CALL = 0xC,
	ASEBA_BYTECODE_SUB_CALL = 0xD,
	ASEBA_BYTECODE_SUB_RET = 0xE
} AsebaBytecodeId;

/*! List of binary operators */
typedef enum
{
	// arithmetic
	ASEBA_OP_SHIFT_LEFT = 0x0,
	ASEBA_OP_SHIFT_RIGHT,
	ASEBA_OP_ADD,
	ASEBA_OP_SUB,
	ASEBA_OP_MULT,
	ASEBA_OP_DIV,
	ASEBA_OP_MOD,
	// binary arithmetic
	ASEBA_OP_BIT_OR,
	ASEBA_OP_BIT_XOR,
	ASEBA_OP_BIT_AND,
	// comparison
	ASEBA_OP_EQUAL,
	ASEBA_OP_NOT_EQUAL,
	ASEBA_OP_BIGGER_THAN,
	ASEBA_OP_BIGGER_EQUAL_THAN,
	ASEBA_OP_SMALLER_THAN,
	ASEBA_OP_SMALLER_EQUAL_THAN,
	// logic
	ASEBA_OP_OR,
	ASEBA_OP_AND
} AsebaBinaryOperator;

/*! Mask of available binary operators */
#define ASEBA_BINARY_OPERATOR_MASK 0xff

/*!  List of unary operators */
typedef enum
{
	ASEBA_UNARY_OP_SUB = 0x0,
	ASEBA_UNARY_OP_ABS,
	ASEBA_UNARY_OP_BIT_NOT,
	ASEBA_UNARY_OP_NOT	// < not used for the VM, only for parsing, reduced by compiler using de Morgan and comparison inversion
} AsebaUnaryOperator;

/*! Mask of available unary operators */
#define ASEBA_UNARY_OPERATOR_MASK 0xff

/*! Bit inside if opcode that indicates it is a when condition */
#define ASEBA_IF_IS_WHEN_BIT 8
/*! Bit inside if opcode that indicates that the last evaluation was true */
#define ASEBA_IF_WAS_TRUE_BIT 9

/*! List of masks for flags in AsebaVMState */
typedef enum
{
	/*! This flag is enabled when a thread is being executed. */
	ASEBA_VM_EVENT_ACTIVE_MASK = 0x1,
	/*! This flag is enabled when the VM is running stey by step. It is disabled when running normally. */
	ASEBA_VM_STEP_BY_STEP_MASK = 0x2,
	/*! This flag is enabled when an event is running inside the debugger's fast loop, and cleared to make the debugger get out of the loop. This is usefull to allow an interrupt to stop the VM. */
	ASEBA_VM_EVENT_RUNNING_MASK = 0x4
} AsebaExecutionStates;

/*! List of special event ID */
typedef enum
{
	ASEBA_EVENT_INIT = 0xFFFF,
	ASEBA_EVENT_LOCAL_EVENTS_START = 0xFFFE,
} AsebaSpecialEventId;

/*! Return a bare bytecode from its identifier */
#define AsebaBytecodeFromId(id) ((id) << 12)

/*! Identifiers for remote bootloader and debug protocol */
typedef enum
{
	/* from bootloader control program to a specific node */
	ASEBA_MESSAGE_BOOTLOADER_RESET = 0x8000,
	ASEBA_MESSAGE_BOOTLOADER_READ_PAGE,
	ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE,
	ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE,
	
	/* from node to bootloader control program */
	ASEBA_MESSAGE_BOOTLOADER_DESCRIPTION,
	ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_READ,
	ASEBA_MESSAGE_BOOTLOADER_ACK,
	
	/* from a specific node */
	ASEBA_MESSAGE_DESCRIPTION = 0x9000,
	ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION,
	ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION,
	ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION,
	ASEBA_MESSAGE_DISCONNECTED,
	ASEBA_MESSAGE_VARIABLES,
	ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS,
	ASEBA_MESSAGE_DIVISION_BY_ZERO,
	ASEBA_MESSAGE_EVENT_EXECUTION_KILLED,
	ASEBA_MESSAGE_NODE_SPECIFIC_ERROR,
	ASEBA_MESSAGE_EXECUTION_STATE_CHANGED,
	ASEBA_MESSAGE_BREAKPOINT_SET_RESULT,
	ASEBA_MESSAGE_NODE_PRESENT,
	
	/* from IDE to all nodes */
	ASEBA_MESSAGE_GET_DESCRIPTION = 0xA000,
	
	/* from IDE to a specific node */
	ASEBA_MESSAGE_SET_BYTECODE,
	ASEBA_MESSAGE_RESET,
	ASEBA_MESSAGE_RUN,
	ASEBA_MESSAGE_PAUSE,
	ASEBA_MESSAGE_STEP,
	ASEBA_MESSAGE_STOP,
	ASEBA_MESSAGE_GET_EXECUTION_STATE,
	ASEBA_MESSAGE_BREAKPOINT_SET,
	ASEBA_MESSAGE_BREAKPOINT_CLEAR,
	ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL,
	ASEBA_MESSAGE_GET_VARIABLES,
	ASEBA_MESSAGE_SET_VARIABLES,
	ASEBA_MESSAGE_WRITE_BYTECODE,
	ASEBA_MESSAGE_REBOOT,
	ASEBA_MESSAGE_SUSPEND_TO_RAM,
	ASEBA_MESSAGE_GET_NODE_DESCRIPTION,
	
	/* from IDE to all nodes, here because it was added later */
	ASEBA_MESSAGE_LIST_NODES,
	
	ASEBA_MESSAGE_INVALID = 0xFFFF
} AsebaSystemMessagesTypes;

/*! Identifiers for destinations */
typedef enum
{
	ASEBA_DEST_DEBUG = 0,
	ASEBA_DEST_INVALID = 0xFFFF
} AsebaMessagesDests;

/*! Limits for static buffers allocation */
typedef enum
{
	ASEBA_MAX_EVENT_ARG_COUNT = 258,	/*!< Maximum number of arguments in an event (in word) */
} AsebaLimits;

/*! Maximum size of arguments in a user-defined event (in bytes) */
#define ASEBA_MAX_EVENT_ARG_SIZE (ASEBA_MAX_EVENT_ARG_COUNT*2)

/*! Maximum size of an event (in bytes), including its type, but not the source and the length */
#define ASEBA_MAX_INNER_PACKET_SIZE (ASEBA_MAX_EVENT_ARG_SIZE+2)

/*! Maximum size of an event (in bytes), including its type plus the source and the length */
#define ASEBA_MAX_OUTER_PACKET_SIZE (ASEBA_MAX_INNER_PACKET_SIZE+4) 

/*! *DEPRECATED*, please use one of the more explicit defines above */
#define ASEBA_MAX_PACKET_SIZE ASEBA_MAX_INNER_PACKET_SIZE

/*! Macro to prevent compiler warnings for unused variables */
#define ASEBA_UNUSED(x) (void)x;

/*@}*/

#endif
