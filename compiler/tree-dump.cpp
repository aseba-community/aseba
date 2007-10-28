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
#include "../utils/FormatableString.h"
#include <ostream>

namespace Aseba
{
	std::string comparaisonOperatorToString(AsebaComparaison op)
	{
		switch (op)
		{
			case ASEBA_CMP_EQUAL: return "=="; break;
			case ASEBA_CMP_NOT_EQUAL: return "!="; break;
			case ASEBA_CMP_BIGGER_THAN: return ">"; break;
			case ASEBA_CMP_BIGGER_EQUAL_THAN: return ">="; break;
			case ASEBA_CMP_SMALLER_THAN: return "<"; break;
			case ASEBA_CMP_SMALLER_EQUAL_THAN: return "<="; break;
			default: return "? (comparaison)";
		}
	}
	
	std::string binaryOperatorToString(AsebaBinaryOperator op)
	{
		switch (op)
		{
			case ASEBA_OP_SHIFT_LEFT: return "<<"; break;
			case ASEBA_OP_SHIFT_RIGHT: return ">>"; break;
			case ASEBA_OP_ADD: return "+"; break;
			case ASEBA_OP_SUB: return "-"; break;
			case ASEBA_OP_MULT: return "*"; break;
			case ASEBA_OP_DIV: return "/"; break;
			case ASEBA_OP_MOD: return "modulo"; break;
			default: return "? (binary operator)";
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
		s += comparaisonOperatorToString(comparaison);
		return s;
	}
	
	std::string WhileNode::toString() const
	{
		std::string s = "While: ";
		s += comparaisonOperatorToString(comparaison);
		return s;
	};
	
	std::string ContextSwitcherNode::toString() const
	{
		if (eventId == ASEBA_EVENT_INIT)
			return "ContextSwitcher: to init";
		else if (eventId == ASEBA_EVENT_PERIODIC)
			return "ContextSwitcher: to periodic";
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

	std::string BinaryArithmeticNode::toString() const
	{
		std::string s = "BinaryArithmetic: ";
		s += binaryOperatorToString(op);
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
}; // Aseba
