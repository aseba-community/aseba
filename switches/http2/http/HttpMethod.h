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

#ifndef ASEBA_HTTP_METHOD
#define ASEBA_HTTP_METHOD

#include <string>
#include <stdexcept>

namespace Aseba
{
	//! Possible HTTP methods
	enum struct HttpMethod
	{
		GET = 0,
		PUT,
		POST,
		OPTIONS,
		DELETE
	};
	
	using domain_error = std::domain_error;
	//! Invalid method
	struct InvalidHttpMethod: public domain_error
	{
		using domain_error::domain_error;
	};
	
	std::string toString(const HttpMethod& method);
	HttpMethod fromMethodString(const std::string& method);
	
} // namespace Aseba

#endif // ASEBA_HTTP_METHOD
