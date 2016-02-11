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

#include <dashel/dashel.h>
#include "../../common/consts.h"
#include "../../common/msg/msg.h"
#include "../../common/utils/utils.h"
#include "../../common/utils/HexFile.h"
#include "../../common/utils/FormatableString.h"
#include "../../common/utils/BootloaderInterface.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"
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

	
	class CmdBootloaderInterface:public BootloaderInterface
	{
	public:
		CmdBootloaderInterface(Stream* stream, int dest):
			BootloaderInterface(stream, dest)
		{}
	
	protected:
		// reporting function
		virtual void writePageStart(unsigned pageNumber, const uint8* data, bool simple)
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
		
		virtual void writeHexGotDescription(unsigned pagesCount)
		{
			cout << "In bootloader, about to write " << pagesCount << " pages" << endl;
		}
		
		virtual void writeHexWritten()
		{
			cout << "Write completed" << endl;
		}
		
		virtual void writeHexExitingBootloader()
		{
			cout << "Exiting bootloader" << endl;
		}
		
		virtual void errorWritePageNonFatal(unsigned pageNumber)
		{
			cerr << "Warning, error while writing page " << pageNumber << ", continuing ..." << endl;
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
	Dashel::initPlugins();
	
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

