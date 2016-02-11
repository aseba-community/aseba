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

#ifndef ASEBA_BOOTLOADER_INTERFACE_H
#define ASEBA_BOOTLOADER_INTERFACE_H

#include <string>
#include <stdexcept>
#include "../types.h"


namespace Dashel
{
	class Stream;
}

namespace Aseba 
{
	// TODO: change API to use HexFile instead of file names
	
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
			Error(const std::string& what): std::runtime_error(what) {}
		};
		
	public:
		// main interface
		
		//! Create an interface to bootloader with id dest using a socket
		BootloaderInterface(Dashel::Stream* stream, int dest);
		
		//! Create an interface to bootloader with id dest and different id within bootloader bootloaderDest (currently only deployed for simple mode), using a socket
		BootloaderInterface(Dashel::Stream* stream, int dest, int bootloaderDest);
		
		//! Return the size of a page
		int getPageSize() const { return pageSize; }
		
		//! Read a page
		bool readPage(unsigned pageNumber, uint8* data);
		
		//! Read a page, simplified protocol
		bool readPageSimple(unsigned pageNumber, uint8 * data);
		
		//! Write a page, if simple is true, use simplified protocol, otherwise use complete protocol
		bool writePage(unsigned pageNumber, const uint8 *data, bool simple);
		
		//! Write an hex file
		void writeHex(const std::string &fileName, bool reset, bool simple);
		
		//! Read an hex file and write it to fileName
		void readHex(const std::string &fileName);
		
	protected:
		// reporting function
		
		// progress
		virtual void writePageStart(unsigned pageNumber, const uint8* data, bool simple) {}
		virtual void writePageWaitAck() {}
		virtual void writePageSuccess() {}
		virtual void writePageFailure() {}
		
		virtual void writeHexStart(const std::string &fileName, bool reset, bool simple) {}
		virtual void writeHexEnteringBootloader() {}
		virtual void writeHexGotDescription(unsigned pagesCount) {}
		virtual void writeHexWritten() {}
		virtual void writeHexExitingBootloader() {}
		
		// non-fatal errors
		
		//! Warn about an error but do not quit
		virtual void errorWritePageNonFatal(unsigned pageNumber) {}
		
	protected:
		// member variables
		Dashel::Stream* stream;
		int dest, bootloaderDest;
		unsigned pageSize;
		unsigned pagesStart;
		unsigned pagesCount;
	};
} // namespace Aseba

#endif // ASEBA_BOOTLOADER_INTERFACE_H
