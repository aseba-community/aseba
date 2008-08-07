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

#ifndef __ASEBA_COMPILER_H
#define __ASEBA_COMPILER_H

#include <vector>
#include <deque>
#include <string>
#include <map>
#include <utility>
#include <istream>

namespace Aseba
{
	/**
		\defgroup compiler AESL Compiler
	*/
	/*@{*/

	// pre-declaration
	struct Node;
	struct ProgramNode;
	struct StatementNode;
	struct BinaryArithmeticNode;
	
	//! Description of target VM
	struct TargetDescription
	{
		//! Description of target VM named variable
		struct NamedVariable
		{
			NamedVariable(const std::string &name, unsigned size) : size(size), name(name) {}
			NamedVariable() {}
			
			unsigned size; //!< size of variable in words
			std::string name; //!< name of the variable
		};
		
		//! Description of local event;
		struct LocalEvent
		{
			std::string name; //!< name of the event
			std::string description; //!< description (some short documentation) of the event
		};
		
		//! Typed parameter of native functions
		struct NativeFunctionParameter
		{
			int size; //!< if > 0 size of the parameter, if < 0 template id, if 0 any size
			std::string name; //!< name of the parameter
		};
		
		//! Description of target VM native function
		struct NativeFunction
		{
			std::string name; //!< name of the function
			std::string description; //!< description (some short documentation) of the function
			std::vector<NativeFunctionParameter> parameters; //!< for each argument of the function, its size in words or, if negative, its template ID
		};
		
		std::string name; //!< node name
		unsigned protocolVersion; //!< version of the aseba protocol
		
		unsigned bytecodeSize; //!< total amount of bytecode space
		unsigned variablesSize; //!< total amount of variables space
		unsigned stackSize; //!< depth of execution stack
		
		std::vector<NamedVariable> namedVariables; //!< named variables
		std::vector<LocalEvent> localEvents; //!< events available locally on target
		std::vector<NativeFunction> nativeFunctions; //!< native functions
		
		TargetDescription() { variablesSize = bytecodeSize = stackSize = 0; }
	};
	
	//! A bytecode element 
	struct BytecodeElement
	{
		BytecodeElement();
		BytecodeElement(unsigned short bytecode) : bytecode(bytecode), line(0) { }
		BytecodeElement(unsigned short bytecode, unsigned short line) : bytecode(bytecode), line(line) { }
		operator unsigned short () const { return bytecode; }
		
		unsigned short bytecode; //! bytecode itself
		unsigned short line; //!< line in source code
	};
	
	//! Bytecode array in the form of a dequeue, for construction
	typedef std::deque<BytecodeElement> BytecodeVector;
	
	//! Bytecode use for compilation previous to linking. Basically a map of events id to events bytecodes
	struct PreLinkBytecode : public std::map<unsigned, BytecodeVector>
	{
		BytecodeVector *currentBytecode; //!< pointer to bytecode being constructed
		
		PreLinkBytecode();
		void fixup();
	};
	
	//! Position in a source file or string. First is line, second is column
	struct SourcePos
	{
		unsigned character; //!< position in source text
		unsigned row; //!< line in source text
		unsigned column; //!< column in source text
		bool valid; //!< true if character, row and column hold valid values
		
		SourcePos(unsigned character, unsigned row, unsigned column) : character(character - 1), row(row), column(column - 1) { valid = true; }
		SourcePos() { valid = false; }
		std::string toString() const;
	};
	
	//! Compilation error
	struct Error
	{
		SourcePos pos; //!< position of error in source string
		std::string message; //!< message
		//! Create an error at pos
		Error(const SourcePos& pos, const std::string& message) : pos(pos), message(message) { }
		//! Create an empty error
		Error() { message = "not defined"; }
		//! Return a string describing the error
		std::string toString() const;
	};
	
	//! Vector of names of variables
	typedef std::vector<std::string> VariablesNamesVector;
	
	//! A name - value pair
	struct NamedValue
	{
		//! Create a filled pair
		NamedValue(const std::string& name, int value) : name(name), value(value) {}
		
		std::string name; //!< name part of the pair
		int value; //!< value part of the pair
	};
	
	//! An event description is a name - value pair
	typedef NamedValue EventDescription;
	
	//! An constant definition is a name - value pair
	typedef NamedValue ConstantDefinition;
	
	//! Generic vector of name - value pairs
	struct NamedValuesVector: public std::vector<NamedValue>
	{
		bool contains(const std::string& s) const;
	};
	
	//! Vector of events descriptions
	typedef NamedValuesVector EventsDescriptionsVector;
	
	//! Vector of constants definitions
	typedef NamedValuesVector ConstantsDefinitions;
	
	//! Definitions common to several nodes, such as events or some constants
	struct CommonDefinitions
	{
		//! All defined events
		EventsDescriptionsVector events;
		//! All globally defined constants
		ConstantsDefinitions constants;
		
		//! Clear all the content
		void clear() { events.clear(); constants.clear(); }
	};
	
	//! Vector of data of variables
	typedef std::vector<short int> VariablesDataVector;
	
