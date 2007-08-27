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

#include "../../vm/vm.h"
#include "../../common/consts.h"

#define vmBytecodeSize 256
#define vmVariablesSize (sizeof(struct EPuckVariables) / sizeof(sint16))
#define vmStackSize 32

// data
struct EPuckVariables
{
	// motor
	sint16 leftSpeed;
	sint16 rightSpeed;
	// leds
	sint16 leds[8];
	sint16 bodyLed;
	sint16 frontLed;
	// prox
	sint16 prox[8];
	sint16 ambientLight[8];
	// free space
	sint16 freeSpace[32];
} ePuckVariables;
uint16 vmBytecode[vmBytecodeSize];
sint16 vmStack[vmStackSize];
AsebaVMState vmState;

void initRobotVariables()
{
	unsigned i;
	ePuckVariables.leftSpeed = ePuckVariables.rightSpeed = 0;
	for (i = 0; i < 8; i++)
		ePuckVariables.leds[i] = 0;
	ePuckVariables.bodyLed = ePuckVariables.frontLed = 0;
}

void updateRobotVariables()
{
	unsigned i;
	
	// motor
	e_set_speed_left(ePuckVariables.leftSpeed);
	e_set_speed_right(ePuckVariables.rightSpeed);
	
	// leds and prox
	for (i = 0; i < 8; i++)
	{
		e_set_led(i, ePuckVariables.leds[i]);
		ePuckVariables.prox[i] = e_get_prox(i);
		ePuckVariables.ambientLight[i] = e_get_ambient_light(i);
	}
	e_set_body_led(ePuckVariables.bodyLed);
	e_set_front_led(ePuckVariables.frontLed);
}

void d(const char *s)
{
	e_send_uart2_char(s, strlen(s));
}

void AssertFalse()
{
	e_set_body_led(1);
	while (1);
}

void initRobot()
{
	// init stuff
	e_init_port();
	e_init_motors();
	e_uart1_int_clr_addr = &vmState.flags;
	e_uart1_int_clr_mask = ~ASEBA_VM_EVENT_RUNNING_MASK;
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
	AsebaVMInit(&vmState);
	vmState.bytecodeSize = vmBytecodeSize;
	vmState.bytecode = vmBytecode;
	vmState.variablesSize = vmVariablesSize;
	vmState.variables = (sint16 *)&ePuckVariables;
	vmState.stackSize = vmStackSize;
	vmState.stack = vmStack;
	
	// no event
	vmState.bytecode[0] = 0;
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

void uartSendDescription()
{
	uartSendUInt16(vmBytecodeSize);
	uartSendUInt16(vmVariablesSize);
	uartSendUInt16(vmStackSize);
	// TODO : send name
	// variables
	uartSendString("leftSpeed");
	uartSendUInt16(1);
	uartSendString("rightSpeed");
	uartSendUInt16(1);
	uartSendString("leds");
	uartSendUInt16(8);
	uartSendString("bodyLed");
	uartSendUInt16(1);
	uartSendString("frontLed");
	uartSendUInt16(1);
	uartSendString("prox");
	uartSendUInt16(8);
	uartSendString("ambientLight");
	uartSendUInt16(8);
	uartSendUInt8(0);
	// TODO : add native functions
}

void uartSendVariablesMemory()
{
	uint16 i;
	for (i = 0; i < vmState.variablesSize; i++)
		uartSendUInt16(vmState.variables[i]);
}

void uartReceiveBytecode()
{
	uint16 len = uartGetUInt16();
	uint16 i;
	for (i = 0; i < len; i++)
		vmState.bytecode[i] = uartGetUInt16();
}


void AsebaDebugHandleCommands()
{
	if (uartIsChar())
	{
		uint8 c = uartGetUInt8();
		
		// TODO: fix connection/disconnection
		
		// 0 might arise from from LMX9820A
		if (c == 0)
			return;
		
		// messages from LMX9820A ?
		if (c == 2)
		{
			uint8 b[5];
			int i;
			for (i = 0; i < 5; i++)
				b[i] = uartGetUInt8();
			
			if (b[0] == 0x69 && b[1] == 0x0c && b[2] == 0x07 && b[3] == 0x00 && b[4] == 0x7c)
			{
				// connection notification
				for (i = 0; i < 8; i++)
					uartGetUInt8();
				AsebaMaskSet(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK);
				d("connection\r\n");
				return;
			}
			else if ((b[0] == 0x69 && b[1] == 0x11 && b[2] == 0x02 && b[3] == 0x00 && b[4] == 0x7c) ||
				(b[0] == 0x69 && b[1] == 0x0e && b[2] == 0x02 && b[3] == 0x00 && b[4] == 0x79))
			{
				// disconnection notification
				for (i = 0; i < 3; i++)
					uartGetUInt8();
				AsebaMaskClear(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK);
				d("disconnection\r\n");
				return;
			}
			else
				AssertFalse();
		}
		
		// aseba message
		{
			char db[3];
			d("got cmd ");
			db[0] = (c >> 4) + '0';
			db[1] = (c & 0xf) + '0';
			db[2] = 0;
			d(db);
			d("\r\n");
		}
		
		switch (c)
		{
			//case ASEBA_DEBUG_ATTACH: AsebaMaskSet(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK); break;
			//case ASEBA_DEBUG_DETACH: AsebaMaskClear(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK); break;
			
			case ASEBA_DEBUG_GET_DESCRIPTION: uartSendDescription(); break;
			case ASEBA_DEBUG_GET_VARIABLES: uartSendVariablesMemory(); break;
			
			case ASEBA_DEBUG_SET_BYTECODE: uartReceiveBytecode(); break;
			
			case ASEBA_DEBUG_GET_PC: uartSendUInt16(vmState.pc); break;
			
			case ASEBA_DEBUG_GET_FLAGS: uartSendUInt16(vmState.flags); break;
			
			case ASEBA_DEBUG_SETUP_EVENT:
			{
				AsebaVMSetupEvent(&vmState, uartGetUInt16());
				uartSendUInt16(vmState.flags);
				uartSendUInt16(vmState.pc);
			}
			break;
			
			case ASEBA_DEBUG_BACKGROUND_RUN: AsebaMaskSet(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK); break;
			case ASEBA_DEBUG_STOP: AsebaMaskClear(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK | ASEBA_VM_RUN_BACKGROUND_MASK);  break;
			
			case ASEBA_DEBUG_STEP:
			{
				if (AsebaVMStepBreakpoint(&vmState))
					uartSendUInt16(vmState.flags | ASEBA_VM_WAS_BREAKPOINT_MASK);
				else
					uartSendUInt16(vmState.flags);
				uartSendUInt16(vmState.pc);
			}
			break;
			
			case ASEBA_DEBUG_BREAKPOINT_SET: uartSendUInt8(AsebaVMSetBreakpoint(&vmState, uartGetUInt16())); break;
			case ASEBA_DEBUG_BREAKPOINT_CLEAR: uartSendUInt8(AsebaVMClearBreakpoint(&vmState, uartGetUInt16())); break;
			case ASEBA_DEBUG_BREAKPOINT_CLEAR_ALL: AsebaVMClearBreakpoints(&vmState); break;
			
			default: AssertFalse(); break;
		}
	}
}

// TODO: problem when async send from within code

void AsebaSendEvent(AsebaVMState *vm, uint16 event, uint16 start, uint16 length)
{
	if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK))
	{
		uartSendUInt8(ASEBA_DEBUG_PUSH_EVENT);
		uartSendVariablesMemory();
		uartSendUInt16(event);
		uartSendUInt16(start);
		uartSendUInt16(length);
	}
}

