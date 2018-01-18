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

#ifndef ASEBA_HEX_FILE_H
#define ASEBA_HEX_FILE_H

#include "../types.h"
#include "FormatableString.h"
#include <map>
#include <utility>
#include <vector>
#include <string>

namespace Aseba
{
	/** \addtogroup utils */
	/*@{*/

	//! A class to read and write .hex files
	class HexFile
	{
	public:
		struct Error : std::exception
		{
			virtual ~Error() = default;
			virtual std::string toString() const = 0;
			virtual const char* what() const noexcept override;
		};

		struct EarlyEOF : Error
		{
			int line;

			EarlyEOF(int line) : line(line) {}
			std::string toString() const override;
		};

		struct InvalidRecord : Error
		{
			int line;

			InvalidRecord(int line) : line(line) {}
			std::string toString() const override;
		};

		struct WrongCheckSum : Error
		{
			int line;
			uint8_t recordCheckSum;
			uint8_t computedCheckSum;

			WrongCheckSum(int line, uint8_t recordCheckSum, uint8_t computedCheckSum) :
				line(line),
				recordCheckSum(recordCheckSum),
				computedCheckSum(computedCheckSum)
			{}
			std::string toString() const override;
		};

		struct UnknownRecordType : Error
		{
			int line;
			uint8_t recordType;

			UnknownRecordType(int line, uint8_t recordType) : line(line), recordType(recordType) {}
			std::string toString() const override;
		};

		struct FileOpeningError : Error
		{
			std::string fileName;

			FileOpeningError(std::string fileName) : fileName(std::move(fileName)) {}
			std::string toString() const override;
		};

	public:
		typedef std::map<uint32_t, std::vector<uint8_t> > ChunkMap;
		ChunkMap data;

	public:
		void read(const std::string& fileName);
		void write(const std::string& fileName) const;
		void strip(unsigned pageSize);

	protected:
		static unsigned getUint4(std::istream& stream);
		static unsigned getUint8(std::istream& stream);
		static unsigned getUint16(std::istream& stream);
		static void writeExtendedLinearAddressRecord(std::ofstream& stream, unsigned addr16);
		static void writeData(std::ofstream& stream, unsigned addr16, unsigned count8, uint8_t* data);
	};

	/*@}*/
}

#endif
