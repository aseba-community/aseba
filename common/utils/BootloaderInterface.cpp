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

#include "BootloaderInterface.h"
#include "../consts.h"
#include "../msg/msg.h"
#include "utils.h"
#include "HexFile.h"
#include "FormatableString.h"
#include <dashel/dashel.h>
#include <memory>
#include <unistd.h>

namespace Aseba 
{
	using namespace Dashel;
	using namespace std;
	
	BootloaderInterface::BootloaderInterface(Stream* stream, int dest) :
		stream(stream),
		dest(dest),
		bootloaderDest(dest),
		pageSize(0),
		pagesStart(0),
		pagesCount(0)
	{
		
	}
	
	BootloaderInterface::BootloaderInterface(Dashel::Stream* stream, int dest, int bootloaderDest) :
		stream(stream),
		dest(dest),
		bootloaderDest(bootloaderDest),
		pageSize(0),
		pagesStart(0),
		pagesCount(0)
	{
		
	}
	
	bool BootloaderInterface::readPage(unsigned pageNumber, uint8* data)
	{
		if ((pageNumber < pagesStart) || (pageNumber >= pagesStart + pagesCount))
		{
			throw Error(FormatableString("Error, page index %0 out of page range [%1:%2]").arg(pageNumber).arg(pagesStart).arg(pagesStart+pagesCount));
		}
		
		// send command
		BootloaderReadPage message;
		message.dest = dest;
		message.pageNumber = pageNumber;
		message.serialize(stream);
		stream->flush();
		unsigned dataRead = 0;
		
		// get data
		while (true)
		{
			auto_ptr<Message> message(Message::receive(stream));
			
			// handle ack
			BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message.get());
			if (ackMessage && (ackMessage->source == dest))
			{
				uint16 errorCode = ackMessage->errorCode;
				if (errorCode == BootloaderAck::SUCCESS)
				{
					if (dataRead < pageSize)
						cerr << "Warning, got acknowledgement but page not fully read (" << dataRead << "/" << pageSize << ") bytes.\n";
					return true;
				}
				else
					return false;
			}
			
			// handle data
			BootloaderDataRead *dataMessage = dynamic_cast<BootloaderDataRead *>(message.get());
			if (dataMessage && (dataMessage->source == dest))
			{
				if (dataRead >= pageSize)
					cerr << "Warning, reading oversized page (" << dataRead << "/" << pageSize << ") bytes.\n";
				copy(dataMessage->data, dataMessage->data + sizeof(dataMessage->data), data);
				data += sizeof(dataMessage->data);
				dataRead += sizeof(dataMessage->data);
				cout << "Page read so far (" << dataRead << "/" << pageSize << ") bytes.\n";
			}
		}
		
