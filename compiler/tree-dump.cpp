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
#include <ostream>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	std::wstring binaryOperatorToString(AsebaBinaryOperator op)
	{
		switch (op)
		{
			case ASEBA_OP_SHIFT_LEFT: return L"<<";
			case ASEBA_OP_SHIFT_RIGHT: return L">>";
			case ASEBA_OP_ADD: return L"+";
			case ASEBA_OP_SUB: return L"-";
			case ASEBA_OP_MULT: return L"*";
			case ASEBA_OP_DIV: return L"/";
			case ASEBA_OP_MOD: return L"modulo";
			
			case ASEBA_OP_BIT_OR: return L"binary or";
			case ASEBA_OP_BIT_XOR: return L"binary xor";
			case ASEBA_OP_BIT_AND: return L"binary and";
			
			case ASEBA_OP_EQUAL: return L"==";
			case ASEBA_OP_NOT_EQUAL: return L"!=";
			case ASEBA_OP_BIGGER_THAN: return L">";
			case ASEBA_OP_BIGGER_EQUAL_THAN: return L">=";
			case ASEBA_OP_SMALLER_THAN: return L"<";
			case ASEBA_OP_SMALLER_EQUAL_THAN: return L"<=";
			
			case ASEBA_OP_OR: return L"or";
			case ASEBA_OP_AND: return L"and";
			
			default: return L"? (binary operator)";
		}
	}
	
	std::wstring unaryOperatorToString(AsebaUnaryOperator op)
	{
		switch (op)
		{
			case ASEBA_UNARY_OP_SUB: return L"unary -";
			case ASEBA_UNARY_OP_ABS: return L"abs";
			case ASEBA_UNARY_OP_BIT_NOT: return L"binary not";
			case ASEBA_UNARY_OP_NOT: return L"not";
			
			default: return L"? (unary operator)";
		}
	}
	
	void Node::dump(std::wostream& dest, unsigned& indent) const
	{
		for (unsigned i = 0; i < indent; i++)
			dest << L"    ";
		dest << toWString() << L"\n";
		indent++;
		for (size_t i = 0; i < children.size(); i++)
			children[i]->dump(dest, indent);
		indent--;
	}

	void TupleVectorNode::dump(std::wostream& dest, unsigned& indent) const
	{
		for (unsigned i = 0; i < indent; i++)
			dest << L"    ";
		dest << toWString() << L"\n";
		indent++;
		for (size_t i = 0; i < children.size(); i++)
		{
			for (unsigned j = 0; j < indent; j++)
				dest << L"    ";
			dest << WFormatableString(L"[%0]:\n").arg(i);
			indent++;
			children[i]->dump(dest, indent);
			indent--;
		}
		indent--;
	}
	
	std::wstring IfWhenNode::toWString() const
	{
		std::wstring s;
		if (edgeSensitive)
			s += L"When: ";
		else
			s += L"If: ";
		return s;
	}
	
	std::wstring FoldedIfWhenNode::toWString() const
	{
		std::wstring s;
		if (edgeSensitive)
			s += L"Folded When: ";
		else
			s += L"Folded If: ";
		s += binaryOperatorToString(op);
		return s;
	}
	
	std::wstring WhileNode::toWString() const
	{
		std::wstring s = L"While: ";
		return s;
	};
	
	std::wstring FoldedWhileNode::toWString() const
	{
		std::wstring s = L"Folded While: ";
		s += binaryOperatorToString(op);
		return s;
	};
	
	std::wstring EventDeclNode::toWString() const
	{
		if (eventId == ASEBA_EVENT_INIT)
			return L"ContextSwitcher: to init";
		else
			return WFormatableString(L"ContextSwitcher: to event %0").arg(eventId);
	}
	
	std::wstring EmitNode::toWString() const
	{
		std::wstring s = WFormatableString(L"Emit: %0 ").arg(eventId);
		if (arraySize)
			s += WFormatableString(L"addr %0 size %1 ").arg(arrayAddr).arg(arraySize);
		return s;
	}
	
	std::wstring SubDeclNode::toWString() const
	{
		return WFormatableString(L"Sub: %0").arg(subroutineId);
	}
	
	std::wstring CallSubNode::toWString() const
	{
		std::wstring s = L"CallSub: ";
		s += subroutineName;
		return s;
	}

	std::wstring BinaryArithmeticNode::toWString() const
	{
		std::wstring s = L"BinaryArithmetic: ";
		s += binaryOperatorToString(op);
		return s;
	}
	
	std::wstring UnaryArithmeticNode::toWString() const
	{
		std::wstring s = L"UnaryArithmetic: ";
		s += unaryOperatorToString(op);
		return s;
	}
	
	std::wstring ImmediateNode::toWString() const
	{
		return WFormatableString(L"Immediate: %0").arg(value);
	}

	std::wstring LoadNode::toWString() const
	{
		return WFormatableString(L"Load: addr %0").arg(varAddr);
	}

	std::wstring StoreNode::toWString() const
	{
		return WFormatableString(L"Store: addr %0").arg(varAddr);
	}

	std::wstring ArrayReadNode::toWString() const
	{
		return WFormatableString(L"ArrayRead: addr %0 size %1 (var %2)").arg(arrayAddr).arg(arraySize).arg(arrayName);
	}
	
	std::wstring ArrayWriteNode::toWString() const
	{
		return WFormatableString(L"ArrayWrite: addr %0 size %1 (var %2)").arg(arrayAddr).arg(arraySize).arg(arrayName);
	}
	
	std::wstring LoadNativeArgNode::toWString() const
	{
		return WFormatableString(L"LoadNativeArgNode: addr %0 size %1 (var %2) using temp %3").arg(arrayAddr).arg(arraySize).arg(arrayName).arg(tempAddr);
	}

	std::wstring CallNode::toWString() const
	{
		std::wstring s = WFormatableString(L"Call: id %0").arg(funcId);
		for (unsigned i = 0; i < templateArgs.size(); i++)
			s += WFormatableString(L", template %0 has size %1").arg(i).arg(templateArgs[i]);
		return s;
	}
	
	std::wstring TupleVectorNode::toWString() const
	{
		return WFormatableString(L"Tuple Vector: size %0").arg(getVectorSize());
	}

	std::wstring MemoryVectorNode::toWString() const
	{
		return WFormatableString(L"Memory Vector Access: addr %0 size %1 (var %2)").arg(arrayAddr).arg(arraySize).arg(arrayName);
	}

	std::wstring ArithmeticAssignmentNode::toWString() const
	{
		std::wstring s = L"BinaryArithmeticAssign: ";
		s += binaryOperatorToString(op);
		return s;
	}

	std::wstring UnaryArithmeticAssignmentNode::toWString() const
	{
		 std::wstring s = L"UnaryArithmeticAssign: ";
		 if (arithmeticOp == ASEBA_OP_ADD)
			 s += L"++";
		 else if (arithmeticOp == ASEBA_OP_SUB)
			 s += L"--";
		 else
			 s += L"unknown";
		 return s;
	}

	/*@}*/
	
} // namespace Aseba
