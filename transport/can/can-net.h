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

#ifndef ASEBA_CAN
#define ASEBA_CAN

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/types.h"

/**
	\defgroup can Transport layer over CAN bus
	
	This layer is able to transmit messages of arbitrary length
	(up to the available amount of memory) over a serie of 8 bytes
	CAN frames. If the message is under 8 bytes, the layer uses
	a single frame.
	
	The layer does not uses acknowledgment, instead it trusts the
	CAN checksum mechanism to ensure that other nodes on the CAN bus
	received the data correctly.
	
	This transport layer only works on little-endian systems for now,
	as it does not perform endian correction.
*/
/*@{*/

// Network

/*!	the data that physically go on the CAN bus. Used to communicate with the CAN data layer */
typedef struct
{
	uint8 data[8] __attribute__((aligned(sizeof(int)))); /*!< data payload */
	unsigned id:11; /*!< CAN identifier */
	unsigned len:4; /*!< amount of bytes used in data */
	unsigned used:1; /*!< when frame is in a circular buffer, tell if it frame is used */
} CanFrame;

/*! Pointer to a void function */
typedef void (*AsebaCanVoidVoidFP)(void);

/*! Pointer to a void function returning int*/
typedef int (*AsebaCanIntVoidFP)(void);

/*! Pointer to a function that sends a CAN frame (max 8 bytes) */
typedef void (*AsebaCanSendFrameFP)(const CanFrame *frame);

/*! Init the CAN connection.
	@param id the identifier of this node on the aseba CAN network
	@param sendFrameFP pointer to a function that sends CAN frames to the data layer
	@param isFrameRoomFP pointer to a function that returns if the data layer is ready to send frames
	@param receivedPacketDroppedFP pointer to a function that is called when a received packet has been dropped, for various reasons but mostly related to insufficient memory
	@param sentPacketDroppedFP pointer to a function that is called when a sent packet has been dropped (AsebaCanSend() returned 0), for various reasons but mostly related to insufficient memory
	@param sendQueue pointer to send queue data
	@param sendQueueSize number of frame in sendQueue
	@param recvQueue pointer to receive queue data
	@param recvQueueSize number of frame in recvQueue
*/
void AsebaCanInit(uint16 id, AsebaCanSendFrameFP sendFrameFP, AsebaCanIntVoidFP isFrameRoomFP, AsebaCanVoidVoidFP receivedPacketDroppedFP, AsebaCanVoidVoidFP sentPacketDroppedFP, CanFrame* sendQueue, size_t sendQueueSize, CanFrame* recvQueue, size_t recvQueueSize);

/*! Send data as an aseba packet.
	@param data pointer to the data to send
	@param size amount of data to send
	@return 1 on success, 0 on failure
*/
uint16 AsebaCanSend(const uint8 *data, size_t size);

/*! Send data as an aseba packet.
	@param data pointer to the data to send
	@param size amount of data to send
	@param source identifier to use as source
	@return 1 on success, 0 on failure
*/
uint16 AsebaCanSendSpecificSource(const uint8 *data, size_t size, uint16 source);

/*! Copy data from a received packet to the caller-provided buffer.
	Remove the packet from the reception queue afterwards.
	@param data pointer where to copy the data
	@param size makimum number of byte to copy. Actual number of byte might be smaller if packet was smaller. If packet was bigger, the rest of the data are dropped.
	@param source the identifier of the source of the received packet
	@return the amount of data copied. 0 if no data were available.
*/
uint16 AsebaCanRecv(uint8 *data, size_t size, uint16 *source);

/*! Wait until the send queue is empty
*/
void AsebaCanFlushQueue(void);

/*! Free everything in the Rx queue. Warning, this is a low-level function which should 
	only be called if the underlaying CAN driver is disabled. */
void AsebaCanRecvFreeQueue(void);

// to be implemented by the glue 

/*! Busy wait until the can buffer has room. At worst, can be an empty function */
void AsebaIdle(void);

/*! Return true if VM will ignore the packet, false otherwise */
uint16 AsebaShouldDropPacket(uint16 source, const uint8* data);

// to be called by data layer on interrupts

/*! Data layer should call this function when a new CAN frame (max 8 bytes) is available */
void AsebaCanFrameReceived(const CanFrame *frame);

/*! Data layer should call this function when the CAN frame (max 8 bytes) was sent successfully */
void AsebaCanFrameSent(void);

/*! Return true if the recv buffer is empty, false otherwise */
uint16 AsebaCanRecvBufferEmpty(void);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
