/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

#ifndef __ASEBA_TYPES_H
#define __ASEBA_TYPES_H

#include <stdlib.h>

/** \addtogroup common */
/*@{*/

/* Typedefs to be able to run aseba-mcu on 32 bits plateforms too */

typedef signed long sint32;			//!< 32 bits signed integer
typedef unsigned long uint32;		//!< 32 bits unsigned integer
typedef signed short sint16;		//!< 16 bits signed integer
typedef unsigned short uint16;		//!< 16 bits unsigned integer
typedef signed char sint8;			//!< 8 bits signed integer
typedef unsigned char uint8;		//!< 8 bits unsigned integer

/*@}*/

#endif
