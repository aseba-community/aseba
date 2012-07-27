/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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

#include "compiler.h"
#include "tree.h"
#include "../utils/FormatableString.h"
#include <memory>
#include <valarray>
#include <iostream>
#include <cassert>
#include <typeinfo>

#define IS_ONE_OF(array) (isOneOf<sizeof(array)/sizeof(Token::Type)>(array))
#define EXPECT_ONE_OF(array) (expectOneOf<sizeof(array)/sizeof(Token::Type)>(array))

#define STRICT		false

namespace Aseba
{
	//! There is a bug in the compiler, ask for a bug report
	void Compiler::internalCompilerError() const
	{
		throw Error(tokens.front().pos, L"Internal compiler error, please report a bug containing the source which triggered this error");
	}
	
	//! Check if next token is of type, produce an exception otherwise
	void Compiler::expect(const Token::Type& type) const
	{
		if (tokens.front() != type)
			throw Error(tokens.front().pos,
				WFormatableString(L"Expecting %0, found %1 instead")
					.arg(Token(type).typeName())
					.arg(tokens.front().typeName())
				);
	}
	
	//! Check if next token is an unsigned 12 bits integer literal. If so, return it, if not, throw an exception
	unsigned Compiler::expectUInt12Literal() const
	{
		expect(Token::TOKEN_INT_LITERAL);
		if (tokens.front().iValue < 0 || tokens.front().iValue > 4095)
			throw Error(tokens.front().pos,
				WFormatableString(L"Integer value %0 out of [0;4095] range")
					.arg(tokens.front().iValue)
				);
		return tokens.front().iValue;
	}
	
	//! Check if next token is an unsigned 16 bits integer literal. If so, return it, if not, throw an exception
	unsigned Compiler::expectUInt16Literal() const
	{
		expect(Token::TOKEN_INT_LITERAL);
		if (tokens.front().iValue < 0 || tokens.front().iValue > 65535)
			throw Error(tokens.front().pos,
				WFormatableString(L"Integer value %0 out of [0;65535] range")
					.arg(tokens.front().iValue)
				);
		return tokens.front().iValue;
	}
	
	//! Check if next token is the positive part of a 16 bits signed integer literal. If so, return it, if not, throw an exception
	unsigned Compiler::expectPositiveInt16Literal() const
	{
		expect(Token::TOKEN_INT_LITERAL);
		if (tokens.front().iValue < 0 || tokens.front().iValue > 32767)
			throw Error(tokens.front().pos,
				WFormatableString(L"Integer value %0 out of [0;32767] range")
					.arg(tokens.front().iValue)
				);
		return tokens.front().iValue;
	}
	
	//! Check if next token is the absolute part of a 16 bits signed integer literal. If so, return it, if not, throw an exception
	int Compiler::expectAbsoluteInt16Literal(bool negative) const
	{
		expect(Token::TOKEN_INT_LITERAL);
		int limit(32767);
		if (negative)
			limit++;
		if (tokens.front().iValue < 0 || tokens.front().iValue > limit)
			throw Error(tokens.front().pos,
				WFormatableString(L"Integer value %0 out of [-32768;32767] range")
					.arg(tokens.front().iValue)
				);
		return tokens.front().iValue;
	}
	
	//! Check if next toxen is a valid positive part of a 16 bits signed integer constant
	unsigned Compiler::expectPositiveConstant() const
	{
		expect(Token::TOKEN_STRING_LITERAL);
		const std::wstring name = tokens.front().sValue;
		const SourcePos pos = tokens.front().pos;
		const ConstantsMap::const_iterator constIt(findConstant(name, pos));
		
		const int value = constIt->second;
		if (value < 0 || value > 32767)
			throw Error(tokens.front().pos,
				WFormatableString(L"Constant %0 has value %1, which is out of [0;32767] range")
					.arg(tokens.front().sValue)
					.arg(value)
				);
		return value;
	}
	
	//! Check if next toxen is a valid 16 bits signed integer constant
	int Compiler::expectConstant() const
	{
		expect(Token::TOKEN_STRING_LITERAL);
		const std::wstring name = tokens.front().sValue;
		const SourcePos pos = tokens.front().pos;
		const ConstantsMap::const_iterator constIt(findConstant(name, pos));
		
		const int value = constIt->second;
		if (value < -32768 || value > 32767)
			throw Error(tokens.front().pos,
				WFormatableString(L"Constant %0 has value %1, which is out of [-32768;32767] range")
					.arg(tokens.front().sValue)
					.arg(value)
				);
		return value;
	}
	
	//! Check and return either the positive part of a 16 bits signed integer or the value of a valid constant
	unsigned Compiler::expectPositiveInt16LiteralOrConstant() const
	{
		if (tokens.front() == Token::TOKEN_INT_LITERAL)
			return expectPositiveInt16Literal();
		else
			return expectPositiveConstant();
	}
	
	//! Check and return either a 16 bits signed integer or the value of a valid constant
	int Compiler::expectInt16LiteralOrConstant()
	{
		if (tokens.front() == Token::TOKEN_OP_NEG)
		{
			tokens.pop_front();
			return -expectAbsoluteInt16Literal(true);
		}
		else if (tokens.front() == Token::TOKEN_INT_LITERAL)
		{
			return expectAbsoluteInt16Literal(false);
		}
		else
			return expectConstant();
	}
	
