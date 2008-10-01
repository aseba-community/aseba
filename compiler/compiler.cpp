/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
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

#include "compiler.h"
#include "../common/consts.h"
#include "tree.h"
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/

	//! Return the string version of this position
	std::string SourcePos::toString() const
	{
		if (valid)
		{
			std::ostringstream oss;
			oss << row + 1 << ":" << column;
			return oss.str();
		}
		else
			return "";
	}
	
	bool NamedValuesVector::contains(const std::string& s) const
	{
		for (size_t i = 0; i < size(); i++)
			if ((*this)[i].name == s)
				return true;
		return false;
	}
	
	//! Constructor. You must setup a description using setTargetDescription() before any call to compile().
	Compiler::Compiler()
	{
		targetDescription = 0;
		commonDefinitions = 0;
	}
	
	//! Set the description of the target as returned by the microcontroller. You must call this function before any call to compile().
	void Compiler::setTargetDescription(const TargetDescription *description)
	{
		targetDescription = description;
		buildMaps();
	}
	
	//! Set the common definitions, such as events or some constants
	void Compiler::setCommonDefinitions(const CommonDefinitions *definitions)
	{
		commonDefinitions = definitions;
	}
	
	//! Compile a new condition
	//! \param source stream to read the source code from
	//! \param bytecode destination array for bytecode
	//! \param allocatedVariablesCount amount of allocated variables
	//! \param errorDescription error is copied there on error
	//! \param dump stream to send dump messages to
	//! \return returns true on success 
	bool Compiler::compile(std::istream& source, BytecodeVector& bytecode, unsigned& allocatedVariablesCount, Error &errorDescription, std::ostream* dump)
	{
		assert(targetDescription);
		
		Node *program;
		unsigned indent = 0;
		
		// we need to build maps at each compilation in case previous ones produced errors and messed maps up
		buildMaps();
		if (freeVariableIndex > targetDescription->variablesSize)
		{
			errorDescription = Error(SourcePos(), "Broken target description: not enough room for internal variables");
			return false;
		}
		
		// tokenization
		try
		{
			tokenize(source);
		}
		catch (Error error)
		{
			errorDescription = error;
			return false;
		}
		
		if (dump)
		{
			*dump << "Dumping tokens:\n";
			dumpTokens(*dump);
			*dump << "\n\n";
		}
		
		// parsing
		try
		{
			program = parseProgram();
		}
		catch (Error error)
		{
			errorDescription = error;
			return false;
		}
		
		if (dump)
		{
			*dump << "Syntax tree before optimisation:\n" << std::endl;
			program->dump(*dump, indent);
			*dump << "\n\n";
			*dump << "Optimizations:\n" << std::endl;
		}
		
		// optimization
		try
		{
			program = program->optimize(dump);
		}
		catch (Error error)
		{
			delete program;
			errorDescription = error;
			return false;
		}
		
		if (dump)
		{
			*dump << "\n\n";
			*dump << "Syntax tree after optimization:" << std::endl;
			program->dump(*dump, indent);
			*dump << "\n\n";
		}
		
		// set the number of allocated variables
		allocatedVariablesCount = freeVariableIndex;
		
		// code generation
		PreLinkBytecode preLinkBytecode;
		program->emit(preLinkBytecode);
		delete program;
		
		// fix-up (add of missing STOP and RET bytecodes at code generation)
		preLinkBytecode.fixup(subroutineTable);
		
		// TODO: check recursive calls for stack overflow
		
		// linking (flattening of complex structure into linear vector)
		if (!link(preLinkBytecode, bytecode))
		{
			errorDescription = Error(SourcePos(), "Script too big for target bytecode size.");
			return false;
		}
		
		if (dump)
		{
			*dump << "Bytecode:" << std::endl;
			disassemble(bytecode, preLinkBytecode, *dump);
			*dump << "\n\n";
		}
		
		return true;
	}
	
	//! Create the final bytecode for a microcontroller
	bool Compiler::link(const PreLinkBytecode& preLinkBytecode, BytecodeVector& bytecode)
	{
		bytecode.clear();
		
		// event vector table size
		unsigned addr = preLinkBytecode.events.size() * 2 + 1;
		bytecode.push_back(addr);
		
		// events
		for (PreLinkBytecode::EventsBytecode::const_iterator it = preLinkBytecode.events.begin();
			it != preLinkBytecode.events.end();
			++it
		)
		{
			bytecode.push_back(it->first);		// id
			bytecode.push_back(addr);			// addr
			addr += it->second.size();			// next bytecode addr
		}
		
		// evPreLinkBytecode::ents bytecode
		for (PreLinkBytecode::EventsBytecode::const_iterator it = preLinkBytecode.events.begin();
			it != preLinkBytecode.events.end();
			++it
		)
		{
			std::copy(it->second.begin(), it->second.end(), std::back_inserter(bytecode));
		}
		
		// subrountines bytecode
		for (PreLinkBytecode::SubroutinesBytecode::const_iterator it = preLinkBytecode.subroutines.begin();
			it != preLinkBytecode.subroutines.end();
			++it
		)
		{
			subroutineTable[it->first].address = bytecode.size();
			std::copy(it->second.begin(), it->second.end(), std::back_inserter(bytecode));
		}
		
		// resolve subroutines call addresses
		for (size_t pc = 0; pc < bytecode.size();)
		{
			switch (bytecode[pc] >> 12)
			{
				case ASEBA_BYTECODE_SUB_CALL:
				{
					unsigned id = bytecode[pc] & 0x0fff;
					assert(id < subroutineTable.size());
					unsigned address = subroutineTable[id].address;
					bytecode[pc].bytecode &= 0xf000;
					bytecode[pc].bytecode |= address;
					pc += 1;
				}
				break;
				
				case ASEBA_BYTECODE_LARGE_IMMEDIATE:
				case ASEBA_BYTECODE_LOAD_INDIRECT:
				case ASEBA_BYTECODE_STORE_INDIRECT:
				case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
					pc += 2;
				break;
				
				case ASEBA_BYTECODE_EMIT:
					pc += 3;
				break;
				
				default:
					pc += 1;
				break;
			}
		}
		
		// check size
		return bytecode.size() <= targetDescription->bytecodeSize;
	}
	
	//! Disassemble a microcontroller bytecode and dump it
	void Compiler::disassemble(BytecodeVector& bytecode, const PreLinkBytecode& preLinkBytecode, std::ostream& dump) const
	{
		// address of threads and subroutines
		std::map<unsigned, unsigned> eventAddr;
		std::map<unsigned, unsigned> subroutinesAddr;
		
		// build subroutine map
		for (size_t id = 0; id < subroutineTable.size(); ++id)
			subroutinesAddr[subroutineTable[id].address] = id;
		
		// event table
		unsigned eventVectSize = bytecode[0];
		unsigned pc = 1;
		while (pc < eventVectSize)
		{
			eventAddr[bytecode[pc + 1]] = bytecode[pc];
			pc += 2;
		}
		unsigned eventCount = (eventVectSize - 1 ) / 2;
		dump << "Disassembling " << eventCount + subroutineTable.size() << " segments:\n";
		
		// bytecode
		while (pc < bytecode.size())
		{
			if (eventAddr.find(pc) != eventAddr.end())
			{
				unsigned eventId = eventAddr[pc];
				if (eventId == ASEBA_EVENT_INIT)
					dump << "init:       ";
				else
				{
					if (eventId < 0x1000)
					{
						if (eventId < commonDefinitions->events.size())
							dump << "event " << commonDefinitions->events[eventId].name << ": ";
						else
							dump << "unknown global event " << eventId << ": ";
					}
					else
					{
						int index = ASEBA_EVENT_LOCAL_EVENTS_START - eventId;
						if (index < targetDescription->localEvents.size())
							dump << "event " << targetDescription->localEvents[index].name << ": ";
						else
							dump << "unknown local event " << index << ": ";
					}
				}
				
				PreLinkBytecode::EventsBytecode::const_iterator it = preLinkBytecode.events.find(eventId);
				assert(it != preLinkBytecode.events.end());
				dump << " (max stack " << it->second.maxStackDepth << ")\n";
			}
			
			if (subroutinesAddr.find(pc) != subroutinesAddr.end())
			{
				PreLinkBytecode::EventsBytecode::const_iterator it = preLinkBytecode.subroutines.find(subroutinesAddr[pc]);
				assert(it != preLinkBytecode.subroutines.end());
				dump << "sub " << subroutineTable[subroutinesAddr[pc]].name << ": (max stack " << it->second.maxStackDepth << ")\n";
			}
			
			dump << "    ";
			dump << pc << " (" << bytecode[pc].line << ") : ";
			switch (bytecode[pc] >> 12)
			{
				case ASEBA_BYTECODE_STOP:
				dump << "STOP\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_SMALL_IMMEDIATE:
				dump << "SMALL_IMMEDIATE " << ((signed short)(bytecode[pc] << 4) >> 4) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_LARGE_IMMEDIATE:
				dump << "LARGE_IMMEDIATE " << ((signed short)bytecode[pc+1]) << "\n";
				pc += 2;
				break;
				
				case ASEBA_BYTECODE_LOAD:
				dump << "LOAD " << (bytecode[pc] & 0x0fff) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_STORE:
				dump << "STORE " << (bytecode[pc] & 0x0fff) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_LOAD_INDIRECT:
				dump << "LOAD_INDIRECT in array at " << (bytecode[pc] & 0x0fff) << " of size " << bytecode[pc+1] << "\n";
				pc += 2;
				break;
				
				case ASEBA_BYTECODE_STORE_INDIRECT:
				dump << "STORE_INDIRECT in array at " << (bytecode[pc] & 0x0fff) << " of size " << bytecode[pc+1] << "\n";
				pc += 2;
				break;
				
				case ASEBA_BYTECODE_UNARY_ARITHMETIC:
				dump << "UNARY_ARITHMETIC ";
				dump << unaryOperatorToString((AsebaUnaryOperator)(bytecode[pc] & ASEBA_UNARY_OPERATOR_MASK)) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_BINARY_ARITHMETIC:
				dump << "BINARY_ARITHMETIC ";
				dump << binaryOperatorToString((AsebaBinaryOperator)(bytecode[pc] & ASEBA_BINARY_OPERATOR_MASK)) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_JUMP:
				dump << "JUMP " << ((signed short)(bytecode[pc] << 4) >> 4) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
				dump << "CONDITIONAL_BRANCH ";
				dump << binaryOperatorToString((AsebaBinaryOperator)(bytecode[pc] & ASEBA_BINARY_OPERATOR_MASK)) << " ";
				if (bytecode[pc] & (1 << ASEBA_IF_IS_WHEN_BIT))
					dump << "(edge) ";
				dump << ((signed short)bytecode[pc+1]) << "\n";
				pc += 2;
				break;
				
				case ASEBA_BYTECODE_EMIT:
				{
					unsigned eventId = (bytecode[pc] & 0x0fff);
					dump << "EMIT ";
					if (eventId < commonDefinitions->events.size())
						dump << commonDefinitions->events[eventId].name;
					else
						dump << eventId;
					dump << " addr " << bytecode[pc+1] << " size " << bytecode[pc+2] << "\n";
					pc += 3;
				}
				break;
				
				case ASEBA_BYTECODE_NATIVE_CALL:
				dump << "CALL " << (bytecode[pc] & 0x0fff) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_SUB_CALL:
				{
					unsigned address = (bytecode[pc] & 0x0fff);
					std::string name("unknown");
					for (size_t i = 0; i < subroutineTable.size(); i++)
						if (subroutineTable[i].address == address)
							name = subroutineTable[i].name;
					dump << "SUB_CALL to " << name << " @ " << address << "\n";
					pc++;
				}
				break;
				
				case ASEBA_BYTECODE_SUB_RET:
				dump << "SUB_RET\n";
				pc++;
				break;
				
				default:
				dump << "?\n";
				pc++;
				break;
			}
		}
	}
	
	//! Build variables and functions maps
	void Compiler::buildMaps()
	{
		assert(targetDescription);
		freeVariableIndex = 0;
		
		// erase tables
		implementedEvents.clear();
		subroutineTable.clear();
		subroutineReverseTable.clear();
		
		// fill variables map
		variablesMap.clear();
		for (unsigned i = 0; i < targetDescription->namedVariables.size(); i++)
		{
			variablesMap[targetDescription->namedVariables[i].name] =
				std::make_pair(freeVariableIndex, targetDescription->namedVariables[i].size);
			freeVariableIndex += targetDescription->namedVariables[i].size;
		}
		
		// fill functions map
		functionsMap.clear();
		for (unsigned i = 0; i < targetDescription->nativeFunctions.size(); i++)
		{
			functionsMap[targetDescription->nativeFunctions[i].name] = i;
		}
	}
	
	/*@}*/
	
} // Aseba
