/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

#include "../common/consts.h"
#include "../msg/msg.h"
#include "../utils/utils.h"
#include <dashel/dashel.h>
#include "../utils/HexFile.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <cassert>
#include <cstring>

namespace Aseba 
{
	using namespace Dashel;
	using namespace std;
	
	//! Show list of known commands
	void dumpCommandList(ostream &stream)
	{
		stream << "* presence : broadcast presence message\n";
		stream << "* usermsg: user message [type] [length in 16 bit words]\n";
		stream << "* rdpage : bootloader read page [dest] [page number]\n";
		stream << "* rdpageusb : bootloader read page usb [dest] [page number]\n";
		stream << "* whex : write hex file [dest] [file name] [reset]\n";
		stream << "* rhex : read hex file [source] [file name]\n";
		stream << "* eb: exit from bootloader, go back into user mode [dest]\n";
		stream << "* sb: switch into bootloader: reboot node, then enter bootloader for a while [dest]\n";
		stream << "* sleep: put the vm to sleep [dest]\n";
		stream << "* wusb : write hex file to [dest] [file name] [reset]\n";
	}
	
	//! Show usage
	void dumpHelp(ostream &stream, const char *programName)
	{
		stream << "Aseba cmd, send message over the aseba network, usage:\n";
		stream << programName << " [-t target] [cmd destId (args)] ... [cmd destId (args)]\n";
		stream << "where cmd is one af the following:\n";
		dumpCommandList(stream);
		stream << std::endl;
		stream << "Other options:\n";
		stream << "    -h, --help      : shows this help\n";
		stream << "    -V, --version   : shows the version number\n";
		stream << "Report bugs to: aseba-dev@gna.org" << std::endl;
	}
	
	//! Show version
	void dumpVersion(std::ostream &stream)
	{
		stream << "Aseba cmd " << ASEBA_VERSION << std::endl;
		stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
		stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
	}

	//! Produce an error message and dump help and quit
	void errorMissingArgument(const char *programName)
	{
		cerr << "Error, missing argument.\n";
		dumpHelp(cerr, programName);
		exit(4);
	}
	
	//! Produce an error message and dump help and quit
	void errorUnknownCommand(const char *cmd)
	{
		cerr << "Error, unknown command " << cmd << endl;
		cerr << "Known commands are:\n";
		dumpCommandList(cerr);
		exit(5);
	}
	
	//! Produce an error message and quit
	void errorReadPage(int pageNumber)
	{
		cerr << "Error, can't read page " << pageNumber << endl;
		exit(6);
	}
	
	//! Produce an error message and quit
	void errorOpenFile(const char *fileName)
	{
		cerr << "Error, can't open file " << fileName << endl;
		exit(7);
	}
	
	//! Produce an error message and quit
	void errorWriteFile(const char *fileName)
	{
		cerr << "Error, can't write file " << fileName << endl;
		exit(8);
	}
	
	//! Produce an error message and quit
	void errorServerDisconnected()
	{
		cerr << "Server closed connection before command was completely sent." << endl;
		exit(9);
	}
	
	//! Produce an error message and quit
	void errorHexFile(const string &message)
	{
		cerr << "HEX file error: " << message << endl;
		exit(10);
	}
	
	//! Produce an error message and quit
	void errorWritePage(unsigned pageIndex)
	{
		cerr << "Error while writing page " << pageIndex << endl;
		exit(11);
	}
	
	//! Warn about an error but do not quit
	void errorWritePageNonFatal(unsigned pageIndex)
	{
		cerr << "Error while writing page " << pageIndex << ", continuing ..." << endl;
	}
	
	//! Produce an error message and quit
	void errorReadPage(unsigned pageIndex)
	{
		cerr << "Error while reading page " << pageIndex << endl;
		exit(12);
	}
	
	//! Produce an error message and quit
	void errorReadPage(unsigned pageIndex, unsigned pagesStart, unsigned pagesCount)
	{
		cerr << "Error, page index " << pageIndex << " out of page range [ " << 	pagesStart << " : " << pagesStart + pagesCount << " ]" << endl;
		exit(13);
	}
	
	//! Manage interactions with an aseba-compatible bootloader
	class BootloaderInterface
	{
	public:
		//! Create an interface to bootloader with id dest using a socket
		BootloaderInterface(Stream* stream, int dest) :
			stream(stream),
			dest(dest)
		{
			// Wait until the bootloader answers

		}
		
		//! Return the size of a page
		int getPageSize() { return pageSize; }
		
