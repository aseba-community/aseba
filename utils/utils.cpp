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

#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include "utils.h"

namespace Aseba
{
	void read(int fd, void *buf, size_t count)
	{
		char *ptr = (char *)buf;
		while (count)
		{
			ssize_t len = ::read(fd, ptr, count);
			if (len < 0)
			{
				throw Exception::FileDescriptorError(fd, errno);
			}
			else if (len == 0)
			{
				throw Exception::FileDescriptorClosed(fd);
			}
			else
			{
				ptr += len;
				count -= len;
			}
		}
	}
	
	void write(int fd, const void *buf, size_t count)
	{
		const char *ptr = (const char *)buf;
		while (count)
		{
			ssize_t len = ::write(fd, ptr, count);
			if (len < 0)
			{
				throw Exception::FileDescriptorError(fd, errno);
			}
			else if (len == 0)
			{
				throw Exception::FileDescriptorClosed(fd);
			}
			else
			{
				ptr += len;
				count -= len;
			}
		}
	}
	
	void dumpTime(std::ostream &stream)
	{
		stream << "[";
		time_t t;
		time(&t);
		char *timeString = ctime(&t);
		timeString[strlen(timeString) - 1] = 0;
		stream << timeString;
		stream << "] ";
	}
}
