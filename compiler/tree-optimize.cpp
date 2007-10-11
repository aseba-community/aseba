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
#include "power-of-two.h"
#include "../utils/FormatableString.h"
#include <cassert>

namespace Aseba
{
	Node* BlockNode::optimize(std::ostream* dump)
	{
		for (NodesVector::iterator it = children.begin(); it != children.end();)
		{
			// generic optimization and removal
			Node *optimizedChild = (*it)->optimize(dump);
			if (!optimizedChild)
			{
				it = children.erase(it);
				continue;
			}
			else
				*it = optimizedChild;
			
			// special case for empty blocks
			if (dynamic_cast<BlockNode *>(*it) && (*it)->children.empty())
			{
				delete *it;
				it = children.erase(it);
				continue;
			}
			
			++it;
		}
		
		return this;
	}
	
	Node* AssignmentNode::optimize(std::ostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		children[1] = children[1]->optimize(dump);
		assert(children[1]);
		return this;
	}
	
	Node* IfWhenNode::optimize(std::ostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		children[1] = children[1]->optimize(dump);
		assert(children[1]);
		children[2] = children[2]->optimize(dump);
		assert(children[2]);
		
		Node *trueBlock = children[2];
		Node *falseBlock = 0;
		if (children.size() > 3)
		{
			children[3] = children[3]->optimize(dump);
			assert(children[3]);
			falseBlock = children[3];
		}
		
		if (trueBlock->children.empty() && (!falseBlock || falseBlock->children.empty()))
		{
			if (dump)
				*dump << sourcePos.toString() << ": if removed because it contained no statement.\n";
			delete this;
			return NULL;
		}
		else
			return this;
	}
	
	Node* WhileNode::optimize(std::ostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		children[1] = children[1]->optimize(dump);
		assert(children[1]);
		children[2] = children[2]->optimize(dump);
		assert(children[2]);
		
		if (children[2]->children.empty())
		{
			if (dump)
				*dump << sourcePos.toString() << ": while removed because it contained no statement.\n";
			delete this;
			return NULL;
		}
		else
			return this;
	}
	
	Node* ContextSwitcherNode::optimize(std::ostream* dump)
	{
		return this;
	}
	
	Node* EmitNode::optimize(std::ostream* dump)
	{
		return this;
	}
	
	Node* BinaryArithmeticNode::optimize(std::ostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		children[1] = children[1]->optimize(dump);
		assert(children[1]);
		
		// constants elimination
		ImmediateNode* immediateLeftChild = dynamic_cast<ImmediateNode*>(children[0]);
		ImmediateNode* immediateRightChild = dynamic_cast<ImmediateNode*>(children[1]);
		if (immediateLeftChild && immediateRightChild)
		{
			int valueOne = immediateLeftChild->value;
			int valueTwo = immediateRightChild->value;
			int result;
			SourcePos pos = sourcePos;
			
			switch (op)
			{
				case ASEBA_OP_SHIFT_LEFT: result = valueOne << valueTwo; break;
				case ASEBA_OP_SHIFT_RIGHT: result = valueOne >> valueTwo; break;
				case ASEBA_OP_ADD: result = valueOne + valueTwo; break;
				case ASEBA_OP_SUB: result = valueOne - valueTwo; break;
				case ASEBA_OP_MULT: result = valueOne * valueTwo; break;
				case ASEBA_OP_DIV: result = valueOne / valueTwo; break;
				case ASEBA_OP_MOD: result = valueOne % valueTwo; break;
				default: assert(false);
			}
			
			if (dump)
				*dump << sourcePos.toString() << ": binary arithmetic expression simplified\n";
			delete this;
			return new ImmediateNode(pos, result);
		}
		
		// POT mult/div to shift conversion
		if (immediateRightChild && isPOT(immediateRightChild->value))
		{
			if (op == ASEBA_OP_MULT)
			{
				op = ASEBA_OP_SHIFT_LEFT;
				immediateRightChild->value = shiftFromPOT(immediateRightChild->value);
				if (dump)
					*dump << sourcePos.toString() << ": multiplication transformed to left shift\n";
			}
			else if (op == ASEBA_OP_DIV)
			{
				op = ASEBA_OP_SHIFT_RIGHT;
				immediateRightChild->value = shiftFromPOT(immediateRightChild->value);
				if (dump)
					*dump << sourcePos.toString() << ": division transformed to right shift\n";
			}
		}
		
		return this;
	}
	
	Node* UnaryArithmeticNode::optimize(std::ostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		
		// constants elimination
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		if (immediateChild)
		{
			int value = -immediateChild->value;
			SourcePos pos = sourcePos;
			
			if (dump)
				*dump << sourcePos.toString() << ": unary arithmetic expression simplified\n";
			delete this;
			return new ImmediateNode(pos, value);
		}
		else
			return this;
	}
	
	Node* ImmediateNode::optimize(std::ostream* dump)
	{
		return this;
	}
	
	Node* LoadNode::optimize(std::ostream* dump)
	{
		return this;
	}
	
	Node* StoreNode::optimize(std::ostream* dump)
	{
		return this;
	}
	
	Node* ArrayReadNode::optimize(std::ostream* dump)
	{
		// optimize index expression
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		
		// if the index is just an integer and not an expression of any variable,
		// replace array acces by simple load and verify boundary conditions
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		if (immediateChild)
		{
			unsigned index = immediateChild->value;
			if (index > arraySize)
			{
				throw Error(sourcePos,
					FormatableString("Out of bound static array access. Trying to read index %0 of array %1 of size %2").arg(index).arg(arrayName).arg(arraySize));
			}
			
			unsigned varAddr = arrayAddr + index;
			SourcePos pos = sourcePos;
			
			if (dump)
				*dump << sourcePos.toString() << ": array access transformed to single variable access\n";
			delete this;
			return new LoadNode(pos, varAddr);
		}
		else
			return this;
	}
	
	Node* ArrayWriteNode::optimize(std::ostream* dump)
	{
		// optimize index expression
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		
		// if the index is just an integer and not an expression of any variable,
		// replace array acces by simple load and verify boundary conditions
		ImmediateNode* immediateChild = dynamic_cast<ImmediateNode*>(children[0]);
		if (immediateChild)
		{
			unsigned index = immediateChild->value;
			if (index > arraySize)
			{
				throw Error(sourcePos,
					FormatableString("Out of bound static array access. Trying to write index %0 of array %1 of size %2").arg(index).arg(arrayName).arg(arraySize));
			}
			
			unsigned varAddr = arrayAddr + index;
			SourcePos pos = sourcePos;
			
			if (dump)
				*dump << sourcePos.toString() << ": array access transformed to single variable access\n";
			delete this;
			return new StoreNode(pos, varAddr);
		}
		else
			return this;
	}
	
	Node* CallNode::optimize(std::ostream* dump)
	{
		return this;
	}
}; // Aseba
