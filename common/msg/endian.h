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

#ifndef ASEBA_ENDIAN
#define ASEBA_ENDIAN

namespace Aseba
{
	/** \addtogroup msg */
	/*@{*/
	
	//! ByteSwapper, code inspired by DPS (http://dps.epfl.ch/), released in GPL
	/** Mail to ask for LPGL relicensing sent to the author, Sebastian Gerlach */
	struct ByteSwapper
	{
		/*! Swap bytes at data (generic) */
		/** \param data pointer to data of size to swap */
		template<size_t size>
		static inline void swapp(void *data)
		{
			uint8 temp[size];
			for (size_t i = 0; i < size; ++i)
				temp[i] = reinterpret_cast<uint8*>(data);
			for (size_t i = 0; i < size; ++i)
				reinterpret_cast<uint8*>(data)[i] = temp[size-1-i];
		}
		
		/*! Swap value v (generic, mutable-value version) */
		/** \param data reference to value to swap */
		template<typename T> 
		static void swap(T& v)
		{
			swapp<sizeof(T)>(&v);
		}
		
		/*! Swap value v (generic, const-value version) */
		/** \param data const reference to value to swap 
		 *  \return swapped value
		 */
		template<typename T> 
		static T swap(const T& v)
		{
			T temp(v);
			swapp<sizeof(T)>(&temp);
			return temp;
		}
	};
	//! Swap bytes for 8-bit word, do nothing
	template<> inline void ByteSwapper::swapp<1>(void * /*data*/)
	{
	}
	//! Swap bytes for 16-bit word
	template<> inline void ByteSwapper::swapp<2>(void *data)
	{
		const uint16 a=*reinterpret_cast<uint16*>(data);
		*reinterpret_cast<uint16*>(data)=
			((a&0x00ff)<< 8)|
			((a>> 8)&0x00ff);
	}
	//! Swap bytes for 32-bit word
	template<> inline void ByteSwapper::swapp<4>(void *data)
	{
		const uint32 a=*reinterpret_cast<uint32*>(data);
		*reinterpret_cast<uint32*>(data)=
			((a&0x000000ff)<<24)|
			((a&0x0000ff00)<< 8)|
			((a>> 8)&0x0000ff00)|
			((a>>24)&0x000000ff);
	}
	//! Swap bytes for 64-bit word
	template<> inline void ByteSwapper::swapp<8>(void *data)
	{
		const uint64 a=*reinterpret_cast<uint64*>(data);
		*reinterpret_cast<uint64*>(data)=
			((a&0x00000000000000ffLL)<<56)|
			((a&0x000000000000ff00LL)<<40)|
			((a&0x0000000000ff0000LL)<<24)|
			((a&0x00000000ff000000LL)<< 8)|
			((a>> 8)&0x00000000ff000000LL)|
			((a>>24)&0x0000000000ff0000LL)|
			((a>>40)&0x000000000000ff00LL)|
			((a>>56)&0x00000000000000ffLL);
	}
	
	#ifdef __BIG_ENDIAN__
	
	template<typename T>
	T swapEndianCopy(const T& v) { return ByteSwapper::swap<T>(v); }
	template<typename T>
	void swapEndian(T& v) { ByteSwapper::swap<T>(v); }
	
	#else
	
	template<typename T>
	T swapEndianCopy(const T& v) { return v; }
	template<typename T>
	void swapEndian(T& v) { /* do nothing */ }
	
	#endif
	
	/*@}*/
} // namespace Aseba

#endif // ASEBA_ENDIAN
