/*
	Playground - An active arena to learn multi-robots programming
	Copyright (C) 1999 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Mobots group (http://mobots.epfl.ch)
		Laboratory of Robotics Systems (http://lsro.epfl.ch)
		EPFL, Lausanne (http://www.epfl.ch)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../../vm/natives.h"


AsebaVMDescription vmDescription = {
	"e-puck  ",
	{
		{ 1, "id" },	// Do not touch it
		{ 1, "source" }, // nor this one
		{ 32, "args" },	// neither this one
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

