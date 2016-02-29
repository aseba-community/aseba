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
#include "power-of-two.h"
#include "../common/utils/FormatableString.h"
#include "../common/utils/utils.h"
#include <cassert>
#include <cstdlib>


namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	Node* BlockNode::optimize(std::wostream* dump)
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
	
	Node* AssignmentNode::optimize(std::wostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		children[1] = children[1]->optimize(dump);
		assert(children[1]);
		return this;
	}
	
	Node* IfWhenNode::optimize(std::wostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		
		// optimise true block, which may be NULL afterwards
		children[1] = children[1]->optimize(dump);
		Node *trueBlock = children[1];
		
		// optimise false block, which may be NULL afterwards
		Node *falseBlock;
		if (children.size() > 2)
		{
			children[2] = children[2]->optimize(dump);
			falseBlock = children[2];
			if (children[2] == 0)
				children.resize(2);
		}
		else
			falseBlock = 0;
		
		// check if both block are NULL or do not contain any data, in this case return
		if (
			((trueBlock == 0) || (dynamic_cast<BlockNode*>(trueBlock) && trueBlock->children.empty())) && 
			((falseBlock == 0) || (dynamic_cast<BlockNode*>(falseBlock) && falseBlock->children.empty()))
		)
		{
			if (dump)
				*dump << sourcePos.toWString() << L": if test removed because it had no associated code\n";
			delete this;
			return NULL;
		}
		
		// check for if on constants
		ImmediateNode* constantExpression = dynamic_cast<ImmediateNode*>(children[0]);
		if (constantExpression)
		{
			if (constantExpression->value != 0)
			{
				if (dump)
					*dump << sourcePos.toWString() << L": if test simplified because condition was always true\n";
				children[1] = 0;
				delete this;
				return trueBlock;
			}
			else
			{
				if (dump)
					*dump << sourcePos.toWString() << L": if test simplified because condition was always false\n";
				if (children.size() > 2)
					children[2] = 0;
				delete this;
				return falseBlock;
			}
		}
		
		// create a dummy block for true if none exist
		if (trueBlock == 0)
			children[1] = new BlockNode(sourcePos);
		
		// fold operation inside if
		BinaryArithmeticNode* operation = polymorphic_downcast<BinaryArithmeticNode*>(children[0]);
		FoldedIfWhenNode *foldedNode = new FoldedIfWhenNode(sourcePos);
		foldedNode->op = operation->op;
		foldedNode->edgeSensitive = edgeSensitive;
		foldedNode->endLine = endLine;
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
			*dump << sourcePos.toWString() << L": if condition folded inside node\n";
		
		delete this;
		
		return foldedNode;
	}
	
	Node* FoldedIfWhenNode::optimize(std::wostream* dump)
	{
		abort();
		return 0;
	}
	
	Node* WhileNode::optimize(std::wostream* dump)
	{
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		
		// block may be NULL
		children[1] = children[1]->optimize(dump);
		
		// check for loops on constants
		ImmediateNode* constantExpression = dynamic_cast<ImmediateNode*>(children[0]);
		if (constantExpression)
		{
			if (constantExpression->value != 0)
			{
				throw TranslatableError(sourcePos, ERROR_INFINITE_LOOP);
			}
			else
			{
				if (dump)
					*dump << sourcePos.toWString() << L": while removed because condition is always false\n";
				delete this;
				return NULL;
			}
		}
		
		// check for loops with empty content
		if ((children[1] == 0) || (dynamic_cast<BlockNode*>(children[1]) && children[1]->children.empty()))
		{
			if (dump)
				*dump << sourcePos.toWString() << L": while removed because it contained no statement\n";
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
			*dump << sourcePos.toWString() << L": while condition folded inside node\n";
		
		delete this;
		
		return foldedNode;
	}
	
	Node* FoldedWhileNode::optimize(std::wostream* dump)
	{
		abort();
		return 0;
	}
	
	Node* EventDeclNode::optimize(std::wostream* dump)
	{
		return this;
	}
	
	Node* EmitNode::optimize(std::wostream* dump)
	{
		for (NodesVector::iterator it = children.begin(); it != children.end();)
		{
			// generic optimization and removal
			*it = (*it)->optimize(dump);
			++it;
		}

		return this;
	}
	
	Node* SubDeclNode::optimize(std::wostream* dump)
	{
		return this;
	}
	
	Node* CallSubNode::optimize(std::wostream* dump)
	{
		return this;
	}
	
	Node* BinaryArithmeticNode::optimize(std::wostream* dump)
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
						throw TranslatableError(sourcePos, ERROR_DIVISION_BY_ZERO);
					else
						result = valueOne / valueTwo;
				break;
				case ASEBA_OP_MOD:
					if (valueTwo == 0)
						throw TranslatableError(sourcePos, ERROR_DIVISION_BY_ZERO);
					else
						result = valueOne % valueTwo;
				break;
				
				case ASEBA_OP_BIT_OR: result = valueOne | valueTwo; break;
				case ASEBA_OP_BIT_XOR: result = valueOne ^ valueTwo; break;
				case ASEBA_OP_BIT_AND: result = valueOne & valueTwo; break;
				
				case ASEBA_OP_EQUAL: result = valueOne == valueTwo; break;
				case ASEBA_OP_NOT_EQUAL: result = valueOne != valueTwo; break;
				case ASEBA_OP_BIGGER_THAN: result = valueOne > valueTwo; break;
				case ASEBA_OP_BIGGER_EQUAL_THAN: result = valueOne >= valueTwo; break;
				case ASEBA_OP_SMALLER_THAN: result = valueOne < valueTwo; break;
				case ASEBA_OP_SMALLER_EQUAL_THAN: result = valueOne <= valueTwo; break;
				
				case ASEBA_OP_OR: result = valueOne || valueTwo; break;
				case ASEBA_OP_AND: result = valueOne && valueTwo; break;
				
				default: abort();
			}
			
			if (dump)
				*dump << sourcePos.toWString() << L": binary arithmetic expression simplified\n";
			delete this;
			return new ImmediateNode(pos, result);
		}
		
		// multiplications by 1 or addition of 0
		// TODO: make more generic the concept of neutral element
		if (op == ASEBA_OP_MULT || op == ASEBA_OP_DIV || op == ASEBA_OP_ADD || op == ASEBA_OP_SUB)
		{
			Node **survivor(0);
			if (op == ASEBA_OP_MULT || op == ASEBA_OP_DIV)
			{
				if (immediateRightChild && (immediateRightChild->value == 1))
					survivor = &children[0];
				if (immediateLeftChild && (immediateLeftChild->value == 1) && (op == ASEBA_OP_MULT))
					survivor = &children[1];
			}
			else
			{
				if (immediateRightChild && (immediateRightChild->value == 0))
					survivor = &children[0];
				if (immediateLeftChild && (immediateLeftChild->value == 0) && (op == ASEBA_OP_ADD))
					survivor = &children[1];
			}
			if (survivor)
			{
				if (dump)
					*dump << sourcePos.toWString() << L": operation with neutral element removed\n";
				Node *returNode = *survivor;
				*survivor = 0;
				delete this;
				return returNode;
			}
		}
		
		// POT mult/div to shift conversion
		if (immediateRightChild && isPOT(immediateRightChild->value))
		{
			if (op == ASEBA_OP_MULT)
			{
				op = ASEBA_OP_SHIFT_LEFT;
				immediateRightChild->value = shiftFromPOT(immediateRightChild->value);
				if (dump)
					*dump << sourcePos.toWString() << L": multiplication transformed to left shift\n";
			}
			else if (op == ASEBA_OP_DIV)
			{
				if (immediateRightChild->value == 0)
					throw TranslatableError(sourcePos, ERROR_DIVISION_BY_ZERO);
				op = ASEBA_OP_SHIFT_RIGHT;
				immediateRightChild->value = shiftFromPOT(immediateRightChild->value);
				if (dump)
					*dump << sourcePos.toWString() << L": division transformed to right shift\n";
			}
		}
		
		return this;
	}
	
	//! Recursively apply de Morgan law as long as nodes are logic operations, and then invert comparisons
	void BinaryArithmeticNode::deMorganNotRemoval()
	{
		switch (op)
		{
			// comparison: invert
			case ASEBA_OP_EQUAL: op = ASEBA_OP_NOT_EQUAL; break;
			case ASEBA_OP_NOT_EQUAL: op = ASEBA_OP_EQUAL; break;
			case ASEBA_OP_BIGGER_THAN: op = ASEBA_OP_SMALLER_EQUAL_THAN; break;
			case ASEBA_OP_BIGGER_EQUAL_THAN: op = ASEBA_OP_SMALLER_THAN; break;
			case ASEBA_OP_SMALLER_THAN: op = ASEBA_OP_BIGGER_EQUAL_THAN; break;
			case ASEBA_OP_SMALLER_EQUAL_THAN: op = ASEBA_OP_BIGGER_THAN; break;
			// logic: invert and call recursively
			case ASEBA_OP_OR:
				op = ASEBA_OP_AND;
				polymorphic_downcast<BinaryArithmeticNode*>(children[0])->deMorganNotRemoval();
				polymorphic_downcast<BinaryArithmeticNode*>(children[1])->deMorganNotRemoval();
			break;
			case ASEBA_OP_AND:
				op = ASEBA_OP_OR;
				polymorphic_downcast<BinaryArithmeticNode*>(children[0])->deMorganNotRemoval();
				polymorphic_downcast<BinaryArithmeticNode*>(children[1])->deMorganNotRemoval();
			break;
			default: abort();
		};
	}
	
	Node* UnaryArithmeticNode::optimize(std::wostream* dump)
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
						throw TranslatableError(sourcePos, ERROR_ABS_NOT_POSSIBLE);
					else
						result = abs(immediateChild->value);
				break;
				case ASEBA_UNARY_OP_BIT_NOT: result = ~immediateChild->value; break;
				case ASEBA_UNARY_OP_NOT: result = !immediateChild->value; break;
				default: abort();
			}
			
			if (dump)
				*dump << sourcePos.toWString() << L": unary arithmetic expression simplified\n";
			delete this;
			return new ImmediateNode(pos, result);
		}
		else if (op == ASEBA_UNARY_OP_NOT)
		{
			BinaryArithmeticNode* binaryNodeChild(dynamic_cast<BinaryArithmeticNode*>(children[0]));
			if (binaryNodeChild)
			{
				// de Morgan removal of not
				if (dump)
					*dump << sourcePos.toWString() << L": not removed using de Morgan\n";
				binaryNodeChild->deMorganNotRemoval();
				children.clear();
				delete this;
				return binaryNodeChild;
			}
			else
				return this;
		}
		else
			return this;
	}
	
	Node* ImmediateNode::optimize(std::wostream* dump)
	{
		return this;
	}
	
	Node* LoadNode::optimize(std::wostream* dump)
	{
		return this;
	}
	
	Node* StoreNode::optimize(std::wostream* dump)
	{
		return this;
	}
	
	Node* ArrayReadNode::optimize(std::wostream* dump)
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
			if (index >= arraySize)
			{
				throw TranslatableError(sourcePos,
					ERROR_ARRAY_OUT_OF_BOUND_READ).arg(index).arg(arrayName).arg(arraySize);
			}
			
			unsigned varAddr = arrayAddr + index;
			SourcePos pos = sourcePos;
			
			if (dump)
				*dump << sourcePos.toWString() << L": array access transformed to single variable access\n";
			delete this;
			return new LoadNode(pos, varAddr);
		}
		else
			return this;
	}
	
	Node* ArrayWriteNode::optimize(std::wostream* dump)
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
			if (index >= arraySize)
			{
				throw TranslatableError(sourcePos,
					ERROR_ARRAY_OUT_OF_BOUND_WRITE).arg(index).arg(arrayName).arg(arraySize);
			}
			
			unsigned varAddr = arrayAddr + index;
			SourcePos pos = sourcePos;
			
			if (dump)
				*dump << sourcePos.toWString() << L": array access transformed to single variable access\n";
			delete this;
			return new StoreNode(pos, varAddr);
		}
		else
			return this;
	}
	
	Node* LoadNativeArgNode::optimize(std::wostream* dump)
	{
		// optimize index expression
		children[0] = children[0]->optimize(dump);
		assert(children[0]);
		// we should not have an immediate
		assert(dynamic_cast<ImmediateNode*>(children[0]) == 0);
		return this;
	}
	
	Node* CallNode::optimize(std::wostream* dump)
	{
		for (NodesVector::iterator it = children.begin(); it != children.end();)
		{
			// generic optimization and removal
			*it = (*it)->optimize(dump);
			assert(*it);
			++it;
		}

		return this;
	}
	
	/*@}*/
	
} // namespace Aseba
