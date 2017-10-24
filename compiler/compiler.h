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

#ifndef __ASEBA_COMPILER_H
#define __ASEBA_COMPILER_H

#include <utility>
#include <vector>
#include <deque>
#include <string>
#include <map>
#include <set>
#include <utility>
#include <istream>

#include "errors_code.h"
#include "../common/types.h"
#include "../common/msg/TargetDescription.h"
#include "../common/utils/FormatableString.h"

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
	struct AssignmentNode;
	struct TupleVectorNode;
	struct MemoryVectorNode;
	
	//! A bytecode element 
	struct BytecodeElement
	{
		BytecodeElement() = default;
		BytecodeElement(unsigned short bytecode) : bytecode(bytecode), line(0) { }
		BytecodeElement(unsigned short bytecode, unsigned short line) : bytecode(bytecode), line(line) { }
		operator unsigned short () const { return bytecode; }
		unsigned getWordSize() const;
		
		unsigned short bytecode{0xffff}; //! bytecode itself
		unsigned short line{0}; //!< line in source code
	};
	
	//! Bytecode array in the form of a dequeue, for construction
	struct BytecodeVector: std::deque<BytecodeElement>
	{
		//! Constructor
		BytecodeVector() = default;
		
		unsigned maxStackDepth{0}; //!< maximum depth of the stack used by all the computations of the bytecode
		unsigned callDepth{0}; //!< for callable bytecode (i.e. subroutines), used in analysis of stack check
		unsigned lastLine{0}; //!< last line added, normally equal *this[this->size()-1].line, but may differ for instance on loops
		
		void push_back(const BytecodeElement& be)
		{
			std::deque<BytecodeElement>::push_back(be);
			lastLine = be.line;
		}
		
		void changeStopToRetSub();
		unsigned short getTypeOfLast() const;
		
		//! A map of event addresses to identifiers
		typedef std::map<unsigned, unsigned> EventAddressesToIdsMap;
		EventAddressesToIdsMap getEventAddressesToIds() const;
	};
	
	// predeclaration
	struct PreLinkBytecode;
	
	//! Position in a source file or string. First is line, second is column
	struct SourcePos
	{
		unsigned character{0}; //!< position in source text
		unsigned row{0}; //!< line in source text
		unsigned column{0}; //!< column in source text
		bool valid{false}; //!< true if character, row and column hold valid values
		
		SourcePos(unsigned character, unsigned row, unsigned column) : character(character - 1), row(row), column(column - 1) { valid = true; }
		SourcePos() = default;
		std::wstring toWString() const;
	};
	
	//! Return error messages based on error ID (needed to translate messages for other applications)
	class ErrorMessages
	{
		public:
			ErrorMessages();

			//! Type of the callback
			using ErrorCallback = const std::wstring (*)(ErrorCode);
			//! Default callback
			static const std::wstring defaultCallback(ErrorCode error);
	};

	//! Compilation error
	struct Error
	{
		SourcePos pos; //!< position of error in source string
		std::wstring message; //!< message
		//! Create an error at pos
		Error(const SourcePos& pos, std::wstring  message) : pos(pos), message(std::move(message)) { }
		//! Create an empty error
		Error() { message = L"not defined"; }

		//! Return a string describing the error
		std::wstring toWString() const;
	};

	struct TranslatableError : public Error
	{
		TranslatableError() : Error() {}
		TranslatableError(const SourcePos& pos, ErrorCode error);
		TranslatableError &arg(int value, int fieldWidth = 0, int base = 10, wchar_t fillChar = ' ');
		TranslatableError &arg(long int value, int fieldWidth = 0, int base = 10, wchar_t fillChar = ' ');
		TranslatableError &arg(unsigned value, int fieldWidth = 0, int base = 10, wchar_t fillChar = ' ');
		TranslatableError &arg(float value, int fieldWidth = 0, int precision = 6, wchar_t fillChar = ' ');
		TranslatableError &arg(const std::wstring& value);

		Error toError();
		static void setTranslateCB(ErrorMessages::ErrorCallback newCB);

		static ErrorMessages::ErrorCallback translateCB;
		WFormatableString message;
	};

	//! Vector of names of variables
	using VariablesNamesVector = std::vector<std::wstring>;
	
	//! A name - value pair
	struct NamedValue
	{
		//! Create a filled pair
		NamedValue(std::wstring  name, int value) : name(std::move(name)), value(value) {}
		
		std::wstring name; //!< name part of the pair
		int value; //!< value part of the pair
	};
	
	//! An event description is a name - value pair
	using EventDescription = NamedValue;
	
	//! An constant definition is a name - value pair
	using ConstantDefinition = NamedValue;
	
	//! Generic vector of name - value pairs
	struct NamedValuesVector: public std::vector<NamedValue>
	{
		bool contains(const std::wstring& s, size_t* position = nullptr) const;
	};
	
	//! Vector of events descriptions
	using EventsDescriptionsVector = NamedValuesVector;
	
	//! Vector of constants definitions
	using ConstantsDefinitions = NamedValuesVector;
	
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
	using VariablesDataVector = std::vector<short>;
	
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
				TOKEN_STR_hidden_emit,
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
				TOKEN_STR_const,
				TOKEN_STR_call,
				TOKEN_STR_sub,
				TOKEN_STR_callsub,
				TOKEN_STR_onevent,
				TOKEN_STR_abs,
				TOKEN_STR_return,
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

				TOKEN_OP_EQUAL,			// group of tokens, don't split or mess up
				TOKEN_OP_NOT_EQUAL,		//
				TOKEN_OP_BIGGER,		//
				TOKEN_OP_BIGGER_EQUAL,		//
				TOKEN_OP_SMALLER,		//
				TOKEN_OP_SMALLER_EQUAL,		//

				TOKEN_OP_ADD,			// group of 2 tokens, don't split or mess up
				TOKEN_OP_NEG,			//
				TOKEN_OP_MULT,			// group of 3 tokens, don't split or mess up
				TOKEN_OP_DIV,			//
				TOKEN_OP_MOD,			//
				TOKEN_OP_SHIFT_LEFT,		// group of 2 tokens, don't split or mess up
				TOKEN_OP_SHIFT_RIGHT,		//
				TOKEN_OP_BIT_OR,		// group of 4 tokens, don't split or mess up
				TOKEN_OP_BIT_XOR,		//
				TOKEN_OP_BIT_AND,		//
				TOKEN_OP_BIT_NOT,		//

				TOKEN_OP_ADD_EQUAL,		// group of 12 tokens, don't split or mess up
				TOKEN_OP_NEG_EQUAL,		//
				TOKEN_OP_MULT_EQUAL,		//
				TOKEN_OP_DIV_EQUAL,		//
				TOKEN_OP_MOD_EQUAL,		//
				TOKEN_OP_SHIFT_LEFT_EQUAL,	//
				TOKEN_OP_SHIFT_RIGHT_EQUAL,	//
				TOKEN_OP_BIT_OR_EQUAL,		//
				TOKEN_OP_BIT_XOR_EQUAL,		//
				TOKEN_OP_BIT_AND_EQUAL,		//

				TOKEN_OP_PLUS_PLUS,
				TOKEN_OP_MINUS_MINUS

			} type{TOKEN_END_OF_STREAM}; //!< type of this token
			std::wstring sValue; //!< string version of the value
			int iValue{0}; //!< int version of the value, 0 if not applicable
			SourcePos pos;//!< position of token in source code
			
			Token()  = default;
			Token(Type type, SourcePos pos = SourcePos(), const std::wstring& value = L"");
			const std::wstring typeName() const;
			std::wstring toWString() const;
			operator Type () const { return type; }
		};
		
		//! Description of a subroutine
		struct SubroutineDescriptor
		{
			std::wstring name;
			unsigned address;
			unsigned line;
			
			SubroutineDescriptor(std::wstring  name, unsigned address, unsigned line) : name(std::move(name)), address(address), line(line) {}
		};
		//! Lookup table for subroutines id => (name, address, line)
		using SubroutineTable = std::vector<SubroutineDescriptor>;
		//! Reverse Lookup table for subroutines name => id
		typedef std::map<std::wstring, unsigned> SubroutineReverseTable;
		//! Lookup table to keep track of implemented events
		using ImplementedEvents = std::set<unsigned int>;
		//! Lookup table for constant name => value
		typedef std::map<std::wstring, int> ConstantsMap;
		//! Lookup table for event name => id
		typedef std::map<std::wstring, unsigned> EventsMap;

		friend struct AssignmentNode;
		friend struct CallSubNode;
	
	public:
		Compiler();
		void setTargetDescription(const TargetDescription *description);
		const TargetDescription *getTargetDescription() const { return targetDescription;}
		const VariablesMap *getVariablesMap() const { return &variablesMap; }
		const SubroutineTable *getSubroutineTable() const { return &subroutineTable; }
		void setCommonDefinitions(const CommonDefinitions *definitions);
		bool compile(std::wistream& source, BytecodeVector& bytecode, unsigned& allocatedVariablesCount, Error &errorDescription, std::wostream* dump = nullptr);
		void setTranslateCallback(ErrorMessages::ErrorCallback newCB) { TranslatableError::setTranslateCB(newCB); }
		static std::wstring translate(ErrorCode error) { return TranslatableError::translateCB(error); }
		static bool isKeyword(const std::wstring& word);
		
	protected:
		void internalCompilerError() const;
		void expect(const Token::Type& type) const;
		unsigned expectUInt12Literal() const;
		unsigned expectUInt16Literal() const;
		unsigned expectPositiveInt16Literal() const;
		int expectAbsoluteInt16Literal(bool negative) const;
		unsigned expectPositiveConstant() const;
		int expectConstant() const;
		unsigned expectPositiveInt16LiteralOrConstant() const;
		int expectInt16Literal();
		int expectInt16LiteralOrConstant();
		unsigned expectGlobalEventId() const;
		unsigned expectAnyEventId() const;
		std::wstring eventName(unsigned eventId) const;
		template <int length>
		bool isOneOf(const Token::Type types[length]) const;
		template <int length>
		void expectOneOf(const Token::Type types[length]) const;

		void freeTemporaryMemory();
		unsigned allocateTemporaryMemory(const SourcePos varPos, const unsigned size);
		AssignmentNode* allocateTemporaryVariable(const SourcePos varPos, Node* rValue);

		VariablesMap::const_iterator findVariable(const std::wstring& name, const SourcePos& pos) const;
		FunctionsMap::const_iterator findFunction(const std::wstring& name, const SourcePos& pos) const;
		ConstantsMap::const_iterator findConstant(const std::wstring& name, const SourcePos& pos) const;
		EventsMap::const_iterator findGlobalEvent(const std::wstring& name, const SourcePos& pos) const;
		EventsMap::const_iterator findAnyEvent(const std::wstring& name, const SourcePos& pos) const;
		SubroutineReverseTable::const_iterator findSubroutine(const std::wstring& name, const SourcePos& pos) const;
		bool constantExists(const std::wstring& name) const;
		void buildMaps();
		void tokenize(std::wistream& source);
		wchar_t getNextCharacter(std::wistream& source, SourcePos& pos);
		bool testNextCharacter(std::wistream& source, SourcePos& pos, wchar_t test, Token::Type tokenIfTrue);
		void dumpTokens(std::wostream &dest) const;
		bool verifyStackCalls(PreLinkBytecode& preLinkBytecode);
		bool link(const PreLinkBytecode& preLinkBytecode, BytecodeVector& bytecode);
		void disassemble(BytecodeVector& bytecode, const PreLinkBytecode& preLinkBytecode, std::wostream& dump) const;
		
	protected:
		Node* parseProgram();
		
		Node* parseStatement();
		
		Node* parseBlockStatement();
		
		Node* parseReturn();
		void parseConstDef();
		Node* parseVarDef();
		AssignmentNode* parseVarDefInit(MemoryVectorNode* lValue);
		Node* parseAssignment();
		Node* parseIfWhen(bool edgeSensitive);
		Node* parseFor();
		Node* parseWhile();
		Node* parseOnEvent();
		Node* parseEmit(bool shorterArgsAllowed = false);
		Node* parseSubDecl();
		Node* parseCallSub();
		
		Node* parseOr();
		Node* parseAnd();
		Node* parseNot();
		
		Node* parseCondition();
		
		Node *parseBinaryOrExpression();
		Node *parseBinaryXorExpression();
		Node *parseBinaryAndExpression();
		
		Node* parseShiftExpression();
		Node* parseAddExpression();
		Node* parseMultExpression();
		Node* parseUnaryExpression();
		Node* parseFunctionCall();

		TupleVectorNode* parseTupleVector(bool compatibility = false);
		Node* parseConstantAndVariable();
		MemoryVectorNode* parseVariable();
		unsigned parseVariableDefSize();
		Node* tryParsingConstantExpression(SourcePos pos, int& constantResult);
		int expectConstantExpression(SourcePos pos, Node* tree);
	
	protected:
		std::deque<Token> tokens; //!< parsed tokens
		VariablesMap variablesMap; //!< variables lookup
		ImplementedEvents implementedEvents; //!< list of implemented events
		FunctionsMap functionsMap; //!< functions lookup
		ConstantsMap constantsMap; //!< constants map
		EventsMap globalEventsMap; //!< global-events map
		EventsMap allEventsMap; //!< all-events map
		SubroutineTable subroutineTable; //!< subroutine lookup
		SubroutineReverseTable subroutineReverseTable; //!< subroutine reverse lookup
		unsigned freeVariableIndex; //!< index pointing to the first free variable
		unsigned endVariableIndex; //!< (endMemory - endVariableIndex) is pointing to the first free variable at the end
		const TargetDescription *targetDescription; //!< description of the target VM
		const CommonDefinitions *commonDefinitions; //!< common definitions, such as events or some constants

		ErrorMessages translator;
	}; // Compiler
	
	//! Bytecode use for compilation previous to linking
	struct PreLinkBytecode
	{
		//! Map of events id to event bytecode
		typedef std::map<unsigned, BytecodeVector> EventsBytecode;
		EventsBytecode events; //!< bytecode for events
		
		//! Map of routines id to routine bytecode
		typedef std::map<unsigned, BytecodeVector> SubroutinesBytecode;
		SubroutinesBytecode subroutines; //!< bytecode for routines
		
		BytecodeVector *current; //!< pointer to bytecode being constructed
		
		PreLinkBytecode();
		
		void fixup(const Compiler::SubroutineTable &subroutineTable);
	};
	
	/*@}*/
	
} // namespace Aseba

#endif

