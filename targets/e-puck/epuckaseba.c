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

#include <p30f6014a.h>

#include <e_epuck_ports.h>
#include <e_init_port.h>
#include <e_led.h>
#include <e_motors.h>
#include <e_uart_char.h>
#include <e_prox.h>
#include <e_accelerometer.h>

#include <string.h>

#define ASEBA_ASSERT
#include "../../vm/vm.h"
#include "../../common/consts.h"

#define vmBytecodeSize 256
#define vmVariablesSize (sizeof(struct EPuckVariables) / sizeof(sint16))
#define vmStackSize 32
#define argsSize 32

// data
struct EPuckVariables
{
	// args
	sint16 args[argsSize];
	// motor
	sint16 leftSpeed;
	sint16 rightSpeed;
	// leds
	sint16 leds[8];
	// prox
	sint16 prox[8];
	sint16 ambiant[8];
	// free space
	sint16 freeSpace[64];
} ePuckVariables;
uint16 vmBytecode[vmBytecodeSize];
sint16 vmStack[vmStackSize];
AsebaVMState vmState;


void updateRobotVariables()
{
	unsigned i;
	
	// motor
	static int leftSpeed = 0, rightSpeed = 0;
	if (ePuckVariables.leftSpeed != leftSpeed)
	{
		leftSpeed = ePuckVariables.leftSpeed;
		e_set_speed_left(leftSpeed);
	}
	if (ePuckVariables.rightSpeed != rightSpeed)
	{
		rightSpeed = ePuckVariables.rightSpeed;
		e_set_speed_right(rightSpeed);
	}
	
	// leds and prox
	for (i = 0; i < 8; i++)
	{
		e_set_led(i, ePuckVariables.leds[i]);
		ePuckVariables.ambiant[i] = e_get_ambient_light(i);
		ePuckVariables.prox[i] = e_get_prox(i);
	}
}

void d(const char *s)
{
	e_send_uart2_char(s, strlen(s));
}

void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	e_set_body_led(1);
	while (1);
}

void initRobot()
{
	// init stuff
	e_init_port();
	e_init_motors();
	e_init_uart1();
	e_init_uart2();
	e_init_prox();

	// reset if power on (some problem for few robots)
	if (RCONbits.POR)
	{
		RCONbits.POR = 0;
		RESET();
	}
}

void initAseba()
{
	// VM
	vmState.bytecodeSize = vmBytecodeSize;
	vmState.bytecode = vmBytecode;
	vmState.variablesSize = vmVariablesSize;
	vmState.variables = (sint16 *)&ePuckVariables;
	vmState.stackSize = vmStackSize;
	vmState.stack = vmStack;
	AsebaVMInit(&vmState, 1);
}

int uartIsChar()
{
	return e_ischar_uart1();
}

uint8 uartGetUInt8()
{
	char c;
	while (!e_ischar_uart1());
	e_getchar_uart1(&c);
	return c;
}

uint16 uartGetUInt16()
{
	uint16 value;
	// little endian
	value = uartGetUInt8();
	value |= (uartGetUInt8() << 8);
	return value;
}

void uartSendUInt8(uint8 value)
{
	e_send_uart1_char((char *)&value, 1);
	while (e_uart1_sending());
}

void uartSendUInt16(uint16 value)
{
	e_send_uart1_char((char *)&value, 2);
	while (e_uart1_sending());
}


void uartSendString(const char *s)
{
	unsigned len = strlen(s);
	uartSendUInt8(len);
	e_send_uart1_char(s, len);
	while (e_uart1_sending());
}

////

void AsebaSendMessage(AsebaVMState *vm, uint16 id, void *data, uint16 size)
{
	uartSendUInt16(size);
	uartSendUInt16(vm->nodeId);
	uartSendUInt16(id);
	unsigned i;
	for (i = 0; i < size; i++)
		uartSendUInt8(((unsigned char*)data)[i]);
}

void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length)
{
	uint16 i;
	uartSendUInt16(length*2+2);
	uartSendUInt16(vm->nodeId);
	uartSendUInt16(ASEBA_MESSAGE_VARIABLES);
	uartSendUInt16(start);
	for (i = start; i < start + length; i++)
		uartSendUInt16(vmState.variables[i]);
}

void AsebaSendDescription(AsebaVMState *vm)
{
	uartSendUInt16(73);
	uartSendUInt16(vm->nodeId);
	uartSendUInt16(ASEBA_MESSAGE_DESCRIPTION);
	
	uartSendString("e-puck");			// 7
	
	uartSendUInt16(vmBytecodeSize);		// 2
	uartSendUInt16(vmStackSize);		// 2
	uartSendUInt16(vmVariablesSize);	// 2
	
	uartSendUInt16(6);					// 2
	
	uartSendUInt16(argsSize);			// 2
	uartSendString("args");				// 5
	uartSendUInt16(1);					// 2
	uartSendString("leftSpeed");		// 10
	uartSendUInt16(1);					// 2
	uartSendString("rightSpeed");		// 11
	uartSendUInt16(8);					// 2
	uartSendString("leds");				// 5
	uartSendUInt16(8);					// 2
	uartSendString("prox");				// 5
	uartSendUInt16(8);					// 2
	uartSendString("ambiant");			// 8
	
	uartSendUInt16(0);					// 2
		// TODO : add native functions
}

void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	// TODO : add native functions
}


void AsebaDebugHandleCommands()
{
	if (uartIsChar())
	{
		uint16 len = uartGetUInt16();
		uint16 source = uartGetUInt16();
		uint16 type = uartGetUInt16();
		sint16* data;
		unsigned i = 0;
		
		// read data into the right place
		if (type == ASEBA_MESSAGE_SET_BYTECODE)
		{
			data = vmState.bytecode;
			while (i < vmBytecodeSize && i < len / 2)
			{
				data[i] = uartGetUInt16();
				i++;
			}
		}
		else
		{
			data = ePuckVariables.args;
			while (i < argsSize && i < len / 2)
			{
				data[i] = uartGetUInt16();
				i++;
			}
		}
		
		// drop excessive data
		while (i < len / 2)
		{
			uartGetUInt16();
			i++;
			// TODO: blink a led
		}
		
		if (type < 0x8000)
		{
			AsebaVMSetupEvent(&vmState, type);
		}
		else if (type >= 0xA000)
		{
			AsebaVMDebugMessage(&vmState, type, data, len);
		}
	}
}


int main()
{
	initRobot();
	initAseba();
	int i;
	LED5 = 1;
	for (i = 0; i < 14; i++)
	{
		uartGetUInt8();
	}
	LED5 = 0;
	d("epuck aseba debug\r\n");
	while (1)
	{
		updateRobotVariables();
		AsebaDebugHandleCommands();
		AsebaVMRun(&vmState, 100);
		if (!AsebaVMIsExecutingThread(&vmState))
			AsebaVMSetupEvent(&vmState, ASEBA_EVENT_PERIODIC);
	}
	
	return 0;
}

