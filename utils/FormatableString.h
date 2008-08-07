/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
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

#ifndef FORMATABLESTRING_H
#define FORMATABLESTRING_H

#include <string>
#include <sstream>

namespace Aseba
{
	/** \addtogroup utils */
	/*@{*/
	
	/*!
	* string that can be used for argument substitution.
	* Example :
	* FormatableString fs("Hello %0");
	* cout << fs.arg("World");
	*/
	class FormatableString : public std::string
	{
		private:
			/*!
			* Next argument to be replaced.
			*/
			int argLevel;
		
			/*!
			* Replace the next argument by replacement.
			*/
			void proceedReplace(const std::string &replacement);
			
		public:
			
			FormatableString() : std::string(), argLevel(0) { }
			/*!
			* Creates a new FormatableString with format string set to s.
			* \param s A string with indicators for argument substitution.
			* Each indicator is the % symbol followed by a number. The number
			* is the index of the corresponding argument (starting at %0).
			*/
			FormatableString(const std::string &s)
		: std::string(s), argLevel(0) { }
			
			/*!
			* Replace the next arg by an int value.
			* \param value Value used to replace the current argument.
			* \param fieldWidth min width of the displayed number
			* \param base Radix of the number (8, 10 or 16)
			* \param fillChar Character used to pad the number to reach fieldWidth
			* \see arg(const T& value)
			*/
			FormatableString &arg(int value, int fieldWidth = 0, int base = 10, char fillChar = ' ');
			
			/*!
			* Replace the next arg by an int value.
			* \param value Value used to replace the current argument.
			* \param fieldWidth min width of the displayed number
			* \param base Radix of the number (8, 10 or 16)
			* \param fillChar Character used to pad the number to reach fieldWidth
			* \see arg(const T& value)
			*/
			FormatableString &arg(unsigned value, int fieldWidth = 0, int base = 10, char fillChar = ' ');
			
			/*!
			* Replace the next arg by a float value.
			* \param value Value used to replace the current argument.
			* \param fieldWidth min width of the displayed number.
			* \param precision Number of digits displayed.
			* \param fillChar Character used to pad the number to reach fieldWidth.
			* \see arg(const T& value)
			*/
			FormatableString &arg(float value, int fieldWidth = 0, int precision = 6, char fillChar = ' ');
			
			/*!
			* Replace the next arg by a value that can be passed to an ostringstream.
			* The first call to arg replace %0, the second %1, and so on.
			* \param value Value used to replace the current argument.
			*/
			template <typename T> FormatableString &arg(const T& value)
			{
				// transform value into std::string
				std::ostringstream oss;
				oss << value;
			
				proceedReplace(oss.str());
	
				// return reference to this so that .arg can proceed further
				return *this;
			}
			
			/*!
			* Affects a new value to the format string and reset the arguments
			* counter.
			* \param str New format string.
			*/
			FormatableString& operator=(const std::string& str) ;
	};
	
	/*@}*/
}

#endif // FORMATABLESTRING_H //
