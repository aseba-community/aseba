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

// Asserts a dynamic cast.	Similar to the one in boost/cast.hpp
template<typename Derived, typename Base>
static inline Derived polymorphic_downcast(Base base)
{
	Derived derived = dynamic_cast<Derived>(base);
	if (!derived)
		abort();
	return derived;
}

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
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
		
		Node *trueBlock = children[1];
		Node *falseBlock = 0;
		if (children.size() > 2)
		{
			children[2] = children[2]->optimize(dump);
			assert(children[2]);
			falseBlock = children[2];
		}
		
		// check for if on constants
		ImmediateNode* constantExpression = dynamic_cast<ImmediateNode*>(children[0]);
		if (constantExpression)
		{
			if (constantExpression->value != 0)
			{
				if (dump)
					*dump << sourcePos.toString() << ": if test removed because condition was always true\n";
				children[1] = 0;
				delete this;
				return trueBlock;
			}
			else if (falseBlock)
			{
				if (dump)
					*dump << sourcePos.toString() << ": if test removed because condition was always false\n";
				// false block
				children[2] = 0;
				delete this;
				return falseBlock;
			}
			else
			{
				// no false block
				if (dump)
					*dump << sourcePos.toString() << ": if removed because condition was always false and no code was associated\n";
				delete this;
				return NULL;
			}
		}
		
		// check remove
		if (trueBlock->children.empty() && (!falseBlock || falseBlock->children.empty()))
		{
			if (dump)
				*dump << sourcePos.toString() << ": if removed because it contained no statement\n";
			delete this;
			return NULL;
		}
		
		// fold operation inside if
		BinaryArithmeticNode* operation = polymorphic_downcast<BinaryArithmeticNode*>(children[0]);
		FoldedIfWhenNode *foldedNode = new FoldedIfWhenNode(sourcePos);
		foldedNode->edgeSensitive = edgeSensitive;
		foldedNode->op = operation->op;
		foldedNode->children.push_back(operation->children[0]);
		foldedNode->children.push_back(operation->children[1]);
		operation->children.clear();
		foldedNode->children.push_back(children[1]);
		children[1] = 0;
		if (children.size() > 2)
		{
			foldedNode->children.push_back(children[2]);
			children[2] = 0;
		}
		
		if (dump)
			*dump << sourcePos.toString() << ": if condition folded inside node\n";
		
		delete this;
		
		return foldedNode;
	}
	
	Node* FoldedIfWhenNode::optimize(std::ostream* dump)
	{
		assert(false);
	}
	
	Node* WhileNode::optimize(std::ostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		children[1] = children[1]->optimize(dump);
		assert(children[1]);
		
		// check for loops on constants
		ImmediateNode* constantExpression = dynamic_cast<ImmediateNode*>(children[0]);
		if (constantExpression)
		{
			if (constantExpression->value != 0)
			{
				throw Error(sourcePos, "Infinite loops not allowed");
			}
			else
			{
				if (dump)
					*dump << sourcePos.toString() << ": while removed because condition is always false\n";
				delete this;
				return NULL;
			}
		}
		
		// check for loops with empty content
		if (children[1]->children.empty())
		{
			if (dump)
				*dump << sourcePos.toString() << ": while removed because it contained no statement\n";
			delete this;
			return NULL;
		}
		
		// fold operation inside loop
		BinaryArithmeticNode* operation = polymorphic_downcast<BinaryArithmeticNode*>(children[0]);
		FoldedWhileNode *foldedNode = new FoldedWhileNode(sourcePos);
		foldedNode->op = operation->op;
		foldedNode->children.push_back(operation->children[0]);
		foldedNode->children.push_back(operation->children[1]);
		operation->children.clear();
		foldedNode->children.push_back(children[1]);
		children[1] = 0;
		
		if (dump)
			*dump << sourcePos.toString() << ": while condition folded inside node\n";
		
		delete this;
		
		return foldedNode;
	}
	
	Node* FoldedWhileNode::optimize(std::ostream* dump)
	{
		assert(false);
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
				case ASEBA_OP_DIV: 
					if (valueTwo == 0)
						throw Error(sourcePos, "Division by zero.");
					else
						result = valueOne / valueTwo;
				break;
				case ASEBA_OP_MOD: result = valueOne % valueTwo; break;
				
				case ASEBA_OP_EQUAL: result = valueOne == valueTwo; break;
				case ASEBA_OP_NOT_EQUAL: result = valueOne != valueTwo; break;
				case ASEBA_OP_BIGGER_THAN: result = valueOne > valueTwo; break;
				case ASEBA_OP_BIGGER_EQUAL_THAN: result = valueOne >= valueTwo; break;
				case ASEBA_OP_SMALLER_THAN: result = valueOne < valueTwo; break;
				case ASEBA_OP_SMALLER_EQUAL_THAN: result = valueOne <= valueTwo; break;
				
				case ASEBA_OP_OR: result = valueOne || valueTwo; break;
				case ASEBA_OP_AND: result = valueOne && valueTwo; break;
				
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
			int result;
			SourcePos pos = sourcePos;
			
			switch (op)
			{
				case ASEBA_UNARY_OP_SUB: result = -immediateChild->value; break;
				case ASEBA_UNARY_OP_ABS: 
					if (immediateChild->value == -32768)
						throw Error(sourcePos, "-32768 has no positive correspondance in 16 bits integers.");
					else
						result = abs(immediateChild->value);
				break;
				
				default: assert(false);
			}
			
			if (dump)
				*dump << sourcePos.toString() << ": unary arithmetic expression simplified\n";
			delete this;
			return new ImmediateNode(pos, result);
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
	
	/*@}*/
	
}; // Aseba
