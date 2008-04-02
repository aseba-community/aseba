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

#include "compiler.h"
#include "tree.h"
#include "../utils/FormatableString.h"
#include <memory>
#include <valarray>
#include <iostream>
#include <cassert>

namespace Aseba
{
	//! There is a bug in the compiler, ask for a bug report
	void Compiler::internalCompilerError() const
	{
		throw Error(tokens.front().pos, "Internal compiler error, please report a bug containing the source which triggered this error");
	}
	
	//! Check if next token is of type, produce an exception otherwise
	void Compiler::expect(const Token::Type& type) const
	{
		if (tokens.front() != type)
			throw Error(tokens.front().pos,
				FormatableString("Expecting %0, found %1 instead")
					.arg(Token(type).typeName())
					.arg(tokens.front().typeName())
				);
	}
	
	//! Check if next token is a signed 16 bits int literal. If so, return it, if not, throw an exception
	int Compiler::expectInt16Literal() const
	{
		expect(Token::TOKEN_INT_LITERAL);
		if ((tokens.front().iValue > 32767) || (tokens.front().iValue < -32768))
			throw Error(tokens.front().pos,
				FormatableString("Integer value %0 out of [-32768;32767] range")
					.arg(tokens.front().iValue)
				);
		return tokens.front().iValue;
	}
	
	//! Check if next token is an unsigned 12 bits int literal. If so, return it, if not, throw an exception
	unsigned Compiler::expectUInt12Literal() const
	{
		expect(Token::TOKEN_INT_LITERAL);
		if (tokens.front().iValue > 4095)
			throw Error(tokens.front().pos,
				FormatableString("Integer value %0 out of [0;4095] range")
					.arg(tokens.front().iValue)
				);
		return tokens.front().iValue;
	}
	
	//! Check if next token is a valid event id. That is either an unsigned 12 bits int literal or a known event string identifier
	unsigned Compiler::expectEventId() const
	{
		assert(commonDefinitions);
		
		expect(Token::TOKEN_STRING_LITERAL);
		
		unsigned eventId = 0xFFFF;
		const std::string & eventName = tokens.front().sValue;
		for (size_t i = 0; i < commonDefinitions->events.size(); ++i)
			if (commonDefinitions->events[i].name == eventName)
			{
				eventId = i;
				break;
			}
		
		if (eventId == 0xFFFF)
			throw Error(tokens.front().pos, FormatableString("%0 is not a known event").arg(eventName));
		
		return eventId;
	}
	
	//! Return true if next token is of the following types
	bool Compiler::isOneOf(const Token::Type *types, size_t length) const
	{
		for (size_t i = 0; i < length; i++)
		{
			if (tokens.front() == types[i])
				return true;
		}
		return false;
	}
	
	//! Check if next token is of one of the following types, produce an exception otherwise
	void Compiler::expectOneOf(const Token::Type *types, size_t length) const
	{
		if (!isOneOf(types, length))
		{
			std::string errorMessage("Expecting one of");
			for (size_t i = 0; i < length; i++)
			{
				errorMessage += " ";
				errorMessage += Token(types[i]).typeName();
			}
			errorMessage += ", found ";
			errorMessage += tokens.front().typeName();
			errorMessage += " instead";
			throw Error(tokens.front().pos, errorMessage);
		}
	}
	
	//! Parse "program" grammar element.
	Node* Compiler::parseProgram()
	{
		std::auto_ptr<BlockNode> block(new BlockNode(tokens.front().pos));
		while (tokens.front() != Token::TOKEN_END_OF_STREAM)
		{
			// program may receive NULL pointers because non initialized variables produce no code
			Node *child = parseStatement();
			if (child)
				block->children.push_back(child);
		}
		return block.release();
	}
	
	//! Parse "statement" grammar element.
	Node* Compiler::parseStatement()
	{
		switch (tokens.front())
		{
			case Token::TOKEN_STR_var: return parseVarDef();
			case Token::TOKEN_STR_onevent: return parseOnEvent();
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
			case Token::TOKEN_STR_onevent: return parseOnEvent();
			case Token::TOKEN_STR_ontimer: return parseOnTimer();
			case Token::TOKEN_STR_emit: return parseEmit();
			case Token::TOKEN_STR_call: return parseFunctionCall();
			default: return parseAssignment();
		}
	}
	
