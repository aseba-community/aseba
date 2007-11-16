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

#ifndef ASEBA_UTILS_H
#define ASEBA_UTILS_H

#include <iostream>

namespace Aseba
{
	/**
	\defgroup utils General helper functions and classes
	*/
	/*@{*/

	namespace Exception
	{
		//! Parent class of exceptions on file descriptors
		struct FileDescriptor
		{
			FileDescriptor(int fd) : fd(fd) {}
			int fd;
		};
	
		//! An access on a file descriptor produced an error
		struct FileDescriptorError : public FileDescriptor
		{
			FileDescriptorError(int fd, int errNumber) : FileDescriptor(fd), errNumber(errNumber) {}
			int errNumber;
		};
		
		//! A file descriptor has been closed
		struct FileDescriptorClosed : public FileDescriptor
		{
			FileDescriptorClosed(int fd) : FileDescriptor(fd) {}
		};
	}
	
	void read(int fd, void *buf, size_t count);
	
	void write(int fd, const void *buf, size_t count);
	
	void dumpTime(std::ostream &stream);
	
	/*@}*/
	
};

#endif
