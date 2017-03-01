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

#ifndef ASEBA_SWITCH_GLOBALS
#define ASEBA_SWITCH_GLOBALS

#include "../../common/utils/utils.h"

#ifndef _WIN32
#define COLOR_OUTPUT_ANSI(X) # X
#else // _WIN32
#define COLOR_OUTPUT_ANSI(X) ""
#endif // _WIN32

#define LOG_DUMP \
    if (!Aseba::_globals.dump) {} \
    else std::cout << COLOR_OUTPUT_ANSI(\x1B[30;1m), Aseba::dumpTime(std::cout, Aseba::_globals.rawTime), std::cout

#define LOG_VERBOSE \
    if (!Aseba::_globals.verbose) {} \
    else std::cout << COLOR_OUTPUT_ANSI(\x1B[0m), Aseba::dumpTime(std::cout, Aseba::_globals.rawTime), std::cout

#define LOG_ERROR \
	std::cerr << COLOR_OUTPUT_ANSI(\x1B[31m), Aseba::dumpTime(std::cerr, Aseba::_globals.rawTime), std::cerr

namespace Aseba
{
	//! Global switch-wide settings, set through command line arguments
	struct Globals
	{
		bool verbose; //!< whether to be verbose
		bool dump; //!< whether to print the content of all messages
		bool rawTime; //!< if true, use raw time for logging
	};
	
	extern Globals _globals;
	
}; // Aseba

#endif // ASEBA_SWITCH_GLOBALS

