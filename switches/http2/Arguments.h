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

#ifndef ASEBA_SWITCH_ARGUMENTS
#define ASEBA_SWITCH_ARGUMENTS

#include <vector>
#include <string>

namespace Aseba
{
	//! The description of a command-line argument
	struct ArgumentDescription
	{
		std::string name; //!< name of the argument
		int paramCount; //!< number of parameters of the argument
	};

	//! A command-line argument filled with values
	struct Argument: public ArgumentDescription
	{
		//! Construct using a copy of base class
		Argument(const ArgumentDescription& descr): 
			ArgumentDescription(descr)
		{}
		
		std::vector<std::string> values; //!< values as string, could be converted by user class
	};

	//! Possible command-line arguments
	typedef std::vector<ArgumentDescription> ArgumentDescriptions;
	//! Filled command-line arguments
	typedef std::vector<Argument> Arguments;

} // namespace Aseba

#endif // ASEBA_SWITCH_ARGUMENTS
