#include "can-net.h"
#include <stdlib.h>
#include <string.h>

#define TYPE_SMALL_PACKET 0x3
#define TYPE_PACKET_NORMAL 0x0
#define TYPE_PACKET_START 0x1
#define TYPE_PACKET_STOP 0x2

#define CANID_TO_TYPE(canid) ((canid) >> 8)
#define CANID_TO_ID(canid) ((canid) & 0xff)
#define TO_CANID(type, id) (((type) << 8) | (id))

#define ASEBA_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define barrier() __asm__ __volatile__("": : :"memory")

/*!	This contains the state of the CAN implementation of Aseba network */
static struct AsebaCan
{
	// data
	uint16 id; /*!< identifier of this node on CAN */
	
	// pointer to physical layer functions
	AsebaCanSendFrameFP sendFrameFP;
	AsebaCanIntVoidFP isFrameRoomFP;
	AsebaCanVoidVoidFP receivedPacketDroppedFP;
	AsebaCanVoidVoidFP sentPacketDroppedFP;
	
	// send buffer
	CanFrame* sendQueue;
	size_t sendQueueSize;
	uint16 sendQueueInsertPos;
	uint16 sendQueueConsumePos;
	
	// reception buffer
	CanFrame* recvQueue;
	size_t recvQueueSize;
	uint16 recvQueueInsertPos;
	uint16 recvQueueConsumePos;

	uint16 insideSendQueueToPhysicalLayer;
	
} asebaCan;

/** \addtogroup can */
/*@{*/

/*! Returned the minimum number of multiple of height to fit v */
uint16 AsebaCanGetMinMultipleOfHeight(uint16 v)
{
	uint16 m = v >> 3;
	if ((v & 0x7) != 0)
		m++;
	return m;
}

/*! Returned the number of used frames in the send queue*/
static uint16 AsebaCanSendQueueGetUsedFrames()
{
	uint16 ipos, cpos;
	ipos = asebaCan.sendQueueInsertPos;
	cpos =  asebaCan.sendQueueConsumePos;
	
	if (ipos >= cpos)
		return ipos - cpos;
	else
		return asebaCan.sendQueueSize - cpos + ipos;
}

/*! Returned the number of free frames in the send queue */
static uint16 AsebaCanSendQueueGetFreeFrames()
{
	return asebaCan.sendQueueSize - AsebaCanSendQueueGetUsedFrames();
}

/*! Insert a frame in the send queue, do not check for overwrite */
static void AsebaCanSendQueueInsert(uint16 canid, const uint8 *data, size_t size)
{
	uint16 temp;
	memcpy(asebaCan.sendQueue[asebaCan.sendQueueInsertPos].data, data, size);
	asebaCan.sendQueue[asebaCan.sendQueueInsertPos].id = canid;
	asebaCan.sendQueue[asebaCan.sendQueueInsertPos].len = size;

	
	temp = asebaCan.sendQueueInsertPos + 1;
	if (temp >= asebaCan.sendQueueSize)
		temp = 0;
	asebaCan.sendQueueInsertPos = temp;
}

/*! Send frames in send queue to physical layer until it is full */
static void AsebaCanSendQueueToPhysicalLayer()
{
	uint16 temp;
	asebaCan.insideSendQueueToPhysicalLayer = 1;
	barrier();
	while (asebaCan.isFrameRoomFP() && (asebaCan.sendQueueConsumePos != asebaCan.sendQueueInsertPos))
	{
		asebaCan.sendFrameFP(asebaCan.sendQueue + asebaCan.sendQueueConsumePos);
		
		temp = asebaCan.sendQueueConsumePos + 1;
		if (temp >= asebaCan.sendQueueSize)
			temp = 0;
		asebaCan.sendQueueConsumePos = temp; 
	}
	barrier();
	asebaCan.insideSendQueueToPhysicalLayer = 0;
}

