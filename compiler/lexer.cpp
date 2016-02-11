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

#include "compiler.h"
#include "../common/utils/FormatableString.h"
#include <cstdlib>
#include <sstream>
#include <ostream>
#include <cctype>
#include <cstdio>

namespace Aseba
{
	#ifdef ANDROID
	long int wcstol_fix( const wchar_t * str, wchar_t ** endptr, int base )
	{
		long int v(0);
		while (*str)
		{
			v *= base;
			if (str[0] <= '9')
			{
				v += (str[0] - '0');
			}
			else
			{
				const long int maskedVal(str[0] & (~(1<<5)));
				v += (maskedVal - 'A') + 10;
			}
			++str;
		}
		return v;
	}
	#define wcstol wcstol_fix
	#endif // ANDROID
	
	//! Construct a new token of given type and value
	Compiler::Token::Token(Type type, SourcePos pos, const std::wstring& value) :
		type(type),
		sValue(value),
		pos(pos)
	{
		if (type == TOKEN_INT_LITERAL)
		{
			long int decode;
			bool wasUnsigned = false;
			// all values are assumed to be signed 16-bits
			if ((value.length() > 1) && (value[1] == 'x')) {
				decode = wcstol(value.c_str() + 2, NULL, 16);
				wasUnsigned = true;
			}
			else if ((value.length() > 1) && (value[1] == 'b')) {
				decode = wcstol(value.c_str() + 2, NULL, 2);
				wasUnsigned = true;
			}
			else
				decode = wcstol(value.c_str(), NULL, 10);
			if (decode >= 65536)
				throw TranslatableError(pos, ERROR_INT16_OUT_OF_RANGE).arg(decode);
			if (wasUnsigned && decode > 32767)
				decode -= 65536;
			iValue = decode;
		}
		else
			iValue = 0;
		pos.column--; // column has already been incremented when token is created, so we remove one
		pos.character--; // character has already been incremented when token is created, so we remove one
	}
	
