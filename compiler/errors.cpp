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
		error_map[ERROR_01] = L"arg";
		error_map[ERROR_02] = L"oups";
		error_map[ERROR_03] = L"another error %0";
	}

	const std::wstring ErrorMessages::defaultCallback(ErrorCode error)
	{
		if (error >= ERROR_END)
			return L"Boulet";
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
	
}; // Aseba