	//! Parse "var def" grammar element.
	Node* Compiler::parseVarDef()
	{
		tokens.pop_front();
		
		// check for string literal
		if (tokens.front() != Token::TOKEN_STRING_LITERAL)
			throw Error(tokens.front().pos,
				FormatableString("Expecting identifier, found %0").arg(tokens.front().toString()));
		
		// save variable
		std::string varName = tokens.front().sValue;
		SourcePos varPos = tokens.front().pos;
		unsigned varSize = 1;
		unsigned varAddr = freeVariableIndex;
		
		tokens.pop_front();
		
		// optional array
		if (tokens.front() == Token::TOKEN_BRACKET_OPEN)
		{
			tokens.pop_front();
			
			expect(Token::TOKEN_INT_LITERAL);
			varSize = tokens.front().iValue;
			if (varSize == 0)
				throw Error(tokens.front().pos, "Error, arrays must have a non-zero size");
			tokens.pop_front();
			
			expect(Token::TOKEN_BRACKET_CLOSE);
			tokens.pop_front();
		}
		
		// check if variable exists
		if (variablesMap.find(varName) != variablesMap.end())
			throw Error(varPos, FormatableString("Variable %0 is already defined").arg(varName));
		
		// save variable
		variablesMap[varName] = std::make_pair(varAddr, varSize);
		freeVariableIndex += varSize;
		
		// check space
		if (freeVariableIndex > targetDescription->variablesSize)
			throw Error(varPos, "No more free variable space");
		
		// optional assignation
		if (tokens.front() == Token::TOKEN_ASSIGN)
		{
			std::auto_ptr<BlockNode> block(new BlockNode(tokens.front().pos));
			
			tokens.pop_front();
			
			for (unsigned i = 0; i < varSize; i++)
			{
				block->children.push_back(parseShiftExpression());
				block->children.push_back(new StoreNode(tokens.front().pos, varAddr + i));
				
				if (i + 1 < varSize)
				{
					expect(Token::TOKEN_COMMA);
					tokens.pop_front();
				}
			}
			
			return block.release();
		}
		else
			return NULL;
	}
	
	//! Parse "assignment" grammar element
	Node* Compiler::parseAssignment()
	{
		expect(Token::TOKEN_STRING_LITERAL);
		
		std::string varName = tokens.front().sValue;
		SourcePos varPos = tokens.front().pos;
		VariablesMap::const_iterator varIt = variablesMap.find(varName);
		
		// check if variable exists
		if (varIt == variablesMap.end())
			throw Error(varPos, FormatableString("Variable %0 is not defined").arg(varName));
		
		unsigned varAddr = varIt->second.first;
		unsigned varSize = varIt->second.second;
		
		tokens.pop_front();
		
		std::auto_ptr<AssignmentNode> assignement(new AssignmentNode(varPos));
		
		// check if it is an array access
		if (tokens.front() == Token::TOKEN_BRACKET_OPEN)
		{
			tokens.pop_front();
			
			std::auto_ptr<Node> array(new ArrayWriteNode(tokens.front().pos, varAddr, varSize, varName));
		
			array->children.push_back(parseShiftExpression());

			expect(Token::TOKEN_BRACKET_CLOSE);
			tokens.pop_front();
		
			assignement->children.push_back(array.release());
		}
		else
		{
			if (varSize != 1)
				throw Error(varPos, FormatableString("Accessing variable %0 as a scalar, but is an array of size %1 instead").arg(varName).arg(varSize));
			
			assignement->children.push_back(new StoreNode(tokens.front().pos, varAddr));
		}
		
		expect(Token::TOKEN_ASSIGN);
		tokens.pop_front();
		
		assignement->children.push_back(parseShiftExpression());
		
		return assignement.release();
	}
	
