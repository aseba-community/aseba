/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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
#include "../utils/FormatableString.h"

#include <cassert>
#include <memory>
#include <iostream>

namespace Aseba
{
	Node* Node::expandToAsebaTree(std::wostream *dump, unsigned int index)
	{
		// recursively walk the tree and expand children
		for (NodesVector::iterator it = children.begin(); it != children.end();)
		{
			*(it) = (*it)->expandToAsebaTree(dump, index);
			++it;
		}
		return this;
	}


	Node* AssignmentNode::expandToAsebaTree(std::wostream *dump, unsigned int index)
	{
		assert(children.size() == 2);

		MemoryVectorNode* leftVector = dynamic_cast<MemoryVectorNode*>(children[0]);
		if (!leftVector)
			throw TranslatableError(sourcePos, ERROR_INCORRECT_LEFT_VALUE).arg(children[0]->toNodeName());
		leftVector->setWrite(true);
		Node* rightVector = children[1];

		std::auto_ptr<BlockNode> block(new BlockNode(sourcePos));
		for (unsigned int i = 0; i < leftVector->getVectorSize(); i++)
		{
			// expand to left[i] = right[i]
			std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(sourcePos));
			assignment->children.push_back(leftVector->expandToAsebaTree(dump, i));
			assignment->children.push_back(rightVector->expandToAsebaTree(dump, i));
			block->children.push_back(assignment.release());
		}

