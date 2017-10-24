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

#ifndef ASEBA_TARGET_DESCRIPTION
#define ASEBA_TARGET_DESCRIPTION

#include <map>
#include <string>
#include <vector>

namespace Aseba
{
	/** \addtogroup msg */
	/*@{*/
	
	//! Lookup table for variables name => (pos, size))
	using VariablesMap = std::map<std::wstring, std::pair<unsigned, unsigned> >;
	
	//! Lookup table for functions (name => id in target description)
	using FunctionsMap = std::map<std::wstring, unsigned>;
	
	//! Description of target VM
	struct TargetDescription
	{
		//! Description of target VM named variable
		struct NamedVariable
		{
			NamedVariable(std::wstring name, unsigned size) : name(std::move(name)), size(size) {}
			NamedVariable() = default;
			
			std::wstring name; //!< name of the variable
			unsigned size = 0; //!< size of variable in words
		};
		
		//! Description of local event;
		struct LocalEvent
		{
			std::wstring name; //!< name of the event
			std::wstring description; //!< description (some short documentation) of the event
		};
		
		//! Typed parameter of native functions
		struct NativeFunctionParameter
		{
			NativeFunctionParameter(std::wstring name, int size) : name(std::move(name)), size(size) {}
			NativeFunctionParameter()  = default;
			
			std::wstring name; //!< name of the parameter
			int size = 0; //!< if > 0 size of the parameter, if < 0 template id, if 0 any size
		};
		
		//! Description of target VM native function
		struct NativeFunction
		{
			std::wstring name; //!< name of the function
			std::wstring description; //!< description (some short documentation) of the function
			std::vector<NativeFunctionParameter> parameters; //!< for each argument of the function, its size in words or, if negative, its template ID
		};
		
		std::wstring name; //!< node name
		unsigned protocolVersion{0}; //!< version of the aseba protocol
		
		unsigned bytecodeSize{0}; //!< total amount of bytecode space
		unsigned variablesSize{0}; //!< total amount of variables space
		unsigned stackSize{0}; //!< depth of execution stack
		
		std::vector<NamedVariable> namedVariables; //!< named variables
		std::vector<LocalEvent> localEvents; //!< events available locally on target
		std::vector<NativeFunction> nativeFunctions; //!< native functions
		
		TargetDescription()  = default;
		uint16_t crc() const;
		VariablesMap getVariablesMap(unsigned& freeVariableIndex) const;
		FunctionsMap getFunctionsMap() const;
	};
	
	/*@}*/
} // namespace Aseba

#endif // ASEBA_TARGET_DESCRIPTION
