/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
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
#include "../utils/FormatableString.h"
#include <ostream>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	std::string binaryOperatorToString(AsebaBinaryOperator op)
	{
		switch (op)
		{
			case ASEBA_OP_SHIFT_LEFT: return "<<";
			case ASEBA_OP_SHIFT_RIGHT: return ">>";
			case ASEBA_OP_ADD: return "+";
			case ASEBA_OP_SUB: return "-";
			case ASEBA_OP_MULT: return "*";
			case ASEBA_OP_DIV: return "/";
			case ASEBA_OP_MOD: return "modulo";
			
			case ASEBA_OP_BIT_OR: return "binary or";
			case ASEBA_OP_BIT_XOR: return "binary xor";
			case ASEBA_OP_BIT_AND: return "binary and";
			
			case ASEBA_OP_EQUAL: return "==";
			case ASEBA_OP_NOT_EQUAL: return "!=";
			case ASEBA_OP_BIGGER_THAN: return ">";
			case ASEBA_OP_BIGGER_EQUAL_THAN: return ">=";
			case ASEBA_OP_SMALLER_THAN: return "<";
			case ASEBA_OP_SMALLER_EQUAL_THAN: return "<=";
			
			case ASEBA_OP_OR: return "or";
			case ASEBA_OP_AND: return "and";
			
			default: return "? (binary operator)";
		}
	}
	
	std::string unaryOperatorToString(AsebaUnaryOperator op)
	{
		switch (op)
		{
			case ASEBA_UNARY_OP_SUB: return "unary -";
			case ASEBA_UNARY_OP_ABS: return "abs";
			case ASEBA_UNARY_OP_BIT_NOT: return "binary not";
			
			default: return "? (unary operator)";
		}
	}
	
	void Node::dump(std::ostream& dest, unsigned& indent) const
	{
		for (unsigned i = 0; i < indent; i++)
			dest << "    ";
		dest << toString() << "\n";
		indent++;
		for (size_t i = 0; i < children.size(); i++)
			children[i]->dump(dest, indent);
		indent--;
	}
	
	std::string IfWhenNode::toString() const
	{
		std::string s;
		if (edgeSensitive)
			s += "When: ";
		else
			s += "If: ";
		return s;
	}
	
	std::string FoldedIfWhenNode::toString() const
	{
		std::string s;
		if (edgeSensitive)
			s += "Folded When: ";
		else
			s += "Folded If: ";
		s += binaryOperatorToString(op);
		return s;
	}
	
	std::string WhileNode::toString() const
	{
		std::string s = "While: ";
		return s;
	};
	
	std::string FoldedWhileNode::toString() const
	{
		std::string s = "Folded While: ";
		s += binaryOperatorToString(op);
		return s;
	};
	
	std::string EventDeclNode::toString() const
	{
		if (eventId == ASEBA_EVENT_INIT)
			return "ContextSwitcher: to init";
		else
			return FormatableString("ContextSwitcher: to event %0").arg(eventId);
	}
	
	std::string EmitNode::toString() const
	{
		std::string s = FormatableString("Emit: %0 ").arg(eventId);
		if (arraySize)
			s += FormatableString("addr %0 size %1 ").arg(arrayAddr).arg(arraySize);
		return s;
	}
	
	std::string SubDeclNode::toString() const
	{
		return FormatableString("Sub: %0").arg(subroutineId);
	}
	
	std::string CallSubNode::toString() const
	{
		return FormatableString("CallSub: %0").arg(subroutineId);
	}

	std::string BinaryArithmeticNode::toString() const
	{
		std::string s = "BinaryArithmetic: ";
		s += binaryOperatorToString(op);
		return s;
	}
	
	std::string UnaryArithmeticNode::toString() const
	{
		std::string s = "UnaryArithmetic: ";
		s += unaryOperatorToString(op);
		return s;
	}
	
	std::string ImmediateNode::toString() const
	{
		return FormatableString("Immediate: %0").arg(value);
	}

	std::string LoadNode::toString() const
	{
		return FormatableString("Load: addr %0").arg(varAddr);
	}

	std::string StoreNode::toString() const
	{
		return FormatableString("Store: addr %0").arg(varAddr);
	}

	std::string ArrayReadNode::toString() const
	{
		return FormatableString("ArrayRead: addr %0 size %1").arg(arrayAddr).arg(arraySize);
	}
	
	std::string ArrayWriteNode::toString() const
	{
		return FormatableString("ArrayWrite: addr %0 size %1").arg(arrayAddr).arg(arraySize);
	}
	
	std::string CallNode::toString() const
	{
		std::string s = FormatableString("Call: id %0").arg(funcId);
		for (unsigned i = 0; i < argumentsAddr.size(); i++)
			s += FormatableString(", arg %0 is addr %1").arg(i).arg(argumentsAddr[i]);
		return s;
	}
	
	/*@}*/
	
}; // Aseba
