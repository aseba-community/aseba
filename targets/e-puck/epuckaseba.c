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

/*
	WARNING: you have to change the size of the UART 1 e-puck reception buffer to hold
	at the biggest possible packet (probably bytecode + header). Otherwise if you set
	a new bytecode while busy (for instance while sending description), you might end up
	in a dead-lock.
*/

#include <p30f6014a.h>

#include <e_epuck_ports.h>
#include <e_init_port.h>
#include <e_led.h>
#include <e_motors.h>
#include <e_agenda.h>
#include <e_po3030k.h>
#include <e_uart_char.h>
#include <e_ad_conv.h>
#include <e_prox.h>
#include <e_acc.h>


#include <string.h>

#define ASEBA_ASSERT
#include "vm.h"
#include "consts.h"
#include "natives.h"

#define vmBytecodeSize 1024
#define vmVariablesSize (sizeof(struct EPuckVariables) / sizeof(sint16))
#define vmStackSize 32
#define argsSize 32


unsigned int __attribute((far)) cam_data[60];



// we receive the data as big endian and read them as little, so we have to acces the bit the right way
#define CAM_RED(pixel) ( 3 * (((pixel) >> 3) & 0x1f))

#define CAM_GREEN(pixel) ((3 * ((((pixel) & 0x7) << 3) | ((pixel) >> 13))) >> 1)

#define CAM_BLUE(pixel) ( 3 * (((pixel) >> 8) & 0x1f))

#define CLAMP(v, vmin, vmax) ((v) < (vmin) ? (vmin) : (v) > (vmax) ? (vmax) : (v))

// data
struct EPuckVariables
{
	// NodeID
	sint16 id;
	// source
	sint16 source;
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
	// acc
	sint16 acc[3];
	// camera
	sint16 camLine;
	sint16 camR[60];
	sint16 camG[60];
	sint16 camB[60];
	// free space
	sint16 freeSpace[256];
} __attribute__ ((far)) ePuckVariables;
// physical bytecode isize is one word bigger than really used for byte code because this
// buffer is also used to receive the data
// the maximum packet size we have is the whole bytecode + the dest id
uint16 vmBytecode[vmBytecodeSize+1];
sint16 vmStack[vmStackSize];
AsebaVMState vmState;


#define nativeFunctionsCount 2
static AsebaNativeFunctionPointer nativeFunctions[nativeFunctionsCount] =
{
	AsebaNative_vecdot,
	AsebaNative_vecstat
};
static AsebaNativeFunctionDescription* nativeFunctionsDescriptions[nativeFunctionsCount] =
{
	&AsebaNativeDescription_vecdot,
	&AsebaNativeDescription_vecstat
};