		//! Read a page
		bool readPage(unsigned pageNumber, uint8* data)
		{
			if ((pageNumber < pagesStart) || (pageNumber >= pagesStart + pagesCount))
			{
				errorReadPage(pageNumber, pagesStart, pagesCount);
				return false;
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
				Message *message = Message::receive(stream);
				
				// handle ack
				BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message);
				if (ackMessage && (ackMessage->source == dest))
				{
					uint16 errorCode = ackMessage->errorCode;
					delete message;
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
				BootloaderDataRead *dataMessage = dynamic_cast<BootloaderDataRead *>(message);
				if (dataMessage && (dataMessage->source == dest))
				{
					if (dataRead >= pageSize)
						cerr << "Warning, reading oversized page (" << dataRead << "/" << pageSize << ") bytes.\n";
					copy(dataMessage->data, dataMessage->data + sizeof(dataMessage->data), data);
					data += sizeof(dataMessage->data);
					dataRead += sizeof(dataMessage->data);
					cout << "Page read so far (" << dataRead << "/" << pageSize << ") bytes.\n";
				}
				
				delete message;
			}
			
			return true;
		}
		
		bool readPageUsb(unsigned pageNumber, uint8 * data) {
			BootloaderReadPage message;
			message.dest = dest;
			message.pageNumber = pageNumber;
			message.serialize(stream);
			stream->flush();
			
			stream->read(data, 2048);
			return true;
		}
		
		//! Write a page
		bool writePage(int pageNumber, const uint8 *data)
		{
			// send command
			BootloaderWritePage writePage;
			writePage.dest = dest;
			writePage.pageNumber = pageNumber;
			writePage.serialize(stream);
			stream->flush();
			
			cout << "Writing page " << pageNumber << endl;
			// wait ACK
			while (true)
			{
				Message *message = Message::receive(stream);
				
				// handle ack
				BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message);
				if (ackMessage && (ackMessage->source == dest))
				{
					uint16 errorCode = ackMessage->errorCode;
					delete message;
					if(errorCode == BootloaderAck::SUCCESS)
						break;
					else
						return false;
				}
				
				delete message;
			}
			
			// write data
			for (unsigned dataWritten = 0; dataWritten < pageSize;)
			{
				BootloaderPageDataWrite pageData;
				pageData.dest = dest;
				copy(data + dataWritten, data + dataWritten + sizeof(pageData.data), pageData.data);
				pageData.serialize(stream);
				dataWritten += sizeof(pageData.data);
//				cout << "." << std::flush;
				
/*
				while (true)
				{
					Message *message = Message::receive(stream);
					
					// handle ack
					BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message);
					if (ackMessage && (ackMessage->source == dest))
					{
						uint16 errorCode = ackMessage->errorCode;
						delete message;
						if(errorCode == BootloaderAck::SUCCESS)
							break;
						else
							return false;
					}
					
					delete message;
				}
*/
			}
			// Flush only here so we 3n14rg3 our bandwidth
			stream->flush();
			
			while (true)
			{
				Message *message = Message::receive(stream);
				
				// handle ack
				BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message);
				if (ackMessage && (ackMessage->source == dest))
				{
					uint16 errorCode = ackMessage->errorCode;
					delete message;
					if(errorCode == BootloaderAck::SUCCESS)
						break;
					else
						return false;
				}
				
				delete message;
			}
			
