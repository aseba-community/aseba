/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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
#include "../utils/FormatableString.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <cassert>
#include <cstring>
#include <memory>

namespace Aseba 
{
	using namespace Dashel;
	using namespace std;
	
	//! Show list of known commands
	void dumpCommandList(ostream &stream)
	{
		stream << "* presence : broadcast presence message\n";
		stream << "* usermsg: user message [type] [word0] ... [wordN]\n";
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
	void errorBootloader(const string& message)
	{
		cerr << "Error while interfacing with bootloader: " << message << endl;
		exit(9);
	}
	
	//! Manage interactions with an aseba-compatible bootloader
	/**
		There are two versions of the bootloader: the complete and the simple.
		The complete version works on any Aseba network including over switches
		as it transmits all data using the Aseba message protocol.
		The simple version requires direct access to the device to be flashed,
		because it breaks the Aseba message protocol for page transmission.
	*/
	class BootloaderInterface
	{
	public:
		//! An error in link with the bootloader
		struct Error:public std::runtime_error
		{
			Error(const string& what): std::runtime_error(what) {}
		};
		
	public:
		// main interface
		
		//! Create an interface to bootloader with id dest using a socket
		BootloaderInterface(Stream* stream, int dest);
		
		//! Return the size of a page
		int getPageSize() const { return pageSize; }
		
		//! Read a page
		bool readPage(unsigned pageNumber, uint8* data);
		
		//! Read a page, simplified protocol
		bool readPageSimple(unsigned pageNumber, uint8 * data);
		
		//! Write a page, if simple is true, use simplified protocol, otherwise use complete protocol
		bool writePage(int pageNumber, const uint8 *data, bool simple);
		
		//! Write an hex file
		void writeHex(const string &fileName, bool reset, bool simple);
		
		//! Read an hex file and write it to fileName
		void readHex(const string &fileName);
		
	protected:
		// reporting function
		
		// progress
		virtual void writePageStart(int pageNumber, const uint8* data, bool simple) {}
		virtual void writePageWaitAck() {}
		virtual void writePageSuccess() {}
		virtual void writePageFailure() {}
		
		virtual void writeHexStart(const string &fileName, bool reset, bool simple) {}
		virtual void writeHexEnteringBootloader() {}
		virtual void writeHexGotDescription() {}
		virtual void writeHexWritten() {}
		virtual void writeHexExitingBootloader() {}
		
		// non-fatal errors
		
		//! Warn about an error but do not quit
		virtual void errorWritePageNonFatal(unsigned pageIndex) {}
		
	protected:
		// member variables
		Stream* stream;
		int dest;
		unsigned pageSize;
		unsigned pagesStart;
		unsigned pagesCount;
	};
	
	BootloaderInterface::BootloaderInterface(Stream* stream, int dest) :
		stream(stream),
		dest(dest),
		pageSize(0),
		pagesStart(0),
		pagesCount(0)
	{
		// Wait until the bootloader answers

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
	
	bool BootloaderInterface::writePage(int pageNumber, const uint8 *data, bool simple)
	{
		writePageStart(pageNumber, data, simple);
		
		// send command
		BootloaderWritePage writePage;
		writePage.dest = dest;
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
				if (ackMessage && (ackMessage->source == dest))
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
				pageData.dest = dest;
				copy(data + dataWritten, data + dataWritten + sizeof(pageData.data), pageData.data);
				pageData.serialize(stream);
				dataWritten += sizeof(pageData.data);
				//cout << "." << std::flush;
				
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
			
		}
		
		// flush only here to save bandwidth
		stream->flush();
		
		while (true)
		{
			writePageWaitAck();
			auto_ptr<Message> message(Message::receive(stream));
			
			// handle ack
			BootloaderAck *ackMessage = dynamic_cast<BootloaderAck *>(message.get());
			if (ackMessage && (ackMessage->source == dest))
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
			// wait for disconnected message
			while (true)
			{
				auto_ptr<Message> message(Message::receive(stream));
				Disconnected* disconnectedMessage(dynamic_cast<Disconnected*>(message.get()));
				if (disconnectedMessage)
					break;
			}
			pageSize = 2048;
		}
		else
		{
			// get bootloader description
			while (true)
			{
				auto_ptr<Message> message(Message::receive(stream));
				BootloaderDescription *bDescMessage = dynamic_cast<BootloaderDescription *>(message.get());
				if (bDescMessage && (bDescMessage->source == dest))
				{
					pageSize = bDescMessage->pageSize;
					pagesStart = bDescMessage->pagesStart;
					pagesCount = bDescMessage->pagesCount;
					break;
				}
			}
		}
		
		writeHexGotDescription();
		
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
			BootloaderReset msg(dest);
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
	
	class CmdBootloaderInterface:public BootloaderInterface
	{
	public:
		CmdBootloaderInterface(Stream* stream, int dest):
			BootloaderInterface(stream, dest)
		{}
	
	protected:
		// reporting function
		virtual void writePageStart(int pageNumber, const uint8* data, bool simple)
		{
			cout << "Writing page " << pageNumber << "... ";
			cout.flush();
			//cout << "First data: " << hex << (int) data[0] << "," << hex << (int) data[1] << "," << hex << (int) data[2] << endl;
		}
		
		virtual void writePageWaitAck()
		{
			cout << "Waiting ack ... ";
			cout.flush();
		}
		
		virtual void writePageSuccess()
		{
			cout << "Success" << endl;
		}
		
		virtual void writePageFailure()
		{
			cout << "Failure" << endl;
		}
		
		virtual void writeHexStart(const string &fileName, bool reset, bool simple)
		{
			cout << "Flashing " << fileName << endl;
		}
		
		virtual void writeHexEnteringBootloader()
		{
			cout << "Entering bootloader" << endl;
		}
		
		virtual void writeHexGotDescription()
		{
			cout << "In bootloader" << endl;
		}
		
		virtual void writeHexWritten()
		{
			cout << "Write completed" << endl;
		}
		
		virtual void writeHexExitingBootloader()
		{
			cout << "Exiting bootloader" << endl;
		}
		
		virtual void errorWritePageNonFatal(unsigned pageIndex)
		{
			cerr << "Error while writing page " << pageIndex << ", continuing ..." << endl;
		}
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
			if (argc < 2)
				errorMissingArgument(argv[0]);
			argEaten = argc;
			uint16 type = atoi(argv[1]);
			uint16 length = argc-2;
			
			UserMessage::DataVector data(length);
			for (size_t i = 0; i < length; i++)
				data[i] = atoi(argv[i+2]);
			
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
			
			CmdBootloaderInterface bootloader(stream, atoi(argv[1]));
			
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
			
			CmdBootloaderInterface bootloader(stream, atoi(argv[1]));
			vector <uint8> data(2048);
			cout << "Page: " << atoi(argv[2]) << endl;
			if(bootloader.readPageSimple(atoi(argv[2]), &data[0])) {
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
				CmdBootloaderInterface bootloader(stream, atoi(argv[1]));
				bootloader.writeHex(argv[2], reset, false);
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
				CmdBootloaderInterface bootloader(stream, atoi(argv[1]));
				bootloader.writeHex(argv[2], reset, true);
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
				CmdBootloaderInterface bootloader(stream, atoi(argv[1]));
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
	
	if (argc == 1)
	{
		Aseba::dumpHelp(std::cout, argv[0]);
		return 0;
	}
	
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
			catch (Aseba::BootloaderInterface::Error e)
			{
				Aseba::errorBootloader(e.what());
			}
		}
		argCounter++;
	}
	return 0;
}

