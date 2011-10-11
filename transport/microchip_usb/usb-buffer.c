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

#include <usb/usb.h>
#include <usb/usb_device.h>
#include "usb_function_cdc.h"
#include "usb-buffer.h"
#include "usb_uart.h"

struct fifo {
	unsigned char * buffer;
	size_t size;
	size_t insert;
	size_t consume;
};

static struct {
	struct fifo rx;
	struct fifo tx;
} AsebaUsb;

/* Basic assumption in order to protect concurrent access to the fifos:
	- If the code in "main()" access the fifo it need to disable the usb interrupt 
*/

static inline size_t get_used(struct fifo * f) {
	size_t ipos, cpos;
	ipos = f->insert;
	cpos = f->consume;
	if (ipos >= cpos)
		return ipos - cpos;
	else
		return f->size - cpos + ipos;
}

static inline size_t get_free(struct fifo * f) {
	return f->size - get_used(f);
}

/* you MUST ensure that you pass correct size to thoses two function, 
 * no check are done .... 
 */
static inline void memcpy_out_fifo(unsigned char * dest, struct fifo * f, size_t size) {
	while(size--) {
		*dest++ = f->buffer[f->consume++];
		if(f->consume == f->size)
			f->consume = 0;
	}
}

static inline void memcpy_to_fifo(struct fifo * f, const unsigned char * src, size_t size) {
	while(size--) {
		f->buffer[f->insert++] = *src++;
		if(f->insert == f->size)
			f->insert = 0;
	}
}

static inline void fifo_peek(unsigned char * d, struct fifo * f,size_t size) {
	int ct = f->consume;
	while(size--) {
		*d++ = f->buffer[ct++];
		if(ct == f->size)
			ct = 0;
	}
}	

static inline void fifo_reset(struct fifo * f) {
	f->insert = f->consume = 0;
}

/* USB interrupt part */
// They can be called from the main, but with usb interrupt disabled, so it's OK
static int tx_busy;
static int debug;

unsigned char AsebaTxReady(unsigned char *data) {
	size_t size = get_used(&AsebaUsb.tx);
	
	if(size == 0) {
		tx_busy = 0;
		debug = 0;
		return 0;
	}
	
	if(size > ASEBA_USB_MTU)
		size = ASEBA_USB_MTU;
	
	memcpy_out_fifo(data, &AsebaUsb.tx, size);
	debug ++;
	return size;
}

int AsebaUsbBulkRecv(unsigned char *data, unsigned char size) {
	size_t free = get_free(&AsebaUsb.rx);
	
	if(size >= free)
		return 1;
	
	memcpy_to_fifo(&AsebaUsb.rx, data, size);
	
	return 0;
}

/* main() part */

void AsebaSendBuffer(AsebaVMState *vm, const uint8 *data, uint16 length) {
	int flags;
	// Here we must loop until we can send the data.
	// BUT if the usb connection is not available, we drop the packet
	if(!usb_uart_serial_port_open())
		return;
	
	// Sanity check, should never be true
	if (length < 2)
		return;
	
	do {
		USBMaskInterrupts(flags);
		if(get_free(&AsebaUsb.tx) > length + 4) {
			length -= 2;
			memcpy_to_fifo(&AsebaUsb.tx, (unsigned char *) &length, 2);
			memcpy_to_fifo(&AsebaUsb.tx, (unsigned char *) &vm->nodeId, 2);
			memcpy_to_fifo(&AsebaUsb.tx, (unsigned char *) data, length + 2);
			
			// Will callback AsebaUsbTxReady
			if (!tx_busy) {
				tx_busy = 1;
				USBCDCKickTx();
			}
			
			length = 0;
		}
		
	
		
		// Usb can be disconnected while sending ...
		if(!usb_uart_serial_port_open()) {
			fifo_reset(&AsebaUsb.tx);
			USBUnmaskInterrupts(flags);
			break;
		}		
		
		USBUnmaskInterrupts(flags);
	} while(length);
}

uint16 AsebaGetBuffer(AsebaVMState *vm, uint8 * data, uint16 maxLength, uint16* source) {
	int flags;
	uint16 ret = 0;
	size_t u;
	// Touching the FIFO, mask the interrupt ...
	USBMaskInterrupts(flags);

	u = get_used(&AsebaUsb.rx);

	/* Minium packet size == len + src + msg_type == 6 bytes */
	if(u >= 6) {
		int len;
		fifo_peek((unsigned char *) &len, &AsebaUsb.rx, 2);
		if (u >= len + 6) {
			memcpy_out_fifo((unsigned char *) &len, &AsebaUsb.rx, 2);
			memcpy_out_fifo((unsigned char *) source, &AsebaUsb.rx, 2);
			// msg_type is not in the len but is always present
			len = len + 2;
			/* Yay ! We have a complete packet ! */
			if(len > maxLength)
				len = maxLength;
			memcpy_out_fifo(data, &AsebaUsb.rx, len);
			ret = len;
		}
	}
	if(usb_uart_serial_port_open())
		USBCDCKickRx();
	else	
		fifo_reset(&AsebaUsb.rx);

	USBUnmaskInterrupts(flags);
	return ret;
}

void AsebaUsbInit(unsigned char * sendQueue, size_t sendQueueSize, unsigned char * recvQueue, size_t recvQueueSize) {
	AsebaUsb.tx.buffer = sendQueue;
	AsebaUsb.tx.size = sendQueueSize;
	
	AsebaUsb.rx.buffer = recvQueue;
	AsebaUsb.rx.size = recvQueueSize;
}

int AsebaUsbRecvBufferEmpty(void) {
	// We are called with interrupt disabled ! Check if rx contain something meaningfull
	
	int u;
	
	u = get_used(&AsebaUsb.rx);
	if(u > 6) {
		int len;
		fifo_peek((unsigned char *) &len, &AsebaUsb.rx, 2);
		if (u >= len + 6) 
			return 0;
	}
	return 1;
}

int AsebaUsbTxBusy(void) {
	return tx_busy;
}
