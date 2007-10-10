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
		assert(eventsNames);
		
		unsigned eventId = 0xFFFF;
		if (tokens.front() == Token::TOKEN_STRING_LITERAL)
		{
			const std::string & eventName = tokens.front().sValue;
			for (size_t i = 0; i < eventsNames->size(); ++i)
				if ((*eventsNames)[i] == eventName)
				{
					eventId = i;
					break;
				}
			
			if (eventId == 0xFFFF)
				throw Error(tokens.front().pos, FormatableString("%0 is not a known event").arg(eventName));
		}
		else
			eventId = expectUInt12Literal();
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
			case Token::TOKEN_STR_while: return parseWhile();
			case Token::TOKEN_STR_onevent: return parseOnEvent();
			case Token::TOKEN_STR_ontimer: return parseOnTimer();
			case Token::TOKEN_STR_emit: return parseEvent();
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
				int value = expectInt16Literal();
				
				block->children.push_back(new ImmediateNode(tokens.front().pos, value));
				block->children.push_back(new StoreNode(tokens.front().pos, varAddr + i));
				
				tokens.pop_front();
				
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
		const Token::Type conditionTypes[4] = { Token::TOKEN_OP_EQUAL, Token::TOKEN_OP_NOT_EQUAL, Token::TOKEN_OP_BIGGER, Token::TOKEN_OP_SMALLER };
		
		std::auto_ptr<IfWhenNode> ifNode(new IfWhenNode(tokens.front().pos));
		
		// eat "if" / "when"
		ifNode->edgeSensitive = edgeSensitive;
		tokens.pop_front();
		
		// left part of conditional expression
		ifNode->children.push_back(parseShiftExpression());
		
		// comparison
		expectOneOf(conditionTypes, 4);
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
	
	//! Parse "while" grammar element.
	Node* Compiler::parseWhile()
	{
		const Token::Type conditionTypes[4] = { Token::TOKEN_OP_EQUAL, Token::TOKEN_OP_NOT_EQUAL, Token::TOKEN_OP_BIGGER, Token::TOKEN_OP_SMALLER };
		
		std::auto_ptr<WhileNode> whileNode(new WhileNode(tokens.front().pos));
		
		// eat "while"
		tokens.pop_front();
		
		// left part of conditional expression
		whileNode->children.push_back(parseShiftExpression());
		
		// comparison
		expectOneOf(conditionTypes, 4);
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
	Node* Compiler::parseEvent()
	{
		SourcePos pos = tokens.front().pos;
		tokens.pop_front();
		
		std::auto_ptr<EmitNode> emitNode(new EmitNode(pos));
		
		// event id
		emitNode->eventId = expectEventId();
		tokens.pop_front();
		
		// event argument
		if (tokens.front() == Token::TOKEN_STRING_LITERAL)
		{
			std::string varName = tokens.front().sValue;
			SourcePos varPos = tokens.front().pos;
			VariablesMap::const_iterator varIt = variablesMap.find(varName);
			
			// check if variable exists
			if (varIt == variablesMap.end())
				throw Error(varPos, FormatableString("Variable %0 is not defined").arg(varName));
			
			emitNode->arrayAddr = varIt->second.first;
			emitNode->arraySize = varIt->second.second;
			
			tokens.pop_front();
		}
		else
		{
			emitNode->arrayAddr = 0;
			emitNode->arraySize = 0;
		}
		
		return emitNode.release();
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
		
		while (isOneOf(opTypes, 2))
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
			// TODO: use template
			
			// trees for arguments
			for (unsigned i = 0; i < function.parameters.size(); i++)
			{
				// check if it is an argument
				if (tokens.front() == Token::TOKEN_PAR_CLOSE)
					throw Error(tokens.front().pos, FormatableString("Function %0 requires %1 arguments, only %2 are provided").arg(funcName).arg(function.parameters.size()).arg(i));
				else
					expect(Token::TOKEN_STRING_LITERAL);
				
				// check if variable exists
				std::string varName = tokens.front().sValue;
				SourcePos varPos = tokens.front().pos;
				VariablesMap::const_iterator varIt = variablesMap.find(varName);
				if (varIt == variablesMap.end())
					throw Error(varPos, FormatableString("Argument %0 of function %1 is not a defined variable").arg(varName).arg(funcName));
				
				// check if variable size is correct
				unsigned varAddr = varIt->second.first;
				unsigned varSize = varIt->second.second;
				if (varSize != function.parameters[i].size)
					throw Error(varPos, FormatableString("Argument %0 of function %1 is of size %2, function definition demands size %3").arg(varName).arg(funcName).arg(varSize).arg(function.parameters[i].size));
				
				// store variable address
				callNode->argumentsAddr.push_back(varAddr);
				tokens.pop_front();
				
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
		} // if
		
		// return the node for the function call
		return callNode.release();
	}
}; // Aseba
