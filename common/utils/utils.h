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
#include <functional>
#include <memory>
#include <cmath>
#include "../types.h"

// workaround for broken libstdc++ on Android
// see https://github.com/android-ndk/ndk/issues/82
#ifdef __ANDROID__
namespace std
{
    size_t log2(size_t v);
}
#endif // __ANDROID__

namespace Aseba
{
	/**
	\defgroup utils General helper functions and classes
	*/
	/*@{*/
	
	//! Asserts a dynamic cast. Similar to the one in boost/cast.hpp
	template<typename Derived, typename Base>
	static inline Derived polymorphic_downcast(Base base)
	{
		auto derived = dynamic_cast<Derived>(base);
		if (!derived)
			abort();
		return derived;
	}
	
	//! Asserts a dynamic cast or a null. Similar to the one in boost/cast.hpp
	template<typename Derived, typename Base>
	static inline Derived polymorphic_downcast_or_null(Base base)
	{
		if (!base)
			return nullptr;
		auto derived = dynamic_cast<Derived>(base);
		if (!derived)
			abort();
		return derived;
	}
	
	//! Create a unique_ptr by perfect forwarding, to be removed once we switch to C++14
	template<typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args&&... args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
	
	//! Hash references to T using their underlying pointer
	template<typename T>
	struct PointerHash
	{
		//! Use the pointer shifted right by the number of 0 LSB
		size_t operator()(const T& value) const
		{
			static const auto shift = (size_t)std::log2(1 + sizeof(&value));
			return (size_t)(&value) >> shift;
		}
	};
	
	//! Time or durations, in milliseconds
	struct UnifiedTime
	{
		//! storage for time
		using Value = unsigned long long;
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
		UnifiedTime operator +(const UnifiedTime &that) const { return {value + that.value}; }
		//! Substract times
		UnifiedTime operator -(const UnifiedTime &that) const { return {value - that.value}; }
		//! Divide time by an amount
		UnifiedTime operator /(const long long unsigned factor) const { assert(factor); return {value / factor}; }
		//! Multiply time by an amount
		UnifiedTime operator *(const long long unsigned factor) const { return {value * factor}; }
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
	
	// Soft timer
	class SoftTimer
	{
	public:
		//! A callback function
		using Callback = const std::function<void ()>;
		//! The callback function, cannot change once initialized
		Callback callback;
		//! The current period, 0 disables the timer
		double period;
		
	protected:
		//! time left until next call to callback
		double left = 0;
		
	public:
		//! Constructor, using callback and firing every period s, 0 disables the timer
		SoftTimer(Callback callback, double period);
		//! Step dt s, calls callback any necessary number of times
		void step(double dt);
		//! Set the period in s, 0 disables the timer
		void setPeriod(double period);
	};
	
	//! Transform a wstring into an UTF8 string, this function is thread-safe
	std::string WStringToUTF8(const std::wstring& s);
	
	//! Transform a UTF8 string into a wstring, this function is thread-safe
	std::wstring UTF8ToWString(const std::string& s);
	
	//! Update the XModem CRC (x^16 + x^12 + x^5 + 1 (0x1021)) with a wstring
	uint16_t crcXModem(const uint16_t oldCrc, const std::wstring& s);
	
	//! Update the XModem CRC (x^16 + x^12 + x^5 + 1 (0x1021)) with a uint16_t value
	uint16_t crcXModem(const uint16_t oldCrc, const uint16_t v);
	
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

namespace std
{
	//! Generic hash for reference_wrapper
	template<typename T>
	struct hash<reference_wrapper<T>>
	{
		//! Hashes the reference to the underlying value
		size_t operator()(const reference_wrapper<T>& r) const
		{
			return std::hash<T>()(r.get());
		}
	};
}

#endif
