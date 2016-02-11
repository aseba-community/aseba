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

#include "errors_code.h"
#include "compiler.h"
#include <sstream>
#include <string>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	static const wchar_t* error_map[ERROR_END];

	ErrorMessages::ErrorMessages()
	{
		// compiler.cpp
		error_map[ERROR_BROKEN_TARGET] =			L"Broken target description: not enough room for internal variables";
		error_map[ERROR_STACK_OVERFLOW] =			L"Execution stack will overflow, check for any recursive subroutine call and cut long mathematical expressions";
		error_map[ERROR_SCRIPT_TOO_BIG] =			L"Script too big for target bytecode size";
		// identifier-lookup.cpp
		error_map[ERROR_VARIABLE_NOT_DEFINED] =			L"%0 is not a defined variable";
		error_map[ERROR_VARIABLE_NOT_DEFINED_GUESS] =		L"%0 is not a defined variable, do you mean %1?";
		error_map[ERROR_FUNCTION_NOT_DEFINED] =			L"Target does not provide function %0";
		error_map[ERROR_FUNCTION_NOT_DEFINED_GUESS] =		L"Target does not provide function %0, do you mean %1?";
		error_map[ERROR_CONSTANT_NOT_DEFINED] =			L"Constant %0 not defined";
		error_map[ERROR_CONSTANT_NOT_DEFINED_GUESS] =		L"Constant %0 not defined, do you mean %1?";
		error_map[ERROR_EVENT_NOT_DEFINED] =			L"%0 is not a known event";
		error_map[ERROR_EVENT_NOT_DEFINED_GUESS] =		L"%0 is not a known event, do you mean %1?";
		error_map[ERROR_EMIT_LOCAL_EVENT] =			L"%0 is a local event that you cannot emit";
		error_map[ERROR_SUBROUTINE_NOT_DEFINED] =		L"Subroutine %0 does not exists";
		error_map[ERROR_SUBROUTINE_NOT_DEFINED_GUESS] =		L"Subroutine %0 does not exists, do you mean %1?";
		// lexer.cpp
		error_map[ERROR_LINE] =					L"Line: ";
		error_map[ERROR_COL] =					L" Col: ";
		error_map[ERROR_UNBALANCED_COMMENT_BLOCK] =		L"Unbalanced comment block";
		error_map[ERROR_SYNTAX] =				L"Syntax error";
		error_map[ERROR_INVALID_IDENTIFIER] =			L"Identifiers must begin with _ or an alphanumeric character, found unicode character 0x%0 instead";
		error_map[ERROR_INVALID_HEXA_NUMBER] =			L"Error in hexadecimal number";
		error_map[ERROR_INVALID_BINARY_NUMBER] =		L"Error in binary number";
		error_map[ERROR_NUMBER_INVALID_BASE] =			L"Error in number, invalid base";
		error_map[ERROR_IN_NUMBER] =				L"Error in number";
		// parser.cpp
		error_map[ERROR_INTERNAL] =				L"Internal compiler error, please report a bug containing the source which triggered this error";
		error_map[ERROR_EXPECTING] =				L"Expecting %0, found %1 instead";
		error_map[ERROR_UINT12_OUT_OF_RANGE] =			L"Integer value %0 out of [0;4095] range";
		error_map[ERROR_UINT16_OUT_OF_RANGE] =			L"Integer value %0 out of [0;65535] range";
		error_map[ERROR_PINT16_OUT_OF_RANGE] =			L"Integer value %0 out of [0;32767] range";
		error_map[ERROR_INT16_OUT_OF_RANGE] =			L"Integer value %0 out of [-32768;32767] range";
		error_map[ERROR_PCONSTANT_OUT_OF_RANGE] =		L"Constant %0 has value %1, which is out of [0;32767] range";
		error_map[ERROR_CONSTANT_OUT_OF_RANGE] =		L"Constant %0 has value %1, which is out of [-32768;32767] range";
		error_map[ERROR_EXPECTING_ONE_OF] =			L"Expecting one of %0; but found %1 instead";
		error_map[ERROR_NOT_ENOUGH_TEMP_SPACE] =		L"Not enough free space to allocate this tempory variable";
		error_map[ERROR_MISPLACED_VARDEF] =			L"Variable definition is allowed only at the beginning of the program before any statement";
		error_map[ERROR_EXPECTING_IDENTIFIER] =			L"Expecting identifier, found %0";
		error_map[ERROR_CONST_ALREADY_DEFINED] =		L"Constant %0 is already defined";
		error_map[ERROR_VAR_ALREADY_DEFINED] =			L"Variable %0 is already defined";
		error_map[ERROR_VAR_CONST_COLLISION] =			L"Variable %0 has the same name as a constant";
		error_map[ERROR_UNDEFINED_SIZE] =			L"Array %0 has undefined size";
		error_map[ERROR_NOT_ENOUGH_SPACE] =			L"No more free variable space";
		error_map[ERROR_EXPECTING_ASSIGNMENT] =			L"Expecting assignment, found %0 instead";
		error_map[ERROR_FOR_NULL_STEPS] =			L"Null steps are not allowed in for loops";
		error_map[ERROR_FOR_START_HIGHER_THAN_END] =		L"Start index must be lower than end index in increasing loops";
		error_map[ERROR_FOR_START_LOWER_THAN_END] =		L"Start index must be higher than end index in decreasing loops";
		error_map[ERROR_FOR_INVALID_INC_END_INDEX] = 	L"End index cannot be 32767 in increasing loops";
		error_map[ERROR_FOR_INVALID_DEC_END_INDEX] = 	L"End index cannot be -32768 in decreasing loops";
		error_map[ERROR_EVENT_ALREADY_IMPL] =			L"Event %0 is already implemented";
		error_map[ERROR_EVENT_ARG_TOO_BIG] = 			L"Event %0 needs an array of size %1 or smaller, but one of size %2 is passed";
		error_map[ERROR_EVENT_WRONG_ARG_SIZE] =			L"Event %0 needs an array of size %1, but one of size %2 is passed";
		error_map[ERROR_SUBROUTINE_ALREADY_DEF] =		L"Subroutine %0 is already defined";
		error_map[ERROR_INDEX_EXPECTING_CONSTANT] =		L"Expecting a constant expression as a second index";
		error_map[ERROR_INDEX_WRONG_END] =			L"End of range index must be lower or equal to start of range index";
		error_map[ERROR_SIZE_IS_NEGATIVE] =			L"Array size: result is negative (%0)";
		error_map[ERROR_SIZE_IS_NULL] =				L"Array size: result is null";
		error_map[ERROR_NOT_CONST_EXPR] =			L"Not a valid constant expression";
		error_map[ERROR_FUNCTION_HAS_NO_ARG] =			L"Function %0 requires no argument, some are used";
		error_map[ERROR_FUNCTION_NO_ENOUGH_ARG] =		L"Function %0 requires %1 arguments, only %2 are provided";
		error_map[ERROR_FUNCTION_WRONG_ARG_SIZE] =		L"Argument %0 (%1) of function %2 is of size %3, function definition demands size %4";
		error_map[ERROR_FUNCTION_WRONG_ARG_SIZE_TEMPLATE] =	L"Argument %0 (%1) of function %2 is of size %3, while a previous instance of the template parameter was of size %4";
		error_map[ERROR_FUNCTION_TOO_MANY_ARG] =		L"Function %0 requires %1 arguments, more are used";
		// tree-build.cpp
		error_map[ERROR_UNARY_ARITH_BUILD_UNEXPECTED] =		L"Unexpected token when building UnaryArithmeticAssignmentNode";
		// tree-expand.cpp
		error_map[ERROR_INCORRECT_LEFT_VALUE] =			L"Expecting an assignment to a variable, found %0 instead";
		error_map[ERROR_ARRAY_OUT_OF_BOUND] =			L"Access of array %0 out of bounds: accessing index %1 while array is of size %2";
		error_map[ERROR_ARRAY_SIZE_MISMATCH] =			L"Size error! Size of array1 = %0 ; size of array2 = %1";
		error_map[ERROR_IF_VECTOR_CONDITION] =			L"Condition of the if cannot be a vector";
		error_map[ERROR_WHILE_VECTOR_CONDITION] =		L"Condition of the while cannot be a vector";
		error_map[ERROR_ARRAY_ILLEGAL_ACCESS] =			L"MemoryVectorNode::getVectorSize: illegal operation";
		// tree-optimize.cpp
		error_map[ERROR_INFINITE_LOOP] =			L"Infinite loops not allowed";
		error_map[ERROR_DIVISION_BY_ZERO] =			L"Division by zero";
		error_map[ERROR_ABS_NOT_POSSIBLE] =			L"-32768 has no positive correspondance in 16 bits integers";
		error_map[ERROR_ARRAY_OUT_OF_BOUND_READ] =		L"Out of bound static array access. Trying to read index %0 of array %1 of size %2";
		error_map[ERROR_ARRAY_OUT_OF_BOUND_WRITE] =		L"Out of bound static array access. Trying to write index %0 of array %1 of size %2";
		// tree-typecheck.cpp
		error_map[ERROR_EXPECTING_TYPE] =			L"Expecting %0 type, found %1 type instead";
		error_map[ERROR_EXPECTING_CONDITION] =			L"Expecting a condition, found a %0 instead";

		// lexer.cpp Token::typeName()
		error_map[ERROR_TOKEN_END_OF_STREAM] =			L"end of stream";
		error_map[ERROR_TOKEN_STR_when] =			L"when keyword";
		error_map[ERROR_TOKEN_STR_emit] =			L"emit keyword";
		error_map[ERROR_TOKEN_STR_hidden_emit] =	L"_emit keyword";
		error_map[ERROR_TOKEN_STR_for] =			L"for keyword";
		error_map[ERROR_TOKEN_STR_in] =				L"in keyword";
		error_map[ERROR_TOKEN_STR_step] =			L"step keyword";
		error_map[ERROR_TOKEN_STR_while] =			L"while keyword";
		error_map[ERROR_TOKEN_STR_do] =				L"do keyword";
		error_map[ERROR_TOKEN_STR_if] =				L"if keyword";
		error_map[ERROR_TOKEN_STR_then] =			L"then keyword";
		error_map[ERROR_TOKEN_STR_else] =			L"else keyword";
		error_map[ERROR_TOKEN_STR_elseif] =			L"elseif keyword";
		error_map[ERROR_TOKEN_STR_end] =			L"end keyword";
		error_map[ERROR_TOKEN_STR_var] =			L"var keyword";
		error_map[ERROR_TOKEN_STR_const] =			L"const keyword";
		error_map[ERROR_TOKEN_STR_call] =			L"call keyword";
		error_map[ERROR_TOKEN_STR_sub] =			L"sub keyword";
		error_map[ERROR_TOKEN_STR_callsub] =			L"callsub keyword";
		error_map[ERROR_TOKEN_STR_onevent] =			L"onevent keyword";
		error_map[ERROR_TOKEN_STR_abs] =			L"abs keyword";
		error_map[ERROR_TOKEN_STR_return] =			L"return keyword";
		error_map[ERROR_TOKEN_STRING_LITERAL] =			L"string";
		error_map[ERROR_TOKEN_INT_LITERAL] =			L"integer";
		error_map[ERROR_TOKEN_PAR_OPEN] =			L"( (open parenthesis)";
		error_map[ERROR_TOKEN_PAR_CLOSE] =			L") (close parenthesis)";
		error_map[ERROR_TOKEN_BRACKET_OPEN] =			L"[ (open bracket)";
		error_map[ERROR_TOKEN_BRACKET_CLOSE] =			L"] (close bracket)";
		error_map[ERROR_TOKEN_COLON] =				L": (colon)";
		error_map[ERROR_TOKEN_COMMA] =				L", (comma)";
		error_map[ERROR_TOKEN_ASSIGN] =				L"= (assignation)";
		error_map[ERROR_TOKEN_OP_OR] =				L"or";
		error_map[ERROR_TOKEN_OP_AND] =				L"and";
		error_map[ERROR_TOKEN_OP_NOT] =				L"not";
		error_map[ERROR_TOKEN_OP_BIT_OR] =			L"binary or";
		error_map[ERROR_TOKEN_OP_BIT_XOR] =			L"binary xor";
		error_map[ERROR_TOKEN_OP_BIT_AND] =			L"binary and";
		error_map[ERROR_TOKEN_OP_BIT_NOT] =			L"binary not";
		error_map[ERROR_TOKEN_OP_BIT_OR_EQUAL] =		L"binary or equal";
		error_map[ERROR_TOKEN_OP_BIT_XOR_EQUAL] =		L"binary xor equal";
		error_map[ERROR_TOKEN_OP_BIT_AND_EQUAL] =		L"binary and equal";
		error_map[ERROR_TOKEN_OP_EQUAL] =			L"== (equal to)";
		error_map[ERROR_TOKEN_OP_NOT_EQUAL] =			L"!= (not equal to)";
		error_map[ERROR_TOKEN_OP_BIGGER] =			L"> (bigger than)";
		error_map[ERROR_TOKEN_OP_BIGGER_EQUAL] =		L">= (bigger or equal than)";
		error_map[ERROR_TOKEN_OP_SMALLER] =			L"< (smaller than)";
		error_map[ERROR_TOKEN_OP_SMALLER_EQUAL] =		L"<= (smaller or equal than)";
		error_map[ERROR_TOKEN_OP_SHIFT_LEFT] =			L"<< (shift left)";
		error_map[ERROR_TOKEN_OP_SHIFT_RIGHT] =			L">> (shift right)";
		error_map[ERROR_TOKEN_OP_SHIFT_LEFT_EQUAL] =		L"<<= (shift left equal)";
		error_map[ERROR_TOKEN_OP_SHIFT_RIGHT_EQUAL] =		L">>= (shift right equal)";
		error_map[ERROR_TOKEN_OP_ADD] =				L"+ (plus)";
		error_map[ERROR_TOKEN_OP_NEG] =				L"- (minus)";
		error_map[ERROR_TOKEN_OP_ADD_EQUAL] =			L"+= (plus equal)";
		error_map[ERROR_TOKEN_OP_NEG_EQUAL] =			L"-= (minus equal)";
		error_map[ERROR_TOKEN_OP_PLUS_PLUS] =			L"++ (plus plus)";
		error_map[ERROR_TOKEN_OP_MINUS_MINUS] =			L"-- (minus minus)";
		error_map[ERROR_TOKEN_OP_MULT] =			L"* (time)";
		error_map[ERROR_TOKEN_OP_DIV] =				L"/ (divide)";
		error_map[ERROR_TOKEN_OP_MOD] =				L"modulo";
		error_map[ERROR_TOKEN_OP_MULT_EQUAL] =			L"*= (time equal)";
		error_map[ERROR_TOKEN_OP_DIV_EQUAL] =			L"/= (divide equal)";
		error_map[ERROR_TOKEN_OP_MOD_EQUAL] =			L"modulo equal";
		error_map[ERROR_TOKEN_UNKNOWN] =			L"unknown";

		error_map[ERROR_UNKNOWN_ERROR] =			L"Unknown error";
	}

	const std::wstring ErrorMessages::defaultCallback(ErrorCode error)
	{
		if (error >= ERROR_END)
			return std::wstring(error_map[ERROR_UNKNOWN_ERROR]);
		else
			return std::wstring(error_map[error]);
	}
	
	//! Return the string version of this error
	std::wstring Error::toWString() const
	{
		std::wostringstream oss;
		if (pos.valid)
			oss << "Error at Line: " << pos.row + 1 << " Col: " << pos.column + 1 << " : " << message;
		else
			oss << "Error : " << message;
		return oss.str();
	}

	ErrorMessages::ErrorCallback TranslatableError::translateCB = NULL;

	TranslatableError::TranslatableError(const SourcePos& pos, ErrorCode error)
	{
		this->pos = pos;
		message = translateCB(error);
	}

	Error TranslatableError::toError(void)
	{
		return Error(pos, message);
	}

	void TranslatableError::setTranslateCB(ErrorMessages::ErrorCallback newCB)
	{
		translateCB = newCB;
	}

	TranslatableError &TranslatableError::arg(int value, int fieldWidth, int base, wchar_t fillChar)
	{
		message.arg(value, fieldWidth, base, fillChar);
		return *this;
	}

	TranslatableError &TranslatableError::arg(long int value, int fieldWidth, int base, wchar_t fillChar)
	{
		message.arg(value, fieldWidth, base, fillChar);
		return *this;
	}

	TranslatableError &TranslatableError::arg(unsigned value, int fieldWidth, int base, wchar_t fillChar)
	{
		message.arg(value, fieldWidth, base, fillChar);
		return *this;
	}

	TranslatableError &TranslatableError::arg(float value, int fieldWidth, int precision, wchar_t fillChar )
	{
		message.arg(value, fieldWidth, precision, fillChar);
		return *this;
	}

	TranslatableError &TranslatableError::arg(const std::wstring& value)
	{
		message.arg(value);
		return *this;
	}

	/*@}*/
	
} // namespace Aseba
