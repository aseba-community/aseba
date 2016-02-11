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
#include "errors_code.h"
#include "../common/consts.h"
#include "../common/utils/utils.h"
#include "../common/utils/FormatableString.h"
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <limits>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	//! Compute the XModem CRC of the description, as defined in AS001 at https://aseba.wikidot.com/asebaspecifications
	uint16 TargetDescription::crc() const
	{
		uint16 crc(0);
		crc = crcXModem(crc, bytecodeSize);
		crc = crcXModem(crc, variablesSize);
		crc = crcXModem(crc, stackSize);
		for (size_t i = 0; i < namedVariables.size(); ++i)
		{
			crc = crcXModem(crc, namedVariables[i].size);
			crc = crcXModem(crc, namedVariables[i].name);
		}
		for (size_t i = 0; i < localEvents.size(); ++i)
		{
			crc = crcXModem(crc, localEvents[i].name);
		}
		for (size_t i = 0; i < nativeFunctions.size(); ++i)
		{
			crc = crcXModem(crc, nativeFunctions[i].name);
			for (size_t j = 0; j < nativeFunctions[i].parameters.size(); ++j)
			{
				crc = crcXModem(crc, nativeFunctions[i].parameters[j].size);
				crc = crcXModem(crc, nativeFunctions[i].parameters[j].name);
			}
		}
		return crc;
	}
	
	//! Get a VariablesMap out of namedVariables, overwrite freeVariableIndex
	VariablesMap TargetDescription::getVariablesMap(unsigned& freeVariableIndex) const
	{
		freeVariableIndex = 0;
		VariablesMap variablesMap;
		for (unsigned i = 0; i < namedVariables.size(); i++)
		{
			variablesMap[namedVariables[i].name] =
			std::make_pair(freeVariableIndex, namedVariables[i].size);
			freeVariableIndex += namedVariables[i].size;
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
	
	//! Return the number of words this element takes in memory
	unsigned BytecodeElement::getWordSize() const
	{
		switch (bytecode >> 12)
		{
			case ASEBA_BYTECODE_LARGE_IMMEDIATE:
			case ASEBA_BYTECODE_LOAD_INDIRECT:
			case ASEBA_BYTECODE_STORE_INDIRECT:
			case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
			return 2;
			
			case ASEBA_BYTECODE_EMIT:
			return 3;
			
			default:
			return 1;
		}
	}
	
	//! Constructor. You must setup a description using setTargetDescription() before any call to compile().
	Compiler::Compiler()
	{
		targetDescription = 0;
		commonDefinitions = 0;
		freeVariableIndex = 0;
		endVariableIndex = 0;
		TranslatableError::setTranslateCB(ErrorMessages::defaultCallback);
	}
	
	//! Set the description of the target as returned by the microcontroller. You must call this function before any call to compile().
	void Compiler::setTargetDescription(const TargetDescription *description)
	{
		targetDescription = description;
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
	bool Compiler::compile(std::wistream& source, BytecodeVector& bytecode, unsigned& allocatedVariablesCount, Error &errorDescription, std::wostream* dump)
	{
		assert(targetDescription);
		assert(commonDefinitions);
		
		unsigned indent = 0;
		
		// we need to build maps at each compilation in case previous ones produced errors and messed maps up
		buildMaps();
		if (freeVariableIndex > targetDescription->variablesSize)
		{
			errorDescription = TranslatableError(SourcePos(), ERROR_BROKEN_TARGET).toError();
			return false;
		}
		
		// tokenization
		try
		{
			tokenize(source);
		}
		catch (TranslatableError error)
		{
			errorDescription = error.toError();
			return false;
		}
		
		if (dump)
		{
			*dump << "Dumping tokens:\n";
			dumpTokens(*dump);
			*dump << "\n\n";
		}
		
		// parsing
		std::auto_ptr<Node> program;
		try
		{
			program.reset(parseProgram());
		}
		catch (TranslatableError error)
		{
			errorDescription = error.toError();
			return false;
		}
		
		if (dump)
		{
			*dump << "Vectorial syntax tree:\n";
			program->dump(*dump, indent);
			*dump << "\n\n";
			*dump << "Checking the vectors' size:\n";
		}
		
		// check vectors' size
		try
		{
			program->checkVectorSize();
		}
		catch(TranslatableError error)
		{
			errorDescription = error.toError();
			return false;
		}

		if (dump)
		{
			*dump << "Ok\n";
			*dump << "\n\n";
			*dump << "Expanding the syntax tree...\n";
		}

		// expand the syntax tree to Aseba-like syntax
		try
		{
			Node* expandedProgram(program->expandAbstractNodes(dump));
			program.release();
			program.reset(expandedProgram);
		}
		catch (TranslatableError error)
		{
			errorDescription = error.toError();
			return false;
		}

		if (dump)
		{
			*dump << "Expanded syntax tree (pass 1):\n";
			program->dump(*dump, indent);
			*dump << "\n\n";
			*dump << "Second pass for vectorial operations:\n";
		}

		// expand the vectorial nodes into scalar operations
		try
		{
			Node* expandedProgram(program->expandVectorialNodes(dump, this));
			program.release();
			program.reset(expandedProgram);
		}
		catch (TranslatableError error)
		{
			errorDescription = error.toError();
			return false;
		}

		if (dump)
		{
			*dump << "Expanded syntax tree (pass 2):\n";
			program->dump(*dump, indent);
			*dump << "\n\n";
			*dump << "Type checking:\n";
		}

		// typecheck
		try
		{
			program->typeCheck(this);
		}
		catch(TranslatableError error)
		{
			errorDescription = error.toError();
			return false;
		}
		
		if (dump)
		{
			*dump << "correct.\n";
			*dump << "\n\n";
			*dump << "Optimizations:\n";
		}
		
		// optimization
		try
		{
			Node* optimizedProgram(program->optimize(dump));
			program.release();
			program.reset(optimizedProgram);
		}
		catch (TranslatableError error)
		{
			errorDescription = error.toError();
			return false;
		}
		
		if (dump)
		{
			*dump << "\n\n";
			*dump << "Syntax tree after optimization:\n";
			program->dump(*dump, indent);
			*dump << "\n\n";
		}
		
		// set the number of allocated variables
		allocatedVariablesCount = freeVariableIndex;
		
		if (dump)
		{
			const float fillPercentage = float(allocatedVariablesCount * 100.f) / float(targetDescription->variablesSize);
			*dump << "Using " << allocatedVariablesCount << " on " << targetDescription->variablesSize << " (" << fillPercentage << " %) words of variable space\n";
			*dump << "\n\n";
		}
		
		// code generation
		PreLinkBytecode preLinkBytecode;
		program->emit(preLinkBytecode);
		
		// fix-up (add of missing STOP and RET bytecodes at code generation)
		preLinkBytecode.fixup(subroutineTable);
		
		// stack check
		if (!verifyStackCalls(preLinkBytecode))
		{
			errorDescription = TranslatableError(SourcePos(), ERROR_STACK_OVERFLOW).toError();
			return false;
		}
		
		// linking (flattening of complex structure into linear vector)
		if (!link(preLinkBytecode, bytecode))
		{
			errorDescription = TranslatableError(SourcePos(), ERROR_SCRIPT_TOO_BIG).toError();
			return false;
		}
		
		if (dump)
		{
			*dump << "Bytecode:\n";
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
			BytecodeElement &element(bytecode[pc]);
			if (element.bytecode >> 12 == ASEBA_BYTECODE_SUB_CALL)
			{
				const unsigned id = element.bytecode & 0x0fff;
				assert(id < subroutineTable.size());
				const unsigned address = subroutineTable[id].address;
				element.bytecode &= 0xf000;
				element.bytecode |= address;
			}
			pc += element.getWordSize();
		}
		
		// check size
		return bytecode.size() <= targetDescription->bytecodeSize;
	}
	
	//! Change "stop" bytecode to "return from subroutine"
	void BytecodeVector::changeStopToRetSub()
	{
		const unsigned bytecodeEndPos(size());
		for (unsigned pc = 0; pc < bytecodeEndPos;)
		{
			BytecodeElement &element((*this)[pc]);
			if ((element.bytecode >> 12) == ASEBA_BYTECODE_STOP)
				element.bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_SUB_RET);
			pc += element.getWordSize();
		}
	}
	
	//! Return the type of last bytecode element
	unsigned short BytecodeVector::getTypeOfLast() const
	{
		unsigned short type(16); // invalid type
		for (size_t pc = 0; pc < size();)
		{
			const BytecodeElement &element((*this)[pc]);
			type = element.bytecode >> 12;
			pc += element.getWordSize();
		}
		return type;
	}
	
	//! Get the map of event addresses to identifiers
	BytecodeVector::EventAddressesToIdsMap BytecodeVector::getEventAddressesToIds() const
	{
		EventAddressesToIdsMap eventAddr;
		const unsigned eventVectSize = (*this)[0];
		unsigned pc = 1;
		while (pc < eventVectSize)
		{
			eventAddr[(*this)[pc + 1]] = (*this)[pc];
			pc += 2;
		}
		return eventAddr;
	}
	
	//! Disassemble a microcontroller bytecode and dump it
	void Compiler::disassemble(BytecodeVector& bytecode, const PreLinkBytecode& preLinkBytecode, std::wostream& dump) const
	{
		// address of threads and subroutines
		const BytecodeVector::EventAddressesToIdsMap eventAddr(bytecode.getEventAddressesToIds());
		std::map<unsigned, unsigned> subroutinesAddr;
		
		// build subroutine map
		for (size_t id = 0; id < subroutineTable.size(); ++id)
			subroutinesAddr[subroutineTable[id].address] = id;
		
		// event table
		const unsigned eventCount = eventAddr.size();
		const float fillPercentage = float(bytecode.size() * 100.f) / float(targetDescription->bytecodeSize);
		dump << "Disassembling " << eventCount + subroutineTable.size() << " segments (" << bytecode.size() << " words on " << targetDescription->bytecodeSize << ", " << fillPercentage << "% filled):\n";
		
		// bytecode
		unsigned pc = eventCount*2 + 1;
		while (pc < bytecode.size())
		{
			if (eventAddr.find(pc) != eventAddr.end())
			{
				const unsigned eventId = eventAddr.at(pc);
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
						if (index < (int)targetDescription->localEvents.size())
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
				dump << binaryOperatorToString((AsebaBinaryOperator)(bytecode[pc] & ASEBA_BINARY_OPERATOR_MASK));
				if (bytecode[pc] & (1 << ASEBA_IF_IS_WHEN_BIT))
					dump << " (edge), ";
				else
					dump << ", ";
				dump << "skip " << ((signed short)bytecode[pc+1]) << " if false" << "\n";
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
					std::wstring name(L"unknown");
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
	
	/*@}*/
	
} // namespace Aseba
