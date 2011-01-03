#ifndef _USB_BUFFER_H_
#define _USB_BUFFER_H_

#include "../../vm/vm.h"
#include "../../common/types.h"

/************** 
 * Interface USB-HW (Interrupt) <-> Usb buffer 
 **************/

/* The MTU of the underling hardware */
#define ASEBA_USB_MTU	64

/* callback from the usb layer asking for more data
 * put the datas in the pointer and return the size written to send more data
 * return 0 to send nothing
 */
unsigned char AsebaTxReady(unsigned char *data);

/* callback from the usb layer, data is a pointer to the data and size is the size .... 
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

#endif
