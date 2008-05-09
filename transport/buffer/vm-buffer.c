/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		Laboratory of Robotics Systems, Mobots group, EPFL, Lausanne
		http://mobots.epfl.ch
	
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

#include "vm-buffer.h"
#include "../../common/consts.h"
#include <string.h>

static unsigned char buffer[ASEBA_MAX_PACKET_SIZE];
static unsigned buffer_pos;

static void buffer_add(const uint8* data, uint16 len)
{
	uint16 i = 0;
	while (i < len)
		buffer[buffer_pos++] = data[i++];
}

static void buffer_add_uint8(uint8 value)
{
	buffer_add(&value, 1);
}

static void buffer_add_uint16(uint16 value)
{
	buffer_add((unsigned char *) &value, 2);
}

static void buffer_add_string(const char* s)
{
	uint16 len = strlen(s);
	buffer_add_uint8(len);
	while (*s)
		buffer_add_uint8(*s++);
}

/* implementation of vm hooks */

void AsebaSendMessage(AsebaVMState *vm, uint16 type, void *data, uint16 size)
{
	uint16 i;

	buffer_pos = 0;
	buffer_add_uint16(type);
	for (i = 0; i < size; i++)
		buffer_add_uint8(((unsigned char*)data)[i]);

	AsebaSendBuffer(vm, buffer, buffer_pos);
}


void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length)
{
	uint16 i;

	buffer_pos = 0;
	buffer_add_uint16(ASEBA_MESSAGE_VARIABLES);
	buffer_add_uint16(start);
	for (i = start; i < start + length; i++)
		buffer_add_uint16(vm->variables[i]);

	AsebaSendBuffer(vm, buffer, buffer_pos);
}

void AsebaSendDescription(AsebaVMState *vm)
{
	const AsebaVMDescription *vmDescription = AsebaGetVMDescription(vm);
	const AsebaNativeFunctionDescription** nativeFunctionsDescription = AsebaGetNativeFunctionsDescriptions(vm);
	
	uint16 i = 0;
	buffer_pos = 0;
	
	buffer_add_uint16(ASEBA_MESSAGE_DESCRIPTION);

	buffer_add_string(vmDescription->name);
	
	buffer_add_uint16(1);

	buffer_add_uint16(vm->bytecodeSize);
	buffer_add_uint16(vm->stackSize);
	buffer_add_uint16(vm->variablesSize);

	// compute the number of variables descriptions
	for (i = 0; vmDescription->variables[i].size; i++)
		;
	buffer_add_uint16(i);
	// send variables description
	for (i = 0; vmDescription->variables[i].size; i++)
	{
		buffer_add_uint16(vmDescription->variables[i].size);
		buffer_add_string(vmDescription->variables[i].name);
	}

	// compute the number of native functions
	for (i = 0; nativeFunctionsDescription[i]; i++)
		;
	buffer_add_uint16(i);
	// send native functions description
	for (i = 0; nativeFunctionsDescription[i]; i++)
	{
		uint16 j;
		buffer_add_string(nativeFunctionsDescription[i]->name);
		buffer_add_string(nativeFunctionsDescription[i]->doc);
		for (j = 0; j < nativeFunctionsDescription[i]->arguments[j].size; j++)
			;
		buffer_add_uint16(j);
		for (j = 0; j < nativeFunctionsDescription[i]->arguments[j].size; j++)
		{
			buffer_add_uint16(nativeFunctionsDescription[i]->arguments[j].size);
			buffer_add_string(nativeFunctionsDescription[i]->arguments[j].name);
		}
	}

	AsebaSendBuffer(vm, buffer, buffer_pos);
}

void AsebaDebugHandleCommands(AsebaVMState *vm)
{
	uint16 source;
	const AsebaVMDescription *desc = AsebaGetVMDescription(vm);
	
	uint16 amount = AsebaGetBuffer(vm, buffer, ASEBA_MAX_PACKET_SIZE, &source);

	if (amount > 0)
	{
		uint16 type = ((uint16*)buffer)[0];
		uint16* payload = (uint16*)(buffer+2);
		uint16 payloadSize = (amount-2)/2;
		if (type < 0x8000)
		{
			// user message
			// by convention. the source begin at variables, address 1
			// then it's followed by the args
			uint16 argPos = desc->variables[1].size;
			uint16 argsSize = desc->variables[2].size;
			uint16 i;
			vm->variables[argPos++] = source;
			for (i = 0; (i < argsSize) && (i < payloadSize); i++)
				vm->variables[argPos + i] = payload[i];
			AsebaVMSetupEvent(vm, type);
		}
		else
		{
			// debug message
			AsebaVMDebugMessage(vm, type, payload, payloadSize);
		}
	}
}

