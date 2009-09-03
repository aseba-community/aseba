/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
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

#ifndef ASEBA_UTILS_H
#define ASEBA_UTILS_H

#include <iostream>
#include <string>

namespace Aseba
{
	/**
	\defgroup utils General helper functions and classes
	*/
	/*@{*/
	
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
		//! Add times
		UnifiedTime operator +(const UnifiedTime &that) const { return UnifiedTime(value + that.value); }
		//! Substract times
		UnifiedTime operator -(const UnifiedTime &that) const { return UnifiedTime(value - that.value); }
		//! Time comparison
		bool operator <(const UnifiedTime &that) const { return value < that.value; }
		
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
	
	/*@}*/
	
};

#endif