	//! Check if next token is a known global event identifier
	unsigned Compiler::expectGlobalEventId() const
	{
		assert(commonDefinitions);
		
		expect(Token::TOKEN_STRING_LITERAL);
		
		const std::wstring & name = tokens.front().sValue;
		const SourcePos pos = tokens.front().pos;
		const EventsMap::const_iterator eventIt(findGlobalEvent(name, pos));
		
		return eventIt->second;
	}
	
	//! Check if next token is a known local or global event identifier
	unsigned Compiler::expectAnyEventId() const
	{
		assert(targetDescription);
		
		expect(Token::TOKEN_STRING_LITERAL);
		
		const std::wstring & name = tokens.front().sValue;
		const SourcePos pos = tokens.front().pos;
		const EventsMap::const_iterator eventIt(findAnyEvent(name, pos));
		
		return eventIt->second;
	}
	
	//! Return the name of an event given its identifier
	std::wstring Compiler::eventName(unsigned eventId) const
	{
		// search for global
		if (eventId < commonDefinitions->events.size())
			return commonDefinitions->events[eventId].name;
		
		// then for local
		int localId = ASEBA_EVENT_LOCAL_EVENTS_START - eventId;
		if ((localId >= 0) && (localId < (int)targetDescription->localEvents.size()))
			return targetDescription->localEvents[localId].name;
		
		return L"unknown";
	}
	
	//! Return true if next token is of the following types
	template <int length>
	bool Compiler::isOneOf(const Token::Type types[length]) const
	{
		for (size_t i = 0; i < length; i++)
		{
			if (tokens.front() == types[i])
				return true;
		}
		return false;
	}
	
	//! Check if next token is of one of the following types, produce an exception otherwise
	template <int length>
	void Compiler::expectOneOf(const Token::Type types[length]) const
	{
		if (!isOneOf<length>(types))
		{
			std::wstring errorMessage(L"Expecting one of ");
			for (size_t i = 0; i < length; i++)
			{
				errorMessage += Token(types[i]).typeName();
				if (i + 1 < length)
					errorMessage += L", ";
			}
			errorMessage += L"; but found ";
			errorMessage += tokens.front().typeName();
			errorMessage += L" instead";
			throw Error(tokens.front().pos, errorMessage);
		}
	}

	void Compiler::freeTemporaryMemory()
	{
		endVariableIndex = 0;
	}

	void Compiler::allocateTemporaryMemory(const SourcePos varPos, const unsigned size, unsigned& varAddr)
	{
		// allocate space at the end of the variables' memory
		unsigned endOfMemory = targetDescription->variablesSize - endVariableIndex;
		varAddr = endOfMemory - size;
		endVariableIndex += size;

		// free space check
		if (freeVariableIndex + endVariableIndex > targetDescription->variablesSize)
			throw Error(varPos, WFormatableString(L"Not enough free space to allocate this tempory variable"));
	}

