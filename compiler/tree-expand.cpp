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
#include "../common/utils/FormatableString.h"

#include <cassert>
#include <memory>
#include <iostream>

namespace Aseba
{
	/*
	 * Tree expansion: PASS 1 (Abstract nodes)
	 *   - Nodes inherited from AbstractTreeNode are expanded into "real" nodes (one that can emit bytecode)
	 *   - However, data and memory access nodes (TupleVectorNode and MemoryVectorNode) are not expanded, as they
	 *     will be expanded during the second pass
	 *   - Memory management rule: if a node must transform itself into another node, it create the new node, reparent
	 *     the children to the new node, and return the newly created node. It is the responsability of the parent
	 *     to remark the new node and delete the orphaned child (generically implemented in Node::expandAbstractNodes())
	 *
	 * Ex:
	 *                  i++                                             i = i + 1
	 *
	 *     UnaryArithmeticAssignmentNode (++)      => =>              AssignmentNode
	 *                   |                                                   |
	 *                   |                                      ---------------------------
	 *           MemoryVectorNode (i)                           |                         |
	 *                                                MemoryVectorNode (i)     BinaryArithmeticNode (+)
	 *                                                                                    |
	 *                                                                       ----------------------------
	 *                                                                       |                          |
	 *                                                              MemoryVectorNode (i)         TupleVectorNode (1)
	 *
	 *     In this case, UnaryArithmeticAssignmentNode will create the new AssignmentNode, reparent its child, create the new subtree,
	 *     and return the pointer to AssignmentNode. The parent of UnaryArithmeticAssignmentNode must delete its old child, and
	 *     perform the substitution with the new one. Failing to do so will result in memory leaks.
	 */

	//! Generically traverse the tree, expand the children, and perform garbagge collection
	Node* Node::expandAbstractNodes(std::wostream *dump)
	{
		Node * child;
		// recursively walk the tree and expand children
		// if the child return a new pointer, delete the orphaned node
		for (NodesVector::iterator it = children.begin(); it != children.end();)
		{
			child = (*it)->expandAbstractNodes(dump);
			if (child != *(it)) {
				delete *(it);
			}
			*(it) = child;
			++it;
		}
		return this;
	}

	//! Dummy function, the node will be expanded during the vectorial pass
	Node* TupleVectorNode::expandAbstractNodes(std::wostream *dump)
	{
		return this;
	}

	//! Dummy function, the node will be expanded during the vectorial pass
	Node* MemoryVectorNode::expandAbstractNodes(std::wostream *dump)
	{
		return this;
	}

	//! Expand "vector (opop)" to "vector (op)= 1", and then call for the expansion of this last expression
	Node* UnaryArithmeticAssignmentNode::expandAbstractNodes(std::wostream* dump)
	{
		assert(children.size() == 1);

		Node* memoryVector = children[0];

		// create a vector of 1's
		std::auto_ptr<TupleVectorNode> constant(new TupleVectorNode(sourcePos));
		for (unsigned int i = 0; i < memoryVector->getVectorSize(); i++)
			constant->addImmediateValue(1);

		// expand to "vector (op)= 1"
		std::auto_ptr<ArithmeticAssignmentNode> assignment(new ArithmeticAssignmentNode(sourcePos, arithmeticOp, memoryVector, constant.get()));
		constant.release();

		// perform the expansion of ArithmeticAssignmentNode
		std::auto_ptr<Node> finalBlock(assignment->expandAbstractNodes(dump));

		if (assignment.get() != finalBlock.get())
			// delete the orphaned node
			assignment.reset();
		else
			assignment.release();

		// let our children to stay with the new node
		// otherwise they will be freed when we are cleared
		children.clear();

		return finalBlock.release();
	}

	//! Expand "left (op)= right" to "left = left (op) right"
	Node* ArithmeticAssignmentNode::expandAbstractNodes(std::wostream* dump)
	{
		assert(children.size() == 2);

		// expand the sub-trees
		Node::expandAbstractNodes(dump);

		Node* leftVector = children[0];
		Node* rightVector = children[1];

		// create the replacement node
		std::auto_ptr<BinaryArithmeticNode> binary(new BinaryArithmeticNode(sourcePos, op, leftVector, rightVector));
		std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(sourcePos, leftVector->deepCopy(), binary.release()));

