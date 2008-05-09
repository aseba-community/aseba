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

#ifndef __ASEBA_CONSTS_H
#define __ASEBA_CONSTS_H

/**
	\defgroup common Constants and types common to all subsystems
*/
/*@{*/

/*! version of aseba protocol */
#define ASEBA_PROTOCOL_VERSION 1

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
	ASEBA_BYTECODE_CALL = 0xC,
	ASEBA_BYTECODE_BREAKPOINT = 0xD
} AsebaBytecodeId;

/*! List of binary operators */
typedef enum
{
	ASEBA_OP_SHIFT_LEFT = 0,
	ASEBA_OP_SHIFT_RIGHT = 1,
	ASEBA_OP_ADD = 2,
	ASEBA_OP_SUB = 3,
	ASEBA_OP_MULT = 4,
	ASEBA_OP_DIV = 5,
	ASEBA_OP_MOD = 6
} AsebaBinaryOperator;

/*! List of comparaisons */
typedef enum
{
	ASEBA_CMP_EQUAL = 0,
	ASEBA_CMP_NOT_EQUAL = 1,
	ASEBA_CMP_BIGGER_THAN = 2,
	ASEBA_CMP_BIGGER_EQUAL_THAN = 3,
	ASEBA_CMP_SMALLER_THAN = 4,
	ASEBA_CMP_SMALLER_EQUAL_THAN = 5
} AsebaComparaison;

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
	ASEBA_EVENT_PERIODIC = 0xFFFE,
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
	ASEBA_MESSAGE_DISCONNECTED,
	ASEBA_MESSAGE_VARIABLES,
	ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS,
	ASEBA_MESSAGE_DIVISION_BY_ZERO,
	ASEBA_MESSAGE_NODE_SPECIFIC_ERROR,
	ASEBA_MESSAGE_EXECUTION_STATE_CHANGED,
	ASEBA_MESSAGE_BREAKPOINT_SET_RESULT,
	
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
	ASEBA_MAX_PACKET_SIZE = 512+6	/*!< maximum size an aseba packet is allowed to be (in byte), including all its aseab headers */
} AsebaLimits;

/*@}*/

#endif