	//! Return the name of the type of this token
	const std::wstring Compiler::Token::typeName() const
	{
		switch (type)
		{
			case TOKEN_END_OF_STREAM: return translate(ERROR_TOKEN_END_OF_STREAM);
			case TOKEN_STR_when: return translate(ERROR_TOKEN_STR_when);
			case TOKEN_STR_emit: return translate(ERROR_TOKEN_STR_emit);
			case TOKEN_STR_hidden_emit: return translate(ERROR_TOKEN_STR_hidden_emit);
			case TOKEN_STR_for: return translate(ERROR_TOKEN_STR_for);
			case TOKEN_STR_in: return translate(ERROR_TOKEN_STR_in);
			case TOKEN_STR_step: return translate(ERROR_TOKEN_STR_step);
			case TOKEN_STR_while: return translate(ERROR_TOKEN_STR_while);
			case TOKEN_STR_do: return translate(ERROR_TOKEN_STR_do);
			case TOKEN_STR_if: return translate(ERROR_TOKEN_STR_if);
			case TOKEN_STR_then: return translate(ERROR_TOKEN_STR_then);
			case TOKEN_STR_else: return translate(ERROR_TOKEN_STR_else);
			case TOKEN_STR_elseif: return translate(ERROR_TOKEN_STR_elseif);
			case TOKEN_STR_end: return translate(ERROR_TOKEN_STR_end);
			case TOKEN_STR_var: return translate(ERROR_TOKEN_STR_var);
			case TOKEN_STR_const: return translate(ERROR_TOKEN_STR_const);
			case TOKEN_STR_call: return translate(ERROR_TOKEN_STR_call);
			case TOKEN_STR_sub: return translate(ERROR_TOKEN_STR_sub);
			case TOKEN_STR_callsub: return translate(ERROR_TOKEN_STR_callsub);
			case TOKEN_STR_onevent: return translate(ERROR_TOKEN_STR_onevent);
			case TOKEN_STR_abs: return translate(ERROR_TOKEN_STR_abs);
			case TOKEN_STR_return: return translate(ERROR_TOKEN_STR_return);
			case TOKEN_STRING_LITERAL: return translate(ERROR_TOKEN_STRING_LITERAL);
			case TOKEN_INT_LITERAL: return translate(ERROR_TOKEN_INT_LITERAL);
			case TOKEN_PAR_OPEN: return translate(ERROR_TOKEN_PAR_OPEN);
			case TOKEN_PAR_CLOSE: return translate(ERROR_TOKEN_PAR_CLOSE);
			case TOKEN_BRACKET_OPEN: return translate(ERROR_TOKEN_BRACKET_OPEN);
			case TOKEN_BRACKET_CLOSE: return translate(ERROR_TOKEN_BRACKET_CLOSE);
			case TOKEN_COLON: return translate(ERROR_TOKEN_COLON);
			case TOKEN_COMMA: return translate(ERROR_TOKEN_COMMA);
			case TOKEN_ASSIGN: return translate(ERROR_TOKEN_ASSIGN);
			case TOKEN_OP_OR: return translate(ERROR_TOKEN_OP_OR);
			case TOKEN_OP_AND: return translate(ERROR_TOKEN_OP_AND);
			case TOKEN_OP_NOT: return translate(ERROR_TOKEN_OP_NOT);
			case TOKEN_OP_BIT_OR: return translate(ERROR_TOKEN_OP_BIT_OR);
			case TOKEN_OP_BIT_XOR: return translate(ERROR_TOKEN_OP_BIT_XOR);
			case TOKEN_OP_BIT_AND: return translate(ERROR_TOKEN_OP_BIT_AND);
			case TOKEN_OP_BIT_NOT: return translate(ERROR_TOKEN_OP_BIT_NOT);
			case TOKEN_OP_BIT_OR_EQUAL: return translate(ERROR_TOKEN_OP_BIT_OR_EQUAL);
			case TOKEN_OP_BIT_XOR_EQUAL: return translate(ERROR_TOKEN_OP_BIT_XOR_EQUAL);
			case TOKEN_OP_BIT_AND_EQUAL: return translate(ERROR_TOKEN_OP_BIT_AND_EQUAL);
			case TOKEN_OP_EQUAL: return translate(ERROR_TOKEN_OP_EQUAL);
			case TOKEN_OP_NOT_EQUAL: return translate(ERROR_TOKEN_OP_NOT_EQUAL);
			case TOKEN_OP_BIGGER: return translate(ERROR_TOKEN_OP_BIGGER);
			case TOKEN_OP_BIGGER_EQUAL: return translate(ERROR_TOKEN_OP_BIGGER_EQUAL);
			case TOKEN_OP_SMALLER: return translate(ERROR_TOKEN_OP_SMALLER);
			case TOKEN_OP_SMALLER_EQUAL: return translate(ERROR_TOKEN_OP_SMALLER_EQUAL);
			case TOKEN_OP_SHIFT_LEFT: return translate(ERROR_TOKEN_OP_SHIFT_LEFT);
			case TOKEN_OP_SHIFT_RIGHT: return translate(ERROR_TOKEN_OP_SHIFT_RIGHT);
			case TOKEN_OP_SHIFT_LEFT_EQUAL: return translate(ERROR_TOKEN_OP_SHIFT_LEFT_EQUAL);
			case TOKEN_OP_SHIFT_RIGHT_EQUAL: return translate(ERROR_TOKEN_OP_SHIFT_RIGHT_EQUAL);
			case TOKEN_OP_ADD: return translate(ERROR_TOKEN_OP_ADD);
			case TOKEN_OP_NEG: return translate(ERROR_TOKEN_OP_NEG);
			case TOKEN_OP_ADD_EQUAL: return translate(ERROR_TOKEN_OP_ADD_EQUAL);
			case TOKEN_OP_NEG_EQUAL: return translate(ERROR_TOKEN_OP_NEG_EQUAL);
			case TOKEN_OP_PLUS_PLUS: return translate(ERROR_TOKEN_OP_PLUS_PLUS);
			case TOKEN_OP_MINUS_MINUS: return translate(ERROR_TOKEN_OP_MINUS_MINUS);
			case TOKEN_OP_MULT: return translate(ERROR_TOKEN_OP_MULT);
			case TOKEN_OP_DIV: return translate(ERROR_TOKEN_OP_DIV);
			case TOKEN_OP_MOD: return translate(ERROR_TOKEN_OP_MOD);
			case TOKEN_OP_MULT_EQUAL: return translate(ERROR_TOKEN_OP_MULT_EQUAL);
			case TOKEN_OP_DIV_EQUAL: return translate(ERROR_TOKEN_OP_DIV_EQUAL);
			case TOKEN_OP_MOD_EQUAL: return translate(ERROR_TOKEN_OP_MOD_EQUAL);
			default: return translate(ERROR_TOKEN_UNKNOWN);
		}
	}
	
