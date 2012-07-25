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
#include <typeinfo>

namespace Aseba
{
	Node* Node::treeExpand(std::wostream *dump, unsigned int index)
	{
		// recursively walk the tree
		for (NodesVector::iterator it = children.begin(); it != children.end();)
		{
			*(it) = (*it)->treeExpand(dump, index);
			++it;
		}
		return this;
	}


	Node* AssignmentNode::treeExpand(std::wostream *dump, unsigned int index)
	{
		assert(children.size() == 2);

		unsigned lSize = children[0]->getMemorySize();
		unsigned rSize = children[1]->getMemorySize();

		// consistency check
		if (lSize != rSize)
			throw Error(sourcePos, WFormatableString(L"Size error! Size of array1 = %0 ; size of array2 = %1").arg(lSize).arg(rSize));

		MemoryVectorNode* leftVector = dynamic_cast<MemoryVectorNode*>(children[0]);
		assert(leftVector);
		leftVector->setWrite(true);
		Node* rightVector = children[1];

		std::auto_ptr<BlockNode> block(new BlockNode(sourcePos));
		// for each dimension
		for (unsigned int i = 0; i < lSize; i++)
		{
			std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(sourcePos));
			assignment->children.push_back(leftVector->treeExpand(dump, i));
			assignment->children.push_back(rightVector->treeExpand(dump, i));
			block->children.push_back(assignment.release());
		}

		delete this;
		return block.release();
	}


	Node* BinaryArithmeticNode::treeExpand(std::wostream *dump, unsigned int index)
	{
		std::auto_ptr<Node> left(children[0]->treeExpand(dump, index));
		std::auto_ptr<Node> right(children[1]->treeExpand(dump, index));
		return new BinaryArithmeticNode(sourcePos, op, left.release(), right.release());
	}


	Node* UnaryArithmeticNode::treeExpand(std::wostream *dump, unsigned int index)
	{
		std::auto_ptr<Node> left(children[0]->treeExpand(dump, index));
		return new UnaryArithmeticNode(sourcePos, op, left.release());
	}


	Node* StaticVectorNode::treeExpand(std::wostream *dump, unsigned int index)
	{
		return new ImmediateNode(sourcePos, getValue(index));
	}


	Node* MemoryVectorNode::treeExpand(std::wostream *dump, unsigned int index)
	{
		assert(index < getMemorySize());

		StaticVectorNode* accessIndex = NULL;
		if (children.size() > 0)
			accessIndex = dynamic_cast<StaticVectorNode*>(children[0]);

		if (accessIndex || children.size() == 0)
		{
			// immediate index / full array access
			unsigned pointer = getMemoryAddr() + index;
			// check if index is within bounds
			if (pointer >= arrayAddr + arraySize)
				throw Error(sourcePos, WFormatableString(L"Access of array %0 out of bounds").arg(arrayName));

			if (write == true)
				return new StoreNode(sourcePos, pointer);
			else
				return new LoadNode(sourcePos, pointer);
		}
		else
		{
			// indirect access
			std::auto_ptr<Node> array;
			if (write == true)
			{
				array.reset(new ArrayWriteNode(sourcePos, arrayAddr, arraySize, arrayName));
			}
			else
			{
				array.reset(new ArrayReadNode(sourcePos, arrayAddr, arraySize, arrayName));
			}

			array->children.push_back(children[0]->treeExpand(dump, index));
			children[0] = 0;
			return array.release();
		}
	}


	Node* ArithmeticAssignmentNode::treeExpand(std::wostream* dump, unsigned int index)
	{
		assert(children.size() == 2);

		Node* leftVector = children[0];
		Node* rightVector = children[1];

		std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(sourcePos));
		std::auto_ptr<BinaryArithmeticNode> binary(new BinaryArithmeticNode(sourcePos, op, leftVector->deepCopy(), rightVector->deepCopy()));
		assignment->children.push_back(leftVector->deepCopy());
		assignment->children.push_back(binary.release());

		std::auto_ptr<Node> finalBlock(assignment->treeExpand(dump, index));

		assignment.release();
		delete this;

		return finalBlock.release();
	}


	Node* UnaryArithmeticAssignmentNode::treeExpand(std::wostream* dump, unsigned int index)
	{
		assert(children.size() == 1);

		Node* memoryVector = children[0];

		// create a vector of 1's
		std::auto_ptr<StaticVectorNode> constant(new StaticVectorNode(sourcePos));
		for (unsigned int i = 0; i < memoryVector->getMemorySize(); i++)
			constant->addValue(1);

		// expand to "vector op= 1"
		std::auto_ptr<ArithmeticAssignmentNode> assignment(new ArithmeticAssignmentNode(sourcePos, arithmeticOp, memoryVector->deepCopy(), constant.release()));
		std::auto_ptr<Node> finalBlock(assignment->treeExpand(dump, index));

		assignment.release();
		delete this;

		return finalBlock.release();
	}


	unsigned Node::getMemorySize() const
	{
		unsigned size = E_NOVAL;
		unsigned new_size = E_NOVAL;

		for (NodesVector::const_iterator it = children.begin(); it != children.end(); ++it)
		{
			new_size = (*it)->getMemorySize();
			if (size == E_NOVAL)
				size = new_size;
			else if (size != new_size)
				throw Error(sourcePos, WFormatableString(L"Size error! Size of array1 = %0 ; size of array2 = %1").arg(size).arg(new_size));
		}

		return size;
	}


	unsigned Node::getMemoryAddr() const
	{
		if (children.size() > 0)
			return children[0]->getMemoryAddr();
		else
			return E_NOVAL;
	}


	int StaticVectorNode::getLonelyImmediate() const
	{
		assert(getMemorySize() >= 1);
		return values[0];
	}

	int StaticVectorNode::getValue(unsigned index) const
	{
		assert(index < getMemorySize());
		return values[index];
	}

	unsigned MemoryVectorNode::getMemoryAddr() const
	{
		assert(children.size() <= 1);

		unsigned shift = 0;

		// index(es) given?
		if (children.size() == 1)
		{
			StaticVectorNode* index = dynamic_cast<StaticVectorNode*>(children[0]);
			if (index)
			{
				shift = index->getValue(0);
			}
			else
				// not know at compile time
				return E_NOVAL;
		}

		return arrayAddr + shift;
	}


	unsigned MemoryVectorNode::getMemorySize() const
	{
		assert(children.size() <= 1);

		if (children.size() == 1)
		{
			StaticVectorNode* index = dynamic_cast<StaticVectorNode*>(children[0]);
			if (index)
			{
				unsigned numberOfIndex = index->getMemorySize();
				// immediate indexes
				if (numberOfIndex == 1)
				{
					// 1 index is given -> 1 dimension
					return 1;
				}
				else if (numberOfIndex == 2)
				{
					// 2 indexes are given -> compute the span
					return index->getValue(1) - index->getValue(0) + 1;
				}
				else
					// whaaaaat? Are you trying foo[[1,2,3]]?
					throw Error(sourcePos, L"Illegal operation");
			}
			else
				// 1 index, random access
				return 1;
		}
		else
			// full array access
			return arraySize;
	}
}
