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

#include "tree.h"
#include <cassert>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/

	//! Add init event and point to currentBytecode it
	PreLinkBytecode::PreLinkBytecode()
	{
		(*this)[ASEBA_EVENT_INIT] = BytecodeVector();
		currentBytecode = &(*this)[ASEBA_EVENT_INIT];
	};
	
	//! Fixup prelinked bytecodes by making sure each vector has at least a STOP token
	void PreLinkBytecode::fixup()
	{
		// add terminal STOP
		size_t currentBytecodeSize = currentBytecode->size();
		if (currentBytecodeSize > 0)
			currentBytecode->push_back(BytecodeElement(AsebaBytecodeFromId(ASEBA_BYTECODE_STOP), (*currentBytecode)[currentBytecodeSize-1].line));
		
		// clear empty entries and add missing STOP
		for (std::map<unsigned, BytecodeVector>::iterator it = begin(); it != end();)
		{
			std::map<unsigned, BytecodeVector>::iterator curIt = it;
			++it;
			size_t bytecodeSize = curIt->second.size();
			if (bytecodeSize == 0)
			{
				erase(curIt);
			}
			else if ((curIt->second[bytecodeSize-1].bytecode >> 12) != ASEBA_BYTECODE_STOP)
				curIt->second.push_back(BytecodeElement(AsebaBytecodeFromId(ASEBA_BYTECODE_STOP), curIt->second[bytecodeSize-1].line));
		}
	}
	
	void BlockNode::emit(PreLinkBytecode& bytecodes) const
	{
		for (size_t i = 0; i < children.size(); i++)
			children[i]->emit(bytecodes);
	}
	
	void AssignmentNode::emit(PreLinkBytecode& bytecodes) const
	{
		children[1]->emit(bytecodes);
		children[0]->emit(bytecodes);
	}
	
	void IfWhenNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode;
		
		/*
			What we want in bytecodes.currentBytecode afterwards:
			
			[ bytecode of left expression (ble) ]
			[ bytecode of right expression (bre) ]
			CondBranch bytecode
			3
			3 + btb.size + 1
			[ bytecode of true block (btb) ]
			Jump bfb.size + 1
			[ bytecode of false block (bfb) ]
		*/
		BytecodeVector ble;
		BytecodeVector bre;
		BytecodeVector btb;
		BytecodeVector bfb;
		
		// save real current bytecode
		BytecodeVector* currentBytecode = bytecodes.currentBytecode;
		
		// generate code for left expression
		bytecodes.currentBytecode = &ble;
		children[0]->emit(bytecodes);
		// generate code for right expression
		bytecodes.currentBytecode = &bre;
		children[1]->emit(bytecodes);
		// generate code for true block
		bytecodes.currentBytecode = &btb;
		children[2]->emit(bytecodes);
		// generate code for false block
		if (children.size() == 4)
		{
			bytecodes.currentBytecode = &bfb;
			children[3]->emit(bytecodes);
		}
		
		// restore real current bytecode
		bytecodes.currentBytecode = currentBytecode;
		
		// fit everything together
		std::copy(ble.begin(), ble.end(), std::back_inserter(*bytecodes.currentBytecode));
		
		std::copy(bre.begin(), bre.end(), std::back_inserter(*bytecodes.currentBytecode));
		
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_CONDITIONAL_BRANCH);
		bytecode |= comparaison;
		bytecode |= edgeSensitive ? (1 << 3) : 0;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.currentBytecode->push_back(BytecodeElement(3, sourcePos.row));
		
		if (children.size() == 4)
			bytecodes.currentBytecode->push_back(BytecodeElement(3 + btb.size() + 1, sourcePos.row));
		else
			bytecodes.currentBytecode->push_back(BytecodeElement(3 + btb.size(), sourcePos.row));
		
		std::copy(btb.begin(), btb.end(), std::back_inserter(*bytecodes.currentBytecode));
		
		if (children.size() == 4)
		{
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_JUMP) | bfb.size() + 1;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode , sourcePos.row));
			
			std::copy(bfb.begin(), bfb.end(), std::back_inserter(*bytecodes.currentBytecode));
		}
	}
	
	void WhileNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode;
		
		/*
			What we want in bytecodes.currentBytecode afterwards:
			
			[ bytecode of left expression (ble) ]
			[ bytecode of right expression (bre) ]
			CondBranch bytecode
			3
			3 + bb.size + 1
			[ bytecode of block (bb) ]
			Jump -(bb.size + ble.size + bre.size + 3)
		*/
		BytecodeVector ble;
		BytecodeVector bre;
		BytecodeVector bb;
		
		// save real current bytecode
		BytecodeVector* currentBytecode = bytecodes.currentBytecode;
		
		// generate code for left expression
		bytecodes.currentBytecode = &ble;
		children[0]->emit(bytecodes);
		// generate code for right expression
		bytecodes.currentBytecode = &bre;
		children[1]->emit(bytecodes);
		// generate code for block
		bytecodes.currentBytecode = &bb;
		children[2]->emit(bytecodes);
		
		// restore real current bytecode
		bytecodes.currentBytecode = currentBytecode;
		
		// fit everything together
		std::copy(ble.begin(), ble.end(), std::back_inserter(*bytecodes.currentBytecode));
		
		std::copy(bre.begin(), bre.end(), std::back_inserter(*bytecodes.currentBytecode));
		
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_CONDITIONAL_BRANCH) | comparaison;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.currentBytecode->push_back(BytecodeElement(3, sourcePos.row));
		bytecodes.currentBytecode->push_back(BytecodeElement(3 + bb.size() + 1, sourcePos.row));
		
		std::copy(bb.begin(), bb.end(), std::back_inserter(*bytecodes.currentBytecode));
		
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_JUMP);
		bytecode |= ((unsigned)(-(int)(ble.size() + bre.size() + bb.size() + 3))) & 0x0fff;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	void ContextSwitcherNode::emit(PreLinkBytecode& bytecodes) const
	{
		// push stop
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STOP);
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		// change event bytecode
		if (bytecodes.find(eventId) == bytecodes.end())
			bytecodes[eventId] = BytecodeVector();
		bytecodes.currentBytecode = &bytecodes[eventId];
	}
	
	void EmitNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_EMIT) | eventId;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.currentBytecode->push_back(BytecodeElement(arrayAddr, sourcePos.row));
		bytecodes.currentBytecode->push_back(BytecodeElement(arraySize, sourcePos.row));
	}
	
	void BinaryArithmeticNode::emit(PreLinkBytecode& bytecodes) const
	{
		children[0]->emit(bytecodes);
		children[1]->emit(bytecodes);
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_BINARY_ARITHMETIC) | op;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	void UnaryArithmeticNode::emit(PreLinkBytecode& bytecodes) const
	{
		children[0]->emit(bytecodes);
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_UNARY_ARITHMETIC);
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	void ImmediateNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode;
		
		if ((abs(value) >> 11) == 0)
		{
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_SMALL_IMMEDIATE);
			bytecode |= ((unsigned)value) & 0x0fff;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		}
		else
		{
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LARGE_IMMEDIATE);
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
			bytecodes.currentBytecode->push_back(BytecodeElement(value, sourcePos.row));
		}
	}
	
	void LoadNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD) | varAddr;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	void StoreNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STORE) | varAddr;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	void ArrayReadNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode;
		
		// optimise when index is constant
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		if (immediateChild)
		{
			assert(immediateChild->value >= 0);
			unsigned addr = immediateChild->value + arrayAddr;
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD) | addr;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		}
		else
		{
			children[0]->emit(bytecodes);
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_SMALL_IMMEDIATE) | arrayAddr;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_BINARY_ARITHMETIC) | ASEBA_OP_ADD;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		}
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD_INDIRECT);
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	void ArrayWriteNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode;
		
		// optimise when index is constant
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		if (immediateChild)
		{
			assert(immediateChild->value >= 0);
			unsigned addr = immediateChild->value + arrayAddr;
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD) | addr;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		}
		else
		{
			children[0]->emit(bytecodes);
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_SMALL_IMMEDIATE) | arrayAddr;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_BINARY_ARITHMETIC) | ASEBA_OP_ADD;
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		}
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STORE_INDIRECT);
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	void CallNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode;
		
		// emit code for children. They might contain code to store constants somewhere
		for (size_t i = 0; i < children.size(); i++)
			children[i]->emit(bytecodes);
		
		// generate load for arguments
		for (unsigned i = 0; i < argumentsAddr.size(); i++)
		{
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_SMALL_IMMEDIATE) | argumentsAddr[i];
			bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
		}
		
		// generate call itself
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_CALL) | funcId;
		bytecodes.currentBytecode->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	/*@}*/
	
}; // Aseba
