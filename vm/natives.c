/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
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

#include "../common/consts.h"
#include "../common/types.h"
#include "natives.h"
#include <string.h>

/**
	\file vm.c
	Implementation of standard natives functions for Aseba Virtual Machine
*/

/** \addtogroup vm */
/*@{*/

// standard natives functions

void AsebaNative_vecfill(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = vm->stack[0];
	uint16 value = vm->stack[1];
	
	// variable size
	uint16 length = vm->stack[2];
	
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[value];
	}
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecfill =
{
	"vecfill",
	"fill vector with constant",
	{
		{ -1, "dest" },
		{ 1, "value" },
		{ 0, 0 }
	}
};


void AsebaNative_veccopy(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = vm->stack[0];
	uint16 src = vm->stack[1];
	
	// variable size
	uint16 length = vm->stack[2];
	
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src++];
	}
}

AsebaNativeFunctionDescription AsebaNativeDescription_veccopy =
{
	"veccopy",
	"copy vector content",
	{
		{ -1, "dest" },
		{ -1, "src" },
		{ 0, 0 }
	}
};

void AsebaNative_vecadd(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = vm->stack[0];
	uint16 src1 = vm->stack[1];
	uint16 src2 = vm->stack[2];
	
	// variable size
	uint16 length = vm->stack[3];
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src1++] + vm->variables[src2++];
	}
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecadd =
{
	"vecadd",
	"add two vectors",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};

void AsebaNative_vecsub(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = vm->stack[0];
	uint16 src1 = vm->stack[1];
	uint16 src2 = vm->stack[2];
	
	// variable size
	uint16 length = vm->stack[3];
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src1++] - vm->variables[src2++];
	}
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecsub =
{
	"vecsub",
	"substract two vectors",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};


void AsebaNative_vecdot(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = vm->stack[0];
	uint16 src1 = vm->stack[1];
	uint16 src2 = vm->stack[2];
	sint32 shift = vm->variables[vm->stack[3]];
	
	// variable size
	uint16 length = vm->stack[4];
	
	sint32 res = 0;
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		res += (sint32)vm->variables[src1++] * (sint32)vm->variables[src2++];
	}
	res >>= shift;
	vm->variables[dest] = (sint16)res;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecdot =
{
	"vecdot",
	"scalar product between two vectors",
	{
		{ 1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 1, "shift" },
		{ 0, 0 }
	}
};


void AsebaNative_vecstat(AsebaVMState *vm)
{
	// variable pos
	uint16 src = vm->stack[0];
	uint16 min = vm->stack[1];
	uint16 max = vm->stack[2];
	uint16 mean = vm->stack[3];
	
	// variable size
	uint16 length = vm->stack[4];
	sint16 val;
	sint32 acc;
	uint16 i;
	
	if (!length)
		return;
	
	val = vm->variables[src++];
	acc = val;
	vm->variables[min] = val;
	vm->variables[max] = val;
	
	for (i = 1; i < length; i++)
	{
		val = vm->variables[src++];
		if (val < vm->variables[min])
			vm->variables[min] = val;
		if (val > vm->variables[max])
			vm->variables[max] = val;
		acc += (sint32)val;
	}
	
	vm->variables[mean] = (sint16)(acc / (sint32)length);
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecstat =
{
	"vecstat",
	"statistics on a vector",
	{
		{ -1, "src" },
		{ 1, "min" },
		{ 1, "max" },
		{ 1, "mean" },
		{ 0, 0 }
	}
};

/*@}*/
