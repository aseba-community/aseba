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

#include "hardware.h"
#include <transport/can/can-net.h>
#include <common/consts.h>
#include <can/can.h>
#include <dma/dma.h>
#include <gpio/gpio.h>
#include <uart/uart-software-fc.h>
#include <clock/clock.h>
#include <error/error.h>
#include <types/types.h>
#include <timer/timer.h>
#include <ei/ei.h>
#include <adc/adc.h>

#include "morse.h"


_FWDT(FWDTEN_OFF); 		// Watchdog Timer disabled
_FOSCSEL(FNOSC_FRCPLL);	// override oscillator configuration bits
_FOSC(POSCMD_NONE & OSCIOFNC_ON);
// ICD communicates on PGC1/EMUC1 and disable JTAG
_FICD(ICS_PGD1 & JTAGEN_OFF);

#define PRIO_CAN 5
#define PRIO_UART 5
#define PRIO_UART_TH 6

#define TIMER_UART TIMER_1

/* buffers for can-net */
#define SEND_QUEUE_SIZE 260
static  __attribute((far)) CanFrame sendQueue[SEND_QUEUE_SIZE];
#define RECV_QUEUE_SIZE 1100
static  __attribute((far)) CanFrame recvQueue[RECV_QUEUE_SIZE];

/* buffers for uart */
static __attribute((far)) __attribute((aligned(2))) unsigned char uartSendBuffer[ASEBA_MAX_OUTER_PACKET_SIZE];
static unsigned uartSendPos;
static unsigned uartSendSize;
static __attribute((far))  __attribute((aligned(2))) unsigned char uartRecvBuffer[ASEBA_MAX_OUTER_PACKET_SIZE];
static unsigned uartRecvPos;



volatile int global_flag = 0;

#define OUTPUT_ERROR(err)  do{ for(;;) output_error(err,LED0,0,1); }while(0)

static void can_tx_cb(void)
{
	if(global_flag)
		OUTPUT_ERROR(global_flag);
	global_flag = 1;

	AsebaCanFrameSent();
	
	// A message is waiting on the uart (it's complete but still waiting because we were full)
	// Check if we can send it now
	if ((uartRecvPos >= 6) && (uartRecvPos == 6 + ((uint16 *)uartRecvBuffer)[0])) 
	{
		uint16 source = ((uint16 *)uartRecvBuffer)[1];
		if (AsebaCanSendSpecificSource(uartRecvBuffer+4, uartRecvPos-4, source) == 1)
		{
			uartRecvPos = 0;
			uart_read_pending_data(U_UART);
		} 
	}
	global_flag = 0;
}

static void can_rx_cb(const can_frame* frame)
{
	if(global_flag)
		OUTPUT_ERROR(global_flag);
	global_flag = 2;
	
	// we do a nasty cast because the two data structure are compatible, i.e. they have the same leading members
	AsebaCanFrameReceived((CanFrame *) frame);
	
	uint16 source;
		
	if(uartSendPos == uartSendSize) 
	{
		int amount = AsebaCanRecv(uartSendBuffer+4, ASEBA_MAX_INNER_PACKET_SIZE, &source);
		if (amount > 0)
		{
			((uint16 *)uartSendBuffer)[0] = (uint16)amount - 2;
			((uint16 *)uartSendBuffer)[1] = source;
			uartSendSize = (unsigned)amount + 4;

			if(uart_transmit_byte(U_UART, uartSendBuffer[0]))
				uartSendPos = 1;
			else
				uartSendPos = 0;
		}
	}
	global_flag = 0;
	
}