	//! Aseba Event Scripting Language compiler
	class Compiler
	{
	public:
		//! A token is a parsed element of inputs
		struct Token
		{
			enum Type
			{
				TOKEN_END_OF_STREAM = 0,
				TOKEN_STR_when,
				TOKEN_STR_emit,
				TOKEN_STR_for,
				TOKEN_STR_in,
				TOKEN_STR_step,
				TOKEN_STR_while,
				TOKEN_STR_do,
				TOKEN_STR_if,
				TOKEN_STR_then,
				TOKEN_STR_else,
				TOKEN_STR_elseif,
				TOKEN_STR_end,
				TOKEN_STR_var,
				TOKEN_STR_call,
				TOKEN_STR_onevent,
				TOKEN_STRING_LITERAL,
				TOKEN_INT_LITERAL,
				TOKEN_PAR_OPEN,
				TOKEN_PAR_CLOSE,
				TOKEN_BRACKET_OPEN,
				TOKEN_BRACKET_CLOSE,
				TOKEN_COLON,
				TOKEN_COMMA,
				TOKEN_ASSIGN,
				TOKEN_OP_OR,
				TOKEN_OP_AND,
				TOKEN_OP_NOT,
				TOKEN_OP_EQUAL,
				TOKEN_OP_NOT_EQUAL,
				TOKEN_OP_BIGGER,
				TOKEN_OP_BIGGER_EQUAL,
				TOKEN_OP_SMALLER,
				TOKEN_OP_SMALLER_EQUAL,
				TOKEN_OP_SHIFT_LEFT,
				TOKEN_OP_SHIFT_RIGHT,
				TOKEN_OP_ADD,
				TOKEN_OP_NEG,
				TOKEN_OP_MULT,
				TOKEN_OP_DIV,
				TOKEN_OP_MOD,
				TOKEN_OP_ABS
				
			} type; //!< type of this token
			std::string sValue; //!< string version of the value
			int iValue; //!< int version of the value, 0 if not applicable
			SourcePos pos;//!< position of token in source code
			
			Token() : type(TOKEN_END_OF_STREAM), iValue(0) {}
			Token(Type type, SourcePos pos = SourcePos(), const std::string& value = "");
			const char* typeName() const;
			std::string toString() const;
			operator Type () const { return type; }
		};
		
		//! Lookup table for variables name => (pos, size))
		typedef std::map<std::string, std::pair<unsigned, unsigned> > VariablesMap;
		//! Lookup table for functions (name => id in target description)
		typedef std::map<std::string, unsigned> FunctionsMap;
	
	public:
		Compiler();
		void setTargetDescription(const TargetDescription *description);
		const TargetDescription *getTargetDescription() const { return targetDescription;}
		const VariablesMap *getVariablesMap() const { return &variablesMap; }
		void setCommonDefinitions(const CommonDefinitions *definitions);
		bool compile(std::istream& source, BytecodeVector& bytecode, unsigned& allocatedVariablesCount, Error &errorDescription, std::ostream* dump = 0);
		
	protected:
		void internalCompilerError() const;
		void expect(const Token::Type& type) const;
		int expectInt16Literal() const;
		int expectConstant() const;
		int expectInt16LiteralOrConstant() const;
		unsigned expectUInt12Literal() const;
		unsigned expectGlobalEventId() const;
		unsigned expectAnyEventId() const;
		template <int length>
		bool isOneOf(const Token::Type types[length]) const;
		template <int length>
		void expectOneOf(const Token::Type types[length]) const;
		void buildMaps();
		void tokenize(std::istream& source);
		void dumpTokens(std::ostream &dest) const;
		bool link(const PreLinkBytecode& preLinkBytecode, BytecodeVector& bytecode) const;
		void disassemble(BytecodeVector& bytecode, std::ostream& dump) const;
		
	protected:
		Node* parseProgram();
		
		Node* parseStatement();
		
		Node* parseBlockStatement();
		
		Node* parseVarDef();
		Node* parseAssignment();
		Node* parseIfWhen(bool edgeSensitive);
		Node* parseFor();
		Node* parseWhile();
		Node* parseOnEvent();
		Node* parseEmit();
		
		BinaryArithmeticNode* parseOr();
		BinaryArithmeticNode* parseAnd();
		BinaryArithmeticNode* parseNot();
		
		BinaryArithmeticNode* parseCondition();
		
		Node* parseShiftExpression();
		Node* parseAddExpression();
		Node* parseMultExpression();
		Node* parseUnaryExpression();
		Node* parseFunctionCall();
		
		void parseReadVarArrayAccess(unsigned* addr, unsigned* size);
	
	protected:
		std::deque<Token> tokens; //!< parsed tokens
		VariablesMap variablesMap; //!< variables lookup
		FunctionsMap functionsMap; //!< functions lookup
		unsigned freeVariableIndex; //!< index pointing to the first free variable
		const TargetDescription *targetDescription; //!< description of the target VM
		const CommonDefinitions *commonDefinitions; //!< common definitions, such as events or some constants
	}; // Compiler
	
	/*@}*/
	
}; // Aseba

#endif

