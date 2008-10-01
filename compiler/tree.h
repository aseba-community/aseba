/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
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

#ifndef __TREE_H
#define __TREE_H

#include "compiler.h"
#include "../common/consts.h"
#include <vector>
#include <string>
#include <ostream>



namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/

	//! Return the string corresponding to the binary operator
	std::string binaryOperatorToString(AsebaBinaryOperator op);
	
	//! Return the string corresponding to the unary operator
	std::string unaryOperatorToString(AsebaUnaryOperator op);
	
	//! An abstract node of syntax tree
	struct Node
	{
		//! Constructor
		Node(const SourcePos& sourcePos) : sourcePos(sourcePos) { }
		
		virtual ~Node();
		
		//! Optimize this node, reture true if any optimization was successful
		virtual Node* optimize(std::ostream* dump) = 0;
		//! Return the stack depth requirement for this node and its children
		virtual unsigned getStackDepth() const;
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
		//! Constructor
		BlockNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const { return "Block"; }
	};
	
	//! Node for "program", i.e. a block node with some special behaviour later on
	struct ProgramNode: BlockNode
	{
		//! Constructor
		ProgramNode(const SourcePos& sourcePos) : BlockNode(sourcePos) { }
		
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const { return "ProgramBlock"; }
	};
	
	//! Node for assignation.
	//! children[0] is store code
	//! children[1] expression to store
	struct AssignmentNode : Node
	{
		//! Constructor
		AssignmentNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const { return "Assign"; }
	};
	
	//! Node for "if" and "when".
	//! children[0] is expression
	//! children[1] is true block
	//! children[2] is false block
	struct IfWhenNode : Node
	{
		bool edgeSensitive; //!< if true, true block is triggered only if previous comparison was false ("when" block). If false, true block is triggered every time comparison is true
		
		//! Constructor
		IfWhenNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "if" and "when" with operator folded inside.
	//! children[0] is left part of expression
	//! children[1] is right part of expression
	//! children[2] is true block
	//! children[3] is false block
	struct FoldedIfWhenNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		bool edgeSensitive; //!< if true, true block is triggered only if previous comparison was false ("when" block). If false, true block is triggered every time comparison is true
		
		//! Constructor
		FoldedIfWhenNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "while".
	//! children[0] is expression
	//! children[1] is block
	struct WhileNode : Node
	{
		//! Constructor
		WhileNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "while" with operator folded inside.
	//! children[0] is left part of expression
	//! children[1] is right part of expression
	//! children[2] is block
	struct FoldedWhileNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		
		//! Constructor
		FoldedWhileNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "onevent" 
	//! no children
	struct EventDeclNode : Node
	{
		unsigned eventId; //!< the event id associated with this context
		
		EventDeclNode(const SourcePos& sourcePos, unsigned eventId = 0);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "emit".
	//! no children
	struct EmitNode : Node
	{
		unsigned eventId; //!< id of event to emit
		unsigned arrayAddr; //!< address of the first element of the array to send
		unsigned arraySize; //!< size of the array to send. 0 if event has no argument
		
		//! Constructor
		EmitNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "sub"
	//! no children
	struct SubDeclNode : Node
	{
		unsigned subroutineId; //!< the associated subroutine
		
		SubDeclNode(const SourcePos& sourcePos, unsigned subroutineId);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for "callsub"
	//! no children
	struct CallSubNode : Node
	{
		unsigned subroutineId; //!< the subroutine to call
		
		CallSubNode(const SourcePos& sourcePos, unsigned subroutineId);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for binary arithmetic.
	//! children[0] is left expression
	//! children[1] is right expression
	struct BinaryArithmeticNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		
		BinaryArithmeticNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		BinaryArithmeticNode(const SourcePos& sourcePos, AsebaBinaryOperator op, Node *left, Node *right);
		
		void deMorganNotRemoval();
		
		virtual Node* optimize(std::ostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
		
		static BinaryArithmeticNode *fromComparison(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromShiftExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromAddExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromMultExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
	};
	
	//! Node for unary arithmetic
	//! children[0] is the expression to negate
	struct UnaryArithmeticNode : Node
	{
		AsebaUnaryOperator op; //!< operator
		
		//! Constructor
		UnaryArithmeticNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		UnaryArithmeticNode(const SourcePos& sourcePos, AsebaUnaryOperator op, Node *child);
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for pushing immediate value on stack.
	//! Might generate either Small Immediate or Large Immediate bytecode
	//! depending on the range of value
	//! no children
	struct ImmediateNode : Node
	{
		int value; //!< value to push on stack
		
		//! Constructor
		ImmediateNode(const SourcePos& sourcePos, int value) : Node(sourcePos), value(value) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for loading a variable on stack.
	//! no children
	struct LoadNode : Node
	{
		unsigned varAddr; //!< address of variable to load from
		
		//! Constructor
		LoadNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for storing a variable from stack.
	//! no children
	struct StoreNode : Node
	{
		unsigned varAddr; //!< address of variable to store to
		
		//! Constructor
		StoreNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		
		virtual Node* optimize(std::ostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	//! Node for reading from an array.
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
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::string toString() const;
	};
	
	/*@}*/
	
}; // Aseba

#endif
