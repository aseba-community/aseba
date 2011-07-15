/*
	Playground - An active arena to learn multi-robots programming
	Copyright (C) 1999--2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
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

#include "../../vm/natives.h"
#include "../../common/productids.h"


AsebaVMDescription vmDescription = {
	"e-puck  ",
	{
		{ 1, "id" },	// Do not touch it
		{ 1, "source" }, // nor this one
		{ 32, "args" },	// neither this one
		{ 1, ASEBA_PID_VAR_NAME },
		{ 1, "leftSpeed" },
		{ 1, "rightSpeed" },
		{ 1, "colorR" },
		{ 1, "colorG" },
		{ 1, "colorB" },
		{ 8, "prox" },
		{ 60, "camR" },
		{ 60, "camG" },
		{ 60, "camB" },
		{ 1, "energy" },
		{ 0, NULL }
	}
};

AsebaNativeFunctionDescription PlaygroundNativeDescription_energysend =
{
	"energy.send",
	"send energy to pool",
	{
		{ 1, "amount" },
		{ 0, 0 }
	}
};

AsebaNativeFunctionDescription PlaygroundNativeDescription_energyreceive =
{
	"energy.receive",
	"receive energy from pool",
	{
		{ 1, "amount" },
		{ 0, 0 }
	}
};

AsebaNativeFunctionDescription PlaygroundNativeDescription_energyamount =
{
	"energy.amount",
	"get the amount of energy available in pool",
	{
		{ 1, "amount" },
		{ 0, 0 }
	}
};

