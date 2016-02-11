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
#include <cstdlib>
#include <iostream>


namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/

	const AsebaBinaryOperator ArithmeticAssignmentNode::operatorMap[] = {
		ASEBA_OP_ADD,			// TOKEN_OP_ADD_EQUAL
		ASEBA_OP_SUB,			// TOKEN_OP_NEG_EQUAL
		ASEBA_OP_MULT,			// TOKEN_OP_MULT_EQUAL
		ASEBA_OP_DIV,			// TOKEN_OP_DIV_EQUAL
		ASEBA_OP_MOD,			// TOKEN_OP_MOD_EQUAL
		ASEBA_OP_SHIFT_LEFT,		// TOKEN_OP_SHIFT_LEFT_EQUAL
		ASEBA_OP_SHIFT_RIGHT,		// TOKEN_OP_SHIFT_RIGHT_EQUAL
		ASEBA_OP_BIT_OR,		// TOKEN_OP_BIT_OR_EQUAL
		ASEBA_OP_BIT_XOR,		// TOKEN_OP_BIT_XOR_EQUAL
		ASEBA_OP_BIT_AND		// TOKEN_OP_BIT_AND_EQUAL
	};

	//! Destructor, delete all children.
	Node::~Node()
	{
		// we assume that if children is 0, another node has taken ownership of it
		for (size_t i = 0; i < children.size(); i++)
			if (children[i]) {
				delete children[i];
				children[i] = 0;
			}
	}

	Node* Node::deepCopy()
	{
		Node* newCopy = shallowCopy();
		for (size_t i = 0; i < children.size(); i++)
			if (children[i])
				newCopy->children[i] = children[i]->deepCopy();
		return newCopy;
	}

	//! Constructor
	AssignmentNode::AssignmentNode(const SourcePos &sourcePos, Node *left, Node *right) :
		Node(sourcePos)
	{
		children.push_back(left);
		children.push_back(right);
	}

	//! Constructor
	EventDeclNode::EventDeclNode(const SourcePos& sourcePos, unsigned eventId) :
		Node(sourcePos),
		eventId(eventId)
	{
	
	}
	
	//! Constructor
	SubDeclNode::SubDeclNode(const SourcePos& sourcePos, unsigned subroutineId) :
		Node(sourcePos),
		subroutineId(subroutineId)
	{
	
	}
	
	//! Constructor
	CallSubNode::CallSubNode(const SourcePos& sourcePos, const std::wstring& subroutineName) :
		Node(sourcePos),
		subroutineName(subroutineName),
		subroutineId(-1)
	{
	
	}
	
	//! Constructor
	BinaryArithmeticNode::BinaryArithmeticNode(const SourcePos& sourcePos, AsebaBinaryOperator op, Node *left, Node *right) :
		Node(sourcePos),
		op(op)
	{
		children.push_back(left);
		children.push_back(right);
	}
	
	//! Create a binary arithmetic node for comparaison operation op
	BinaryArithmeticNode *BinaryArithmeticNode::fromComparison(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right)
	{
		return new BinaryArithmeticNode(
			sourcePos,
			static_cast<AsebaBinaryOperator>((op - Compiler::Token::TOKEN_OP_EQUAL) + ASEBA_OP_EQUAL),
			left,
			right
		);
	}
	
	//! Create a binary arithmetic node for shift operation op
	BinaryArithmeticNode *BinaryArithmeticNode::fromShiftExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right)
	{
		return new BinaryArithmeticNode(
			sourcePos,
			static_cast<AsebaBinaryOperator>((op - Compiler::Token::TOKEN_OP_SHIFT_LEFT) + ASEBA_OP_SHIFT_LEFT),
			left,
			right
		);
	}
	
	//! Create a binary arithmetic node for add/sub operation op
	BinaryArithmeticNode *BinaryArithmeticNode::fromAddExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right)
	{
		return new BinaryArithmeticNode(
			sourcePos,
			static_cast<AsebaBinaryOperator>((op - Compiler::Token::TOKEN_OP_ADD) + ASEBA_OP_ADD),
			left,
			right
		);
	}
	
	//! Create a binary arithmetic node for mult/div/mod operation op
	BinaryArithmeticNode *BinaryArithmeticNode::fromMultExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right)
	{
		return new BinaryArithmeticNode(
			sourcePos,
			static_cast<AsebaBinaryOperator>((op - Compiler::Token::TOKEN_OP_MULT) + ASEBA_OP_MULT),
			left,
			right
		);
	}

	//! Create a binary arithmetic node for or/xor/and operation op
	BinaryArithmeticNode *BinaryArithmeticNode::fromBinaryExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right)
	{
		return new BinaryArithmeticNode(
			sourcePos,
			static_cast<AsebaBinaryOperator>((op - Compiler::Token::TOKEN_OP_BIT_OR) + ASEBA_OP_BIT_OR),
			left,
			right
		);
	}
	
	//! Constructor
	UnaryArithmeticNode::UnaryArithmeticNode(const SourcePos& sourcePos, AsebaUnaryOperator op, Node *child) :
		Node(sourcePos),
		op(op)
	{
		children.push_back(child);
	}
	
	//! Constructor
	ArrayWriteNode::ArrayWriteNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName) :
		Node(sourcePos),
		arrayAddr(arrayAddr),
		arraySize(arraySize),
		arrayName(arrayName)
	{
	
	}
	
	//! Constructor
	ArrayReadNode::ArrayReadNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName) :
		Node(sourcePos),
		arrayAddr(arrayAddr),
		arraySize(arraySize),
		arrayName(arrayName)
	{
	
	}
	
	//! Constructor, delete the provided memoryNode
	LoadNativeArgNode::LoadNativeArgNode(MemoryVectorNode* memoryNode, unsigned tempAddr):
		Node(memoryNode->sourcePos),
		tempAddr(tempAddr),
		arrayAddr(memoryNode->arrayAddr),
		arraySize(memoryNode->arraySize),
		arrayName(memoryNode->arrayName)
	{
		// safety check
		assert(memoryNode);
		assert(memoryNode->children.size() == 1);
		assert(memoryNode->children[0]);
		assert(!dynamic_cast<TupleVectorNode*>(memoryNode->children[0]));
		
		// get the child from memoryNode
		children.push_back(memoryNode->children[0]);
		memoryNode->children[0] = 0;
		delete memoryNode;
	}

	//! Constructor
	MemoryVectorNode::MemoryVectorNode(const SourcePos &sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName) :
		AbstractTreeNode(sourcePos),
		arrayAddr(arrayAddr),
		arraySize(arraySize),
		arrayName(arrayName),
		write(false)
	{

	}
	
	//! Constructor
	CallNode::CallNode(const SourcePos& sourcePos, unsigned funcId) :
		Node(sourcePos),
		funcId(funcId)
	{
	
	}

	//! Constructor
	ArithmeticAssignmentNode::ArithmeticAssignmentNode(const SourcePos& sourcePos, AsebaBinaryOperator op, Node *left, Node *right) :
		AbstractTreeNode(sourcePos),
		op(op)
	{
		children.push_back(left);
		children.push_back(right);
	}

	ArithmeticAssignmentNode* ArithmeticAssignmentNode::fromArithmeticAssignmentToken(const SourcePos& sourcePos, Compiler::Token::Type token, Node *left, Node *right)
	{
		return new ArithmeticAssignmentNode(sourcePos, getBinaryOperator(token), left, right);
	}

	AsebaBinaryOperator ArithmeticAssignmentNode::getBinaryOperator(Compiler::Token::Type token)
	{
		return operatorMap[token - Compiler::Token::TOKEN_OP_ADD_EQUAL];
	}

	//! Constructor
	UnaryArithmeticAssignmentNode::UnaryArithmeticAssignmentNode(const SourcePos& sourcePos, Compiler::Token::Type token, Node *memory) :
		AbstractTreeNode(sourcePos)
	{
		switch (token)
		{
		case Compiler::Token::TOKEN_OP_PLUS_PLUS:
			arithmeticOp = ASEBA_OP_ADD;
			break;

		case Compiler::Token::TOKEN_OP_MINUS_MINUS:
			arithmeticOp = ASEBA_OP_SUB;
			break;

		default:
			throw TranslatableError(sourcePos, ERROR_UNARY_ARITH_BUILD_UNEXPECTED);
			break;
		}

		children.push_back(memory);
	}
	
	/*@}*/
	
} // namespace Aseba
