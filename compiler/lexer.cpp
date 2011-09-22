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

#include "compiler.h"
#include "../utils/FormatableString.h"
#include <cstdlib>
#include <sstream>
#include <ostream>
#include <cctype>
#include <cstdio>

namespace Aseba
{
	//! Construct a new token of given type and value
	Compiler::Token::Token(Type type, SourcePos pos, const std::wstring& value) :
		type(type),
		sValue(value),
		pos(pos)
	{
		if (type == TOKEN_INT_LITERAL)
		{
			bool wasSigned = false;
			if ((value.length() > 1) && (value[1] == 'x'))
				iValue = wcstol(value.c_str() + 2, NULL, 16);
			else if ((value.length() > 1) && (value[1] == 'b'))
				iValue = wcstol(value.c_str() + 2, NULL, 2);
			else
			{
				iValue = wcstol(value.c_str(), NULL, 10);
				wasSigned = true;
			}
			if ((wasSigned == false) && (iValue > 32767))
				iValue -= 65536;
		}
		else
			iValue = 0;
		pos.column--; // column has already been incremented when token is created, so we remove one
		pos.character--; // character has already been incremented when token is created, so we remove one
	}
	
	//! Return the name of the type of this token
	const wchar_t* Compiler::Token::typeName() const
	{
		switch (type)
		{
			case TOKEN_END_OF_STREAM: return L"end of stream";
			case TOKEN_STR_when: return L"when keyword";
			case TOKEN_STR_emit: return L"emit keyword";
			case TOKEN_STR_for: return L"for keyword";
			case TOKEN_STR_in: return L"in keyword";
			case TOKEN_STR_step: return L"step keyword";
			case TOKEN_STR_while: return L"while keyword";
			case TOKEN_STR_do: return L"do keyword";
			case TOKEN_STR_if: return L"if keyword";
			case TOKEN_STR_then: return L"then keyword";
			case TOKEN_STR_else: return L"else keyword";
			case TOKEN_STR_elseif: return L"elseif keyword";
			case TOKEN_STR_end: return L"end keyword";
			case TOKEN_STR_var: return L"var keyword";
			case TOKEN_STR_call: return L"call keyword";
			case TOKEN_STR_sub: return L"sub keyword";
			case TOKEN_STR_callsub: return L"callsub keyword";
			case TOKEN_STR_onevent: return L"onevent keyword";
			case TOKEN_STR_abs: return L"abs keyword";
			case TOKEN_STRING_LITERAL: return L"string";
			case TOKEN_INT_LITERAL: return L"integer";
			case TOKEN_PAR_OPEN: return L"( (open parenthesis)";
			case TOKEN_PAR_CLOSE: return L") (close parenthesis)";
			case TOKEN_BRACKET_OPEN: return L"[ (open bracket)";
			case TOKEN_BRACKET_CLOSE: return L"] (close bracket)";
			case TOKEN_COLON: return L": (colon)";
			case TOKEN_COMMA: return L", (comma)";
			case TOKEN_ASSIGN: return L"= (assignation)";
			case TOKEN_OP_OR: return L"or";
			case TOKEN_OP_AND: return L"and";
			case TOKEN_OP_NOT: return L"not";
			case TOKEN_OP_BIT_OR: return L"binary or";
			case TOKEN_OP_BIT_XOR: return L"binary xor";
			case TOKEN_OP_BIT_AND: return L"binary and";
			case TOKEN_OP_BIT_NOT: return L"binary not";
			case TOKEN_OP_BIT_OR_EQUAL: return L"binary or equal";
			case TOKEN_OP_BIT_XOR_EQUAL: return L"binary xor equal";
			case TOKEN_OP_BIT_AND_EQUAL: return L"binary and equal";
			case TOKEN_OP_BIT_NOT_EQUAL: return L"binary not equal";
			case TOKEN_OP_EQUAL: return L"== (equal to)";
			case TOKEN_OP_NOT_EQUAL: return L"!= (not equal to)";
			case TOKEN_OP_BIGGER: return L"> (bigger than)";
			case TOKEN_OP_BIGGER_EQUAL: return L">= (bigger or equal than)";
			case TOKEN_OP_SMALLER: return L"< (smaller than)";
			case TOKEN_OP_SMALLER_EQUAL: return L"<= (smaller or equal than)";
			case TOKEN_OP_SHIFT_LEFT: return L"<< (shift left)";
			case TOKEN_OP_SHIFT_RIGHT: return L">> (shift right)";
			case TOKEN_OP_SHIFT_LEFT_EQUAL: return L"<<= (shift left equal)";
			case TOKEN_OP_SHIFT_RIGHT_EQUAL: return L">>= (shift right equal)";
			case TOKEN_OP_ADD: return L"+ (plus)";
			case TOKEN_OP_NEG: return L"- (minus)";
			case TOKEN_OP_ADD_EQUAL: return L"+= (plus equal)";
			case TOKEN_OP_NEG_EQUAL: return L"-= (minus equal)";
			case TOKEN_OP_PLUS_PLUS: return L"++ (plus) plus";
			case TOKEN_OP_MINUS_MINUS: return L"-- (minus minus)";
			case TOKEN_OP_MULT: return L"* (time)";
			case TOKEN_OP_DIV: return L"/ (divide)";
			case TOKEN_OP_MOD: return L"modulo";
			case TOKEN_OP_MULT_EQUAL: return L"*= (time equal)";
			case TOKEN_OP_DIV_EQUAL: return L"/= (divide equal)";
			case TOKEN_OP_MOD_EQUAL: return L"modulo equal";
			default: return L"unknown";
		}
	}
	