	//! Parse "if" grammar element.
	Node* Compiler::parseIfWhen(bool edgeSensitive)
	{
		const Token::Type elseEndTypes[2] = { Token::TOKEN_STR_else, Token::TOKEN_STR_end };
		const Token::Type conditionTypes[6] = { Token::TOKEN_OP_EQUAL, Token::TOKEN_OP_NOT_EQUAL, Token::TOKEN_OP_BIGGER, Token::TOKEN_OP_BIGGER_EQUAL, Token::TOKEN_OP_SMALLER, Token::TOKEN_OP_SMALLER_EQUAL };
		
		std::auto_ptr<IfWhenNode> ifNode(new IfWhenNode(tokens.front().pos));
		
		// eat "if" / "when"
		ifNode->edgeSensitive = edgeSensitive;
		tokens.pop_front();
		
		// left part of conditional expression
		ifNode->children.push_back(parseShiftExpression());
		
		// comparison
		expectOneOf(conditionTypes, 6);
		ifNode->comparaison = static_cast<AsebaComparaison>(tokens.front() - Token::TOKEN_OP_EQUAL);
		tokens.pop_front();
		
		// right part of conditional expression
		ifNode->children.push_back(parseShiftExpression());
		
		// then keyword
		if (edgeSensitive)
			expect(Token::TOKEN_STR_do);
		else
			expect(Token::TOKEN_STR_then);
		tokens.pop_front();
		
		// parse true condition
		ifNode->children.push_back(new BlockNode(tokens.front().pos));
		while (!isOneOf(elseEndTypes, 2))
			ifNode->children[2]->children.push_back(parseBlockStatement());
		
		// parse false condition (only for if)
		if (!edgeSensitive && (tokens.front() == Token::TOKEN_STR_else))
		{
			tokens.pop_front();
			
			ifNode->children.push_back(new BlockNode(tokens.front().pos));
			while (tokens.front() != Token::TOKEN_STR_end)
				ifNode->children[3]->children.push_back(parseBlockStatement());
		}
		
		// end keyword
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
		expect(Token::TOKEN_STRING_LITERAL);
		std::string varName = tokens.front().sValue;
		SourcePos varPos = tokens.front().pos;
		VariablesMap::const_iterator varIt = variablesMap.find(varName);
		
		// check if variable exists
		if (varIt == variablesMap.end())
			throw Error(varPos, FormatableString("Variable %0 is not defined").arg(varName));
		unsigned varAddr = varIt->second.first;
		tokens.pop_front();
		
		// in keyword
		expect(Token::TOKEN_STR_in);
		tokens.pop_front();
		
		// range start index
		int rangeStartIndex = expectInt16Literal();
		SourcePos rangeStartIndexPos = tokens.front().pos;
		tokens.pop_front();
		
		// : keyword
		expect(Token::TOKEN_COLON);
		tokens.pop_front();
		
		// range end index
		int rangeEndIndex = expectInt16Literal();
		SourcePos rangeEndIndexPos = tokens.front().pos;
		tokens.pop_front();
		
		// check for step
		int step = 1;
		if (tokens.front() == Token::TOKEN_STR_step)
		{
			tokens.pop_front();
			if (tokens.front() == Token::TOKEN_OP_NEG)
			{
				tokens.pop_front();
				step = -expectInt16Literal();
			}
			else
				step = expectInt16Literal();
			if (step == 0)
				throw Error(tokens.front().pos, "Null steps are not allowed in for loops");
			tokens.pop_front();
		}
		
		// check conditions
		if (step > 0)
		{
			if (rangeStartIndex > rangeEndIndex)
				throw Error(rangeStartIndexPos, "Start index must be lower than end index in increasing loops");
		}
		else
		{
			if (rangeStartIndex < rangeEndIndex)
				throw Error(rangeStartIndexPos, "Start index must be higher than end index in decreasing loops");
		}
		
		// do keyword
		expect(Token::TOKEN_STR_do);
		tokens.pop_front();
		
		// create enclosing block and initial variable state
		std::auto_ptr<BlockNode> blockNode(new BlockNode(whilePos));
		blockNode->children.push_back(new ImmediateNode(rangeStartIndexPos, rangeStartIndex));
		blockNode->children.push_back(new StoreNode(varPos, varAddr));
		
		// create while and condition
		WhileNode* whileNode = new WhileNode(whilePos);
		blockNode->children.push_back(whileNode);
		whileNode->children.push_back(new LoadNode(varPos, varAddr));
		if (rangeStartIndex <= rangeEndIndex)
			whileNode->comparaison = ASEBA_CMP_SMALLER_EQUAL_THAN;
		else
			whileNode->comparaison = ASEBA_CMP_BIGGER_EQUAL_THAN;
		whileNode->children.push_back(new ImmediateNode(rangeEndIndexPos, rangeEndIndex));
		
		// block and end keyword
		whileNode->children.push_back(new BlockNode(tokens.front().pos));
		while (tokens.front() != Token::TOKEN_STR_end)
			whileNode->children[2]->children.push_back(parseBlockStatement());
		
		// increment variable
		AssignmentNode* assignmentNode = new AssignmentNode(varPos);
		whileNode->children[2]->children.push_back(assignmentNode);
		assignmentNode->children.push_back(new StoreNode(varPos, varAddr));
		assignmentNode->children.push_back(new BinaryArithmeticNode(varPos, ASEBA_OP_ADD, new LoadNode(varPos, varAddr), new ImmediateNode(varPos, step)));
		
		tokens.pop_front();
		
		return blockNode.release();
	}
	