/*! Returned the maximum number of used frames in the reception queue*/
static uint16 AsebaCanRecvQueueGetMaxUsedFrames()
{
	uint16 ipos, cpos;
	ipos = asebaCan.recvQueueInsertPos;
	cpos = asebaCan.recvQueueConsumePos;
	if (ipos >= cpos)
		return ipos - cpos;
	else
		return asebaCan.recvQueueSize - cpos + ipos;
}

/*! Returned the minimum number of free frames in the reception queue */
static uint16 AsebaCanRecvQueueGetMinFreeFrames()
{
	return asebaCan.recvQueueSize - AsebaCanRecvQueueGetMaxUsedFrames();
}

/*! Free frames associated with an id */
static void AsebaCanRecvQueueGarbageCollect()
{
	uint16 temp;
	while (asebaCan.recvQueueConsumePos != asebaCan.recvQueueInsertPos)
	{
		if (asebaCan.recvQueue[asebaCan.recvQueueConsumePos].used)
			break;
		
		temp = asebaCan.recvQueueConsumePos + 1;
		if (temp >= asebaCan.recvQueueSize)
			temp = 0;
		asebaCan.recvQueueConsumePos = temp;
	}
}

/*! Free frames associated with an id in the reception queue */
static void AsebaCanRecvQueueFreeFrames(uint16 id)
{
	uint16 i;
	
	// for all used frames...
	for (i = asebaCan.recvQueueConsumePos; i != asebaCan.recvQueueInsertPos;)
	{
		// if frame is of a specific id, mark frame as unused
		if (CANID_TO_ID(asebaCan.recvQueue[i].id) == id)
			asebaCan.recvQueue[i].used = 0;
		
		i++;
		if (i >= asebaCan.recvQueueSize)
			i = 0;
	}
}


void AsebaCanInit(uint16 id, AsebaCanSendFrameFP sendFrameFP, AsebaCanIntVoidFP isFrameRoomFP, AsebaCanVoidVoidFP receivedPacketDroppedFP, AsebaCanVoidVoidFP sentPacketDroppedFP, CanFrame* sendQueue, size_t sendQueueSize, CanFrame* recvQueue, size_t recvQueueSize)
{
	asebaCan.id = id;
	
	asebaCan.sendFrameFP = sendFrameFP;
	asebaCan.isFrameRoomFP= isFrameRoomFP;
	asebaCan.receivedPacketDroppedFP = receivedPacketDroppedFP;
	asebaCan.sentPacketDroppedFP = sentPacketDroppedFP;
	
	asebaCan.sendQueue = sendQueue;
	asebaCan.sendQueueSize = sendQueueSize;
	asebaCan.sendQueueInsertPos = 0;
	asebaCan.sendQueueConsumePos = 0;
	
	asebaCan.recvQueue = recvQueue;
	asebaCan.recvQueueSize = recvQueueSize;
	asebaCan.recvQueueInsertPos = 0;
	asebaCan.recvQueueConsumePos = 0;
}

int AsebaCanSend(const uint8 *data, size_t size)
{
	return AsebaCanSendSpecificSource(data, size, asebaCan.id);
}

int AsebaCanSendSpecificSource(const uint8 *data, size_t size, uint16 source)
{
	// send everything we can to maximize the space in the buffer
	AsebaCanSendQueueToPhysicalLayer();
	
	// check free space
	uint16 frameRequired = AsebaCanGetMinMultipleOfHeight(size);
	if (AsebaCanSendQueueGetFreeFrames() <= frameRequired)
	{
		asebaCan.sentPacketDroppedFP();
		return 0;
	}
		
	// insert
	if (size <= 8)
	{
		AsebaCanSendQueueInsert(TO_CANID(TYPE_SMALL_PACKET, source), data, size);
	}
	else
	{
		size_t pos = 8;
		
		AsebaCanSendQueueInsert(TO_CANID(TYPE_PACKET_START, source), data, 8);
		while (pos + 8 < size)
		{
			AsebaCanSendQueueInsert(TO_CANID(TYPE_PACKET_NORMAL, source), data + pos, 8);
			pos += 8;
		}
		AsebaCanSendQueueInsert(TO_CANID(TYPE_PACKET_STOP, source), data + pos, size - pos);
	}
	
	// send everything we can to minimize the transmission delay
	AsebaCanSendQueueToPhysicalLayer();
	
	return 1;
}

