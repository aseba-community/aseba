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
#include "vm/vm.h"
#include "common/consts.h"
#include "vm/natives.h"
#include "transport/buffer/vm-buffer.h"

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

/*** In your code, put "SET_EVENT(EVENT_NUMBER)" when you want to trigger an 
	 event. This macro is interrupt-safe, you can call it anywhere you want.
***/
#define SET_EVENT(event) do {events_flags |= 1 << event;} while(0)
#define CLEAR_EVENT(event) do {events_flags &= ~(1 << event);} while(0)
#define IS_EVENT(event) (events_flags & (1 << event))

/* VM */

/* The number of opcode an aseba script can have */
#define VM_BYTECODE_SIZE 1024
#define VM_STACK_SIZE 32

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

char name[] = "e-puck0";

AsebaVMDescription vmDescription = {
	name, 	// Name of the microcontroller
	{
		{ 1, "id" },	// Do not touch it
		{ 1, "source" }, // nor this one
		{ argsSize, "args" },	// neither this one
		{1, "leftSpeed"},
		{1, "rightSpeed"},
	// leds
		{8, "leds"},
	// prox
	    {8, "prox"},
		{8, "ambiant"},
	// acc
		{3, "acc"},
	// camera
		{1, "camLine"},
		{60, "camR"},
		{60, "camG"},
		{60, "camB"},
		
		{ 0, NULL }	// null terminated
	}
};

static uint16 vmBytecode[VM_BYTECODE_SIZE];

static sint16 vmStack[VM_STACK_SIZE];

static AsebaVMState vmState = {
	0x1,
	
	VM_BYTECODE_SIZE,
	vmBytecode,
	
	sizeof(ePuckVariables) / sizeof(sint16),
	(sint16*)&ePuckVariables,

	VM_STACK_SIZE,
	vmStack
};

const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	return &vmDescription;
}	




static unsigned int events_flags = 0;
enum Events
{
	EVENT_IR_SENSORS = 0,
	EVENT_CAMERA,
	EVENTS_COUNT
};

static const AsebaLocalEventDescription localEvents[] = { 
	{"ir_sensors", "New IR sensors values available"},
	{"camera", "New camera picture available"},
	{ NULL, NULL }
};

const AsebaLocalEventDescription * AsebaGetLocalEventsDescriptions(AsebaVMState *vm)
{
	return localEvents;
}

static const AsebaNativeFunctionDescription* nativeFunctionsDescription[] = {
	&AsebaNativeDescription_vecdot,
	&AsebaNativeDescription_vecstat,
	&AsebaNativeDescription_vecfill,
	&AsebaNativeDescription_veccopy,
	&AsebaNativeDescription_vecadd,
	&AsebaNativeDescription_vecsub,
	0	// null terminated
};

static AsebaNativeFunctionPointer nativeFunctions[] = {
	AsebaNative_vecdot,
	AsebaNative_vecstat,
	AsebaNative_vecfill,
	AsebaNative_veccopy,
	AsebaNative_vecadd,
	AsebaNative_vecsub,
};

void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	nativeFunctions[id](vm);
}

const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	return nativeFunctionsDescription;
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

void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	uartSendUInt16(length - 2);
	uartSendUInt16(vmState.nodeId);
	uint16 i;
	for (i = 0; i < length; i++)
		uartSendUInt8(*data++);
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

uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	BODY_LED = 1;
	uint16 ret = 0;
	if (e_ischar_uart1())
	{
		uint16 len = uartGetUInt16() + 2;
		*source = uartGetUInt16();
	
		uint16 i;
		for (i = 0; i < len; i++)
			*data++ = uartGetUInt8();
		ret = len;
	}
	BODY_LED = 0;
	return ret;
}	


extern int e_ambient_and_reflected_ir[8];
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
	
	
	if(e_ambient_and_reflected_ir[7] != 0xFFFF) {
		SET_EVENT(EVENT_IR_SENSORS); 
		// leds and prox
		for (i = 0; i < 8; i++)
		{
			e_set_led(i, ePuckVariables.leds[i]);
			ePuckVariables.ambiant[i] = e_get_ambient_light(i);
			ePuckVariables.prox[i] =  e_get_calibrated_prox(i);
		}
		e_ambient_and_reflected_ir[7] = 0xFFFF;
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
		if(camline != ePuckVariables.camLine)
		{
			camline = ePuckVariables.camLine;
			e_po3030k_config_cam(camline,0,4,480,4,8,RGB_565_MODE);
			e_po3030k_set_ref_exposure(160);
			e_po3030k_set_ww(0);
			e_po3030k_set_mirror(1,1);
			e_po3030k_write_cam_registers();
		}
		e_po3030k_launch_capture((char *)cam_data);
		SET_EVENT(EVENT_CAMERA);
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

void AsebaWriteBytecode(AsebaVMState *vm) {
	// TODO 
}
void AsebaResetIntoBootloader(AsebaVMState *vm) {
	asm __volatile__ ("reset");
}

void initAseba()
{
	// VM
	int selector = SELECTOR0 | (SELECTOR1 << 1) | (SELECTOR2 << 2);
	vmState.nodeId = selector + 1;
	AsebaVMInit(&vmState);
	ePuckVariables.id = selector + 1;
	ePuckVariables.camLine = 640/2-4;
	name[6] = '0' + selector;
	e_led_clear();
	e_set_led(selector,1);
}

int main()
{	
	int i;
	initRobot();

	do {
		initAseba();
	} while(!e_ischar_uart1());

	for (i = 0; i < 14; i++)
	{
		uartGetUInt8();
	}
	e_calibrate_ir();
	e_acc_calibr();
	while (1)
	{
		AsebaProcessIncomingEvents(&vmState);		
		updateRobotVariables();
		AsebaVMRun(&vmState, 1000);
		
		if (AsebaMaskIsClear(vmState.flags, ASEBA_VM_STEP_BY_STEP_MASK) || AsebaMaskIsClear(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			unsigned i;
			// Find first bit from right (LSB) 
			asm ("ff1r %[word], %[b]" : [b] "=r" (i) : [word] "r" (events_flags) : "cc");
			if(i)
			{
				i--;
				CLEAR_EVENT(i);
				ePuckVariables.source = vmState.nodeId;
				AsebaVMSetupEvent(&vmState, ASEBA_EVENT_LOCAL_EVENTS_START - i);
			}
		}
	}
	
	return 0;
}