	//! Parse "while" grammar element.
	Node* Compiler::parseWhile()
	{
		const Token::Type conditionTypes[6] = { Token::TOKEN_OP_EQUAL, Token::TOKEN_OP_NOT_EQUAL, Token::TOKEN_OP_BIGGER, Token::TOKEN_OP_BIGGER_EQUAL, Token::TOKEN_OP_SMALLER, Token::TOKEN_OP_SMALLER_EQUAL };
		
		std::auto_ptr<WhileNode> whileNode(new WhileNode(tokens.front().pos));
		
		// eat "while"
		tokens.pop_front();
		
		// left part of conditional expression
		whileNode->children.push_back(parseShiftExpression());
		
		// comparison
		expectOneOf(conditionTypes, 6);
		whileNode->comparaison = static_cast<AsebaComparaison>(tokens.front() - Token::TOKEN_OP_EQUAL);
		tokens.pop_front();
		
		// right part of conditional expression
		whileNode->children.push_back(parseShiftExpression());
		
		// do keyword
		expect(Token::TOKEN_STR_do);
		tokens.pop_front();
		
		// block and end keyword
		whileNode->children.push_back(new BlockNode(tokens.front().pos));
		while (tokens.front() != Token::TOKEN_STR_end)
			whileNode->children[2]->children.push_back(parseBlockStatement());
		
		tokens.pop_front();
		
		return whileNode.release();
	}
	
	//! Parse "onevent" grammar element
	Node* Compiler::parseOnEvent()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		unsigned eventId = expectEventId();
		tokens.pop_front();
		
