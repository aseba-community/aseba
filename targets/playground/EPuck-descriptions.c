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

#include "../../vm/natives.h"
#include "../../common/productids.h"


AsebaVMDescription PlaygroundEPuckVMDescription = {
	"e-puck  ",
	{
		{ 1, "id" },	// Do not touch it
		{ 1, "source" }, // nor this one
		{ 32, "args" },	// neither this one
		{ 1, ASEBA_PID_VAR_NAME },
		{ 1, "speed.left" },
		{ 1, "speed.right" },
		{ 1, "color.red" },
		{ 1, "color.green" },
		{ 1, "color.blue" },
		{ 8, "prox" },
		{ 60, "cam.red" },
		{ 60, "cam.green" },
		{ 60, "cam.blue" },
		{ 1, "energy" },
		{ 0, NULL }
	}
};

AsebaNativeFunctionDescription PlaygroundEPuckNativeDescription_energysend =
{
	"energy.send",
	"send energy to pool",
	{
		{ 1, "amount" },
		{ 0, 0 }
	}
};

AsebaNativeFunctionDescription PlaygroundEPuckNativeDescription_energyreceive =
{
	"energy.receive",
	"receive energy from pool",
	{
		{ 1, "amount" },
		{ 0, 0 }
	}
};

AsebaNativeFunctionDescription PlaygroundEPuckNativeDescription_energyamount =
{
	"energy.amount",
	"get the amount of energy available in pool",
	{
		{ 1, "amount" },
		{ 0, 0 }
	}
};

