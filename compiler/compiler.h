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

#ifndef __ASEBA_COMPILER_H
#define __ASEBA_COMPILER_H

#include <vector>
#include <deque>
#include <string>
#include <map>
#include <utility>
#include <istream>

// TODO :
// - code generation
// - linking and external bytecode representation
// - test and debug in VM

namespace Aseba
{
	// pre-declaration
	struct Node;
	struct ProgramNode;
	struct StatementNode;
	
	//! Description of target VM
	struct TargetDescription
	{
		//! Description of target VM named variable
		struct NamedVariable
		{
			NamedVariable(const std::string &name, unsigned size) : name(name), size(size) {}
			std::string name; //!< name of variable
			unsigned size; //!< size of variable in words
		};
		
		//! Description of target VM native function
		struct NativeFunction
		{
			std::string name; //!< name of function
			std::vector<unsigned> argumentsSize; //!< for each argument of the function, its size in words
		};
		
		unsigned bytecodeSize; //!< total amount of bytecode space
		unsigned variablesSize; //!< total amount of variables space
		unsigned stackSize; //!< depth of execution stack
		
		std::vector<NamedVariable> namedVariables; //!< named variables
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
	
	//! Vector of names of events
	typedef std::vector<std::string> EventsNamesVector;
	
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
				TOKEN_STR_while,
				TOKEN_STR_do,
				TOKEN_STR_if,
				TOKEN_STR_then,
				TOKEN_STR_else,
				TOKEN_STR_end,
				TOKEN_STR_var,
				TOKEN_STR_call,
				TOKEN_STR_onevent,
				TOKEN_STR_ontimer,
				TOKEN_STRING_LITERAL,
				TOKEN_INT_LITERAL,
				TOKEN_PAR_OPEN,
				TOKEN_PAR_CLOSE,
				TOKEN_BRACKET_OPEN,
				TOKEN_BRACKET_CLOSE,
				TOKEN_COLON,
				TOKEN_COMMA,
				TOKEN_ASSIGN,
				TOKEN_OP_EQUAL,
				TOKEN_OP_NOT_EQUAL,
				TOKEN_OP_BIGGER,
				TOKEN_OP_SMALLER,
				TOKEN_OP_SHIFT_LEFT,
				TOKEN_OP_SHIFT_RIGHT,
				TOKEN_OP_ADD,
				TOKEN_OP_NEG,
				TOKEN_OP_MULT,
				TOKEN_OP_DIV,
				TOKEN_OP_MOD,
				
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
	
	public:
		Compiler() { targetDescription = 0; eventsNames = 0; }
		void setTargetDescription(const TargetDescription *description) { targetDescription = description; }
		void setEventsNames(const EventsNamesVector *names) { eventsNames = names; }
		bool compile(std::istream& source, BytecodeVector& bytecode, VariablesNamesVector &variablesNames, unsigned& allocatedVariablesCount, Error &errorDescription, std::ostream &dump, bool verbose = false);
		
	protected:
		void internalCompilerError() const;
		void expect(const Token::Type& type) const;
		int expectInt16Literal() const;
		unsigned expectUInt12Literal() const;
		unsigned expectEventId() const;
		bool isOneOf(const Token::Type *types, size_t length) const;
		void expectOneOf(const Token::Type *types, size_t length) const;
		void buildMaps();
		void tokenize(std::istream& source);
		void dumpTokens(std::ostream &dest) const;
		void link(const PreLinkBytecode& preLinkBytecode, BytecodeVector& bytecode) const;
		void disassemble(BytecodeVector& bytecode, std::ostream& dump) const;
		
	protected:
		Node* parseProgram();
		Node* parseStatement();
		Node* parseBlockStatement();
		Node* parseVarDef();
		Node* parseAssignment();
		Node* parseIfWhen(bool edgeSensitive);
		Node* parseWhile();
		Node* parseOnEvent();
		Node* parseOnTimer();
		Node* parseEvent();
		Node* parseCondition();
		Node* parseShiftExpression();
		Node* parseAddExpression();
		Node* parseMultExpression();
		Node* parseUnaryExpression();
		Node* parseFunctionCall();
	
	protected:
		//! Lookup table for variables name => (pos, size))
		typedef std::map<std::string, std::pair<unsigned, unsigned> > VariablesMap;
		//! Lookup table for functions (name => id in target description)
		typedef std::map<std::string, unsigned> FunctionsMap;
	
	protected:
		std::deque<Token> tokens; //!< parsed tokens
		VariablesMap variablesMap; //!< variables lookup
		FunctionsMap functionsMap; //!< functions lookup
		unsigned freeVariableIndex; //!< index pointing to the first free variable
		const TargetDescription *targetDescription; //!< description of the target VM
		const EventsNamesVector *eventsNames; //!< names of events
	}; // Compiler
}; // Aseba

#endif

