/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2013:
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

#include "DirectAsebaGlue.h"
#include "EnkiGlue.h"
#include "../../common/utils/FormatableString.h"
#include "../../transport/buffer/vm-buffer.h"

namespace Aseba
{
	void DirectConnection::sendBuffer(uint16 nodeId, const uint8* data, uint16 length)
	{
		// deserialize message
		Message::SerializationBuffer content;
		std::copy(data+2, data+length, std::back_inserter(content.rawData));
		const uint16 type(*reinterpret_cast<const uint16*>(data));
		outQueue.emplace(Message::create(nodeId, type, content));
	}
	
} // namespace Aseba
