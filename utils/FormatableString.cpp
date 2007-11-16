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

#include <cassert>
#include <string>
#include <sstream>
#include <ios>
#include <iostream>
#include <iomanip>

#include "FormatableString.h"

namespace Aseba
{
	/** \addtogroup utils */
	/*@{*/
	
	void FormatableString::proceedReplace(const std::string &replacement)
	{
		std::ostringstream search;
		search << "%" << this->argLevel;
		std::string::size_type pos = this->find(search.str(), 0);
		assert(pos != std::string::npos);
		this->replace(pos, search.str().length(), replacement);
		++argLevel;
	}
	
	FormatableString &FormatableString::arg(int value, int fieldWidth, int base, char fillChar)
	{
		std::ostringstream oss;
		oss << std::setbase(base);
		oss.width(fieldWidth);
		oss.fill(fillChar);
		
		// transform value into std::string
		oss << value;
	
		proceedReplace(oss.str());
		
		// return reference to this so that .arg can proceed further
		return *this;
	}
	
	FormatableString &FormatableString::arg(unsigned value, int fieldWidth, int base, char fillChar)
	{
		std::ostringstream oss;
		oss << std::setbase(base);
		oss.width(fieldWidth);
		oss.fill(fillChar);
		
		// transform value into std::string
		oss << value;
	
		proceedReplace(oss.str());
		
		// return reference to this so that .arg can proceed further
		return *this;
	}
	
	FormatableString &FormatableString::arg(float value, int fieldWidth, int precision, char fillChar)
	{
		std::ostringstream oss;
		oss.precision(precision);
		oss.width(fieldWidth);
		oss.fill(fillChar);
	
		oss.setf(oss.fixed, oss.floatfield);
		// transform value into std::string
		oss << value;
	
		proceedReplace(oss.str());
		
		// return reference to this so that .arg can proceed further
		return *this;
	}
	
	FormatableString &FormatableString::operator=(const std::string& str)
	{
		this->assign(str);
		this->argLevel = 0;
		return (*this);
	}
	
	/*@}*/
}