void updateRobotVariables()
{
	unsigned i;
	static int camline = 640/2-4;
	// motor
	static int leftSpeed = 0, rightSpeed = 0;

	if (ePuckVariables.leftSpeed != leftSpeed)
	{
		leftSpeed = CLAMP(ePuckVariables.leftSpeed, -1000, 1000);
		e_set_speed_left(leftSpeed);
	}
	if (ePuckVariables.rightSpeed != rightSpeed)
	{
		rightSpeed = CLAMP(ePuckVariables.rightSpeed, -1000, 1000);
		e_set_speed_right(rightSpeed);
	}
	
	// leds and prox
	for (i = 0; i < 8; i++)
	{
		e_set_led(i, ePuckVariables.leds[i]);
		ePuckVariables.ambiant[i] = e_get_ambient_light(i);
		ePuckVariables.prox[i] =  e_get_calibrated_prox(i);
	}
	
	for(i = 0; i < 3; i++)
		ePuckVariables.acc[i] = e_get_acc(i);

	// camera
	if(e_po3030k_is_img_ready())
	{
		for(i = 0; i < 60; i++)
		{
			ePuckVariables.camR[i] = CAM_RED(cam_data[i]);
			ePuckVariables.camG[i] = CAM_GREEN(cam_data[i]);
			ePuckVariables.camB[i] = CAM_BLUE(cam_data[i]);
		}
		if(camline != ePuckVariables.camLine) {
			camline = ePuckVariables.camLine;
			e_po3030k_config_cam(camline,0,4,480,4,8,RGB_565_MODE);
			e_po3030k_set_ref_exposure(160);
			e_po3030k_set_ww(0);
			e_po3030k_set_mirror(1,1);
			e_po3030k_write_cam_registers();
		}
		e_po3030k_launch_capture((char *)cam_data);
	}
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
	e_start_agendas_processing();
	e_init_motors();
	e_init_uart1();
	e_init_ad_scan(0);
	e_po3030k_init_cam();
	e_po3030k_config_cam(640/2-4,0,4,480,4,8,RGB_565_MODE);
	e_po3030k_set_ref_exposure(160);
	e_po3030k_set_ww(0);
	e_po3030k_set_mirror(1,1);
	e_po3030k_write_cam_registers();
	e_po3030k_launch_capture((char *)cam_data);

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
	int selector = SELECTOR0 | (SELECTOR1 << 1) | (SELECTOR2 << 2);
	vmState.bytecodeSize = vmBytecodeSize;
	vmState.bytecode = vmBytecode;
	vmState.variablesSize = vmVariablesSize;
	vmState.variables = (sint16 *)&ePuckVariables;
	vmState.stackSize = vmStackSize;
	vmState.stack = vmStack;
	AsebaVMInit(&vmState, selector + 1);
	ePuckVariables.id = selector + 1;
	ePuckVariables.camLine = 640/2-4;
	e_led_clear();
	e_set_led(selector,1);
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

void uartSendNativeFunction(const AsebaNativeFunctionDescription* description)
{
	unsigned int i;
	uartSendString(description->name);
	uartSendString(description->doc);
	uartSendUInt16(description->argumentCount);
	for (i = 0; i < description->argumentCount; i++)
	{
		uartSendUInt16(description->arguments[i].size);
		uartSendString(description->arguments[i].name);
	}
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
	char name[] = "e-puck 0";
	unsigned int i;
	unsigned nativeFunctionSizes = 0;

	for (i = 0; i < nativeFunctionsCount; i++)
		nativeFunctionSizes += AsebaNativeFunctionGetDescriptionSize(nativeFunctionsDescriptions[i]);

	uartSendUInt16(126+nativeFunctionSizes);
	uartSendUInt16(vm->nodeId);
	uartSendUInt16(ASEBA_MESSAGE_DESCRIPTION);
	
	name[7] += vm->nodeId;
	uartSendString(name);				// 9
	
	uartSendUInt16(vmBytecodeSize);		// 2
	uartSendUInt16(vmStackSize);		// 2
	uartSendUInt16(vmVariablesSize);	// 2
	
	uartSendUInt16(13);					// 2

	uartSendUInt16(1);					// 2
	uartSendString("id");				// 3
	uartSendUInt16(1);					// 2
	uartSendString("source");			// 7
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
	uartSendUInt16(3);					// 2 		
	uartSendString("acc");				// 4
	uartSendUInt16(1);					// 2
	uartSendString("camLine");			// 8		
	uartSendUInt16(60);					// 2
	uartSendString("camR");				// 5
	uartSendUInt16(60);					// 2
	uartSendString("camG");				// 5
	uartSendUInt16(60);					// 2
	uartSendString("camB");				// 5
	
	uartSendUInt16(nativeFunctionsCount);	// 2

	for (i = 0; i < nativeFunctionsCount; i++)
		 uartSendNativeFunction(nativeFunctionsDescriptions[i]);
}

void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	nativeFunctions[id](vm);
}


void AsebaDebugHandleCommands()
{
	BODY_LED = 1;
	if (uartIsChar())
	{
		uint16 len = uartGetUInt16();
		uint16 source = uartGetUInt16();
		uint16 type = uartGetUInt16();
		uint8 recvDataBuffer[ASEBA_MAX_PACKET_SIZE];
		unsigned i = 0;
		
		for (i = 0; i < len; i++)
			recvDataBuffer[i] = uartGetUInt8();
		
		ePuckVariables.source = source;
		if (type < 0x8000)
		{
			// user message
			for (i = 0; (i < argsSize) && (i < len / 2); i++)
				ePuckVariables.args[i] = ((uint16*)recvDataBuffer)[i];
			AsebaVMSetupEvent(&vmState, type);
		}
		else
		{
			
			// debug message
			AsebaVMDebugMessage(&vmState, type, (uint16*)recvDataBuffer, len / 2);
		}
	}
	BODY_LED = 0;
}


int main()
{	
	int i;
	initRobot();
	

	do {
		initAseba();
	} while(!uartIsChar());

	for (i = 0; i < 14; i++)
	{
		uartGetUInt8();
	}
	e_calibrate_ir();
	e_acc_calibr();
	while (1)
	{
		updateRobotVariables();
		AsebaDebugHandleCommands();
		AsebaVMRun(&vmState, 1000);
		if (!AsebaVMIsExecutingThread(&vmState))
		{
			ePuckVariables.source = vmState.nodeId;
			AsebaVMSetupEvent(&vmState, ASEBA_EVENT_PERIODIC);
		}
	}
	
	return 0;
}