		delete this;
		return block.release();
	}


	//! expand to left[index] (op) right[index]
	Node* BinaryArithmeticNode::expandToAsebaTree(std::wostream *dump, unsigned int index)
	{
		std::auto_ptr<Node> left(children[0]->expandToAsebaTree(dump, index));
		std::auto_ptr<Node> right(children[1]->expandToAsebaTree(dump, index));
		return new BinaryArithmeticNode(sourcePos, op, left.release(), right.release());
	}


	//! expand to (op)left[index]
	Node* UnaryArithmeticNode::expandToAsebaTree(std::wostream *dump, unsigned int index)
	{
		std::auto_ptr<Node> left(children[0]->expandToAsebaTree(dump, index));
		return new UnaryArithmeticNode(sourcePos, op, left.release());
	}


	//! expand to vector[index]
	Node* TupleVectorNode::expandToAsebaTree(std::wostream *dump, unsigned int index)
	{
		size_t total = 0;
		size_t prevTotal = 0;
		for (size_t i = 0; i < children.size(); i++)
		{
			prevTotal = total;
			total += children[i]->getVectorSize();
			if (index < total)
			{
				return children[i]->expandToAsebaTree(dump, index-prevTotal);
			}
		}
		assert(0);
		return 0;
	}


	Node* MemoryVectorNode::expandToAsebaTree(std::wostream *dump, unsigned int index)
	{
		assert(index < getVectorSize());

		// get the optional index given in the Aseba code
		TupleVectorNode* accessIndex = NULL;
		if (children.size() > 0)
			accessIndex = dynamic_cast<TupleVectorNode*>(children[0]);

		if (accessIndex || children.size() == 0)
		{
			// immediate index "foo[n]" or "foo[n:m]" / full array access "foo"
			unsigned pointer = getVectorAddr() + index;
			// check if index is within bounds
			if (pointer >= arrayAddr + arraySize)
				throw TranslatableError(sourcePos, ERROR_ARRAY_OUT_OF_BOUND).arg(arrayName).arg(index).arg(arraySize);

			if (write == true)
				return new StoreNode(sourcePos, pointer);
			else
				return new LoadNode(sourcePos, pointer);
		}
		else
		{
			// indirect access foo[expr]
			std::auto_ptr<Node> array;
			if (write == true)
				array.reset(new ArrayWriteNode(sourcePos, arrayAddr, arraySize, arrayName));
			else
				array.reset(new ArrayReadNode(sourcePos, arrayAddr, arraySize, arrayName));

			array->children.push_back(children[0]->expandToAsebaTree(dump, index));
			children[0] = 0;
			return array.release();
		}
	}


	Node* ArithmeticAssignmentNode::expandToAsebaTree(std::wostream* dump, unsigned int index)
	{
		assert(children.size() == 2);

		Node* leftVector = children[0];
		Node* rightVector = children[1];

		// expand to left = left + right
		std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(sourcePos));
		std::auto_ptr<BinaryArithmeticNode> binary(new BinaryArithmeticNode(sourcePos, op, leftVector->deepCopy(), rightVector->deepCopy()));
		assignment->children.push_back(leftVector->deepCopy());
		assignment->children.push_back(binary.release());

		std::auto_ptr<Node> finalBlock(assignment->expandToAsebaTree(dump, index));

		assignment.release();
		delete this;

		return finalBlock.release();
	}


	Node* UnaryArithmeticAssignmentNode::expandToAsebaTree(std::wostream* dump, unsigned int index)
	{
		assert(children.size() == 1);

		Node* memoryVector = children[0];

		// create a vector of 1's
		std::auto_ptr<TupleVectorNode> constant(new TupleVectorNode(sourcePos));
		for (unsigned int i = 0; i < memoryVector->getVectorSize(); i++)
			constant->addImmediateValue(1);

		// expand to "vector (op)= 1"
		std::auto_ptr<ArithmeticAssignmentNode> assignment(new ArithmeticAssignmentNode(sourcePos, arithmeticOp, memoryVector->deepCopy(), constant.release()));
		std::auto_ptr<Node> finalBlock(assignment->expandToAsebaTree(dump, index));

		assignment.release();
		delete this;

		return finalBlock.release();
	}


	void Node::checkVectorSize() const
	{
		// recursively walk the tree and expand children
		for (NodesVector::const_iterator it = children.begin(); it != children.end();)
		{
			(*it)->checkVectorSize();
			++it;
		}
	}


	void AssignmentNode::checkVectorSize() const
	{
		assert(children.size() == 2);

		// will recursively check the whole tree belonging to "="
		unsigned lSize = children[0]->getVectorSize();
		unsigned rSize = children[1]->getVectorSize();

		// top-level check
		if (lSize != rSize)
			throw TranslatableError(sourcePos, ERROR_ARRAY_SIZE_MISMATCH).arg(lSize).arg(rSize);
	}

	void ArithmeticAssignmentNode::checkVectorSize() const
	{
		assert(children.size() == 2);

		// will recursively check the whole tree belonging to "="
		unsigned lSize = children[0]->getVectorSize();
		unsigned rSize = children[1]->getVectorSize();

		// top-level check
		if (lSize != rSize)
			throw TranslatableError(sourcePos, ERROR_ARRAY_SIZE_MISMATCH).arg(lSize).arg(rSize);
	}


	void IfWhenNode::checkVectorSize() const
	{
		assert(children.size() > 0);

		unsigned conditionSize = children[0]->getVectorSize();
		if (conditionSize != 1)
			throw TranslatableError(sourcePos, ERROR_IF_VECTOR_CONDITION);
		// check true block
		if (children.size() > 1 && children[1])
			children[1]->checkVectorSize();
		// check false block
		if (children.size() > 2 && children[2])
			children[2]->checkVectorSize();
	}

	void FoldedIfWhenNode::checkVectorSize() const
	{
		assert(children.size() > 1);

		unsigned conditionLeftSize = children[0]->getVectorSize();
		unsigned conditionRightSize = children[1]->getVectorSize();
		if (conditionLeftSize != 1 || conditionRightSize != 1)
			throw TranslatableError(sourcePos, ERROR_IF_VECTOR_CONDITION);
		// check true block
		if (children.size() > 2 && children[2])
			children[2]->checkVectorSize();
		// check false block
		if (children.size() > 3 && children[3])
			children[3]->checkVectorSize();
	}

	void WhileNode::checkVectorSize() const
	{
		assert(children.size() > 0);

		unsigned conditionSize = children[0]->getVectorSize();
		if (conditionSize != 1)
			throw TranslatableError(sourcePos, ERROR_WHILE_VECTOR_CONDITION);
		// check inner block
		if (children.size() > 1 && children[1])
			children[1]->checkVectorSize();
	}

	void FoldedWhileNode::checkVectorSize() const
	{
		assert(children.size() > 1);

		unsigned conditionLeftSize = children[0]->getVectorSize();
		unsigned conditionRightSize = children[1]->getVectorSize();
		if (conditionLeftSize != 1 || conditionRightSize != 1)
			throw TranslatableError(sourcePos, ERROR_WHILE_VECTOR_CONDITION);
		// check inner block
		if (children.size() > 2 && children[2])
			children[2]->checkVectorSize();
	}


	//! return the address of the left-most operand, or E_NOVAL if none
	unsigned Node::getVectorAddr() const
	{
		if (children.size() > 0)
			return children[0]->getVectorAddr();
		else
			return E_NOVAL;
	}


	//! return the children's size, check for equal size, or E_NOVAL if no child
	unsigned Node::getVectorSize() const
	{
		unsigned size = E_NOVAL;
		unsigned new_size = E_NOVAL;

		for (NodesVector::const_iterator it = children.begin(); it != children.end(); ++it)
		{
			new_size = (*it)->getVectorSize();
			if (size == E_NOVAL)
				size = new_size;
			else if (size != new_size)
				throw TranslatableError(sourcePos, ERROR_ARRAY_SIZE_MISMATCH).arg(size).arg(new_size);
		}

		return size;
	}


	unsigned TupleVectorNode::getVectorSize() const
	{
		unsigned size = 0;
		NodesVector::const_iterator it;
		for (it = children.begin(); it != children.end(); it++)
			size += (*it)->getVectorSize();
		return size;
	}


	//! return the compile-time base address of the memory range, taking
	//! into account an immediate index foo[n] or foo[n:m]
	//! return E_NOVAL if foo[expr]
	unsigned MemoryVectorNode::getVectorAddr() const
	{
		assert(children.size() <= 1);

		unsigned shift = 0;

		// index(es) given?
		if (children.size() == 1)
		{
			TupleVectorNode* index = dynamic_cast<TupleVectorNode*>(children[0]);
			if (index)
			{
				shift = index->getImmediateValue(0);
			}
			else
				// not know at compile time
				return E_NOVAL;
		}

		return arrayAddr + shift;
	}


	//! return the vector's length
	unsigned MemoryVectorNode::getVectorSize() const
	{
		assert(children.size() <= 1);

		if (children.size() == 1)
		{
			TupleVectorNode* index = dynamic_cast<TupleVectorNode*>(children[0]);
			if (index)
			{
				unsigned numberOfIndex = index->getVectorSize();
				// immediate indexes
				if (numberOfIndex == 1)
				{
					// foo[n] -> 1 dimension
					return 1;
				}
				else if (numberOfIndex == 2)
				{
					// foo[n:m] -> compute the span
					return index->getImmediateValue(1) - index->getImmediateValue(0) + 1;
				}
				else
					// whaaaaat? Are you trying foo[[1,2,3]]?
					throw TranslatableError(sourcePos, ERROR_ARRAY_ILLEGAL_ACCESS);
			}
			else
				// random access foo[expr]
				return 1;
		}
		else
			// full array access
			return arraySize;
	}

	bool TupleVectorNode::isImmediateVector() const
	{
		NodesVector::const_iterator it;
		for (it = children.begin(); it != children.end(); it++)
		{
			ImmediateNode* node = dynamic_cast<ImmediateNode*>(*it);
			if (!node)
				return false;
		}
		return true;
	}

	//! return the n-th vector's element
	int TupleVectorNode::getImmediateValue(unsigned index) const
	{
		assert(index < getVectorSize());
		assert(isImmediateVector());
		ImmediateNode* node = dynamic_cast<ImmediateNode*>(children[index]);
		assert(node);
		return node->value;
	}
}
