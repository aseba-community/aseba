/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
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
	
	//! Compile a new condition
	//! \param source stream to read the source code from
	//! \param bytecode destination array for bytecode
	//! \param error error is copied there on error
	//! \param dump stream to send dump messages to
	//! \param verbose if true, produce full dump
	//! \return returns true on success 
	bool Compiler::compile(std::istream& source, BytecodeVector& bytecode, VariablesNamesVector &variablesNames, unsigned& allocatedVariablesCount, Error &errorDescription, std::ostream &dump, bool verbose)
	{
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
		
		if (verbose)
		{
			dump << "Dumping tokens:\n";
			dumpTokens(dump);
			dump << "\n\n";
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
		
		if (verbose)
		{
			dump << "Syntax tree before optimisation:\n" << std::endl;
			program->dump(dump, indent);
			dump << "\n\n";
			dump << "Optimizations:\n" << std::endl;
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
		
		if (verbose)
		{
			dump << "\n\n";
			dump << "Syntax tree after optimization:" << std::endl;
			program->dump(dump, indent);
			dump << "\n\n";
		}
		
		// fill variables names vector
		variablesNames.resize(targetDescription->variablesSize, "");
		for (VariablesMap::const_iterator it = variablesMap.begin(); it != variablesMap.end(); ++it)
		{
			unsigned pos = it->second.first;
			unsigned size = it->second.second;
			assert(pos + size <= variablesNames.size());
			for (unsigned i = pos; i < pos + size; i++)
				variablesNames[i] = it->first;
		}
		allocatedVariablesCount = freeVariableIndex;
		
		// code generation
		PreLinkBytecode preLinkBytecode;
		program->emit(preLinkBytecode);
		delete program;
		
		// fix-up (add of missing STOP bytecodes at code generation)
		preLinkBytecode.fixup();
		
		// linking (flattening of complex structure into linear vector)
		link(preLinkBytecode, bytecode);
		
		if (verbose)
		{
			dump << "Bytecode:" << std::endl;
			disassemble(bytecode, dump);
			dump << "\n\n";
		}
		
		return true;
	}
	
	void Compiler::link(const PreLinkBytecode& preLinkBytecode, BytecodeVector& bytecode) const
	{
		bytecode.clear();
		
		// event vector table size
		unsigned addr = preLinkBytecode.size() * 2 + 1;
		bytecode.push_back(addr);
		
		// events
		for (std::map<unsigned, BytecodeVector>::const_iterator it = preLinkBytecode.begin();
			it != preLinkBytecode.end();
			++it
		)
		{
			bytecode.push_back(it->first);		// id
			bytecode.push_back(addr);			// addr
			addr += it->second.size();			// next bytecode addr
		}
		
		// bytecode
		for (std::map<unsigned, BytecodeVector>::const_iterator it = preLinkBytecode.begin();
			it != preLinkBytecode.end();
			++it
		)
			std::copy(it->second.begin(),
			it->second.end(),
			std::back_inserter(bytecode));
	}
	
	void Compiler::disassemble(BytecodeVector& bytecode, std::ostream& dump) const
	{
		// address of threads
		std::map<unsigned, unsigned> eventAddr;
		
		// event table
		unsigned eventVectSize = bytecode[0];
		unsigned pc = 1;
		while (pc < eventVectSize)
		{
			eventAddr[bytecode[pc + 1]] = bytecode[pc];
			pc += 2;
		}
		unsigned eventCount = (eventVectSize - 1 ) / 2;
		dump << "Disassembling " << eventCount << " segments:\n";
		
		// bytecode
		while (pc < bytecode.size())
		{
			if (eventAddr.find(pc) != eventAddr.end())
			{
				unsigned eventId = eventAddr[pc];
				if (eventId == ASEBA_EVENT_INIT)
					dump << "init:       ";
				else if (eventId == ASEBA_EVENT_PERIODIC)
					dump << "periodic:   ";
				else if (eventId < eventsNames->size())
					dump << "event " << (*eventsNames)[eventId] << " : ";
				else
					dump << "event " << std::setw(3) << eventId << ":  ";
			}
			else
				dump << "            ";
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
				dump << "LOAD_INDIRECT\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_STORE_INDIRECT:
				dump << "STORE_INDIRECT\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_UNARY_ARITHMETIC:
				dump << "UNARY_ARITHMETIC -\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_BINARY_ARITHMETIC:
				dump << "BINARY_ARITHMETIC ";
				dump << binaryOperatorToString((AsebaBinaryOperator)(bytecode[pc] & 0xf)) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_JUMP:
				dump << "JUMP " << ((signed short)(bytecode[pc] << 4) >> 4) << "\n";
				pc++;
				break;
				
				case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
				dump << "CONDITIONAL_BRANCH ";
				dump << comparaisonOperatorToString((AsebaComparaison)(bytecode[pc] & 0x3)) << " ";
				if (bytecode[pc] & (1 << 2))
					dump << "(edge) ";
				dump << ((signed short)bytecode[pc+1]) << " ";
				dump << ((signed short)bytecode[pc+2]) << "\n";
				pc += 3;
				break;
				
				case ASEBA_BYTECODE_EMIT:
				{
					unsigned eventId = (bytecode[pc] & 0x0fff);
					dump << "EMIT ";
					if (eventId < eventsNames->size())
						dump << (*eventsNames)[eventId];
					else
						dump << eventId;
					dump << " addr " << bytecode[pc+1] << " size " << bytecode[pc+2] << "\n";
					pc += 3;
				}
				break;
				
				case ASEBA_BYTECODE_CALL:
				dump << "CALL " << (bytecode[pc] & 0x0fff) << "\n";
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
} // Aseba