			cout << "done" << endl;
			return true;
		}
		
		//! Write an hex file
		void writeHex(const string &fileName, bool reset)
		{
			// Load hex file
			HexFile hexFile;
			hexFile.read(fileName);
			
			if (reset) 
			{
				Reboot msg(dest);
				msg.serialize(stream);
				stream->flush();
			}
				
			while (true)
			{
				Message *message = Message::receive(stream);
				BootloaderDescription *bDescMessage = dynamic_cast<BootloaderDescription *>(message);
				if (bDescMessage && (bDescMessage->source == dest))
				{
					pageSize = bDescMessage->pageSize;
					pagesStart = bDescMessage->pagesStart;
					pagesCount = bDescMessage->pagesCount;
					delete message;
					break;
				}
				delete message;
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
			
			// Write pages
			for (PageMap::iterator it = pageMap.begin(); it != pageMap.end(); it ++)
			{
				unsigned pageIndex = it->first;
				if ((pageIndex >= pagesStart) && (pageIndex < pagesStart + pagesCount))
					if (!writePage(pageIndex, &it->second[0]))
						errorWritePage(pageIndex);
			}

			if (reset) 
			{
				BootloaderReset msg(dest);
				msg.serialize(stream);
				stream->flush();
			}
		}
		
		bool writePageUsb(int pageNumber, const uint8 *data)
		{
			// send command
			BootloaderWritePage writePage;
			writePage.dest = dest;
			writePage.pageNumber = pageNumber;
			writePage.serialize(stream);
			
			cout << "Writing page " << pageNumber << endl;
			cout << "First data: " << hex << (int) data[0] << "," << hex << (int) data[1] << "," << hex << (int) data[2] << endl;
			
			stream->write(data,pageSize);
			stream->flush();
			
			while (true)
			{
				cout << "Waiting ack ..." << endl;
				Message *message = Message::receive(stream);
				// handle ack
				BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message);
				if (ackMessage && (ackMessage->source == dest))
				{
					uint16 errorCode = ackMessage->errorCode;
					delete message;
					if(errorCode == BootloaderAck::SUCCESS) {
						cout << "Success" << endl;
						break;
					} else {
						cout << "Fail" << endl;
						return false;
					}
				}
				
				delete message;
			}
			
			return true;
		}
		
		void writeHexUsb(const string &fileName, bool reset) {
			HexFile hexFile;
			hexFile.read(fileName);
			
			if (reset) 
			{
				Reboot msg(dest);
				msg.serialize(stream);
				stream->flush();
			}
				
/*			while (true)
			{
				Message *message = Message::receive(stream);
				BootloaderDescription *bDescMessage = dynamic_cast<BootloaderDescription *>(message);
				if (bDescMessage && (bDescMessage->source == dest))
				{
					pageSize = bDescMessage->pageSize;
					pagesStart = bDescMessage->pagesStart;
					pagesCount = bDescMessage->pagesCount;
					delete message;
					break;
				}
				delete message;
			}
*/			
			pageSize = 2048;
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
			
			// Write pages
			for (PageMap::iterator it = pageMap.begin(); it != pageMap.end(); it ++)
			{
				unsigned pageIndex = it->first;
				if (pageIndex != 0)
					if (!writePageUsb(pageIndex, &it->second[0]))
						errorWritePageNonFatal(pageIndex);
			}
			// Now look for the index 0 page
			for (PageMap::iterator it = pageMap.begin(); it != pageMap.end(); it ++)
			{
				unsigned pageIndex = it->first;
				if (pageIndex == 0)
					if (!writePageUsb(pageIndex, &it->second[0]))
						errorWritePageNonFatal(pageIndex);
			}

			if (reset) 
			{
				BootloaderReset msg(dest);
				msg.serialize(stream);
				stream->flush();
			}
			
		}
		
		//! Read an hex file
		void readHex(const string &fileName)
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
					errorReadPage(page);
				
				copy(&buffer[0], &buffer[pageSize], back_inserter(hexFile.data[address]));
			}
			
			// Write hex file
			hexFile.strip(pageSize);
			hexFile.write(fileName);
		}
	
	protected:
		Stream* stream;
		int dest;
		unsigned pageSize;
		unsigned pagesStart;
		unsigned pagesCount;
	};
	
	//! Process a command, return the number of arguments eaten (not counting the command itself)
	int processCommand(Stream* stream, int argc, char *argv[])
	{
		const char *cmd = argv[0];
		int argEaten = 0;
		
		if (strcmp(cmd, "presence") == 0)
		{
			GetDescription message;
			message.serialize(stream);
			stream->flush();
		}
		else if (strcmp(cmd, "usermsg") == 0)
		{
			// first arg is type, second is length
			if (argc < 3)
				errorMissingArgument(argv[0]);
			argEaten = 2;
			uint16 type = atoi(argv[1]);
			uint16 length = atoi(argv[2]);
			
			UserMessage::DataVector data(length);
			for (size_t i = 0; i < length; i++)
				data[i] = i;
			
			UserMessage message(type, data);
			message.serialize(stream);
			stream->flush();
		}
		else if (strcmp(cmd, "rdpage") == 0)
		{
			// first arg is dest, second is page number
			if (argc < 3)
				errorMissingArgument(argv[0]);
			argEaten = 2;
			
			BootloaderInterface bootloader(stream, atoi(argv[1]));
			
			vector<uint8> data(bootloader.getPageSize());
			if (bootloader.readPage(atoi(argv[2]), &data[0]))
			{
				ofstream file("page.bin");
				if (file.good())
					copy(data.begin(), data.end(), ostream_iterator<uint8>(file));
				else
					errorOpenFile("page.bin");
			}
			else
				errorReadPage(atoi(argv[2]));
		}
		else if(strcmp(cmd, "rdpageusb") == 0)
		{
			if (argc < 3)
				errorMissingArgument(argv[0]);
			argEaten = 2;
			
			BootloaderInterface bootloader(stream, atoi(argv[1]));
			vector <uint8> data(2048);
			cout << "Page: " << atoi(argv[2]) << endl;
			if(bootloader.readPageUsb(atoi(argv[2]), &data[0])) {
				ofstream file("page.bin");
				if(file.good())
					copy(data.begin(),data.end(),ostream_iterator<uint8>(file));
				else
					errorOpenFile("page.bin");
			}
			else
				errorReadPage(atoi(argv[2]));
		}
		else if (strcmp(cmd, "whex") == 0)
		{
			bool reset = 0;
			// first arg is dest, second is file name
			if (argc < 3)
				errorMissingArgument(argv[0]);
			argEaten = 2;
			
			if (argc > 3 && !strcmp(argv[3], "reset")) 
			{
				reset = 1;
				argEaten = 3;
			}

			// try to write hex file
			try
			{
				BootloaderInterface bootloader(stream, atoi(argv[1]));
				bootloader.writeHex(argv[2], reset);
			}
			catch (HexFile::Error &e)
			{
				errorHexFile(e.toString());
			}
		}
		else if (strcmp(cmd,"wusb") == 0)
		{
			bool reset = 0;
			if (argc < 3)
				errorMissingArgument(argv[0]);
			argEaten = 2;
			if(argc > 3 && !strcmp(argv[3], "reset"))
			{
				reset = 1;
				argEaten = 3;
			}
			try
			{
				BootloaderInterface bootloader(stream, atoi(argv[1]));
				bootloader.writeHexUsb(argv[2], reset);
			}
			catch (HexFile::Error &e)
			{
				errorHexFile(e.toString());
			}
		}
		else if (strcmp(cmd, "rhex") == 0)
		{
			// first arg is source, second is file name
			if (argc < 3)
				errorMissingArgument(argv[0]);
			argEaten = 2;
			
			// try to read hex file
			try
			{
				BootloaderInterface bootloader(stream, atoi(argv[1]));
				bootloader.readHex(argv[2]);
			}
			catch (HexFile::Error &e)
			{
				errorHexFile(e.toString());
			}
		}
		else if (strcmp(cmd, "eb") == 0)
		{
			uint16 dest;
			
			if(argc < 2)
				errorMissingArgument(argv[0]);
			argEaten = 1;
			
			dest = atoi(argv[1]);
			
			BootloaderReset msg(dest);
			msg.serialize(stream);
			stream->flush();
			
			// Wait ack from bootloader (mean we got out of it)
			// Or bootloader description (mean we just entered it)
			// WRONG; FIXME 
			while (true)
			{
				Message *message = Message::receive(stream);
				
				// handle ack
				BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message);
				if (ackMessage && (ackMessage->source == dest))
				{
					cout << "Device is now in user-code" << endl;
					delete message;
					break;
				}
				
				//BootloaderDescription * bMessage = dynamic_cast<BootloaderDescription *>(message);
				
				delete message;
			}
		}
		else if (strcmp(cmd, "sb") == 0)
		{
			uint16 dest;
			
			if(argc < 2)
				errorMissingArgument(argv[0]);
			argEaten = 1;
			
			dest = atoi(argv[1]);
			
			Reboot msg(dest);
			msg.serialize(stream);
			stream->flush();
		}
		else if (strcmp(cmd, "sleep") == 0)
		{
			uint16 dest;
			if(argc < 2)
				errorMissingArgument(argv[0]);
			argEaten = 1;

			dest = atoi(argv[1]);

			Sleep msg(dest);
			msg.serialize(stream);
			stream->flush();
		} else 
			errorUnknownCommand(cmd);
		
		return argEaten;
	}
}

int main(int argc, char *argv[])
{
	const char *target = ASEBA_DEFAULT_TARGET;
	int argCounter = 1;
	
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		if (strcmp(arg, "-t") == 0)
		{
			if (++argCounter < argc)
				target = argv[argCounter];
			else
				Aseba::errorMissingArgument(argv[0]);
		}
		else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
		{
			Aseba::dumpHelp(std::cout, argv[0]);
			return 0;
		}
		else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
		{
			Aseba::dumpVersion(std::cout);
			return 0;
		}
		else
		{
			Dashel::Hub client;
			Dashel::Stream* stream = client.connect(target);
			assert(stream);
			
			// process command
			try
			{
				 argCounter += Aseba::processCommand(stream, argc - argCounter, &argv[argCounter]);
				 stream->flush();
			}
			catch (Dashel::DashelException e)
			{
				Aseba::errorServerDisconnected();
			}
		}
		argCounter++;
	}
	return 0;
}

