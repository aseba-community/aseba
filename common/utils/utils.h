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

#ifndef ASEBA_UTILS_H
#define ASEBA_UTILS_H

#include <iostream>
#include <string>
#include <cassert>
#include <cstdlib>
#include <vector>
#include "../types.h"

namespace Aseba
{
	/**
	\defgroup utils General helper functions and classes
	*/
	/*@{*/
	
	//! Macro to avoid the warning "unused variable", if we have a valid reason to keep this variable
	#define UNUSED(expr) do { (void)(expr); } while (0)
	
	//! Asserts a dynamic cast. Similar to the one in boost/cast.hpp
	template<typename Derived, typename Base>
	static inline Derived polymorphic_downcast(Base base)
	{
		Derived derived = dynamic_cast<Derived>(base);
		if (!derived)
			abort();
		return derived;
	}
	
	//! Asserts a dynamic cast or a null. Similar to the one in boost/cast.hpp
	template<typename Derived, typename Base>
	static inline Derived polymorphic_downcast_or_null(Base base)
	{
		if (!base)
			return 0;
		Derived derived = dynamic_cast<Derived>(base);
		if (!derived)
			abort();
		return derived;
	}
	
	//! Time or durations, in milliseconds
	struct UnifiedTime
	{
		//! storage for time
		typedef long long unsigned Value;
		//! time
		Value value;
		
		//! Constructor, set time to current time
		UnifiedTime();
		//! Constructor, from a specific number of ms
		UnifiedTime(Value ms);
		//! Constructor, from a specific number of seconds and ms
		UnifiedTime(Value seconds, Value milliseconds);
		
		//! Add times
		void operator +=(const UnifiedTime &that) { value += that.value; }
		//! Substract times
		void operator -=(const UnifiedTime &that) { value -= that.value; }
		//! Divide time by an amount
		void operator /=(const long long unsigned factor) {  assert(factor); value /= factor; }
		//! Multiply time by an amount
		void operator *=(const long long unsigned factor) { value *= factor; }
		//! Add times
		UnifiedTime operator +(const UnifiedTime &that) const { return UnifiedTime(value + that.value); }
		//! Substract times
		UnifiedTime operator -(const UnifiedTime &that) const { return UnifiedTime(value - that.value); }
		//! Divide time by an amount
		UnifiedTime operator /(const long long unsigned factor) const { assert(factor); return UnifiedTime(value / factor); }
		//! Multiply time by an amount
		UnifiedTime operator *(const long long unsigned factor) const { return UnifiedTime(value * factor); }
		bool operator <(const UnifiedTime &that) const { return value < that.value; }
		bool operator <=(const UnifiedTime &that) const { return value <= that.value; }
		bool operator >(const UnifiedTime &that) const { return value > that.value; }
		bool operator >=(const UnifiedTime &that) const { return value >= that.value; }
		bool operator ==(const UnifiedTime &that) const { return value == that.value; }
		bool operator !=(const UnifiedTime &that) const { return value != that.value; }
		
		//! Sleep for this amount of time
		void sleep() const;
		
		//! Create a human readable string with this time
		std::string toHumanReadableStringFromEpoch() const;
		
		//! Return the raw time string representing this time
		std::string toRawTimeString() const;
		
		//! Return the time from the raw time string
		static UnifiedTime fromRawTimeString(const std::string& rawTimeString);
	};
	
	//! Dump the current time to a stream
	void dumpTime(std::ostream &stream, bool raw = false);
	
	//! Transform a wstring into an UTF8 string, this function is thread-safe
	std::string WStringToUTF8(const std::wstring& s);
	
	//! Transform a UTF8 string into a wstring, this function is thread-safe
	std::wstring UTF8ToWString(const std::string& s);
	
	//! Update the XModem CRC (x^16 + x^12 + x^5 + 1 (0x1021)) with a wstring
	uint16 crcXModem(const uint16 oldCrc, const std::wstring& s);
	
	//! Update the XModem CRC (x^16 + x^12 + x^5 + 1 (0x1021)) with a uint16 value
	uint16 crcXModem(const uint16 oldCrc, const uint16 v);
	
	//! Split a string using given delimiters
	template<typename T>
	std::vector<T> split(const T& s, const T& delim);
	
	//! Split a string using whitespaces
	template<typename T>
	std::vector<T> split(const T& s);
	
	//! Join a sequence using operator += and adding delim in-between elements
	template<typename T>
	T join(typename std::vector<T>::const_iterator first, typename std::vector<T>::const_iterator last, const T& delim);
	
	//! Join a sequence using operator += and adding delim in-between elements
	template<typename C>
	typename C::value_type join(const C& values, const typename C::value_type& delim);
	
	//! Clamp a value to a range
	template<typename T>
	T clamp(T v, T minV, T maxV)
	{
		return v < minV ? minV : (v > maxV ? maxV : v);
	}
	
	/*@}*/
	
};

#endif