		return new ContextSwitcherNode(pos, eventId);
	}
	
	//! Parse "ontimer" grammar element
	Node* Compiler::parseOnTimer()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		return new ContextSwitcherNode(pos, ASEBA_EVENT_PERIODIC);
	}
	
	//! Parse "event" grammar element
	Node* Compiler::parseEmit()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		std::auto_ptr<EmitNode> emitNode(new EmitNode(pos));
		
		// event id
		emitNode->eventId = expectEventId();
		tokens.pop_front();
		
		// event argument
		unsigned eventSize = commonDefinitions->events[emitNode->eventId].size;
		if (eventSize > 0)
		{
			parseReadVarArrayAccess(&emitNode->arrayAddr, &emitNode->arraySize);
			if (emitNode->arraySize != eventSize)
				throw Error(pos, FormatableString("Event %0 needs an array of size %1, but one of size %2 is passed").arg(commonDefinitions->events[emitNode->eventId].name).arg(eventSize).arg(emitNode->arraySize));
		}
		else
		{
			emitNode->arrayAddr = 0;
			emitNode->arraySize = 0;
		}
		
		return emitNode.release();
	}
	
	//! Parse access to variable, arrray, or array subscript
	void Compiler::parseReadVarArrayAccess(unsigned* addr, unsigned* size)
	{
		expect(Token::TOKEN_STRING_LITERAL);
		
		// we have a variable access, check if variable exists
		SourcePos varPos = tokens.front().pos;
		std::string varName = tokens.front().sValue;
		VariablesMap::const_iterator varIt = variablesMap.find(varName);
		if (varIt == variablesMap.end())
			throw Error(varPos, FormatableString("%0 is not a defined variable").arg(varName));
		
		// store variable address
		*addr = varIt->second.first;
		
		// check if it is a const array access
		tokens.pop_front();
		if (tokens.front() == Token::TOKEN_BRACKET_OPEN)
		{
			tokens.pop_front();
			expect(Token::TOKEN_INT_LITERAL);
			unsigned startIndex = tokens.front().iValue;
			unsigned len = 1;
			
			// check if first index is within bounds
			if (startIndex >= varIt->second.second)
				throw Error(tokens.front().pos, FormatableString("access of array %0 out of bounds").arg(varName));
			tokens.pop_front();
			
			// do we have array subscript?
			if (tokens.front() == Token::TOKEN_COLON)
			{
				tokens.pop_front();
				expect(Token::TOKEN_INT_LITERAL);
				
				// check if second index is within bounds
				unsigned endIndex = tokens.front().iValue;
				if (endIndex >= varIt->second.second)
					throw Error(tokens.front().pos, FormatableString("access of array %0 out of bounds").arg(varName));
				if (endIndex < startIndex)
					throw Error(tokens.front().pos, FormatableString("end of range index must be lower or equal to start of range index"));
				tokens.pop_front();
				
				len = endIndex - startIndex + 1;
			}
			
			expect(Token::TOKEN_BRACKET_CLOSE);
			tokens.pop_front();
			
			*addr += startIndex;
			*size = len;
		}
		else
		{
			// full array
			*size = varIt->second.second;
		}
	}
	
	//! Parse "shift_expression" grammar element.
	Node *Compiler::parseShiftExpression()
	{
		const Token::Type opTypes[2] = { Token::TOKEN_OP_SHIFT_LEFT, Token::TOKEN_OP_SHIFT_RIGHT };
		
		std::auto_ptr<Node> node(parseAddExpression());
		
		while (isOneOf(opTypes, 2))
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
		const Token::Type opTypes[2] = { Token::TOKEN_OP_ADD, Token::TOKEN_OP_NEG };
		
		std::auto_ptr<Node> node(parseMultExpression());
		
		while (isOneOf(opTypes, 2))
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
		const Token::Type opTypes[3] = { Token::TOKEN_OP_MULT, Token::TOKEN_OP_DIV, Token::TOKEN_OP_MOD };
		
		std::auto_ptr<Node> node(parseUnaryExpression());
		
		while (isOneOf(opTypes, 3))
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
		const Token::Type acceptableTypes[4] = { Token::TOKEN_PAR_OPEN, Token::TOKEN_OP_NEG, Token::TOKEN_STRING_LITERAL, Token::TOKEN_INT_LITERAL };
		
		expectOneOf(acceptableTypes, 4);
		SourcePos pos = tokens.front().pos;
		
		switch (tokens.front())
		{
			case Token::TOKEN_PAR_OPEN:
			{
				tokens.pop_front();
				
				std::auto_ptr<Node> expression(parseShiftExpression());
				
				expect(Token::TOKEN_PAR_CLOSE);
				tokens.pop_front();
				
				return expression.release();
			}
			
			case Token::TOKEN_OP_NEG:
			{
				tokens.pop_front();
				
				std::auto_ptr<Node> negation(new UnaryArithmeticNode(pos));
				negation->children.push_back(parseUnaryExpression());
				return negation.release();
			}
			
			case Token::TOKEN_INT_LITERAL:
			{
				int value = expectInt16Literal();
				tokens.pop_front();
				return new ImmediateNode(pos, value);
			}
			
			case Token::TOKEN_STRING_LITERAL:
			{
				std::string varName = tokens.front().sValue;
				SourcePos varPos = tokens.front().pos;
				VariablesMap::const_iterator varIt = variablesMap.find(varName);
				
				// check if variable exists
				if (varIt == variablesMap.end())
					throw Error(varPos, FormatableString("Variable %0 is not defined").arg(varName));
				
				unsigned varAddr = varIt->second.first;
				unsigned varSize = varIt->second.second;
				
				tokens.pop_front();
				
				// check if it is an array access
				if (tokens.front() == Token::TOKEN_BRACKET_OPEN)
				{
					tokens.pop_front();
					
					std::auto_ptr<Node> array(new ArrayReadNode(pos, varAddr, varSize, varName));
				
					array->children.push_back(parseShiftExpression());

					expect(Token::TOKEN_BRACKET_CLOSE);
					tokens.pop_front();
				
					return array.release();
				}
				else
				{
					if (varSize != 1)
						throw Error(varPos, FormatableString("Accessing variable %0 as a scalar, but is an array of size %1 instead").arg(varName).arg(varSize));
					
					return new LoadNode(pos, varAddr);
				}
			}
			
			default: internalCompilerError(); return NULL;
		}
	}
	
	//! Parse "function_call" grammar element.
	Node* Compiler::parseFunctionCall()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		expect(Token::TOKEN_STRING_LITERAL);
		std::string funcName = tokens.front().sValue;
		tokens.pop_front();
		
		FunctionsMap::const_iterator funcIt = functionsMap.find(funcName);
		
		// check if function exists, get it and create node
		if (funcIt == functionsMap.end())
			throw Error(tokens.front().pos, FormatableString("Target does not provide function %0").arg(funcName));
		const TargetDescription::NativeFunction &function = targetDescription->nativeFunctions[funcIt->second];
		std::auto_ptr<CallNode> callNode(new CallNode(pos, funcIt->second));
		
		expect(Token::TOKEN_PAR_OPEN);
		tokens.pop_front();
		
		if (function.parameters.size() == 0)
		{
			if (tokens.front() != Token::TOKEN_PAR_CLOSE)
				throw Error(tokens.front().pos, FormatableString("Function %0 requires no argument, some are used").arg(funcName));
			tokens.pop_front();
		}
		else
		{
			// we store constants for this call at the end of memory
			unsigned localConstsAddr = targetDescription->variablesSize;
			
			// count the number of template parameters and build array
			int minTemplateId = 0;
			for (unsigned i = 0; i < function.parameters.size(); i++)
				minTemplateId = std::min(function.parameters[i].size, minTemplateId);
			std::valarray<int> templateParameters(-1, -minTemplateId);
			
			// trees for arguments
			for (unsigned i = 0; i < function.parameters.size(); i++)
			{
				const Token::Type argTypes[2] = { Token::TOKEN_STRING_LITERAL, Token::TOKEN_INT_LITERAL };
				
				// check if it is an argument
				if (tokens.front() == Token::TOKEN_PAR_CLOSE)
					throw Error(tokens.front().pos, FormatableString("Function %0 requires %1 arguments, only %2 are provided").arg(funcName).arg(function.parameters.size()).arg(i));
				else
					expectOneOf(argTypes, 2);
				
				// we need to fill those two variables
				unsigned varAddr;
				unsigned varSize;
				SourcePos varPos = tokens.front().pos;
				
				if (tokens.front() == Token::TOKEN_STRING_LITERAL)
				{
					parseReadVarArrayAccess(&varAddr, &varSize);
				}
				else
				{
					// we have inline integer, we need to store it and pass the address
					// we store it at the end of the variable space, we test if there is no overflow
					if (localConstsAddr <= freeVariableIndex)
						throw Error(varPos, "No more free variable space for constant");
					varAddr = --localConstsAddr;
					varSize = 1;
					callNode->children.push_back(new ImmediateNode(varPos, tokens.front().iValue));
					callNode->children.push_back(new StoreNode(varPos, varAddr));
					tokens.pop_front();
				}
				
				// check if variable size is correct
				if (function.parameters[i].size > 0)
				{
					if (varSize != (unsigned)function.parameters[i].size)
						throw Error(varPos, FormatableString("Argument %0 (%1) of function %2 is of size %3, function definition demands size %4").arg(i).arg(function.parameters[i].name).arg(funcName).arg(varSize).arg(function.parameters[i].size));
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
						throw Error(varPos, FormatableString("Argument %0 (%1) of function %2 is of size %3, while a previous instance of the template parameter was of size %4").arg(i).arg(function.parameters[i].name).arg(funcName).arg(varSize).arg(templateParameters[templateIndex]));
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
						throw Error(tokens.front().pos, FormatableString("Function %0 requires %1 arguments, more are used").arg(funcName).arg(function.parameters.size()));
					else
						expect(Token::TOKEN_PAR_CLOSE);
				}
				else
				{
					if (tokens.front() == Token::TOKEN_PAR_CLOSE)
						throw Error(tokens.front().pos, FormatableString("Function %0 requires %1 arguments, only %2 are provided").arg(funcName).arg(function.parameters.size()).arg(i + 1));
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