	//! Return a string representation of the token
	std::wstring Compiler::Token::toWString() const
	{
		std::wostringstream oss;
		oss << translate(ERROR_LINE) << pos.row + 1 << translate(ERROR_COL) << pos.column + 1 << L" : ";
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
								throw TranslatableError(begin, ERROR_UNBALANCED_COMMENT_BLOCK);
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
					tokens.push_back(Token(Token::TOKEN_OP_BIT_NOT, pos));
					break;

				case '!':
					if (testNextCharacter(source, pos, '=', Token::TOKEN_OP_NOT_EQUAL))
						break;
					throw TranslatableError(pos, ERROR_SYNTAX);
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
					if (!std::iswalnum(c) && (c != '_'))
						throw TranslatableError(pos, ERROR_INVALID_IDENTIFIER).arg((unsigned)c, 0, 16);
					
					// get a string
					std::wstring s;
					s += c;
					wchar_t nextC = source.peek();
					int posIncrement = 0;
					while ((source.good()) && (std::iswalnum(nextC) || (nextC == '_') || (nextC == '.')))
					{
						s += nextC;
						source.get();
						posIncrement++;
						nextC = source.peek();
					}
					
					// we now have a string, let's check what it is
					if (std::iswdigit(s[0]))
					{
						// check if hex or binary
						if ((s.length() > 1) && (s[0] == '0') && (!std::iswdigit(s[1])))
						{
							// check if we have a valid number
							if (s[1] == 'x')
							{
								for (unsigned i = 2; i < s.size(); i++)
									if (!std::iswxdigit(s[i]))
										throw TranslatableError(pos, ERROR_INVALID_HEXA_NUMBER);
							}
							else if (s[1] == 'b')
							{
								for (unsigned i = 2; i < s.size(); i++)
									if ((s[i] != '0') && (s[i] != '1'))
										throw TranslatableError(pos, ERROR_INVALID_BINARY_NUMBER);
							}
							else
								throw TranslatableError(pos, ERROR_NUMBER_INVALID_BASE);
							
						}
						else
						{
							// check if we have a valid number
							for (unsigned i = 1; i < s.size(); i++)
								if (!std::iswdigit(s[i]))
									throw TranslatableError(pos, ERROR_IN_NUMBER);
						}
						tokens.push_back(Token(Token::TOKEN_INT_LITERAL, pos, s));
					}
					else
					{
						// check if it is a known keyword
						// FIXME: clean-up that with a table
						if (s == L"when")
							tokens.push_back(Token(Token::TOKEN_STR_when, pos));
						else if (s == L"emit")
							tokens.push_back(Token(Token::TOKEN_STR_emit, pos));
						else if (s == L"_emit")
							tokens.push_back(Token(Token::TOKEN_STR_hidden_emit, pos));
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
						else if (s == L"const")
							tokens.push_back(Token(Token::TOKEN_STR_const, pos));
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
						else if (s == L"return")
							tokens.push_back(Token(Token::TOKEN_STR_return, pos));
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
		if ((int)source.peek() == int(test))
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
	
	//! Return whether a string is a language keyword
	bool Compiler::isKeyword(const std::wstring& s)
	{
		// FIXME: clean-up that with a table
		if (s == L"when")
			return true;
		else if (s == L"emit")
			return true;
		else if (s == L"_emit")
			return true;
		else if (s == L"for")
			return true;
		else if (s == L"in")
			return true;
		else if (s == L"step")
			return true;
		else if (s == L"while")
			return true;
		else if (s == L"do")
			return true;
		else if (s == L"if")
			return true;
		else if (s == L"then")
			return true;
		else if (s == L"else")
			return true;
		else if (s == L"elseif")
			return true;
		else if (s == L"end")
			return true;
		else if (s == L"var")
			return true;
		else if (s == L"const")
			return true;
		else if (s == L"call")
			return true;
		else if (s == L"sub")
			return true;
		else if (s == L"callsub")
			return true;
		else if (s == L"onevent")
			return true;
		else if (s == L"abs")
			return true;
		else if (s == L"return")
			return true;
		else if (s == L"or")
			return true;
		else if (s == L"and")
			return true;
		else if (s == L"not")
			return true;
		else
			return false;
	}
} // namespace Aseba
