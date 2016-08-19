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

#include "Arguments.h"

namespace Aseba
{
	//! Parse the current argument arg in whose parameters start at argCounter in argv
	//! Return true as soon as an argument name is matched, and update parseArgs, return false if no match is found
	bool ArgumentDescriptions::parse(const char* arg, int argc, const char* argv[], int& argCounter, Arguments& parsedArgs)
	{
		for (auto const& argDescr: *this)
		{
			if (argDescr.name == arg)
			{
				Argument argument(argDescr);
				for (int i = 0; (i < argDescr.paramCount) && (argCounter < argc); ++i)
					argument.values.push_back(argv[argCounter++]);
				parsedArgs.push_back(argument);
				return true;
			}
		}
		return false;
	}
	
} // namespace Aseba
