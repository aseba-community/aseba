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
	
	template<typename charT>
	void BasicFormatableString<charT>::proceedReplace(const S &replacement)
	{
		typename std::basic_ostringstream<charT> search;
		search << "%" << this->argLevel;
		typename S::size_type pos = this->find(search.str(), 0);
		assert(pos != S::npos);
		this->replace(pos, search.str().length(), replacement);
		++argLevel;
	}
	
	template<typename charT>
	template<typename intT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::argInt(intT value, int fieldWidth, int base, charT fillChar)
	{
		typename std::basic_ostringstream<charT> oss;
		oss << std::setbase(base);
		oss.width(fieldWidth);
		oss.fill(fillChar);
		
		// transform value into S
		oss << value;
	
		proceedReplace(oss.str());
		
		// return reference to this so that .arg can proceed further
		return *this;
	}
	
	template<typename charT>
	template<typename floatT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::argFloat(floatT value, int fieldWidth, int precision, charT fillChar)
	{
		typename std::basic_ostringstream<charT> oss;
		oss.precision(precision);
		oss.width(fieldWidth);
		oss.fill(fillChar);
	
		oss.setf(oss.fixed, oss.floatfield);
		// transform value into S
		oss << value;
	
		proceedReplace(oss.str());
		
		// return reference to this so that .arg can proceed further
		return *this;
	}
	
	template<typename charT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::arg(int value, int fieldWidth, int base, charT fillChar)
	{
		return argInt<int>(value, fieldWidth, base, fillChar);
	}
	
	template<typename charT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::arg(long int value, int fieldWidth, int base, charT fillChar)
	{
		return argInt<long int>(value, fieldWidth, base, fillChar);
	}

	template<typename charT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::arg(unsigned value, int fieldWidth, int base, charT fillChar)
	{
		return argInt<unsigned>(value, fieldWidth, base, fillChar);
	}
	
	template<typename charT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::arg(float value, int fieldWidth, int precision, charT fillChar)
	{
		return argFloat<float>(value, fieldWidth, precision, fillChar);
	}
	
	template<typename charT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::arg(double value, int fieldWidth, int precision, charT fillChar)
	{
		return argFloat<double>(value, fieldWidth, precision, fillChar);
	}
	
	template<typename charT>
	BasicFormatableString<charT> &BasicFormatableString<charT>::operator=(const S& str)
	{
		this->assign(str);
		this->argLevel = 0;
		return (*this);
	}
	
	template class BasicFormatableString<char>;
	template class BasicFormatableString<wchar_t>;
	
	/*@}*/
}