		return true;
	}
	
	bool BootloaderInterface::readPageSimple(unsigned pageNumber, uint8 * data)
	{
		BootloaderReadPage message;
		message.dest = dest;
		message.pageNumber = pageNumber;
		message.serialize(stream);
		stream->flush();
		
		stream->read(data, 2048);
		return true;
	}
	
	bool BootloaderInterface::writePage(unsigned pageNumber, const uint8 *data, bool simple)
	{
		writePageStart(pageNumber, data, simple);
		
		// send command
		BootloaderWritePage writePage;
		writePage.dest = bootloaderDest;
		writePage.pageNumber = pageNumber;
		writePage.serialize(stream);
		
		if (simple)
		{
			// just write the complete page at ounce
			stream->write(data,pageSize);
		}
		else
		{
			// flush command
			stream->flush();
			
			// wait ACK
			while (true)
			{
				auto_ptr<Message> message(Message::receive(stream));
				
				// handle ack
				BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message.get());
				if (ackMessage && (ackMessage->source == bootloaderDest))
				{
					uint16 errorCode = ackMessage->errorCode;
					if(errorCode == BootloaderAck::SUCCESS)
						break;
					else
						return false;
				}
			}
			
			// write data
			for (unsigned dataWritten = 0; dataWritten < pageSize;)
			{
				BootloaderPageDataWrite pageData;
				pageData.dest = bootloaderDest;
				copy(data + dataWritten, data + dataWritten + sizeof(pageData.data), pageData.data);
				pageData.serialize(stream);
				dataWritten += sizeof(pageData.data);
				//cout << "." << std::flush;
				
				/*
				while (true)
				{
					auto_ptr<Message> message(Message::receive(stream));
					
					// handle ack
					BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message);
					if (ackMessage && (ackMessage->source == dest))
					{
						uint16 errorCode = ackMessage->errorCode;
						if(errorCode == BootloaderAck::SUCCESS)
							break;
						else
							return false;
					}
				}
				*/
			}
			
		}
		
		// flush only here to save bandwidth
		stream->flush();
		
		while (true)
		{
			writePageWaitAck();
			auto_ptr<Message> message(Message::receive(stream));
			
			// handle ack
			BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message.get());
			if (ackMessage && (ackMessage->source == bootloaderDest))
			{
				uint16 errorCode = ackMessage->errorCode;
				if(errorCode == BootloaderAck::SUCCESS)
				{
					writePageSuccess();
					return true;
				}
				else
				{
					writePageFailure();
					return false;
				}
			}
		}
		
		// should not happen
		return true;
	}
	
	void BootloaderInterface::writeHex(const string &fileName, bool reset, bool simple)
	{
		// Load hex file
		HexFile hexFile;
		hexFile.read(fileName);
		
		writeHexStart(fileName, reset, simple);
		
		if (reset) 
		{
			Reboot msg(dest);
			msg.serialize(stream);
			stream->flush();
			
			writeHexEnteringBootloader();
		}
	
		// Get page layout
		if (simple)
		{
			/* 
			This would be cleaner for non-broken robot, but would not work for recovery.
			Waiting works for robots in any states
			// wait for disconnected message
			while (true)
			{
				auto_ptr<Message> message(Message::receive(stream));
				Disconnected* disconnectedMessage(dynamic_cast<Disconnected*>(message.get()));
				if (disconnectedMessage)
					break;
			}*/
			// give 10 ms to the robot the time to reset
			usleep(10000);
			pageSize = 2048;
		}
		else
		{
			// get bootloader description
			while (true)
			{
				auto_ptr<Message> message(Message::receive(stream));
				BootloaderDescription *bDescMessage = dynamic_cast<BootloaderDescription *>(message.get());
				if (bDescMessage && (bDescMessage->source == bootloaderDest))
				{
					pageSize = bDescMessage->pageSize;
					pagesStart = bDescMessage->pagesStart;
					pagesCount = bDescMessage->pagesCount;
					break;
				}
			}
		}
		
		// Build a map of pages out of the map of addresses
		typedef map<uint32, vector<uint8> > PageMap;
		PageMap pageMap;
		for (HexFile::ChunkMap::iterator it = hexFile.data.begin(); it != hexFile.data.end(); it ++)
		{
			// get page number
			unsigned chunkAddress = it->first;
			// index inside data chunk
			unsigned chunkDataIndex = 0;
			// size of chunk in bytes
			unsigned chunkSize = it->second.size();
			
			// copy data from chunk to page
			do
			{
				// get page number
				unsigned pageIndex = (chunkAddress + chunkDataIndex) / pageSize;
				// get address inside page
				unsigned byteIndex = (chunkAddress + chunkDataIndex) % pageSize;
			
				// if page does not exists, create it
				if (pageMap.find(pageIndex) == pageMap.end())
				{
				//	std::cout << "New page N° " << pageIndex << " for address 0x" << std::hex << chunkAddress << endl;
					pageMap[pageIndex] = vector<uint8>(pageSize, (uint8)0);
				}
				// copy data
				unsigned amountToCopy = min(pageSize - byteIndex, chunkSize - chunkDataIndex);
				copy(it->second.begin() + chunkDataIndex, it->second.begin() + chunkDataIndex + amountToCopy, pageMap[pageIndex].begin() + byteIndex);
				
				// increment chunk data pointer
				chunkDataIndex += amountToCopy;
			}
			while (chunkDataIndex < chunkSize);
		}
		
		writeHexGotDescription(pageMap.size());
		
		if (simple)
		{
			// Write pages
			for (PageMap::iterator it = pageMap.begin(); it != pageMap.end(); it ++)
			{
				unsigned pageIndex = it->first;
				if (pageIndex != 0)
					if (!writePage(pageIndex, &it->second[0], true))
						errorWritePageNonFatal(pageIndex);
			}
			// Now look for the index 0 page
			for (PageMap::iterator it = pageMap.begin(); it != pageMap.end(); it ++)
			{
				unsigned pageIndex = it->first;
				if (pageIndex == 0)
					if (!writePage(pageIndex, &it->second[0], true))
						errorWritePageNonFatal(pageIndex);
			}
		}
		else
		{
			// Write pages
			for (PageMap::iterator it = pageMap.begin(); it != pageMap.end(); it ++)
			{
				unsigned pageIndex = it->first;
				if ((pageIndex >= pagesStart) && (pageIndex < pagesStart + pagesCount))
					if (!writePage(pageIndex, &it->second[0], false))
						throw Error(FormatableString("Error while writing page %0").arg(pageIndex));
			}
		}
		
		writeHexWritten();

		if (reset) 
		{
			BootloaderReset msg(bootloaderDest);
			msg.serialize(stream);
			stream->flush();
			
			writeHexExitingBootloader();
		}
	}
	
	void BootloaderInterface::readHex(const string &fileName)
	{
		HexFile hexFile;
		
		// Create memory
		unsigned address = pagesStart * pageSize;
		hexFile.data[address] = vector<uint8>();
		hexFile.data[address].reserve(pagesCount * pageSize);
		
		// Read pages
		for (unsigned page = pagesStart; page < pagesCount; page++)
		{
			vector<uint8> buffer((uint8)0, pageSize);
			
			if (!readPage(page, &buffer[0]))
				throw Error(FormatableString("Error, cannot read page %0").arg(page));
			
			copy(&buffer[0], &buffer[pageSize], back_inserter(hexFile.data[address]));
		}
		
		// Write hex file
		hexFile.strip(pageSize);
		hexFile.write(fileName);
	}
	
} // namespace Aseba
