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
	std::wstring binaryOperatorToString(AsebaBinaryOperator op);
	
	//! Return the string corresponding to the unary operator
	std::wstring unaryOperatorToString(AsebaUnaryOperator op);
	
	//! An abstract node of syntax tree
	struct Node
	{
		//! A type a node can return
		enum ReturnType
		{
			TYPE_UNIT = 0,
			TYPE_BOOL,
			TYPE_INT
		};
		
		//! Constructor
		Node(const SourcePos& sourcePos) : sourcePos(sourcePos) { }
		virtual Node* clone() const = 0;
		
		virtual ~Node();
		
		//! Typecheck this node, throw an exception if there is any type violation
		virtual ReturnType typeCheck() const;
		//! Optimize this node, return the optimized node
		virtual Node* optimize(std::wostream* dump) = 0;
		//! Return the stack depth requirement for this node and its children
		virtual unsigned getStackDepth() const;
		//! Generate bytecode
		virtual void emit(PreLinkBytecode& bytecodes) const = 0;
		
		//! Return a string representation of this node
		virtual std::wstring toWString() const = 0;
		//! Return a string representation of the name of this node
		virtual std::wstring toNodeName() const = 0;
		//! Dump this node and the rest of the tree
		virtual void dump(std::wostream& dest, unsigned& indent) const;
		
		//! Return the name of a type
		std::wstring typeName(const Node::ReturnType& type) const;
		//! Check for a specific type, throw an exception otherwise
		void expectType(const Node::ReturnType& expected, const Node::ReturnType& type) const;
		
		//! Vector for children of a node
		typedef std::vector<Node *> NodesVector;
		NodesVector children; //!< children of this node
		SourcePos sourcePos; //!< position is source
	};
	
	//! Node for L"block", i.e. a vector of statements
	struct BlockNode : Node
	{
		//! Constructor
		BlockNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual BlockNode* clone() const {return(new BlockNode(*this));}
		
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const { return L"Block"; }
		virtual std::wstring toNodeName() const { return L"block"; }
	};
	
	//! Node for L"program", i.e. a block node with some special behaviour later on
	struct ProgramNode: BlockNode
	{
		//! Constructor
		ProgramNode(const SourcePos& sourcePos) : BlockNode(sourcePos) { }
		virtual ProgramNode* clone() const {return(new ProgramNode(*this));}

		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const { return L"ProgramBlock"; }
		virtual std::wstring toNodeName() const { return L"program block"; }
	};
	
	//! Node for assignation.
	//! children[0] is store code
	//! children[1] expression to store
	struct AssignmentNode : Node
	{
		//! Constructor
		AssignmentNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual AssignmentNode* clone() const {return(new AssignmentNode(*this));}

		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const { return L"Assign"; }
		virtual std::wstring toNodeName() const { return L"assignment"; }
	};
	
	//! Node for L"if" and L"when".
	//! children[0] is expression
	//! children[1] is true block
	//! children[2] is false block
	struct IfWhenNode : Node
	{
		bool edgeSensitive; //!< if true, true block is triggered only if previous comparison was false ("when" block). If false, true block is triggered every time comparison is true
		unsigned endLine; //!< line of end keyword
		
		//! Constructor
		IfWhenNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual IfWhenNode* clone() const {return(new IfWhenNode(*this));}

		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"if/when"; }
	};
	
	//! Node for L"if" and L"when" with operator folded inside.
	//! children[0] is left part of expression
	//! children[1] is right part of expression
	//! children[2] is true block
	//! children[3] is false block
	struct FoldedIfWhenNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		bool edgeSensitive; //!< if true, true block is triggered only if previous comparison was false ("when" block). If false, true block is triggered every time comparison is true
		unsigned endLine; //!< line of end keyword
		
		//! Constructor
		FoldedIfWhenNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual FoldedIfWhenNode* clone() const {return(new FoldedIfWhenNode(*this));}

		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"folded if/when"; }
	};
	
	//! Node for L"while".
	//! children[0] is expression
	//! children[1] is block
	struct WhileNode : Node
	{
		//! Constructor
		WhileNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual WhileNode* clone() const {return(new WhileNode(*this));}

		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"while"; }
	};
	
	//! Node for L"while" with operator folded inside.
	//! children[0] is left part of expression
	//! children[1] is right part of expression
	//! children[2] is block
	struct FoldedWhileNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		
		//! Constructor
		FoldedWhileNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual FoldedWhileNode* clone() const {return(new FoldedWhileNode(*this));}

		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"folded while"; }
	};
	
	//! Node for L"onevent" 
	//! no children
	struct EventDeclNode : Node
	{
		unsigned eventId; //!< the event id associated with this context
		
		EventDeclNode(const SourcePos& sourcePos, unsigned eventId = 0);
		virtual EventDeclNode* clone() const {return(new EventDeclNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"event declaration"; }
	};
	
	//! Node for L"emit".
	//! no children
	struct EmitNode : Node
	{
		unsigned eventId; //!< id of event to emit
		unsigned arrayAddr; //!< address of the first element of the array to send
		unsigned arraySize; //!< size of the array to send. 0 if event has no argument
		
		//! Constructor
		EmitNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		virtual EmitNode* clone() const {return(new EmitNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"emit"; }
	};
	
	//! Node for L"sub"
	//! no children
	struct SubDeclNode : Node
	{
		unsigned subroutineId; //!< the associated subroutine
		
		SubDeclNode(const SourcePos& sourcePos, unsigned subroutineId);
		virtual SubDeclNode* clone() const {return(new SubDeclNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"subroutine declaration"; }
	};
	
	//! Node for L"callsub"
	//! no children
	struct CallSubNode : Node
	{
		unsigned subroutineId; //!< the subroutine to call
		
		CallSubNode(const SourcePos& sourcePos, unsigned subroutineId);
		virtual CallSubNode* clone() const {return(new CallSubNode(*this));}

		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"subroutine call"; }
	};
	
	//! Node for binary arithmetic.
	//! children[0] is left expression
	//! children[1] is right expression
	struct BinaryArithmeticNode : Node
	{
		AsebaBinaryOperator op; //!< operator
		
		BinaryArithmeticNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		BinaryArithmeticNode(const SourcePos& sourcePos, AsebaBinaryOperator op, Node *left, Node *right);
		virtual BinaryArithmeticNode* clone() const {return(new BinaryArithmeticNode(*this));}

		void deMorganNotRemoval();
		
		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"binary function"; }
		
		static BinaryArithmeticNode *fromComparison(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromShiftExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromAddExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromMultExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
		static BinaryArithmeticNode *fromBinaryExpression(const SourcePos& sourcePos, Compiler::Token::Type op, Node *left, Node *right);
	};
	
	//! Node for unary arithmetic
	//! children[0] is the expression to negate
	struct UnaryArithmeticNode : Node
	{
		AsebaUnaryOperator op; //!< operator
		
		//! Constructor
		UnaryArithmeticNode(const SourcePos& sourcePos) : Node(sourcePos) { }
		UnaryArithmeticNode(const SourcePos& sourcePos, AsebaUnaryOperator op, Node *child);
		virtual UnaryArithmeticNode* clone() const {return(new UnaryArithmeticNode(*this));}

		virtual ReturnType typeCheck() const;
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"unary function"; }
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
		virtual ImmediateNode* clone() const {return(new ImmediateNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_INT; }
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"constant"; }
	};
	
	//! Node for storing a variable from stack.
	//! no children
	struct StoreNode : Node
	{
		unsigned varAddr; //!< address of variable to store to
		
		//! Constructor
		StoreNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		virtual StoreNode* clone() const {return(new StoreNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"variable access (write)"; }
	};
	
	//! Node for loading a variable on stack.
	//! no children
	struct LoadNode : Node
	{
		unsigned varAddr; //!< address of variable to load from

		//! Constructor
		LoadNode(const SourcePos& sourcePos, unsigned varAddr) : Node(sourcePos), varAddr(varAddr) { }
		LoadNode(const StoreNode* store) : Node(store->sourcePos), varAddr(store->varAddr) {}
		virtual LoadNode* clone() const {return(new LoadNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_INT; }
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"variable access (read)"; }
	};

	//! Node for writing to an array. Value to store is supposed to be on the stack already
	//! children[0] is the index in the array
	struct ArrayWriteNode : Node
	{
		unsigned arrayAddr; //!< address of the first element of the array
		unsigned arraySize; //!< size of the array, might be used to assert compile-time access checks
		std::wstring arrayName; //!< name of the array (for debug)
		
		ArrayWriteNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName);
		virtual ArrayWriteNode* clone() const {return(new ArrayWriteNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"array access (write)"; }
	};
	
	//! Node for reading from an array.
	//! children[0] is the index in the array
	struct ArrayReadNode : Node
	{
		unsigned arrayAddr; //!< address of the first element of the array
		unsigned arraySize; //!< size of the array, might be used to assert compile-time access checks
		std::wstring arrayName; //!< name of the array (for debug)

		ArrayReadNode(const SourcePos& sourcePos, unsigned arrayAddr, unsigned arraySize, const std::wstring &arrayName);
		ArrayReadNode(const ArrayWriteNode* write) :
			Node(write->sourcePos), arrayAddr(write->arrayAddr), arraySize(write->arraySize), arrayName(write->arrayName)
		{
			children.push_back((write->children[0])->clone());
		}
		virtual ArrayReadNode* clone() const {return(new ArrayReadNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_INT; }
		virtual Node* optimize(std::wostream* dump);
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"array access (read)"; }
	};

	//! Node for calling a native function
	//! may have children for pushing constants somewhere
	struct CallNode : Node
	{
		unsigned funcId; //!< identifier of the function to be called
		std::vector<unsigned> argumentsAddr; //!< address of all arguments
		
		CallNode(const SourcePos& sourcePos, unsigned funcId);
		virtual CallNode* clone() const {return(new CallNode(*this));}

		virtual ReturnType typeCheck() const { return TYPE_UNIT; }
		virtual Node* optimize(std::wostream* dump);
		virtual unsigned getStackDepth() const;
		virtual void emit(PreLinkBytecode& bytecodes) const;
		virtual std::wstring toWString() const;
		virtual std::wstring toNodeName() const { return L"native function call"; }
	};
	
	/*@}*/
	
}; // Aseba

#endif