static bool uart_rx_cb(int __attribute((unused)) uart_id, unsigned char data, void * __attribute((unused)) user_data)
{
	if(global_flag)
		OUTPUT_ERROR(global_flag);
	global_flag = 3;
	
	// TODO: in case of bug, we might want to test for overflow here, but it should never happen if higher layer conform to specification
	uartRecvBuffer[uartRecvPos++] = data;
	
	// if we have received the header and all the message payload data forward it to the can layer
	if ((uartRecvPos >= 6) && (uartRecvPos == 6 + ((uint16 *)uartRecvBuffer)[0])) 
	{
			
		uint16 source = ((uint16 *)uartRecvBuffer)[1];
		if (AsebaCanSendSpecificSource(uartRecvBuffer+4, uartRecvPos-4, source) == 1)
		{
			// Was able to queue frame to can layer. Setup the buffer pointer for next reception
			uartRecvPos = 0;
			global_flag = 0;
			return true;
		} else {
			// Stop reception, flow control will help us to not lose any packet
			global_flag = 0;
			return false;
		}
	}
	global_flag = 0;
	return true;
}

static bool uart_tx_cb(int __attribute((unused)) uart_id, unsigned char* data, void*  __attribute((unused)) user_data)
{
	if(global_flag)
		OUTPUT_ERROR(global_flag);
	global_flag = 4;
	
	if (uartSendPos < uartSendSize)
	{
		*data = uartSendBuffer[uartSendPos++];
		global_flag = 0;
		return true;
	}
	else
	{
		// We finished sending the previous packet
	
		// ...and if there is a new message on CAN, read it
		uint16 source;
			
		int amount = AsebaCanRecv(uartSendBuffer+4, ASEBA_MAX_INNER_PACKET_SIZE, &source);
		if (amount > 0)
		{
			((uint16 *)uartSendBuffer)[0] = (uint16)amount - 2;
			((uint16 *)uartSendBuffer)[1] = source;
			uartSendSize = (unsigned)amount + 4;
			uartSendPos = 1;
			*data = uartSendBuffer[0];
			global_flag = 0;
			return true;
		}
	
		global_flag = 0;
		return false;
	}
}

void AsebaIdle(void) {
	// Should NEVER be called
	OUTPUT_ERROR(5);
}

uint16 AsebaShouldDropPacket(uint16 source, const uint8* data) {
	return 0;
}	

static void received_packet_dropped(void)
{
	// light a LED, we have dropped an aseba message from the CAN because we were not able to send previous ones fast enough on UART
	gpio_write(LED1, 0);
}

static void sent_packet_dropped(void)
{
	// light a LED, the packet is in fact not dropped because UART has flow control and the translator will try again later
	gpio_write(LED1, 0);
}

void __attribute__((noreturn)) error_handler(const char __attribute((unused))  * file, int __attribute((unused)) line, int id, void* __attribute((unused)) arg) {
	OUTPUT_ERROR(id);	
}

/* initialization and main loop */
int main(void)
{
	int i;
			
	if(_POR) {
		_POR = 0;
		asm __volatile("reset");
	}
	
	clock_init_internal_rc_30();
	
	// Wait about 1 s before really starting. The Bluetooth chip need a LOT of time before 
	// having stable output (else we recieve "Phantom" character on uart)
	
	for(i = 0; i < 1000; i++) 
		clock_delay_us(1000);
	
	gpio_write(LED0, 1);
	gpio_write(LED1, 1);
	gpio_set_dir(LED0, GPIO_OUTPUT);
	gpio_set_dir(LED1, GPIO_OUTPUT);

	error_register_callback(error_handler);


	adc1_init_simple(NULL, 1, 0x0UL, 30);
	
	can_init(can_rx_cb, can_tx_cb, DMA_CHANNEL_0, DMA_CHANNEL_1, CAN_SELECT_MODE, 1000, PRIO_CAN);
	uart_init(U_UART, 921600, UART_CTS, UART_RTS, TIMER_UART, uart_rx_cb, uart_tx_cb, PRIO_UART_TH, PRIO_UART, NULL);

	AsebaCanInit(0, (AsebaCanSendFrameFP)can_send_frame, can_is_frame_room, received_packet_dropped, sent_packet_dropped, sendQueue, SEND_QUEUE_SIZE, recvQueue, RECV_QUEUE_SIZE);

	
	while (1)
	{
		// Sleep until something happens
		Idle();
	};
}

