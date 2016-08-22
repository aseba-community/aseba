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

#ifndef ASEBA_HTTP_STATUS
#define ASEBA_HTTP_STATUS

namespace Aseba
{
	//! A list of used HTTP status
	enum class HttpStatus
	{
		OK = 200,
		CREATED = 201,
		BAD_REQUEST = 400,
		FORBIDDEN = 403,
		NOT_FOUND = 404,
		METHOD_NOT_ALLOWED = 405,
		REQUEST_TIMEOUT = 408,
		INTERNAL_SERVER_ERROR = 500,
		NOT_IMPLEMENTED = 501,
		SERVICE_UNAVAILABLE = 503,
		HTTP_VERSION_NOT_SUPPORTED = 505
	};
} // namespace Aseba

#endif // ASEBA_HTTP_STATUS
