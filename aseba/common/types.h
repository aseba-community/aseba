/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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
#include <stdint.h>

/** \addtogroup common */
/*@{*/

/* Aseba network protocol is little-endian, if big, swap
   We only need to swap 16-bit integers, as this is the largest
   word supported on Aseba targets. */
#ifdef __BIG_ENDIAN__
/* gcc-specific extension, but anyway __BIG_ENDIAN__ is for Mac PPC, which use gcc */
#define bswap16(v) ({uint16_t _v = v; _v = (_v << 8) | (_v >> 8);})
#else
#define bswap16(v) (v)
#endif

/*@}*/

#endif