	AssignmentNode* Compiler::allocateTemporaryVariable(const SourcePos varPos, Node* rValue)
	{
		static unsigned uid = 0;

		// allocate the temporary variable
		const unsigned size = rValue->getVectorSize();
		unsigned addr = Node::E_NOVAL;
		allocateTemporaryMemory(varPos, size, addr);

		// create assignment
		std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(varPos));
		assignment->children.push_back(new MemoryVectorNode(varPos, addr, size, WFormatableString(L"temp%0").arg(uid++)));
		assignment->children.push_back(rValue);
		return assignment.release();
	}
	
	//! Parse "program" grammar element.
	Node* Compiler::parseProgram()
	{
		std::auto_ptr<ProgramNode> block(new ProgramNode(tokens.front().pos));
		// parse all vars declarations
		while (tokens.front() == Token::TOKEN_STR_var)
		{
			// we may receive NULL pointers because non initialized variables produce no code
			Node *child = parseVarDef();
			if (child)
				block->children.push_back(child);
		}
		// parse the rest of the code
		while (tokens.front() != Token::TOKEN_END_OF_STREAM)
		{
			// only var declaration are allowed to return NULL node, so we assert on node
			Node *child = parseStatement();
			assert(child);
			block->children.push_back(child);
		}
		return block.release();
	}
	
	//! Parse "statement" grammar element.
	Node* Compiler::parseStatement()
	{
		// reset temporary variables
		freeTemporaryMemory();

		switch (tokens.front())
		{
			case Token::TOKEN_STR_var: throw Error(tokens.front().pos, L"Variable definition is allowed only at the beginning of the program before any statement");
			case Token::TOKEN_STR_onevent: return parseOnEvent();
			case Token::TOKEN_STR_sub: return parseSubDecl();
			default: return parseBlockStatement();
		}
	}
	
	//! Parse "block statement" grammar element.
	Node* Compiler::parseBlockStatement()
	{
		switch (tokens.front())
		{
			case Token::TOKEN_STR_if: return parseIfWhen(false);
			case Token::TOKEN_STR_when: return parseIfWhen(true);
			case Token::TOKEN_STR_for: return parseFor();
			case Token::TOKEN_STR_while: return parseWhile();
			case Token::TOKEN_STR_emit: return parseEmit();
			case Token::TOKEN_STR_call: return parseFunctionCall();
			case Token::TOKEN_STR_callsub: return parseCallSub();
			case Token::TOKEN_STR_return: return parseReturn();
			default: return parseAssignment();
		}
	}
	
	//! Parse "return statement" grammar element.
	Node* Compiler::parseReturn()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		return new ReturnNode(pos);
	}
	
	//! Parse "var def" grammar element.
	Node* Compiler::parseVarDef()
	{
		tokens.pop_front();
		
		// check for string literal
		if (tokens.front() != Token::TOKEN_STRING_LITERAL)
			throw Error(tokens.front().pos,
				WFormatableString(L"Expecting identifier, found %0").arg(tokens.front().toWString()));
		
		// save variable
		std::wstring varName = tokens.front().sValue;
		SourcePos varPos = tokens.front().pos;
		unsigned varSize = Node::E_NOVAL;
		unsigned varAddr = freeVariableIndex;
		
		tokens.pop_front();
		
		// optional array
		varSize = parseVariableDefSize();
		
		// check if variable exists
		if (variablesMap.find(varName) != variablesMap.end())
			throw Error(varPos, WFormatableString(L"Variable %0 is already defined").arg(varName));

		// check if variable conflicts with a constant
		if(commonDefinitions->constants.contains(varName))
			throw Error(varPos, WFormatableString(L"Variable %0 has the same name as a constant").arg(varName));
		
		// optional assignation
		std::auto_ptr<MemoryVectorNode> me(new MemoryVectorNode(varPos, varAddr, varSize, varName));
		std::auto_ptr<Node> temp;
		temp.reset(parseVarDefInit(me.get()));
		if (temp.get())
		{
			// valid
			varSize = me->getVectorSize();
			me.release();
		}

		// sanity check for array
		if (varSize == 0)
			throw Error(varPos, WFormatableString(L"Array %0 has undefined size").arg(varName));

		// save variable
		variablesMap[varName] = std::make_pair(varAddr, varSize);
		freeVariableIndex += varSize;

		// check space
		if (freeVariableIndex > targetDescription->variablesSize)
			throw Error(varPos, L"No more free variable space");

		if (temp.get())
			return temp.release();
		else
			return NULL;
	}

	AssignmentNode *Compiler::parseVarDefInit(MemoryVectorNode* lValue)
	{
		if (tokens.front() == Token::TOKEN_ASSIGN)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();

			// try old style initialization 1,2,3
			std::auto_ptr<Node> rValue(parseImmediateVector(true));
			if (rValue.get() == NULL)
			{
				// no -> other type of initialization
				rValue.reset(parseBinaryOrExpression());
			}

			if (lValue->getVectorSize() == Node::E_NOVAL)
			{
				// infere the variable's size based on the initilization
				lValue->arraySize = rValue->getVectorSize();
			}

			std::auto_ptr<AssignmentNode> assign(new AssignmentNode(pos));
			assign->children.push_back(lValue);
			assign->children.push_back(rValue.release());
			return assign.release();
		}

		return NULL;
	}
	

	//! Parse "assignment" grammar element
	Node* Compiler::parseAssignment()
	{
		expect(Token::TOKEN_STRING_LITERAL);
		
		static const Token::Type compoundBinaryAssignment[] = { Token::TOKEN_OP_ADD_EQUAL, Token::TOKEN_OP_NEG_EQUAL,
							   Token::TOKEN_OP_MULT_EQUAL, Token::TOKEN_OP_DIV_EQUAL, Token::TOKEN_OP_MOD_EQUAL,
							   Token::TOKEN_OP_BIT_AND_EQUAL, Token::TOKEN_OP_BIT_OR_EQUAL, Token::TOKEN_OP_BIT_XOR_EQUAL,
							   Token::TOKEN_OP_SHIFT_LEFT_EQUAL, Token::TOKEN_OP_SHIFT_RIGHT_EQUAL};

		// parse left value
		std::auto_ptr<Node> lValue(parseBinaryOrExpression());
		
		// parse right value
		if (tokens.front() == Token::TOKEN_ASSIGN)
		{
			std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(tokens.front().pos));
			tokens.pop_front();
			
			assignment->children.push_back(lValue.release());
			assignment->children.push_back(parseBinaryOrExpression());
			return assignment.release();
		}
		else if ((tokens.front() == Token::TOKEN_OP_PLUS_PLUS) || (tokens.front() == Token::TOKEN_OP_MINUS_MINUS))
		{
			SourcePos pos = tokens.front().pos;
			Compiler::Token op = tokens.front();
			tokens.pop_front();

			std::auto_ptr<UnaryArithmeticAssignmentNode> assignment(
						new UnaryArithmeticAssignmentNode(
							pos,
							op,
							lValue.release()));

			return assignment.release();
		}
		else if (IS_ONE_OF(compoundBinaryAssignment))
		{
			SourcePos pos = tokens.front().pos;
			Compiler::Token op = tokens.front();
			tokens.pop_front();

			std::auto_ptr<ArithmeticAssignmentNode> assignment(
						ArithmeticAssignmentNode::fromArithmeticAssignmentToken(
							pos,
							op,
							lValue.release(),
							parseBinaryOrExpression()));

			return assignment.release();
		}
		else
			throw Error(tokens.front().pos, WFormatableString(L"Expecting assignement, found %0 instead").arg(tokens.front().toWString()));
		
		return NULL;
	}

	//! Parse "if" grammar element.
	Node* Compiler::parseIfWhen(bool edgeSensitive)
	{
		const Token::Type elseEndTypes[] = { Token::TOKEN_STR_else, Token::TOKEN_STR_elseif, Token::TOKEN_STR_end };
		
		std::auto_ptr<IfWhenNode> ifNode(new IfWhenNode(tokens.front().pos));
		
		// eat "if" / "when"
		ifNode->edgeSensitive = edgeSensitive;
		tokens.pop_front();
		
		// condition
		ifNode->children.push_back(parseOr());
		
		// then keyword
		if (edgeSensitive)
			expect(Token::TOKEN_STR_do);
		else
			expect(Token::TOKEN_STR_then);
		tokens.pop_front();
		
		// parse true condition
		ifNode->children.push_back(new BlockNode(tokens.front().pos));
		while (!IS_ONE_OF(elseEndTypes))
			ifNode->children[1]->children.push_back(parseBlockStatement());
		
		/*// parse false condition (only for if)
		if (!edgeSensitive && (tokens.front() == Token::TOKEN_STR_else))
		{
			tokens.pop_front();
			
			ifNode->children.push_back(new BlockNode(tokens.front().pos));
			while (tokens.front() != Token::TOKEN_STR_end)
				ifNode->children[2]->children.push_back(parseBlockStatement());
		}*/
		
		// parse false condition (only for if)
		if (!edgeSensitive)
		{
			if (tokens.front() == Token::TOKEN_STR_else)
			{
				tokens.pop_front();
				
				ifNode->children.push_back(new BlockNode(tokens.front().pos));
				while (tokens.front() != Token::TOKEN_STR_end)
					ifNode->children[2]->children.push_back(parseBlockStatement());
			}
			// if elseif, queue new if directly after and return before parsing trailing end
			if (tokens.front() == Token::TOKEN_STR_elseif)
			{
				ifNode->children.push_back(parseIfWhen(false));
				return ifNode.release();
			}
		}
		
		// end keyword
		ifNode->endLine = tokens.front().pos.row;
		expect(Token::TOKEN_STR_end);
		tokens.pop_front();
		
		return ifNode.release();
	}
	
	//! Parse "for" grammar element.
	Node* Compiler::parseFor()
	{
		// eat "for"
		SourcePos whilePos = tokens.front().pos;
		tokens.pop_front();
		
		// variable
		std::auto_ptr<MemoryVectorNode> variable(parseVariable());
		MemoryVectorNode* variableRef = variable.get();		// used to create copies
		SourcePos varPos = variable->sourcePos;
		
		// in keyword
		expect(Token::TOKEN_STR_in);
		tokens.pop_front();
		
		// range start index
		int rangeStartIndex = expectInt16LiteralOrConstant();
		SourcePos rangeStartIndexPos = tokens.front().pos;
		tokens.pop_front();
		
		// : keyword
		expect(Token::TOKEN_COLON);
		tokens.pop_front();
		
		// range end index
		int rangeEndIndex = expectInt16LiteralOrConstant();
		SourcePos rangeEndIndexPos = tokens.front().pos;
		tokens.pop_front();
		
		// check for step
		int step = 1;
		if (tokens.front() == Token::TOKEN_STR_step)
		{
			tokens.pop_front();
			step = expectInt16LiteralOrConstant();
			if (step == 0)
				throw Error(tokens.front().pos, L"Null steps are not allowed in for loops");
			tokens.pop_front();
		}
		
		// check conditions
		if (step > 0)
		{
			if (rangeStartIndex > rangeEndIndex)
				throw Error(rangeStartIndexPos, L"Start index must be lower than end index in increasing loops");
		}
		else
		{
			if (rangeStartIndex < rangeEndIndex)
				throw Error(rangeStartIndexPos, L"Start index must be higher than end index in decreasing loops");
		}
		
		// do keyword
		expect(Token::TOKEN_STR_do);
		tokens.pop_front();
		
		// create enclosing block and initial variable state
		std::auto_ptr<BlockNode> blockNode(new BlockNode(whilePos));
		std::auto_ptr<AssignmentNode> assignment(new AssignmentNode(rangeStartIndexPos));
		assignment->children.push_back(variable.release());
		assignment->children.push_back(new ImmediateVectorNode(rangeStartIndexPos, rangeStartIndex));
		blockNode->children.push_back(assignment.release());
		
		// create while and condition
		WhileNode* whileNode = new WhileNode(whilePos);
		blockNode->children.push_back(whileNode);
		BinaryArithmeticNode* comparisonNode = new BinaryArithmeticNode(whilePos);
		comparisonNode->children.push_back(variableRef->deepCopy());
		if (rangeStartIndex <= rangeEndIndex)
			comparisonNode->op = ASEBA_OP_SMALLER_EQUAL_THAN;
		else
			comparisonNode->op = ASEBA_OP_BIGGER_EQUAL_THAN;
		comparisonNode->children.push_back(new ImmediateVectorNode(rangeEndIndexPos, rangeEndIndex));
		whileNode->children.push_back(comparisonNode);
		
		// block and end keyword
		whileNode->children.push_back(new BlockNode(tokens.front().pos));
		while (tokens.front() != Token::TOKEN_STR_end)
			whileNode->children[1]->children.push_back(parseBlockStatement());
		
		// increment variable
		AssignmentNode* assignmentNode = new AssignmentNode(varPos);
		whileNode->children[1]->children.push_back(assignmentNode);
		assignmentNode->children.push_back(variableRef->deepCopy());
		assignmentNode->children.push_back(new BinaryArithmeticNode(varPos, ASEBA_OP_ADD, variableRef->deepCopy(), new ImmediateVectorNode(varPos, step)));
		
		tokens.pop_front();
		
		return blockNode.release();
	}
	
	//! Parse "while" grammar element.
	Node* Compiler::parseWhile()
	{
		std::auto_ptr<WhileNode> whileNode(new WhileNode(tokens.front().pos));
		
		// eat "while"
		tokens.pop_front();
		
		// condition
		whileNode->children.push_back(parseOr());
		
		// do keyword
		expect(Token::TOKEN_STR_do);
		tokens.pop_front();
		
		// block and end keyword
		whileNode->children.push_back(new BlockNode(tokens.front().pos));
		while (tokens.front() != Token::TOKEN_STR_end)
			whileNode->children[1]->children.push_back(parseBlockStatement());
		
		tokens.pop_front();
		
		return whileNode.release();
	}
	
	//! Parse "onevent" grammar element
	Node* Compiler::parseOnEvent()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		const unsigned eventId = expectAnyEventId();
		if (implementedEvents.find(eventId) != implementedEvents.end())
			throw Error(tokens.front().pos, WFormatableString(L"Event %0 is already implemented").arg(eventName(eventId)));
		implementedEvents.insert(eventId);
		
		tokens.pop_front();
		
		return new EventDeclNode(pos, eventId);
	}
	
	//! Parse "event" grammar element
	Node* Compiler::parseEmit()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		std::auto_ptr<EmitNode> emitNode(new EmitNode(pos));
		
		// event id
		emitNode->eventId = expectGlobalEventId();
		tokens.pop_front();
		
		// event argument
		unsigned eventSize = commonDefinitions->events[emitNode->eventId].value;
		if (eventSize > 0)
		{
			std::auto_ptr<Node> preNode(parseBinaryOrExpression());

			// allocate memory?
			if (!dynamic_cast<MemoryVectorNode*>(preNode.get()))
			{
				preNode.reset(allocateTemporaryVariable(pos, preNode.release()));
			}

			//allocateTemporaryVariable(pos)
			emitNode->arrayAddr = preNode->getVectorAddr();
			emitNode->arraySize = preNode->getVectorSize();
			emitNode->children.push_back(preNode.release());

			if (emitNode->arraySize != eventSize)
				throw Error(pos, WFormatableString(L"Event %0 needs an array of size %1, but one of size %2 is passed").arg(commonDefinitions->events[emitNode->eventId].name).arg(eventSize).arg(emitNode->arraySize));
		}
		else
		{
			emitNode->arrayAddr = 0;
			emitNode->arraySize = 0;
		}
		
		return emitNode.release();
	}
	
	//! Parse "sub" grammar element, declaration of subroutine
	Node* Compiler::parseSubDecl()
	{
		const SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		expect(Token::TOKEN_STRING_LITERAL);
		
		const std::wstring& name = tokens.front().sValue;
		const SubroutineReverseTable::const_iterator it = subroutineReverseTable.find(name);
		if (it != subroutineReverseTable.end())
			throw Error(tokens.front().pos, WFormatableString(L"Subroutine %0 is already defined").arg(name));
		
		const unsigned subroutineId = subroutineTable.size();
		subroutineTable.push_back(SubroutineDescriptor(name, 0, pos.row));
		subroutineReverseTable[name] = subroutineId;
		
		tokens.pop_front();
		
		return new SubDeclNode(pos, subroutineId);
	}
	
	//! Parse "subcall" grammar element, call of subroutine
	Node* Compiler::parseCallSub()
	{
		const SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		expect(Token::TOKEN_STRING_LITERAL);
		
		const std::wstring& name = tokens.front().sValue;
		const SubroutineReverseTable::const_iterator it(findSubroutine(name, pos));
		
		tokens.pop_front();
		
		return new CallSubNode(pos, it->second);
	}
	
	//! Parse "or" grammar element.
	Node* Compiler::parseOr()
	{
		std::auto_ptr<Node> node(parseAnd());
		
		while (tokens.front() == Token::TOKEN_OP_OR)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseAnd();
			node.reset(new BinaryArithmeticNode(pos, ASEBA_OP_OR, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "and" grammar element.
	Node* Compiler::parseAnd()
	{
		std::auto_ptr<Node> node(parseNot());
		
		while (tokens.front() == Token::TOKEN_OP_AND)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseNot();
			node.reset(new BinaryArithmeticNode(pos, ASEBA_OP_AND, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "not" grammar element.
	Node* Compiler::parseNot()
	{
		SourcePos pos = tokens.front().pos;
		
		// eat all trailing not
		bool odd = false;
		while (tokens.front() == Token::TOKEN_OP_NOT)
		{
			odd = !odd;
			tokens.pop_front();
		}
		
		if (odd)
			return new UnaryArithmeticNode(pos, ASEBA_UNARY_OP_NOT, parseCondition());
		else
			return parseCondition();
		/*
		
		std::auto_ptr<BinaryArithmeticNode> expression(parseCondition());
		
		// recurse on parenthesis
		if (tokens.front() == Token::TOKEN_PAR_OPEN)
		{
			tokens.pop_front();
			
			expression.reset(parseOr());
			
			expect(Token::TOKEN_PAR_CLOSE);
			tokens.pop_front();
		}
		else
		{
			expression.reset(parseCondition());
		}
		// apply de Morgan to remove the not
		if (odd)
			expression->deMorganNotRemoval();
		
		return expression.release();
		*/
	}
	
	//! Parse "condition" grammar element.
	Node* Compiler::parseCondition()
	{
		/*const Token::Type conditionTypes[] = { Token::TOKEN_OP_EQUAL, Token::TOKEN_OP_NOT_EQUAL, Token::TOKEN_OP_BIGGER, Token::TOKEN_OP_BIGGER_EQUAL, Token::TOKEN_OP_SMALLER, Token::TOKEN_OP_SMALLER_EQUAL };
		
		std::auto_ptr<Node> leftExprNode(parseBinaryOrExpression());
		
		EXPECT_ONE_OF(conditionTypes);
		
		Token::Type op = tokens.front();
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		Node *rightExprNode = parseBinaryOrExpression();
		return BinaryArithmeticNode::fromComparison(pos, op, leftExprNode.release(), rightExprNode);*/
		
		const Token::Type conditionTypes[] = { Token::TOKEN_OP_EQUAL, Token::TOKEN_OP_NOT_EQUAL, Token::TOKEN_OP_BIGGER, Token::TOKEN_OP_BIGGER_EQUAL, Token::TOKEN_OP_SMALLER, Token::TOKEN_OP_SMALLER_EQUAL };
		
		std::auto_ptr<Node> node(parseBinaryOrExpression());
		
		while (IS_ONE_OF(conditionTypes))
		{
			Token::Type op = tokens.front();
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseBinaryOrExpression();
			node.reset(BinaryArithmeticNode::fromComparison(pos, op, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "binary or" grammar element.
	Node *Compiler::parseBinaryOrExpression()
	{
		std::auto_ptr<Node> node(parseBinaryXorExpression());
		
		while (tokens.front() == Token::TOKEN_OP_BIT_OR)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseBinaryXorExpression();
			node.reset(new BinaryArithmeticNode(pos, ASEBA_OP_BIT_OR, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "binary xor" grammar element.
	Node *Compiler::parseBinaryXorExpression()
	{
		std::auto_ptr<Node> node(parseBinaryAndExpression());
		
		while (tokens.front() == Token::TOKEN_OP_BIT_XOR)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseBinaryAndExpression();
			node.reset(new BinaryArithmeticNode(pos, ASEBA_OP_BIT_XOR, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "binary and" grammar element.
	Node *Compiler::parseBinaryAndExpression()
	{
		std::auto_ptr<Node> node(parseShiftExpression());
		
		while (tokens.front() == Token::TOKEN_OP_BIT_AND)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseShiftExpression();
			node.reset(new BinaryArithmeticNode(pos, ASEBA_OP_BIT_AND, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "shift_expression" grammar element.
	Node *Compiler::parseShiftExpression()
	{
		const Token::Type opTypes[] = { Token::TOKEN_OP_SHIFT_LEFT, Token::TOKEN_OP_SHIFT_RIGHT };
		
		std::auto_ptr<Node> node(parseAddExpression());
		
		while (IS_ONE_OF(opTypes))
		{
			Token::Type op = tokens.front();
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseAddExpression();
			node.reset(BinaryArithmeticNode::fromShiftExpression(pos, op, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "add_expression" grammar element.
	Node *Compiler::parseAddExpression()
	{
		const Token::Type opTypes[] = { Token::TOKEN_OP_ADD, Token::TOKEN_OP_NEG };
		
		std::auto_ptr<Node> node(parseMultExpression());
		
		while (IS_ONE_OF(opTypes))
		{
			Token::Type op = tokens.front();
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseMultExpression();
			node.reset(BinaryArithmeticNode::fromAddExpression(pos, op, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "mult_expression" grammar element.
	Node *Compiler::parseMultExpression()
	{
		const Token::Type opTypes[] = { Token::TOKEN_OP_MULT, Token::TOKEN_OP_DIV, Token::TOKEN_OP_MOD };
		
		std::auto_ptr<Node> node(parseUnaryExpression());
		
		while (IS_ONE_OF(opTypes))
		{
			Token::Type op = tokens.front();
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();
			Node *subExpression = parseUnaryExpression();
			node.reset(BinaryArithmeticNode::fromMultExpression(pos, op, node.release(), subExpression));
		}
		
		return node.release();
	}
	
	//! Parse "unary_expression" grammar element.
	Node *Compiler::parseUnaryExpression()
	{
		const Token::Type acceptableTypes[] = { Token::TOKEN_PAR_OPEN, Token::TOKEN_BRACKET_OPEN, Token::TOKEN_OP_NEG, Token::TOKEN_OP_BIT_NOT, Token::TOKEN_STR_abs, Token::TOKEN_STRING_LITERAL, Token::TOKEN_INT_LITERAL };
		
		EXPECT_ONE_OF(acceptableTypes);
		SourcePos pos = tokens.front().pos;
		
		switch (tokens.front())
		{
			case Token::TOKEN_PAR_OPEN:
			{
				tokens.pop_front();
				
				std::auto_ptr<Node> expression(parseOr());
				
				expect(Token::TOKEN_PAR_CLOSE);
				tokens.pop_front();
				
				return expression.release();
			}

			case Token::TOKEN_BRACKET_OPEN:
			{
				return parseImmediateVector();
			}
			
			case Token::TOKEN_OP_NEG:
			{
				tokens.pop_front();
				
				return new UnaryArithmeticNode(pos, ASEBA_UNARY_OP_SUB, parseUnaryExpression());
			}
			
			case Token::TOKEN_OP_BIT_NOT:
			{
				tokens.pop_front();
				
				return new UnaryArithmeticNode(pos, ASEBA_UNARY_OP_BIT_NOT, parseUnaryExpression());
			};
			
			case Token::TOKEN_STR_abs:
			{
				tokens.pop_front();
				
				return new UnaryArithmeticNode(pos, ASEBA_UNARY_OP_ABS, parseUnaryExpression());
			}
			
			case Token::TOKEN_INT_LITERAL:
			{
				// immediate
				std::auto_ptr<ImmediateVectorNode> arrayCtor(new ImmediateVectorNode(pos, expectUInt16Literal()));
				tokens.pop_front();
				return arrayCtor.release();
			}
			
			case Token::TOKEN_STRING_LITERAL:
			{
				return parseConstantAndVariable();
			}
			
			default: internalCompilerError(); return NULL;
		}
	}

	//! Parse "[ .... ]" grammar element
	ImmediateVectorNode* Compiler::parseImmediateVector(bool compatibility)
	{
		if (!compatibility)
		{
			expect(Token::TOKEN_BRACKET_OPEN);
			tokens.pop_front();
		}
		else
		{
			// check if 2nd token is a comma
			if ( !(tokens.size() >= 2 && tokens[1] == Token::TOKEN_COMMA) )
			{
				// no
				return NULL;
			}
		}

		SourcePos varPos = tokens.front().pos;
		std::auto_ptr<ImmediateVectorNode> arrayCtor(new ImmediateVectorNode(varPos));

		do
		{
			arrayCtor->addValue(expectInt16LiteralOrConstant());
			tokens.pop_front();

			if (tokens.front() != Token::TOKEN_COMMA)
				break;
			else
				tokens.pop_front();
		}
		while (1);

		if (!compatibility)
		{
			expect(Token::TOKEN_BRACKET_CLOSE);
			tokens.pop_front();
		}

		return arrayCtor.release();
	}

	Node* Compiler::parseConstantAndVariable()
	{
		expect(Token::TOKEN_STRING_LITERAL);
		std::wstring varName = tokens.front().sValue;
		if (constantExists(varName))
		{
			std::auto_ptr<ImmediateVectorNode> arrayCtor(new ImmediateVectorNode(tokens.front().pos));
			arrayCtor->addValue(expectConstant());
			tokens.pop_front();
			return arrayCtor.release();
		}
		else
		{
			return parseVariable();
		}
	}

	MemoryVectorNode* Compiler::parseVariable()
	{
		expect(Token::TOKEN_STRING_LITERAL);
		std::wstring varName = tokens.front().sValue;
		SourcePos varPos = tokens.front().pos;
		VariablesMap::const_iterator varIt(findVariable(varName, varPos));

		std::auto_ptr<MemoryVectorNode> vector(
					new MemoryVectorNode(
						varPos,
						varIt->second.first,
						varIt->second.second,
						varName));

		// check if it is a const array access
		tokens.pop_front();
		if (tokens.front() == Token::TOKEN_BRACKET_OPEN)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();

			int start;
			std::auto_ptr<Node> startIndex(tryParsingConstantExpression(pos, start));

			if (startIndex.get() == NULL)
			{
				// constant index
				std::auto_ptr<ImmediateVectorNode> index(new ImmediateVectorNode(pos));
				index->addValue(start);

				// do we have array subscript?
				if (tokens.front() == Token::TOKEN_COLON)
				{
					pos = tokens.front().pos;
					tokens.pop_front();

					int end;
					std::auto_ptr<Node> endIndex(tryParsingConstantExpression(pos, end));

					if (endIndex.get() != NULL)
						throw Error(pos, L"Expecting a constant expression as a second index");

					// check if second index is within bounds
					if (end < start)
						throw Error(tokens.front().pos, WFormatableString(L"end of range index must be lower or equal to start of range index"));

					index->addValue(end);
				}

				vector->children.push_back(index.release());
			}
			else
			{
				// general expression as index
				vector->children.push_back(startIndex.release());
			}

			expect(Token::TOKEN_BRACKET_CLOSE);
			tokens.pop_front();
		}

		return vector.release();
	}

	unsigned Compiler::parseVariableDefSize()
	{
		int result = 0;

		if (tokens.front() == Token::TOKEN_BRACKET_OPEN)
		{
			SourcePos pos = tokens.front().pos;
			tokens.pop_front();

			if (tokens.front() == Token::TOKEN_BRACKET_CLOSE)
			{
				result = Node::E_NOVAL;
			}
			else
			{
				result = expectConstantExpression(pos, parseBinaryOrExpression());

				if (result < 0)
					// what??
					throw Error(pos, WFormatableString(L"Array size: result is negative (%0)").arg(result));
				else if (result == 0)
					throw Error(pos, L"Array size: result is null");

				expect(Token::TOKEN_BRACKET_CLOSE);
			}
			tokens.pop_front();
		}
		else
			// not an array
			result = 1;

		return result;
	}

	//! Use parseBinaryOrExpression() and try to reduce the result into an integer
	//! If successful, return NULL and the integer in constantResult
	//! If unsuccessful, return the parsed tree (constantResult useless in this case)
	Node* Compiler::tryParsingConstantExpression(SourcePos pos, int& constantResult)
	{
		std::auto_ptr<Node> tree(parseBinaryOrExpression());

		try
		{
			constantResult = expectConstantExpression(pos, tree->deepCopy());
			tree.reset();
			return NULL;
		}
		catch (Error error)
		{
			// oops, tree cannot be resolved to a constant, return it
			return tree.release();
		}
	}

	//! This is a generalization of expectPositiveInt16LiteralOrConstant()
	//! Try to reduce the expression into a single figure, if not raise an exception
	//! The tree pointed by "tree" is deleted during execution, not safe to use it after
	int Compiler::expectConstantExpression(SourcePos pos, Node* tree)
	{
		int result = 0;

		// create a temporary "var = expr" tree
		// used to access the facility offered by the AssignmentNode (size check,...)
		std::auto_ptr<Node> tempTree1(new AssignmentNode(pos));
		tempTree1->children.push_back(new MemoryVectorNode(pos, 0, 1, L"fake"));
		tempTree1->children.push_back(tree);

		//tempTree1->children.push_back(parseBinaryOrExpression());

		//unsigned indent = 0;
		//std::cerr << "Tree before expanding" << std::endl;
		//tempTree1->dump(std::wcerr, indent);

		std::auto_ptr<Node> tempTree2(tempTree1->expandToAsebaTree(NULL));

		//std::cerr << "Tree after expanding" << std::endl;
		//tempTree2->dump(std::wcerr, indent);

		tempTree1.release();	// tree already deleted by expandToAsebaTree()
		tempTree2->optimize(NULL);

		//std::cerr << "Tree after optimization" << std::endl;
		//tempTree2->dump(std::wcerr, indent);
		//std::cerr << std::endl;

		// valid optimization?
		if ( !tempTree2.get() || tempTree2->children.size() == 0 )
			throw Error(pos, L"Not a valid constant expression");
		AssignmentNode* assignment = dynamic_cast<AssignmentNode*>(tempTree2->children[0]);
		if ( !assignment || assignment->children.size() != 2 )
			throw Error(pos, L"Not a valid constant expression");

		// resolve to an ImmediateNode?
		ImmediateNode* immediate = dynamic_cast<ImmediateNode*>(assignment->children[1]);
		if (immediate)
			result = immediate->value;
		else
			throw Error(pos, L"Not a valid constant expression");

		// delete the tree
		tempTree2.reset();

		return result;
	}
	
	//! Parse "function_call" grammar element.
	Node* Compiler::parseFunctionCall()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		expect(Token::TOKEN_STRING_LITERAL);
		
		std::wstring funcName = tokens.front().sValue;
		FunctionsMap::const_iterator funcIt(findFunction(funcName, pos));
		
		const TargetDescription::NativeFunction &function = targetDescription->nativeFunctions[funcIt->second];
		std::auto_ptr<CallNode> callNode(new CallNode(pos, funcIt->second));
		
		tokens.pop_front();
		
		expect(Token::TOKEN_PAR_OPEN);
		tokens.pop_front();
		
		if (function.parameters.size() == 0)
		{
			if (tokens.front() != Token::TOKEN_PAR_CLOSE)
				throw Error(tokens.front().pos, WFormatableString(L"Function %0 requires no argument, some are used").arg(funcName));
			tokens.pop_front();
		}
		else
		{
			// count the number of template parameters and build array
			int minTemplateId = 0;
			for (unsigned i = 0; i < function.parameters.size(); i++)
				minTemplateId = std::min(function.parameters[i].size, minTemplateId);
			std::valarray<int> templateParameters(-1, -minTemplateId);
			
			// trees for arguments
			for (unsigned i = 0; i < function.parameters.size(); i++)
			{
				// check if it is an argument
				if (tokens.front() == Token::TOKEN_PAR_CLOSE)
					throw Error(tokens.front().pos, WFormatableString(L"Function %0 requires %1 arguments, only %2 are provided").arg(funcName).arg(function.parameters.size()).arg(i));
				
				// we need to fill those two variables
				unsigned varAddr;
				unsigned varSize;
				SourcePos varPos = tokens.front().pos;

				std::auto_ptr<Node> preNode(parseBinaryOrExpression());
				// get the address and size
				varAddr = preNode->getVectorAddr();
				varSize = preNode->getVectorSize();

				if (!dynamic_cast<MemoryVectorNode*>(preNode.get()))
				{
					preNode.reset(allocateTemporaryVariable(pos, preNode.release()));
					varAddr = preNode->getVectorAddr();
					callNode->children.push_back(preNode.release());
				}
				
				// check if variable size is correct
				if (function.parameters[i].size > 0)
				{
					if (varSize != (unsigned)function.parameters[i].size)
						throw Error(varPos, WFormatableString(L"Argument %0 (%1) of function %2 is of size %3, function definition demands size %4").arg(i).arg(function.parameters[i].name).arg(funcName).arg(varSize).arg(function.parameters[i].size));
				}
				else if (function.parameters[i].size < 0)
				{
					int templateIndex = -function.parameters[i].size - 1;
					if (templateParameters[templateIndex] < 0)
					{
						// template not initialised, store
						templateParameters[templateIndex] = varSize;
					}
					else if ((unsigned)templateParameters[templateIndex] != varSize)
					{
						throw Error(varPos, WFormatableString(L"Argument %0 (%1) of function %2 is of size %3, while a previous instance of the template parameter was of size %4").arg(i).arg(function.parameters[i].name).arg(funcName).arg(varSize).arg(templateParameters[templateIndex]));
					}
				}
				else
				{
					// any size, store variable size
					callNode->argumentsAddr.push_back(varSize);
				}
				
				// store variable address
				callNode->argumentsAddr.push_back(varAddr);
				
				// parse comma or end parenthesis
				if (i + 1 == function.parameters.size())
				{
					if (tokens.front() == Token::TOKEN_COMMA)
						throw Error(tokens.front().pos, WFormatableString(L"Function %0 requires %1 arguments, more are used").arg(funcName).arg(function.parameters.size()));
					else
						expect(Token::TOKEN_PAR_CLOSE);
				}
				else
				{
					if (tokens.front() == Token::TOKEN_PAR_CLOSE)
						throw Error(tokens.front().pos, WFormatableString(L"Function %0 requires %1 arguments, only %2 are provided").arg(funcName).arg(function.parameters.size()).arg(i + 1));
					else
						expect(Token::TOKEN_COMMA);
				}
				tokens.pop_front();
			} // for
			
			// store template parameters size when initialized
			for (unsigned i = 0; i < templateParameters.size(); i++)
				if (templateParameters[i] >= 0)
					callNode->argumentsAddr.push_back(templateParameters[i]);
		} // if
		
		// return the node for the function call
		return callNode.release();
	}
}; // Aseba