void AsebaArrayAccessOutOfBoundsException(AsebaVMState *vm, uint16 index)
{
	if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK))
	{
		uartSendUInt8(ASEBA_DEBUG_PUSH_ARRAY_ACCESS_OUT_OF_BOUND);
		uartSendVariablesMemory();
		uartSendUInt16(vmState.pc);
		uartSendUInt16(index);
	}
	AsebaMaskClear(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK);
}

void AsebaDivisionByZeroException(AsebaVMState *vm)
{
	if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK))
	{
		uartSendUInt8(ASEBA_DEBUG_PUSH_DIVISION_BY_ZERO);
		uartSendVariablesMemory();
		uartSendUInt16(vmState.pc);
	}
	AsebaMaskClear(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK);
}

void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	// TODO
}

// Run without support of breakpoints, only check on uart
void AsebaDebugBareRun(AsebaVMState *vm)
{
	AsebaMaskSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
	// no breakpoint, still poll the uart
	while (AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK) && AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK))
		AsebaVMStep(vm);
	AsebaMaskClear(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
}

// Run with support of breakpoints
void AsebaDebugBreakpointRun(AsebaVMState *vm)
{
	if (vmState.breakpointsCount > 0)
	{
		AsebaMaskSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
		// breakpoints, check at each step and poll the uart
		while (AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK) && AsebaMaskIsSet(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK))
		{
			if (AsebaVMStepBreakpoint(&vmState) != 0)
			{
				AsebaMaskClear(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK);
				uartSendUInt8(ASEBA_DEBUG_PUSH_STEP_BY_STEP);
				uartSendUInt16(vmState.pc);
				break;
			}
		}
		AsebaMaskClear(vm->flags, ASEBA_VM_EVENT_RUNNING_MASK);
	}
	else
	{
		// no breakpoint, just check uart and run
		AsebaDebugBareRun(vm);
	}
}


int main()
{
	initAseba();
	initRobotVariables();
	initRobot();
	d("epuck aseba debug\r\n");
	while (1)
	{
		AsebaDebugHandleCommands();
		if (AsebaMaskIsClear(vmState.flags, ASEBA_VM_DEBUGGER_ATTACHED_MASK))
		{
			// if nothing is running, setup a periodic event
			if (AsebaMaskIsClear(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
				AsebaVMSetupEvent(&vmState, ASEBA_EVENT_PERIODIC);
			// run without breakpoints
			AsebaDebugBareRun(&vmState);
		}
		else if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK))
		{
			// we have debugger but are in background run. Execute and report
			// if nothing is running, setup a periodic event
			if (AsebaMaskIsClear(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
				AsebaVMSetupEvent(&vmState, ASEBA_EVENT_PERIODIC);
			// run with breakpoints
			AsebaDebugBreakpointRun(&vmState);
			uartSendUInt8(ASEBA_DEBUG_PUSH_VARIABLES);
			uartSendVariablesMemory();
		}
		// we have debugger out of background run mode, do nothing
		updateRobotVariables();
	}
	
	return 0;
}

