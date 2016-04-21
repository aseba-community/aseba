/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

#include "../buffer/vm-buffer.h"
#include "can-net.h"
#include "../../common/consts.h"

void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	ASEBA_UNUSED(vm);
	while (AsebaCanSend(data, length) == 0)
		AsebaIdle();
}

uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	ASEBA_UNUSED(vm);
	return AsebaCanRecv(data, maxLength, source);
}



