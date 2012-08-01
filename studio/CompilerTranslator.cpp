
#include "CompilerTranslator.h"
#include "../compiler/errors_code.h"

#include <CompilerTranslator.moc>

namespace Aseba
{
	CompilerTranslator::CompilerTranslator()
	{

	}

	const std::wstring CompilerTranslator::translate(ErrorCode error)
	{
		QString msg;

		switch (error)
		{

			case ERROR_BROKEN_TARGET:
				msg = tr("Broken target description: not enough room for internal variables");
				break;

			case ERROR_STACK_OVERFLOW:
				msg = tr("Execution stack will overflow, check for any recursive subroutine call and cut long mathematical expressions");
				break;

			case ERROR_SCRIPT_TOO_BIG:
				msg = tr("Script too big for target bytecode size");
				break;

			case ERROR_VARIABLE_NOT_DEFINED:
				msg = tr("%0 is not a defined variable");
				break;

			case ERROR_VARIABLE_NOT_DEFINED_GUESS:
				msg = tr("%0 is not a defined variable, do you mean %1?");
				break;

			case ERROR_FUNCTION_NOT_DEFINED:
				msg = tr("Target does not provide function %0");
				break;

			case ERROR_FUNCTION_NOT_DEFINED_GUESS:
				msg = tr("Target does not provide function %0, do you mean %1?");
				break;

			case ERROR_CONSTANT_NOT_DEFINED:
				msg = tr("Constant %0 not defined");
				break;

			case ERROR_CONSTANT_NOT_DEFINED_GUESS:
				msg = tr("Constant %0 not defined, do you mean %1?");
				break;

			case ERROR_EVENT_NOT_DEFINED:
				msg = tr("%0 is not a known event");
				break;

			case ERROR_EVENT_NOT_DEFINED_GUESS:
				msg = tr("%0 is not a known event, do you mean %1?");
				break;

			case ERROR_EMIT_LOCAL_EVENT:
				msg = tr("%0 is a local event that you cannot emit");
				break;

			case ERROR_SUBROUTINE_NOT_DEFINED:
				msg = tr("Subroutine %0 does not exists");
				break;

			case ERROR_SUBROUTINE_NOT_DEFINED_GUESS:
				msg = tr("Subroutine %0 does not exists, do you mean %1?");
				break;

			case ERROR_LINE:
				msg = tr("Line: ");
				break;

			case ERROR_COL:
				msg = tr(" Col: ");
				break;

			case ERROR_UNBALANCED_COMMENT_BLOCK:
				msg = tr("Unbalanced comment block.");
				break;

			case ERROR_SYNTAX:
				msg = tr("syntax error");
				break;

			case ERROR_INVALID_IDENTIFIER:
				msg = tr("identifiers must begin with _ or an alphanumeric character, found unicode character 0x%0 instead");
				break;

			case ERROR_INVALID_HEXA_NUMBER:
				msg = tr("error in hexadecimal number");
				break;

			case ERROR_INVALID_BINARY_NUMBER:
				msg = tr("error in binary number");
				break;

			case ERROR_NUMBER_INVALID_BASE:
				msg = tr("error in number, invalid base");
				break;

			case ERROR_IN_NUMBER:
				msg = tr("error in number");
				break;

			case ERROR_INTERNAL:
				msg = tr("Internal compiler error, please report a bug containing the source which triggered this error");
				break;

			case ERROR_EXPECTING:
				msg = tr("Expecting %0, found %1 instead");
				break;

			case ERROR_UINT12_OUT_OF_RANGE:
				msg = tr("Integer value %0 out of [0;4095] range");
				break;

			case ERROR_UINT16_OUT_OF_RANGE:
				msg = tr("Integer value %0 out of [0;65535] range");
				break;

			case ERROR_PINT16_OUT_OF_RANGE:
				msg = tr("Integer value %0 out of [0;32767] range");
				break;

			case ERROR_INT16_OUT_OF_RANGE:
				msg = tr("Integer value %0 out of [-32768;32767] range");
				break;

			case ERROR_PCONSTANT_OUT_OF_RANGE:
				msg = tr("Constant %0 has value %1, which is out of [0;32767] range");
				break;

			case ERROR_CONSTANT_OUT_OF_RANGE:
				msg = tr("Constant %0 has value %1, which is out of [-32768;32767] range");
				break;

			case ERROR_EXPECTING_ONE_OF:
				msg = tr("Expecting one of %0; but found %1 instead");
				break;

			case ERROR_NOT_ENOUGH_TEMP_SPACE:
				msg = tr("Not enough free space to allocate this tempory variable");
				break;

			case ERROR_MISPLACED_VARDEF:
				msg = tr("Variable definition is allowed only at the beginning of the program before any statement");
				break;

			case ERROR_EXPECTING_IDENTIFIER:
				msg = tr("Expecting identifier, found %0");
				break;

			case ERROR_VAR_ALREADY_DEFINED:
				msg = tr("Variable %0 is already defined");
				break;

			case ERROR_VAR_CONST_COLLISION:
				msg = tr("Variable %0 has the same name as a constant");
				break;

			case ERROR_UNDEFINED_SIZE:
				msg = tr("Array %0 has undefined size");
				break;

			case ERROR_NOT_ENOUGH_SPACE:
				msg = tr("No more free variable space");
				break;

			case ERROR_EXPECTING_ASSIGNMENT:
				msg = tr("Expecting assignment, found %0 instead");
				break;

			case ERROR_FOR_NULL_STEPS:
				msg = tr("Null steps are not allowed in for loops");
				break;

			case ERROR_FOR_START_HIGHER_THAN_END:
				msg = tr("Start index must be lower than end index in increasing loops");
				break;

			case ERROR_FOR_START_LOWER_THAN_END:
				msg = tr("Start index must be higher than end index in decreasing loops");
				break;

			case ERROR_EVENT_ALREADY_IMPL:
				msg = tr("Event %0 is already implemented");
				break;

			case ERROR_EVENT_WRONG_ARG_SIZE:
				msg = tr("Event %0 needs an array of size %1, but one of size %2 is passed");
				break;

			case ERROR_SUBROUTINE_ALREADY_DEF:
				msg = tr("Subroutine %0 is already defined");
				break;

			case ERROR_INDEX_EXPECTING_CONSTANT:
				msg = tr("Expecting a constant expression as a second index");
				break;

			case ERROR_INDEX_WRONG_END:
				msg = tr("end of range index must be lower or equal to start of range index");
				break;

			case ERROR_SIZE_IS_NEGATIVE:
				msg = tr("Array size: result is negative (%0)");
				break;

			case ERROR_SIZE_IS_NULL:
				msg = tr("Array size: result is null");
				break;

			case ERROR_NOT_CONST_EXPR:
				msg = tr("Not a valid constant expression");
				break;

			case ERROR_FUNCTION_HAS_NO_ARG:
				msg = tr("Function %0 requires no argument, some are used");
				break;

			case ERROR_FUNCTION_NO_ENOUGH_ARG:
				msg = tr("Function %0 requires %1 arguments, only %2 are provided");
				break;

			case ERROR_FUNCTION_WRONG_ARG_SIZE:
				msg = tr("Argument %0 (%1) of function %2 is of size %3, function definition demands size %4");
				break;

			case ERROR_FUNCTION_WRONG_ARG_SIZE_TEMPLATE:
				msg = tr("Argument %0 (%1) of function %2 is of size %3, while a previous instance of the template parameter was of size %4");
				break;

			case ERROR_FUNCTION_TOO_MANY_ARG:
				msg = tr("Function %0 requires %1 arguments, more are used");
				break;

			case ERROR_UNARY_ARITH_BUILD_UNEXPECTED:
				msg = tr("Unexpected token when building UnaryArithmeticAssignmentNode");
				break;

			case ERROR_ARRAY_OUT_OF_BOUND:
				msg = tr("Access of array %0 out of bounds: accessing index %1 while array is of size %2");
				break;

			case ERROR_ARRAY_SIZE_MISMATCH:
				msg = tr("Size error! Size of array1 = %0 ; size of array2 = %1");
				break;

			case ERROR_IF_VECTOR_CONDITION:
				msg = tr("Condition of the if cannot be a vector");
				break;

			case ERROR_WHILE_VECTOR_CONDITION:
				msg = tr("Condition of the while cannot be a vector");
				break;

			case ERROR_ARRAY_ILLEGAL_ACCESS:
				msg = tr("MemoryVectorNode::getVectorSize: illegal operation");
				break;

			case ERROR_INFINITE_LOOP:
				msg = tr("Infinite loops not allowed");
				break;

			case ERROR_DIVISION_BY_ZERO:
				msg = tr("Division by zero.");
				break;

			case ERROR_ABS_NOT_POSSIBLE:
				msg = tr("-32768 has no positive correspondance in 16 bits integers.");
				break;

			case ERROR_ARRAY_OUT_OF_BOUND_READ:
				msg = tr("Out of bound static array access. Trying to read index %0 of array %1 of size %2");
				break;

			case ERROR_ARRAY_OUT_OF_BOUND_WRITE:
				msg = tr("Out of bound static array access. Trying to write index %0 of array %1 of size %2");
				break;

			case ERROR_EXPECTING_TYPE:
				msg = tr("Expecting %0 type, found %1 type instead");
				break;

			case ERROR_EXPECTING_CONDITION:
				msg = tr("Expecting a condition, found a %0 instead");
				break;

			default:
				msg = tr("Unknown error");
		}

		return msg.toStdWString();
	}
};
