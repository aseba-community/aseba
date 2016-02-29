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

#ifndef __POWER_OF_TWO_H
#define __POWER_OF_TWO_H

// helper functions for power of two numbers
namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	//! Return true if number is a power of two
	template <typename T>
	bool isPOT(T number)
	{
		if (number == 0)
			return true;
		while ((number & 1) == 0)
			number >>= 1;
		return number == 1;
	}
	
	//! If number is a power of two, number = (1 << shift) and this function returns shift, otherwise return value is undefined
	template <typename T>
	unsigned shiftFromPOT(T number)
	{
		unsigned i = 0;
		if (number == 0)
			return 0;
		while ((number & 1) == 0)
		{
			number >>= 1;
			i++;
		}
		return i;
	}

	/*@}*/
	
} // namespace Aseba

#endif