	//! Return a string representation of the token
	std::wstring Compiler::Token::toWString() const
	{
		std::wostringstream oss;
		oss << L"Line: " << pos.row + 1 << L" Col: " << pos.column + 1 << L" : ";
		oss << typeName();
		if (type == TOKEN_INT_LITERAL)
			oss << L" : " << iValue;
		if (type == TOKEN_STRING_LITERAL)
			oss << L" : " << sValue;
		return oss.str();
	}
	
	
	//! Parse source and build tokens vector
	//! \param source source code
	void Compiler::tokenize(std::wistream& source)
	{
		tokens.clear();
		SourcePos pos(0, 0, 0);
		const unsigned tabSize = 4;
		
		// tokenize text source
		while (source.good())
		{
			wchar_t c = source.get();
			
			if (source.eof())
				break;
			
			pos.column++;
			pos.character++;
			
			switch (c)
			{
				// simple cases of one character
				case ' ': break;
				//case '\t': pos.column += tabSize - 1; break;
				case '\t': break;
				case '\n': pos.row++; pos.column = -1; break; // -1 so next call to pos.column++ result set 0
				case '\r': pos.column = -1; break; // -1 so next call to pos.column++ result set 0
				case '(': tokens.push_back(Token(Token::TOKEN_PAR_OPEN, pos)); break;
				case ')': tokens.push_back(Token(Token::TOKEN_PAR_CLOSE, pos)); break;
				case '[': tokens.push_back(Token(Token::TOKEN_BRACKET_OPEN, pos)); break;
				case ']': tokens.push_back(Token(Token::TOKEN_BRACKET_CLOSE, pos)); break;
				case ':': tokens.push_back(Token(Token::TOKEN_COLON, pos)); break;
				case ',': tokens.push_back(Token(Token::TOKEN_COMMA, pos)); break;
				
				// special case for comment
				case '#':
				{
					// check if it's a comment block #* ... *#
					if (source.peek() == '*')
					{
						// comment block
						// record position of the begining
						SourcePos begin(pos);
						// move forward by 2 characters then search for the end
						int step = 2;
						while ((step > 0) || (c != '*') || (source.peek() != '#'))
						{
							if (step)
								step--;

							if (c == '\t')
								pos.column += tabSize;
							else if (c == '\n')
							{
								pos.row++;
								pos.column = 0;
							}
							else
								pos.column++;
							c = source.get();
							pos.character++;
							if (source.eof())
							{
								// EOF -> unbalanced block
								throw Error(begin, L"Unbalanced comment block.");
							}
						}
						// fetch the #
						getNextCharacter(source, pos);
					}
					else
					{
						// simple comment
						while ((c != '\n') && (c != '\r') && (!source.eof()))
						{
							if (c == '\t')
								pos.column += tabSize;
							else
								pos.column++;
							c = source.get();
							pos.character++;
						}
						if (c == '\n')
						{
							pos.row++;
							pos.column = 0;
						}
						else if (c == '\r')
							pos.column = 0;
					}
				}
				break;
				
				// cases that require one character look-ahead
				case '+':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_ADD_EQUAL))
						break;
					if (testNextCharacter(source, pos, '+', Token::TOKEN_OP_PLUS_PLUS))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_ADD, pos));
					break;

				case '-':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_NEG_EQUAL))
						break;
					if (testNextCharacter(source, pos, '-', Token::TOKEN_OP_MINUS_MINUS))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_NEG, pos));
					break;

				case '*':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_MULT_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_MULT, pos));
					break;

				case '/':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_DIV_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_DIV, pos));
					break;

				case '%':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_MOD_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_MOD, pos));
					break;

				case '|':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_BIT_OR_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_BIT_OR, pos));
					break;

				case '^':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_BIT_XOR_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_BIT_XOR, pos));
					break;

				case '&':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_BIT_AND_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_BIT_AND, pos));
					break;

				case '~':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_BIT_NOT_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_BIT_NOT, pos));
					break;

				case '!':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_NOT_EQUAL))
						break;
					throw Error(pos, L"syntax error");
					break;
				
				case '=':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_ASSIGN, pos));
					break;
				
				// cases that require two characters look-ahead
				case '<':
					if (source.peek() == '<')
					{
						// <<
						getNextCharacter(source, pos);
						if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_SHIFT_LEFT_EQUAL))
							break;
						tokens.push_back(Token(Token::TOKEN_OP_SHIFT_LEFT, pos));
						break;
					}
					// <
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_SMALLER_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_SMALLER, pos));
					break;
				
				case '>':
					if (source.peek() == '>')
					{
						// >>
						getNextCharacter(source, pos);
						if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_SHIFT_RIGHT_EQUAL))
							break;
						tokens.push_back(Token(Token::TOKEN_OP_SHIFT_RIGHT, pos));
						break;
					}
					// >
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_BIGGER_EQUAL))
						break;
					tokens.push_back(Token(Token::TOKEN_OP_BIGGER, pos));
					break;
				
				// cases that require to look for a while
				default:
				{
					// check first character
					if (!iswalnum(c) && (c != '_'))
						throw Error(pos, WFormatableString(L"identifiers must begin with _ and an alphanumeric character, found unicode character 0x%0 instead").arg((unsigned)c, 0, 16));
					
					// get a string
					std::wstring s;
					s += c;
					wchar_t nextC = source.peek();
					int posIncrement = 0;
					while ((source.good()) && (iswalnum(nextC) || (nextC == '_') || (nextC == '.')))
					{
						s += nextC;
						source.get();
						posIncrement++;
						nextC = source.peek();
					}
					
					// we now have a string, let's check what it is
					if (std::isdigit(s[0]))
					{
						// check if hex or binary
						if ((s.length() > 1) && (s[0] == '0') && (!std::isdigit(s[1])))
						{
							// check if we have a valid number
							if (s[1] == 'x')
							{
								for (unsigned i = 2; i < s.size(); i++)
									if (!std::isxdigit(s[i]))
										throw Error(pos, L"error in hexadecimal number");
							}
							else if (s[1] == 'b')
							{
								for (unsigned i = 2; i < s.size(); i++)
									if ((s[i] != '0') && (s[i] != '1'))
										throw Error(pos, L"error in binary number");
							}
							else
								throw Error(pos, L"error in number, invalid base");
							
						}
						else
						{
							// check if we have a valid number
							for (unsigned i = 1; i < s.size(); i++)
								if (!std::isdigit(s[i]))
									throw Error(pos, L"error in number");
						}
						tokens.push_back(Token(Token::TOKEN_INT_LITERAL, pos, s));
					}
					else
					{
						// check if it is a known keyword
						if (s == L"when")
							tokens.push_back(Token(Token::TOKEN_STR_when, pos));
						else if (s == L"emit")
							tokens.push_back(Token(Token::TOKEN_STR_emit, pos));
						else if (s == L"for")
							tokens.push_back(Token(Token::TOKEN_STR_for, pos));
						else if (s == L"in")
							tokens.push_back(Token(Token::TOKEN_STR_in, pos));
						else if (s == L"step")
							tokens.push_back(Token(Token::TOKEN_STR_step, pos));
						else if (s == L"while")
							tokens.push_back(Token(Token::TOKEN_STR_while, pos));
						else if (s == L"do")
							tokens.push_back(Token(Token::TOKEN_STR_do, pos));
						else if (s == L"if")
							tokens.push_back(Token(Token::TOKEN_STR_if, pos));
						else if (s == L"then")
							tokens.push_back(Token(Token::TOKEN_STR_then, pos));
						else if (s == L"else")
							tokens.push_back(Token(Token::TOKEN_STR_else, pos));
						else if (s == L"elseif")
							tokens.push_back(Token(Token::TOKEN_STR_elseif, pos));
						else if (s == L"end")
							tokens.push_back(Token(Token::TOKEN_STR_end, pos));
						else if (s == L"var")
							tokens.push_back(Token(Token::TOKEN_STR_var, pos));
						else if (s == L"call")
							tokens.push_back(Token(Token::TOKEN_STR_call, pos));
						else if (s == L"sub")
							tokens.push_back(Token(Token::TOKEN_STR_sub, pos));
						else if (s == L"callsub")
							tokens.push_back(Token(Token::TOKEN_STR_callsub, pos));
						else if (s == L"onevent")
							tokens.push_back(Token(Token::TOKEN_STR_onevent, pos));
						else if (s == L"abs")
							tokens.push_back(Token(Token::TOKEN_STR_abs, pos));
						else if (s == L"or")
							tokens.push_back(Token(Token::TOKEN_OP_OR, pos));
						else if (s == L"and")
							tokens.push_back(Token(Token::TOKEN_OP_AND, pos));
						else if (s == L"not")
							tokens.push_back(Token(Token::TOKEN_OP_NOT, pos));
						else
							tokens.push_back(Token(Token::TOKEN_STRING_LITERAL, pos, s));
					}
					
					pos.column += posIncrement;
					pos.character += posIncrement;
				}
				break;
			} // switch (c)
		} // while (source.good())
		
		tokens.push_back(Token(Token::TOKEN_END_OF_STREAM, pos));
	}

	wchar_t Compiler::getNextCharacter(std::wistream &source, SourcePos &pos)
	{
		pos.column++;
		pos.character++;
		return source.get();
	}

	bool Compiler::testNextCharacter(std::wistream &source, SourcePos &pos, wchar_t test, Token::Type tokenIfTrue)
	{
		if (source.peek() == test)
		{
			tokens.push_back(Token(tokenIfTrue, pos));
			getNextCharacter(source, pos);
			return true;
		}
		return false;
	}
	
	//! Debug print of tokens
	void Compiler::dumpTokens(std::wostream &dest) const
	{
		for (unsigned i = 0; i < tokens.size(); i++)
			dest << tokens[i].toWString() << std::endl;
	}
}; // Aseba