		// let our children to stay with the new AssignmentNode
		// otherwise they will be freed when we are cleared
		children.clear();

		return assignment.release();;
	}



	/*
	 * Tree expansion: PASS 2 (Vectorial nodes)
	 *   - Nodes performing operations on vectors are expanded into several equivalent operations on scalars
	 *   - Memory management rule: To make it simple, the whole tree is duplicated (each node as to copy itself,
	 *       or create new nodes to replace it). Then the old tree is deleted as a whole. I agree, this could
	 *       be more subtle, but it is the easiest solution. A similar mechanism to pass 1 could be considered.
	 *
	 * Ex:                                                                         buffer[0] = 1
	 *                   buffer = [1,2]                                            buffer[1] = 2
	 *
	 *                   AssignmentNode                   => =>                       BlockNode
	 *                          |                                                         |
	 *                ---------------------                                   ----------------------------
	 *                |                   |                                   |                          |
	 *        MemoryVectorNode     TupleVectorNode                     AssignmentNode             AssignmentNode
	 *            (buffer)                |                                   |                          |
	 *                                    |                          -----------------            ---------------
	 *                         ---------------------                 |               |            |             |
	 *                         |                   |             StoreNode     ImmediateNode  StoreNode   ImmediateNode
	 *                  ImmediateNode (1)   ImmediateNode(2)    (buffer[0])         (1)      (buffer[1])        (2)
	 *
	 */

	/*
	 * helper function to know if a MemoryVectorNode belonging to the tree of 'root' as a name
	 * matching 'name'
	 */
	static bool matchNameInMemoryVector(Node *root, std::wstring name)
	{
		// do I match?
		MemoryVectorNode* vector = dynamic_cast<MemoryVectorNode*>(root);
		if (vector && vector->arrayName == name)
			return true;

		// search inside children
		for (unsigned int i = 0; i < root->children.size(); i++)
			if (matchNameInMemoryVector(root->children[i], name))
				return true;

		// definitly no match
		return false;
	}

	//! This is the root node, take in charge the tree creation / deletion
	Node* ProgramNode::expandVectorialNodes(std::wostream *dump, Compiler* compiler, unsigned int index)
	{
		Node* newMe = Node::expandVectorialNodes(dump, compiler, index);

		delete this;	// delete the whole (obsolete) vectorial tree
		return newMe;
	}

	//! Generic implementation for non-vectorial nodes
	Node* Node::expandVectorialNodes(std::wostream *dump, Compiler* compiler, unsigned int index)
	{
		// duplicate me
		std::auto_ptr<Node> newMe(this->shallowCopy());
		newMe->children.clear();

		// recursively walk the tree and expand children (of the newly created tree)
		for (unsigned int i = 0; i < this->children.size(); i++)
			newMe->children.push_back(this->children[i]->expandVectorialNodes(dump, compiler, index));

		return newMe.release();
	}

	//! Assignment between vectors is expanded into multiple scalar assignments
	Node* AssignmentNode::expandVectorialNodes(std::wostream *dump, Compiler* compiler, unsigned int index)
	{
		assert(children.size() == 2);

		// left vector should reference a memory location
		MemoryVectorNode* leftVector = dynamic_cast<MemoryVectorNode*>(children[0]);
		if (!leftVector)
			throw TranslatableError(sourcePos, ERROR_INCORRECT_LEFT_VALUE).arg(children[0]->toNodeName());
		leftVector->setWrite(true);

		// right vector can be anything
		Node* rightVector = children[1];

		// check if the left vector appears somewhere on the right side
		if (matchNameInMemoryVector(rightVector, leftVector->arrayName) && leftVector->getVectorSize() > 1)
		{
			// in such case, there is a risk of involuntary overwriting the content
			// we need to throw in a temporary variable to avoid this risk
			std::auto_ptr<BlockNode> tempBlock(new BlockNode(sourcePos));

			// tempVar = rightVector
			std::auto_ptr<AssignmentNode> temp(compiler->allocateTemporaryVariable(sourcePos, rightVector->deepCopy()));
			MemoryVectorNode* tempVar = dynamic_cast<MemoryVectorNode*>(temp->children[0]);
			assert(tempVar);
			tempBlock->children.push_back(temp.release());

			// leftVector = tempVar
			temp.reset(new AssignmentNode(sourcePos, leftVector->deepCopy(), tempVar->deepCopy()));
			tempBlock->children.push_back(temp.release());

			return tempBlock->expandVectorialNodes(dump, compiler); // tempBlock will be reclaimed
		}
		// else

		std::auto_ptr<BlockNode> block(new BlockNode(sourcePos)); // top-level block

		for (unsigned int i = 0; i < leftVector->getVectorSize(); i++)
		{
			// expand to left[i] = right[i]
			block->children.push_back(new AssignmentNode(sourcePos,
								      leftVector->expandVectorialNodes(dump, compiler, i),
								      rightVector->expandVectorialNodes(dump, compiler, i)));
		}

		return block.release();
	}

	//! Expand to vector[index]
	Node* TupleVectorNode::expandVectorialNodes(std::wostream *dump, Compiler* compiler, unsigned int index)
	{
		size_t total = 0;
		size_t prevTotal = 0;

		// as a TupleVectorNode can be composed of several sub-vectors, find which child
		// corresponds to the index
		for (size_t i = 0; i < children.size(); i++)
		{
			prevTotal = total;
			total += children[i]->getVectorSize();
			if (index < total)
			{
				// the index points to this child
				return children[i]->expandVectorialNodes(dump, compiler, index-prevTotal);
			}
		}

		// should not happen !!
		assert(0);
		return 0;
	}

	//! Expand to memory[index]
	Node* MemoryVectorNode::expandVectorialNodes(std::wostream *dump, Compiler* compiler, unsigned int index)
	{
		assert(index < getVectorSize());

		// get the optional index given in the Aseba code
		TupleVectorNode* accessIndex = NULL;
		if (children.size() > 0)
			accessIndex = dynamic_cast<TupleVectorNode*>(children[0]);

		if (accessIndex || children.size() == 0)
		{
			// direct access. Several cases:
			// -> an immediate index "foo[n]" or "foo[n:m]"
			// -> full array access "foo"
			// => use a StoreNode (lvalue) or LoadNode (rvalue)

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
			// => use a ArrayWriteNode (lvalue) or ArrayReadNode (rvalue)

			std::auto_ptr<Node> array;
			if (write == true)
				array.reset(new ArrayWriteNode(sourcePos, arrayAddr, arraySize, arrayName));
			else
				array.reset(new ArrayReadNode(sourcePos, arrayAddr, arraySize, arrayName));

			array->children.push_back(children[0]->expandVectorialNodes(dump, compiler, index));
			return array.release();
		}
	}

	//! Expand into a copy of itself
	Node* ImmediateNode::expandVectorialNodes(std::wostream *dump, Compiler* compiler, unsigned int index)
	{
		assert(index == 0);

		return shallowCopy();
	}


	/*
	 * Functions used to check vectors' consistency
	 */

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


	/*
	 * Functions used to compute vectors' size and base memory address
	 */

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

		int shift = 0;

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

		if (shift < 0 || shift >= int(arraySize))
			throw TranslatableError(sourcePos, ERROR_ARRAY_OUT_OF_BOUND).arg(arrayName).arg(shift).arg(arraySize);
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
					const int im0(index->getImmediateValue(0));
					const int im1(index->getImmediateValue(1));
					if (im1 < 0 || im1 >= int(arraySize))
						throw TranslatableError(sourcePos, ERROR_ARRAY_OUT_OF_BOUND).arg(arrayName).arg(im1).arg(arraySize);
					// foo[n:m] -> compute the span
					return im1 - im0 + 1;
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
	

	/*
	 * Convenience functions
	 */

	//! return whether this node accesses a static address
	bool MemoryVectorNode::isAddressStatic() const
	{
		if (children.empty())
			return true;
		TupleVectorNode* index = dynamic_cast<TupleVectorNode*>(children[0]);
		if (index && index->children.size() <= 2 && index->isImmediateVector())
			return true;
		return false;
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
