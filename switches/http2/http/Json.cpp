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

#include "../../../common/utils/utils.h"
#include "Json.h"

namespace Aseba
{
	using namespace std;
	
	json parse(const vector<uint8_t>& rawData)
	{
		membuf sbuf(reinterpret_cast<const char*>(&rawData[0]), rawData.size());
		istream is(&sbuf);
		return json::parse(is);
	}
	
}; // namespace Aseba
