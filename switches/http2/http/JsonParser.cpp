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

#include "JsonParser.h"

namespace Aseba
{

	std::string parseJSONStringOrNumber(std::istream& stream)
	{
		// FIXME: use peek rather than get to avoid eating the character for the next element
	
		int c;
		std::string s;
		// eat everything until " include
		while (((c = stream.get()) != EOF) && (c != '"'))
		{
			if (std::isdigit(c) || c == '-')
				break;
			if (!std::isspace(c))
				throw JSONParserException("Invalid JSON string, non-whitespace before \"");
		}
		if (std::isdigit(c) || c == '-')
		{
			// we have a number
			s.push_back(c);
			while (((c = stream.get()) != EOF) && (!std::isspace(c)))
				s.push_back(c);
		}
		else
		{
			// we have a string
			if (c != '"')
				throw JSONParserException("Invalid JSON string, missing starting \"");
			// store everything until " excluded
			while (((c = stream.get()) != EOF) && (c != '"'))
				s.push_back(c);
			if (c != '"')
				throw JSONParserException("Invalid JSON string, missing ending \"");
		}
		return s;
	}
	
} // namespace Aseba
