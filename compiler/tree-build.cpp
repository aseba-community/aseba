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

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	//! Destructor, delete all children.
	Node::~Node()
	{
		for (size_t i = 0; i < children.size(); i++)
			delete children[i];
	}
	
	//! Constructor
	ContextSwitcherNode::ContextSwitcherNode(const SourcePos& sourcePos, unsigned eventId) :
		Node(sourcePos),
		eventId(eventId)
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
	ArrayReadNode::ArrayReadNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::string &arrayName) :
		Node(sourcePos),
		arrayAddr(arrayAddr),
		arraySize(arraySize),
		arrayName(arrayName)
	{
	
	}
	
	//! Constructor
	ArrayWriteNode::ArrayWriteNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::string &arrayName) :
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
