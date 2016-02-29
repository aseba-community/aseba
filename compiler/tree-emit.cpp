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

#include "tree.h"
#include "../common/utils/utils.h"
#include <cassert>
#include <cstdlib>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	// helper function
	static void addImmediateToBytecodes(int value, const SourcePos& sourcePos, PreLinkBytecode& bytecodes)
	{
		unsigned short bytecode;
		
		if ((abs(value) >> 11) == 0)
		{
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_SMALL_IMMEDIATE);
			bytecode |= ((unsigned)value) & 0x0fff;
			bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		}
		else
		{
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LARGE_IMMEDIATE);
			bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
			bytecodes.current->push_back(BytecodeElement(value, sourcePos.row));
		}
	}

	//! Add init event and point to currentBytecode it
	PreLinkBytecode::PreLinkBytecode()
	{
		events[ASEBA_EVENT_INIT] = BytecodeVector();
		current = &events[ASEBA_EVENT_INIT];
	};
	
	//! Fixup prelinked bytecodes by making sure that each vector is closed correctly,
	//! i.e. with a STOP for events and a RET for subroutines
	void PreLinkBytecode::fixup(const Compiler::SubroutineTable &subroutineTable)
	{
		// clear empty events entries
		for (EventsBytecode::iterator it = events.begin(); it != events.end();)
		{
			EventsBytecode::iterator curIt = it;
			++it;
			size_t bytecodeSize = curIt->second.size();
			if (bytecodeSize == 0)
			{
				events.erase(curIt);
			}
		}
		
		// add terminals
		for (EventsBytecode::iterator it = events.begin(); it != events.end(); ++it)
		{
			BytecodeVector& bytecode = it->second;
			if (bytecode.getTypeOfLast() != ASEBA_BYTECODE_STOP)
				bytecode.push_back(BytecodeElement(AsebaBytecodeFromId(ASEBA_BYTECODE_STOP), bytecode.lastLine));
		}
		for (SubroutinesBytecode::iterator it = subroutines.begin(); it != subroutines.end(); ++it)
		{
			BytecodeVector& bytecode = it->second;
			if (bytecode.size())
			{
				bytecode.changeStopToRetSub();
				if (bytecode.getTypeOfLast() != ASEBA_BYTECODE_SUB_RET)
					bytecode.push_back(BytecodeElement(AsebaBytecodeFromId(ASEBA_BYTECODE_SUB_RET), bytecode.lastLine));
			}
			else
				bytecode.push_back(BytecodeElement(AsebaBytecodeFromId(ASEBA_BYTECODE_SUB_RET), subroutineTable[it->first].line));
		}
	}
	
	
	unsigned Node::getStackDepth() const
	{
		unsigned stackDepth = 0;
		for (size_t i = 0; i < children.size(); i++)
			stackDepth = std::max(stackDepth, children[i]->getStackDepth());
		return stackDepth;
	}
	
	
	void BlockNode::emit(PreLinkBytecode& bytecodes) const
	{
		for (size_t i = 0; i < children.size(); i++)
			children[i]->emit(bytecodes);
	}
	
	void ProgramNode::emit(PreLinkBytecode& bytecodes) const
	{
		BlockNode::emit(bytecodes);
		
		// analyze blocks for stack depth
		BytecodeVector* currentBytecode = &bytecodes.events[ASEBA_EVENT_INIT];
		unsigned maxStackDepth = 0;
		for (size_t i = 0; i < children.size(); i++)
		{
			EventDeclNode* eventDecl = dynamic_cast<EventDeclNode*>(children[i]);
			SubDeclNode* subDecl = dynamic_cast<SubDeclNode*>(children[i]);
			if (eventDecl || subDecl)
			{
				currentBytecode->maxStackDepth = maxStackDepth;
				maxStackDepth = 0;
				
				if (eventDecl)
					currentBytecode = &bytecodes.events[eventDecl->eventId];
				else
					currentBytecode = &bytecodes.subroutines[subDecl->subroutineId];
			}
			else
				maxStackDepth = std::max(maxStackDepth, children[i]->getStackDepth());
		}
		currentBytecode->maxStackDepth = maxStackDepth;
	}
	
	
	void AssignmentNode::emit(PreLinkBytecode& bytecodes) const
	{
		assert(children.size() % 2 == 0);
		for (size_t i = 0; i < children.size(); i += 2)
		{
			children[i+1]->emit(bytecodes);
			children[i+0]->emit(bytecodes);
		}
	}

	
	void IfWhenNode::emit(PreLinkBytecode& bytecodes) const
	{
		abort();
	}
	
	
	void FoldedIfWhenNode::emit(PreLinkBytecode& bytecodes) const
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
		BytecodeVector* currentBytecode = bytecodes.current;
		
		// generate code for left expression
		bytecodes.current = &ble;
		children[0]->emit(bytecodes);
		// generate code for right expression
		bytecodes.current = &bre;
		children[1]->emit(bytecodes);
		// generate code for true block
		bytecodes.current = &btb;
		children[2]->emit(bytecodes);
		// generate code for false block
		bytecodes.current = &bfb;
		if (children.size() > 3)
			children[3]->emit(bytecodes);
		
		// restore real current bytecode
		bytecodes.current = currentBytecode;
		
		// fit everything together
		std::copy(ble.begin(), ble.end(), std::back_inserter(*bytecodes.current));
		
		std::copy(bre.begin(), bre.end(), std::back_inserter(*bytecodes.current));
		
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_CONDITIONAL_BRANCH);
		bytecode |= op;
		bytecode |= edgeSensitive ? (1 << ASEBA_IF_IS_WHEN_BIT) : 0;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		
		if (children.size() == 4)
			bytecodes.current->push_back(BytecodeElement(2 + btb.size() + 1, sourcePos.row));
		else
			bytecodes.current->push_back(BytecodeElement(2 + btb.size(), sourcePos.row));
		
		
		std::copy(btb.begin(), btb.end(), std::back_inserter(*bytecodes.current));
		
		if (children.size() == 4)
		{
			bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_JUMP) | (bfb.size() + 1);
			unsigned short jumpLine;
			if (btb.size())
				jumpLine = (btb.end()-1)->line;
			else
				jumpLine = sourcePos.row;
			bytecodes.current->push_back(BytecodeElement(bytecode , jumpLine));
			
			std::copy(bfb.begin(), bfb.end(), std::back_inserter(*bytecodes.current));
		}
		
		bytecodes.current->lastLine = endLine;
	}
	
	unsigned FoldedIfWhenNode::getStackDepth() const
	{
		unsigned stackDepth = children[0]->getStackDepth() + children[1]->getStackDepth();
		stackDepth = std::max(stackDepth, children[2]->getStackDepth());
		if (children.size() == 4)
			stackDepth = std::max(stackDepth, children[3]->getStackDepth());
		return stackDepth;
	}
	
	
	void WhileNode::emit(PreLinkBytecode& bytecodes) const
	{
		abort();
	}
	
	
	void FoldedWhileNode::emit(PreLinkBytecode& bytecodes) const
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
		BytecodeVector* currentBytecode = bytecodes.current;
		
		// generate code for left expression
		bytecodes.current = &ble;
		children[0]->emit(bytecodes);
		// generate code for right expression
		bytecodes.current = &bre;
		children[1]->emit(bytecodes);
		// generate code for block
		bytecodes.current = &bb;
		children[2]->emit(bytecodes);
		
		// restore real current bytecode
		bytecodes.current = currentBytecode;
		
		// fit everything together
		std::copy(ble.begin(), ble.end(), std::back_inserter(*bytecodes.current));
		
		std::copy(bre.begin(), bre.end(), std::back_inserter(*bytecodes.current));
		
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_CONDITIONAL_BRANCH) | op;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.current->push_back(BytecodeElement(2 + bb.size() + 1, sourcePos.row));
		
		std::copy(bb.begin(), bb.end(), std::back_inserter(*bytecodes.current));
		
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_JUMP);
		bytecode |= ((unsigned)(-(int)(ble.size() + bre.size() + bb.size() + 2))) & 0x0fff;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	unsigned FoldedWhileNode::getStackDepth() const
	{
		unsigned stackDepth = children[0]->getStackDepth() + children[1]->getStackDepth();
		stackDepth = std::max(stackDepth, children[2]->getStackDepth());
		return stackDepth;
	}
	
	
	void EventDeclNode::emit(PreLinkBytecode& bytecodes) const
	{
		// make sure that we do not have twice the same event
		assert(bytecodes.events.find(eventId) == bytecodes.events.end());
		
		// create new bytecode for event
		bytecodes.events[eventId] = BytecodeVector();
		bytecodes.current = &bytecodes.events[eventId];
	}
	
	
	void EmitNode::emit(PreLinkBytecode& bytecodes) const
	{
		// emit code for children. They might contain code to store constants somewhere
		for (size_t i = 0; i < children.size(); i++)
			children[i]->emit(bytecodes);

		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_EMIT) | eventId;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.current->push_back(BytecodeElement(arrayAddr, sourcePos.row));
		bytecodes.current->push_back(BytecodeElement(arraySize, sourcePos.row));
	}
	
	
	void SubDeclNode::emit(PreLinkBytecode& bytecodes) const
	{
		// make sure that we do not have twice the same subroutine
		assert(bytecodes.subroutines.find(subroutineId) == bytecodes.subroutines.end());
		
		// create new bytecode for subroutine
		bytecodes.subroutines[subroutineId] = BytecodeVector();
		bytecodes.current = &bytecodes.subroutines[subroutineId];
	}
	
	
	void CallSubNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_SUB_CALL);
		bytecode |= subroutineId;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	
	void BinaryArithmeticNode::emit(PreLinkBytecode& bytecodes) const
	{
		children[0]->emit(bytecodes);
		children[1]->emit(bytecodes);
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_BINARY_ARITHMETIC) | op;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	unsigned BinaryArithmeticNode::getStackDepth() const
	{
		return children[0]->getStackDepth() + children[1]->getStackDepth();
	}
	
	
	void UnaryArithmeticNode::emit(PreLinkBytecode& bytecodes) const
	{
		children[0]->emit(bytecodes);
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_UNARY_ARITHMETIC) | op;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	
	void ImmediateNode::emit(PreLinkBytecode& bytecodes) const
	{
		addImmediateToBytecodes(value, sourcePos, bytecodes);
	}
	
	unsigned ImmediateNode::getStackDepth() const
	{
		return 1;
	}
	
	
	void LoadNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD) | varAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	unsigned LoadNode::getStackDepth() const
	{
		return 1;
	}
	
	
	void StoreNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STORE) | varAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	
	void ArrayWriteNode::emit(PreLinkBytecode& bytecodes) const
	{
		// constant index should have been optimized out already
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		assert(immediateChild == 0);
		UNUSED(immediateChild);
		
		children[0]->emit(bytecodes);
		
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STORE_INDIRECT) | arrayAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.current->push_back(BytecodeElement(arraySize, sourcePos.row));
	}
	
	
	void ArrayReadNode::emit(PreLinkBytecode& bytecodes) const
	{
		// constant index should have been optimized out already
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		assert(immediateChild == 0);
		UNUSED(immediateChild);
		
		children[0]->emit(bytecodes);
		
		unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD_INDIRECT) | arrayAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.current->push_back(BytecodeElement(arraySize, sourcePos.row));
	}
	
	
	void LoadNativeArgNode::emit(PreLinkBytecode& bytecodes) const
	{
		unsigned short bytecode;
		
		// constant index should have been optimized out already
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		assert(immediateChild == 0);
		UNUSED(immediateChild);
		
		// load variable
		children[0]->emit(bytecodes);
		
		// duplicate it: store once, load twice
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STORE) | tempAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD) | tempAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD) | tempAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		
		// load indirect to check for access error, store to discard
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_LOAD_INDIRECT) | arrayAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		bytecodes.current->push_back(BytecodeElement(arraySize, sourcePos.row));
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STORE) | tempAddr;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
		
		// compute address by pushing array address and adding
		addImmediateToBytecodes(arrayAddr, sourcePos, bytecodes);
		bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_BINARY_ARITHMETIC) | ASEBA_OP_ADD;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	unsigned LoadNativeArgNode::getStackDepth() const
	{
		return std::max(children[0]->getStackDepth(), unsigned(2));
	}
	
	
	void CallNode::emit(PreLinkBytecode& bytecodes) const
	{
		// generate load for template parameters in reverse order
		int i = (int)templateArgs.size() - 1;
		while (i >= 0)
		{
			addImmediateToBytecodes(templateArgs[i], sourcePos, bytecodes);
			i--;
		}
		
		// emit code for argument addresses in reverse order
		for (NodesVector::const_reverse_iterator it(children.rbegin()); it != children.rend(); ++it)
			(*it)->emit(bytecodes);
		
		// generate call itself
		const unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_NATIVE_CALL) | funcId;
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	unsigned CallNode::getStackDepth() const
	{
		unsigned stackDepth = 0;
		for (size_t i = 0; i < children.size(); i++)
			stackDepth = std::max(stackDepth, unsigned(templateArgs.size()+i)+children[i]->getStackDepth());
		
		return stackDepth;
	}
	
	void ReturnNode::emit(PreLinkBytecode& bytecodes) const
	{
		const unsigned short bytecode = AsebaBytecodeFromId(ASEBA_BYTECODE_STOP);
		bytecodes.current->push_back(BytecodeElement(bytecode, sourcePos.row));
	}
	
	/*@}*/
	
} // namespace Aseba
