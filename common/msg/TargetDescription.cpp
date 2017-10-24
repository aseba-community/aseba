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

#include "TargetDescription.h"
#include "../utils/utils.h"

namespace Aseba
{
	//! Compute the XModem CRC of the description, as defined in AS001 at https://aseba.wikidot.com/asebaspecifications
	uint16_t TargetDescription::crc() const
	{
		uint16_t crc(0);
		crc = crcXModem(crc, bytecodeSize);
		crc = crcXModem(crc, variablesSize);
		crc = crcXModem(crc, stackSize);
		for (const auto & namedVariable : namedVariables)
		{
			crc = crcXModem(crc, namedVariable.size);
			crc = crcXModem(crc, namedVariable.name);
		}
		for (const auto & localEvent : localEvents)
		{
			crc = crcXModem(crc, localEvent.name);
		}
		for (const auto & nativeFunction : nativeFunctions)
		{
			crc = crcXModem(crc, nativeFunction.name);
			for (size_t j = 0; j < nativeFunction.parameters.size(); ++j)
			{
				crc = crcXModem(crc, nativeFunction.parameters[j].size);
				crc = crcXModem(crc, nativeFunction.parameters[j].name);
			}
		}
		return crc;
	}
	
	//! Get a VariablesMap out of namedVariables, overwrite freeVariableIndex
	VariablesMap TargetDescription::getVariablesMap(unsigned& freeVariableIndex) const
	{
		freeVariableIndex = 0;
		VariablesMap variablesMap;
		for (const auto & namedVariable : namedVariables)
		{
			variablesMap[namedVariable.name] =
			std::make_pair(freeVariableIndex, namedVariable.size);
			freeVariableIndex += namedVariable.size;
		}
		return variablesMap;
	}
	
	//! Get a FunctionsMap out of nativeFunctions
	FunctionsMap TargetDescription::getFunctionsMap() const
	{
		FunctionsMap functionsMap;
		for (unsigned i = 0; i < nativeFunctions.size(); i++)
		{
			functionsMap[nativeFunctions[i].name] = i;
		}
		return functionsMap;
	}
} // namespace Aseba
