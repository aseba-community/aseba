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

#ifndef __TREE_H
#define __TREE_H

#include "compiler.h"
#include "../common/consts.h"
#include <vector>
#include <string>
#include <ostream>

namespace Aseba
{
	//! Return the string corresponding to the comparaison operator
	std::string comparaisonOperatorToString(AsebaComparaison op);
	//! Return the string corresponding to the binary operator
	std::string binaryOperatorToString(AsebaBinaryOperator op);
	
	//! An abstract node of syntax tree
	struct Node
	{
		Node(const SourcePos& sourcePos) : sourcePos(sourcePos) { }
		//! Delete children upon destruction
		virtual ~Node();
		
		//! Optimize this node, reture true if any optimization was successful
		virtual Node* optimize(std::ostream* dump) = 0;
		//! Generate bytecode
		virtual void emit(PreLinkBytecode& bytecodes) const = 0;
		
		//! Return a string representation of this node
		virtual std::string toString() const = 0;
		//! Dump this node and the rest of the tree
		virtual void dump(std::ostream& dest, unsigned& indent) const;
		
		//! Vector for children of a node
		typedef std::vector<Node *> NodesVector;
		NodesVector children; //!< children of this node
		SourcePos sourcePos; //!< position is source
	};
	
	//! Node for "block", i.e. a vector of statements
	struct BlockNode : Node
	{
		BlockNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const { return "Block"; }
	};
	
	//! Node for assignation
	//! children[0] is store code
	//! children[1] expression to store
	struct AssignmentNode : Node
	{
		AssignmentNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const { return "Assign"; }
	};
	
	//! Node for "if" and "when"
	//! children[0] is left part of conditional expression
	//! children[1] is right part of conditional expression
	//! children[2] is true block
	//! children[3] is false block
	struct IfWhenNode : Node
	{
		AsebaComparaison comparaison; //!< type of condition
		bool edgeSensitive; //!< if true, true block is triggered only if previous comparison was false ("when" block). If false, true block is triggered every time comparison is true
		
		IfWhenNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "while"
	//! children[0] is left part of conditional expression
	//! children[1] is right part of conditional expression
	//! children[2] is block
	struct WhileNode : Node
	{
		AsebaComparaison comparaison; //!< type of condition
		
		WhileNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "onevent" and "ontimer".
	//! no children
	struct ContextSwitcherNode : Node
	{
		unsigned eventId; //!< the event id associated with this context
		
		ContextSwitcherNode(const SourcePos& sourcePos, unsigned eventId = 0);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "emit"
	struct EmitNode : Node
	{
		unsigned eventId; //!< id of event to emit
		unsigned arrayAddr; //!< address of the first element of the array to send
		unsigned arraySize; //!< size of the array to send. 0 if event has no argument
		
		EmitNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for binary arithmetic
	//! children[0] is left expression
	//! children[1] is right expression
	struct BinaryArithmeticNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		
		BinaryArithmeticNode(const SourcePos& sourcePos, AsebaBinaryOperator op, Node *left, Node *right);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
		
		static BinaryArithmeticNode *fromShiftExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromAddExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromMultExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
	};
	
	//! Node for unary arithmetic, only minus for now
	//! children[0] is the expression to negate
	struct UnaryArithmeticNode : Node
	{
		UnaryArithmeticNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const { return "UnaryArithmetic: -"; }
	};
	
	//! Node for pushing immediate value on stack.
	//! Might generate either Small Immediate or Large Immediate bytecode
	//! depending on the range of value
	//! no children
	struct ImmediateNode : Node
	{
		int value; //!< value to push on stack
		
		ImmediateNode(const SourcePos& sourcePos, int value) : Node(sourcePos), value(value) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for loading a variable on stack
	//! no children
	struct LoadNode : Node
	{
		unsigned varAddr; //!< address of variable to load from
		
		LoadNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for storing a variable from stack
	//! no children
	struct StoreNode : Node
	{
		unsigned varAddr; //!< address of variable to store to
		
		StoreNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for reading from an array
	//! children[0] is the index in the array
	struct ArrayReadNode : Node
	{
		unsigned arrayAddr; //!< address of the first element of the array
		unsigned arraySize; //!< size of the array, might be used to assert compile-time access checks
		std::string arrayName; //!< name of the array (for debug)
		
		ArrayReadNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::string &arrayName);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for writing to an array. Value to store is supposed to be on the stack already
	//! children[0] is the index in the array
	struct ArrayWriteNode : Node
	{
		unsigned arrayAddr; //!< address of the first element of the array
		unsigned arraySize; //!< size of the array, might be used to assert compile-time access checks
		std::string arrayName; //!< name of the array (for debug)
		
		ArrayWriteNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::string &arrayName);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for calling a native function
	//! no children
	struct CallNode : Node
	{
		unsigned funcId; //!< identifier of the function to be called
		std::vector<unsigned> argumentsAddr; //!< address of all arguments
		
		CallNode(const SourcePos& sourcePos, unsigned funcId);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
}; // Aseba

#endif
