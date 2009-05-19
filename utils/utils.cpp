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

#include <cstdlib>
#include <cstring>
#ifndef WIN32
	#include <sys/time.h>
#else // WIN32
	#include <sys/types.h>
	#include <sys/timeb.h>
#endif // WIN32
#include <time.h>
#include <errno.h>
#include <iomanip>
#include "utils.h"

namespace Aseba
{
	/** \addtogroup utils */
	/*@{*/
	
	//! Dump the current time to a stream
	void dumpTime(std::ostream &stream, bool raw)
	{
		stream << "[";
		if (raw)
		{
			#ifndef WIN32
			struct timeval tv;
			gettimeofday(&tv, NULL);
			stream << std::dec << tv.tv_sec << "." << (tv.tv_usec / 1000);
			#else // WIN32
			struct __timeb64 tv;
			_ftime64_s(&tv);
			stream << std::dec << tv.time << "." << tv.millitm;
			#endif // WIN32
			
		}
		else
		{
			time_t t;
			time(&t);
			char *timeString = ctime(&t);
			timeString[strlen(timeString) - 1] = 0;
			stream << timeString;
		}
		stream << "] ";
	}
	
	/*@}*/
}
