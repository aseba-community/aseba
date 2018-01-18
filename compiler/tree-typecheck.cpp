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
#include "../common/utils/FormatableString.h"
#include <cstdlib>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/

	std::wstring Node::typeName(const Node::ReturnType& type) const
	{
		switch (type)
		{
			case ReturnType::UNIT: return L"unit";
			case ReturnType::BOOL: return L"bool";
			case ReturnType::INT: return L"integer";
			default: return L""; // cannot because ReturnType is a class enum, but gcc is too dump to notice
		}
	}

	void Node::expectType(const Node::ReturnType& expected, const Node::ReturnType& type) const
	{
		if (type != expected)
			throw TranslatableError(sourcePos, ERROR_EXPECTING_TYPE).arg(typeName(expected)).arg(typeName(type));
	};

	Node::ReturnType Node::typeCheck(Compiler* compiler)
	{
		for (NodesVector::const_iterator it = children.begin(); it != children.end(); ++it)
		{
			(*it)->typeCheck(compiler);
		}
		return ReturnType::UNIT;
	}

	Node::ReturnType AssignmentNode::typeCheck(Compiler* compiler)
	{
		expectType(ReturnType::UNIT, children[0]->typeCheck(compiler));
		expectType(ReturnType::INT, children[1]->typeCheck(compiler));
		return ReturnType::UNIT;
	}

	Node::ReturnType IfWhenNode::typeCheck(Compiler* compiler)
	{
		expectType(ReturnType::BOOL, children[0]->typeCheck(compiler));
		expectType(ReturnType::UNIT, children[1]->typeCheck(compiler));
		if (children.size() > 2)
			expectType(ReturnType::UNIT, children[2]->typeCheck(compiler));

		auto* binaryOp = dynamic_cast<BinaryArithmeticNode*>(children[0]);
		auto* unaryOp = dynamic_cast<UnaryArithmeticNode*>(children[0]);
		bool ok(false);
		if (binaryOp && binaryOp->op >= ASEBA_OP_EQUAL && binaryOp->op <= ASEBA_OP_AND)
			ok = true;
		if (unaryOp && unaryOp->op == ASEBA_UNARY_OP_NOT)
			ok = true;

		if (!ok)
			throw TranslatableError(children[0]->sourcePos, ERROR_EXPECTING_CONDITION).arg(children[0]->toNodeName());
		return ReturnType::UNIT;
	}

	Node::ReturnType WhileNode::typeCheck(Compiler* compiler)
	{
		expectType(ReturnType::BOOL, children[0]->typeCheck(compiler));
		expectType(ReturnType::UNIT, children[1]->typeCheck(compiler));

		auto* binaryOp = dynamic_cast<BinaryArithmeticNode*>(children[0]);
		auto* unaryOp = dynamic_cast<UnaryArithmeticNode*>(children[0]);
		bool ok(false);
		if (binaryOp && binaryOp->op >= ASEBA_OP_EQUAL && binaryOp->op <= ASEBA_OP_AND)
			ok = true;
		if (unaryOp && unaryOp->op == ASEBA_UNARY_OP_NOT)
			ok = true;

		if (!ok)
			throw TranslatableError(children[0]->sourcePos, ERROR_EXPECTING_CONDITION).arg(children[0]->toNodeName());
		return ReturnType::UNIT;
	}

	Node::ReturnType CallSubNode::typeCheck(Compiler* compiler)
	{
		subroutineId = compiler->findSubroutine(subroutineName, sourcePos)->second;
		return ReturnType::UNIT;
	}

	Node::ReturnType BinaryArithmeticNode::typeCheck(Compiler* compiler)
	{
		switch (op)
		{
			case ASEBA_OP_SHIFT_LEFT:
			case ASEBA_OP_SHIFT_RIGHT:
			case ASEBA_OP_ADD:
			case ASEBA_OP_SUB:
			case ASEBA_OP_MULT:
			case ASEBA_OP_DIV:
			case ASEBA_OP_MOD:
			case ASEBA_OP_BIT_OR:
			case ASEBA_OP_BIT_XOR:
			case ASEBA_OP_BIT_AND:
				expectType(ReturnType::INT, children[0]->typeCheck(compiler));
				expectType(ReturnType::INT, children[1]->typeCheck(compiler));
				return ReturnType::INT;

			case ASEBA_OP_EQUAL:
			case ASEBA_OP_NOT_EQUAL:
			case ASEBA_OP_BIGGER_THAN:
			case ASEBA_OP_BIGGER_EQUAL_THAN:
			case ASEBA_OP_SMALLER_THAN:
			case ASEBA_OP_SMALLER_EQUAL_THAN:
				expectType(ReturnType::INT, children[0]->typeCheck(compiler));
				expectType(ReturnType::INT, children[1]->typeCheck(compiler));
				return ReturnType::BOOL;

			case ASEBA_OP_OR:
			case ASEBA_OP_AND:
				expectType(ReturnType::BOOL, children[0]->typeCheck(compiler));
				expectType(ReturnType::BOOL, children[1]->typeCheck(compiler));
				return ReturnType::BOOL;

			default:
				abort();
				return ReturnType::UNIT;
		}
	}

	Node::ReturnType UnaryArithmeticNode::typeCheck(Compiler* compiler)
	{
		switch (op)
		{
			case ASEBA_UNARY_OP_SUB:
			case ASEBA_UNARY_OP_ABS:
			case ASEBA_UNARY_OP_BIT_NOT:
				expectType(ReturnType::INT, children[0]->typeCheck(compiler));
				return ReturnType::INT;

			case ASEBA_UNARY_OP_NOT:
				expectType(ReturnType::BOOL, children[0]->typeCheck(compiler));
				return ReturnType::BOOL;

			default:
				abort();
				return ReturnType::UNIT;
		}
	}

	/*@}*/

} // namespace Aseba

