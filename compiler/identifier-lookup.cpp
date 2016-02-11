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

#include "compiler.h"
#include "tree.h"
#include "../common/consts.h"
#include "../common/utils/FormatableString.h"
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <limits>
#ifndef __APPLE__
	#include <malloc.h>
#endif

namespace Aseba
{
	/**
		\file identifier-lookup.cpp
		Functions for quick lookup of identifiers (variables, events, subroutines, native functions, constants) using maps.
		\addtogroup compiler
	*/
	/*@{*/

	//! Return the string version of this position
	std::wstring SourcePos::toWString() const
	{
		if (valid)
		{
			std::wostringstream oss;
			oss << row + 1 << ':' << column;
			return oss.str();
		}
		else
			return L"";
	}
	
	//! Return whether a named-value vector contains a certain value, by iterating
	bool NamedValuesVector::contains(const std::wstring& s, size_t* position) const
	{
		for (size_t i = 0; i < size(); i++)
		{
			if ((*this)[i].name == s)
			{
				if (position)
					*position = i;
				return true;
			}
		}
		return false;
	}
	
	//! Compute the edit distance between two vector-style containers, inspired from http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
	template <class T> unsigned int editDistance(const T& s1, const T& s2, const unsigned maxDist)
	{
		const size_t len1 = s1.size() + 1, len2 = s2.size() + 1;
		const size_t matSize(len1*len2);
		unsigned *d(reinterpret_cast<unsigned*>(alloca(matSize*sizeof(unsigned))));

		d[0] = 0;
		for(unsigned i = 1; i < len1; ++i)
			d[i*len2+0] = i;
		for(unsigned i = 1; i < len2; ++i)
			d[i] = i;

		for(unsigned i = 1; i < len1; ++i)
		{
			bool wasBelowMax(false);
			for(unsigned j = 1; j < len2; ++j)
			{
				const unsigned cost = std::min(std::min(
					d[(i - 1)*len2+j] + 1,
					d[i*len2+(j - 1)] + 1),
					d[(i - 1)*len2+(j - 1)] + (s1[i - 1] == s2[j - 1] ? 0 : 1)
				);
				if (cost < maxDist)
					wasBelowMax = true;
				d[i*len2+j] = cost;
			}
			if (!wasBelowMax)
				return maxDist;
		}
		return d[matSize-1];
	}
	
	//! Helper function to find for something in one of the map, using edit-distance to check for candidates if not found
	template <typename MapType>
	typename MapType::const_iterator findInTable(const MapType& map, const std::wstring& name, const SourcePos& pos, const ErrorCode notFoundError, const ErrorCode misspelledError)
	{
		typename MapType::const_iterator it(map.find(name));
		if (it == map.end())
		{
			const unsigned maxDist(3);
			std::wstring bestName;
			unsigned bestDist(std::numeric_limits<unsigned>::max());
			for (typename MapType::const_iterator jt(map.begin()); jt != map.end(); ++jt)
			{
				const std::wstring thatName(jt->first);
				const unsigned d(editDistance<std::wstring>(name, thatName, maxDist));
				if (d < bestDist && d < maxDist)
				{
					bestDist = d;
					bestName = thatName;
				}
			}
			if (bestDist < maxDist)
				throw TranslatableError(pos, misspelledError).arg(name).arg(bestName);
			else
				throw TranslatableError(pos, notFoundError).arg(name);
		}
		return it;
	}
	
	//! Look for a variable of a given name, and if found, return an iterator; if not, return an exception
	VariablesMap::const_iterator Compiler::findVariable(const std::wstring& varName, const SourcePos& varPos) const
	{
		return findInTable<VariablesMap>(variablesMap, varName, varPos, ERROR_VARIABLE_NOT_DEFINED, ERROR_VARIABLE_NOT_DEFINED_GUESS);
	}
	
	//! Look for a function of a given name, and if found, return an iterator; if not, return an exception
	FunctionsMap::const_iterator Compiler::findFunction(const std::wstring& funcName, const SourcePos& funcPos) const
	{
		return findInTable<FunctionsMap>(functionsMap, funcName, funcPos, ERROR_FUNCTION_NOT_DEFINED, ERROR_FUNCTION_NOT_DEFINED_GUESS);
	}
	
	//! Look for a constant of a given name, and if found, return an iterator; if not, return an exception
	Compiler::ConstantsMap::const_iterator Compiler::findConstant(const std::wstring& name, const SourcePos& pos) const
	{
		return findInTable<ConstantsMap>(constantsMap, name, pos, ERROR_CONSTANT_NOT_DEFINED, ERROR_CONSTANT_NOT_DEFINED_GUESS);
	}
	
	//! Return true if a constant of a given name exists
	bool Compiler::constantExists(const std::wstring& name) const 
	{
		return constantsMap.find(name) != constantsMap.end();
	}
	
	//! Look for a global event of a given name, and if found, return an iterator; if not, return an exception
	Compiler::EventsMap::const_iterator Compiler::findGlobalEvent(const std::wstring& name, const SourcePos& pos) const
	{
		try
		{
			return findInTable<EventsMap>(globalEventsMap, name, pos, ERROR_EVENT_NOT_DEFINED, ERROR_EVENT_NOT_DEFINED_GUESS);
		}
		catch (TranslatableError e)
		{
			if (allEventsMap.find(name) != allEventsMap.end())
				throw TranslatableError(pos, ERROR_EMIT_LOCAL_EVENT).arg(name);
			else
				throw e;
		}
	}
	
	Compiler::EventsMap::const_iterator Compiler::findAnyEvent(const std::wstring& name, const SourcePos& pos) const
	{
		return findInTable<EventsMap>(allEventsMap, name, pos, ERROR_EVENT_NOT_DEFINED, ERROR_EVENT_NOT_DEFINED_GUESS);
	}
	
	//! Look for a subroutine of a given name, and if found, return an iterator; if not, return an exception
	Compiler::SubroutineReverseTable::const_iterator Compiler::findSubroutine(const std::wstring& name, const SourcePos& pos) const
	{
		return findInTable<SubroutineReverseTable>(subroutineReverseTable, name, pos, ERROR_SUBROUTINE_NOT_DEFINED, ERROR_SUBROUTINE_NOT_DEFINED_GUESS);
	}
	
	//! Build variables and functions maps
	void Compiler::buildMaps()
	{
		assert(targetDescription);
		
		// erase tables
		implementedEvents.clear();
		subroutineTable.clear();
		subroutineReverseTable.clear();
		
		// fill variables map
		variablesMap = targetDescription->getVariablesMap(freeVariableIndex);
		
		// fill functions map
		functionsMap = targetDescription->getFunctionsMap();
		
		// fill contants maps
		constantsMap.clear();
		for (unsigned i = 0; i < commonDefinitions->constants.size(); i++)
		{
			const NamedValue &constant(commonDefinitions->constants[i]);
			constantsMap[constant.name] = constant.value;
		}
		
		// fill global events map
		globalEventsMap.clear();
		for (unsigned i = 0; i < commonDefinitions->events.size(); i++)
		{
			globalEventsMap[commonDefinitions->events[i].name] = i;
		}
		
		// fill all events map
		allEventsMap = globalEventsMap;
		for (unsigned i = 0; i < targetDescription->localEvents.size(); ++i)
		{
			allEventsMap[targetDescription->localEvents[i].name] = ASEBA_EVENT_LOCAL_EVENTS_START - i;
		}
	}
	
	/*@}*/
	
} // namespace Aseba
