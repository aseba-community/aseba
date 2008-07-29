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

// useful math functions used by below

// table is 20 bins (one for each bit of value) of 8 values each
static const sint16 aseba_atan_table[20*8+1] = { 651,732,813,894,974,1055,1136,1216,1297,1457,1616,1775,1933,2090,2246,2401,2555,2859,3159,3453,3742,4024,4301,4572,4836,5344,5826,6282,6711,7116,7497,7855,8192,8804,9346,9825,10250,10630,10969,11273,11547,12021,12415,12746,13028,13270,13481,13665,13828,14103,14325,14508,14661,14791,14903,15001,15086,15229,15344,15438,15516,15583,15640,15689,15732,15805,15862,15910,15949,15983,16011,16036,16058,16094,16123,16146,16166,16183,16197,16210,16221,16239,16253,16265,16275,16283,16290,16297,16302,16311,16318,16324,16329,16333,16337,16340,16343,16347,16351,16354,16356,16358,16360,16362,16363,16365,16367,16369,16370,16371,16372,16373,16373,16374,16375,16376,16377,16377,16378,16378,16378,16379,16379,16380,16380,16380,16381,16381,16381,16381,16381,16382,16382,16382,16382,16382,16382,16382,16382,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16383,16384 };

// atan2, do y/x and return an "aseba" angle that spans the whole 16 bits range
sint16 aseba_atan2(sint16 y, sint16 x)
{
	if (y == 0)
	{
		if (x >= 0)	// we return 0 on devision by zero
			return 0;
		else if (x < 0)
			return -32768;
	}
	
	sint16 res;
	sint16 ax = abs(x);
	sint16 ay = abs(y);
	if (x == 0)
	{
		res = 16384;
	}
	else
	{
		sint32 value = (((sint32)ay << 16)/(sint32)(ax));
		sint16 fb1 = 0;
		
		// find first bit at one
		sint16 fb1_counter;
		for (fb1_counter = 0; fb1_counter < 32; fb1_counter++)
			if ((value >> (sint32)fb1_counter) != 0)
				fb1 = fb1_counter;
		
		// we only keep 4 bits of precision below comma as atan(x) is like x near 0
		sint16 index = fb1 - 12;
		if (index < 0)
		{
			// value is smaller than 2e-4
			res = (sint16)(((sint32)aseba_atan_table[0] * value) >> 12);
		}
		else
		{
			sint32 subprecision_rest = value - (1 << (sint32)fb1);
			sint16 to_shift = fb1 - 8; // fb1 >= 12 otherwise index would have been < 0
			sint16 subprecision_index = (sint16)(subprecision_rest >> (sint32)to_shift);
			sint16 bin = subprecision_index >> 5;
			sint16 delta = subprecision_index & 0x1f;
			res = (sint16)(((sint32)aseba_atan_table[index*8 + bin] * (sint32)(32 - delta) + (sint32)aseba_atan_table[index*8 + bin + 1] * (sint32)delta) >> 5);
		}
		
		// do pi - value if x negative
		if (x < 0)
			res = 32768 - res;
	}
	
	if (y > 0)
		return res;
	else
		return -res;
}

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
	"vec.fill",
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
	"vec.copy",
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
	"vec.add",
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
	"vec.sub",
	"substract two vectors",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};


void AsebaNative_vecmin(AsebaVMState *vm)
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
		sint16 v1 = vm->variables[src1++];
		sint16 v2 = vm->variables[src2++];
		sint16 res = v1 < v2 ? v1 : v2;
		vm->variables[dest++] = res;
	}
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecmin =
{
	"vec.min",
	"element by element minimum",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};


void AsebaNative_vecmax(AsebaVMState *vm)
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
		sint16 v1 = vm->variables[src1++];
		sint16 v2 = vm->variables[src2++];
		sint16 res = v1 > v2 ? v1 : v2;
		vm->variables[dest++] = res;
	}
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecmax =
{
	"vec.max",
	"element by element maximum",
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
	"vec.dot",
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
	"vec.stat",
	"statistics on a vector",
	{
		{ -1, "src" },
		{ 1, "min" },
		{ 1, "max" },
		{ 1, "mean" },
		{ 0, 0 }
	}
};

void AsebaNative_mathmuldiv(AsebaVMState *vm)
{
	uint16 a = vm->variables[vm->stack[1]];
	uint16 b = vm->variables[vm->stack[2]];
	uint16 c = vm->variables[vm->stack[3]];
	
	vm->variables[vm->stack[0]] = (sint16)(((sint32)a * (sint32)b) / (sint32)c);
}

AsebaNativeFunctionDescription AsebaNativeDescription_mathmuldiv =
{
	"math.muldiv",
	"performs dest = (a*b)/c in 32 bits",
	{
		{ 1, "dest" },
		{ 1, "a" },
		{ 1, "b" },
		{ 1, "c" },
		{ 0, 0 }
	}
};

void AsebaNative_mathatan2(AsebaVMState *vm)
{
	uint16 y = vm->variables[vm->stack[1]];
	uint16 x = vm->variables[vm->stack[2]];
	
	vm->variables[vm->stack[0]] = aseba_atan2(y, x);
}

AsebaNativeFunctionDescription AsebaNativeDescription_mathatan2 =
{
	"math.atan2",
	"performs atan2(y,x)",
	{
		{ 1, "dest" },
		{ 1, "y" },
		{ 1, "x" },
		{ 0, 0 }
	}
};


/*@}*/
