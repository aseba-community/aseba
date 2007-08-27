/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASEBA_HEX_FILE_H
#define ASEBA_HEX_FILE_H

#include "../common/types.h"
#include "../utils/FormatableString.h"
#include <map>
#include <vector>
#include <string>

namespace Aseba
{
	class HexFile
	{
	public:
		struct Error
		{
			virtual ~Error () { }
			virtual std::string toString() const = 0;
		};
		
		struct EarlyEOF : Error
		{
			int line;
			
			EarlyEOF(int line) : line(line) { }
			virtual std::string toString() const;
		};
		
		struct InvalidRecord : Error
		{
			int line;
			
			InvalidRecord(int line) : line(line) { }
			virtual std::string toString() const;
		};
		
		struct WrongCheckSum : Error
		{
			int line;
			uint8 recordCheckSum;
			uint8 computedCheckSum;
			
			WrongCheckSum (int line, uint8 recordCheckSum, uint8 computedCheckSum) :
				line(line),
				recordCheckSum(recordCheckSum),
				computedCheckSum(computedCheckSum)
			{ }
			virtual std::string toString() const;
		};
		
		struct UnknownRecordType : Error
		{
			int line;
			uint8 recordType;
			
			UnknownRecordType(int line, uint8 recordType) : line(line), recordType(recordType) { }
			virtual std::string toString() const;
		};
		
		struct FileOpeningError : Error
		{
			std::string fileName;
			
			FileOpeningError(const std::string &fileName) : fileName(fileName) { }
			virtual std::string toString() const;
		};
		
	public:
		typedef std::map<uint32, std::vector<uint8> > ChunkMap;
		ChunkMap data;

	public:
		void read(const std::string &fileName);
		void write(const std::string &fileName) const;
	
	protected:
		unsigned getUint4(std::istream &stream);
		unsigned getUint8(std::istream &stream);
		unsigned getUint16(std::istream &stream);
		void writeExtendedLinearAddressRecord(std::ofstream &stream, unsigned addr16) const;
		void writeData(std::ofstream &stream, unsigned addr16, unsigned count8, uint8 *data) const;
	};
}

#endif

