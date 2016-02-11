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


// No multiple include protection since this file must ONLY BE INCLUDED FROM SKEL.h

// DMA Channels to use with CAN
#define DMA_CAN_RX DMA_CHANNEL_0
#define DMA_CAN_TX DMA_CHANNEL_1

// GPIO to use to set CAN speed
#define CAN_SELECT_MODE GPIO_MAKE_ID(GPIO_PORTD,11)

// CAN interrupt priority
#define PRIO_CAN 5


/* Send queue size may be smaller (64 is the smallest value) something like 70-80 is better
   128 is luxury */
#define SEND_QUEUE_SIZE 128

/* Warning, at least an aseba message MUST be able to fit into this RECV_QUEUE_SIZE buffer */
/* 256 is IMHO the minimum, but maybe it can be lowered with a lot of caution.
 * The bigger you have, the best it is. Fill the empty ram with it :)
 */
#define RECV_QUEUE_SIZE 756

/* This is the number of "private" variable the aseba script can have */
#define VM_VARIABLES_FREE_SPACE 256

/* This is the maximum number of argument an aseba event can recieve */
#define VM_VARIABLES_ARG_SIZE 32


/* The number of opcode an aseba script can have */
#define VM_BYTECODE_SIZE 766  // PUT HERE 766 + 768 * a, where a is >= 0
#define VM_STACK_SIZE 32



struct _vmVariables {
	// NodeID
	sint16 id;
	// source
	sint16 source;
		// args
	sint16 args[VM_VARIABLES_ARG_SIZE];

	/****
    ---> PUT YOUR VARIABLES HERE <---
    ****/


	// free space
	sint16 freeSpace[VM_VARIABLES_FREE_SPACE];
};


enum Events
{
	YOUR_FIRST_EVENT = 0,
	YOUR_SECOND_EVENT,
	/****
	---> PUT YOUR EVENT NUMBER HERE <---
	Must be in the same order as in skel.c
	****/
	EVENTS_COUNT // Do not touch
};

// The content of this structure is implementation-specific.
// The glue provide a way to store and retrive it from flash.
// The only way to write it is to do it from inside the VM (native function)
// The native function access it as a integer array. So, use only int inside this structure
struct private_settings {
	/* ADD here the settings to save into flash */
	/* The minimum size is one integer, the maximum size is 95 integer (Check done at compilation) */
	int first_setting;
	int second_setting;
};

