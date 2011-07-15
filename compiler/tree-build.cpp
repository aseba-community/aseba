/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	//! Destructor, delete all children.
	Node::~Node()
	{
		// we assume that if children is 0, another node has taken ownership of it
		for (size_t i = 0; i < children.size(); i++)
			if (children[i])
				delete children[i];
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
	CallSubNode::CallSubNode(const SourcePos& sourcePos, unsigned subroutineId) :
		Node(sourcePos),
		subroutineId(subroutineId)
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
	
	//! Constructor
	UnaryArithmeticNode::UnaryArithmeticNode(const SourcePos& sourcePos, AsebaUnaryOperator op, Node *child) :
		Node(sourcePos),
		op(op)
	{
		children.push_back(child);
	}
	
	//! Constructor
	ArrayReadNode::ArrayReadNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName) :
		Node(sourcePos),
		arrayAddr(arrayAddr),
		arraySize(arraySize),
		arrayName(arrayName)
	{
	
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
	CallNode::CallNode(const SourcePos& sourcePos, unsigned funcId) :
		Node(sourcePos),
		funcId(funcId)
	{
	
	}
	
	/*@}*/
	
}; // Aseba