void AsebaCanFrameSent()
{
	// send everything we can if we are currently not sending
	if (!asebaCan.insideSendQueueToPhysicalLayer)
		AsebaCanSendQueueToPhysicalLayer();
}

int AsebaCanRecv(uint8 *data, size_t size, uint16 *source)
{
	int stopPos = -1;
	uint16 stopId;
	uint16 pos = 0;
	uint16 i;
	
	// first scan for "short packets" and "stops"
	for (i = asebaCan.recvQueueConsumePos; i != asebaCan.recvQueueInsertPos; )
	{
		if(asebaCan.recvQueue[i].used) {
			
			if (CANID_TO_TYPE(asebaCan.recvQueue[i].id) == TYPE_SMALL_PACKET)
			{
				// copy data
				uint16 len = ASEBA_MIN(size, asebaCan.recvQueue[i].len);
				memcpy(data, asebaCan.recvQueue[i].data, len);
				*source = CANID_TO_ID(asebaCan.recvQueue[i].id);
				asebaCan.recvQueue[i].used = 0;
				
				// garbage collect
				AsebaCanRecvQueueGarbageCollect();
				
				return len;
			}
			// stop found, we thus must have seen start before
			else if (CANID_TO_TYPE(asebaCan.recvQueue[i].id) == TYPE_PACKET_STOP)
			{
				stopPos = i;
				stopId = CANID_TO_ID(asebaCan.recvQueue[i].id);
				break;
			}
		}
		i++;
		if (i >= asebaCan.recvQueueSize)
			i = 0;
	}
	
	// if we haven't found anything
	if (stopPos < 0)
		return 0;
	
	// collect data
	*source = CANID_TO_ID(stopId);
	i = asebaCan.recvQueueConsumePos;
	while (1)
	{
		if(asebaCan.recvQueue[i].used) {

			if (CANID_TO_ID(asebaCan.recvQueue[i].id) == stopId)
			{
			if (pos < size)
					{
					uint16 amount = ASEBA_MIN(asebaCan.recvQueue[i].len, size - pos);
					memcpy(data + pos, asebaCan.recvQueue[i].data, amount);
					pos += amount;
				}
				asebaCan.recvQueue[i].used = 0;
			}
			
			if (i == stopPos)
				break;
	
		}
		i++;
		if (i >= asebaCan.recvQueueSize)
			i = 0;
	}

	// garbage collect
	AsebaCanRecvQueueGarbageCollect();
	
	return pos;
}

void AsebaCanFrameReceived(const CanFrame *frame)
{
	uint16 temp;
	if (AsebaCanRecvQueueGetMinFreeFrames() <= 1)
	{
		// if packet is stop, free associated frames, otherwise this could lead to everlasting used frames
		if (CANID_TO_TYPE(frame->id) == TYPE_PACKET_STOP)
			AsebaCanRecvQueueFreeFrames(CANID_TO_ID(frame->id));
		
		// notify user
		asebaCan.receivedPacketDroppedFP();
	}
	else
	{
		// store and increment pos
		asebaCan.recvQueue[asebaCan.recvQueueInsertPos] = *frame;
		asebaCan.recvQueue[asebaCan.recvQueueInsertPos].used = 1;
		temp = asebaCan.recvQueueInsertPos + 1;
		if (temp >= asebaCan.recvQueueSize)
			temp = 0;
		asebaCan.recvQueueInsertPos = temp;
	}
}

/*@}*/
