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

#ifndef _USB_BUFFER_H_
#define _USB_BUFFER_H_

#include "../../vm/vm.h"
#include "../../common/types.h"

/**
	\defgroup usb Transport layer over USB serial
	
	This transport layer only works on little-endian systems for now,
	as it does not perform endian correction.
*/
/*@{*/

/************** 
 * Interface USB-HW (Interrupt) <-> Usb buffer 
 **************/

/** The MTU of the underling hardware */
#define ASEBA_USB_MTU	64

/** callback from the usb layer asking for more data
 * put the datas in the pointer and return the size written to send more data
 * return 0 to send nothing
 */
unsigned char AsebaTxReady(unsigned char *data);

/** callback from the usb layer, data is a pointer to the data and size is the size .... 
 * Return true if the data where consumed, false if not.
 */
int AsebaUsbBulkRecv(unsigned char *data, unsigned char size);



/*************
 * Interface usb-buffer <-> Aseba VM (Main() code)
 *************/

void AsebaSendBuffer(AsebaVMState *vm, const uint8 * data, uint16 length);
uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source);


/*************
 * Init, and other random stuff 
 *************/
void AsebaUsbInit(unsigned char * sendQueue, size_t sendQueueSize, unsigned char * recvQueue, size_t recvQueueSize);
int AsebaUsbRecvBufferEmpty(void);
int AsebaUsbTxBusy(void);

/*@}*/

#endif
