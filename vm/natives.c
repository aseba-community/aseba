/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
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

#include <assert.h>

/**
	\file vm.c
	Implementation of standard natives functions for Aseba Virtual Machine
*/

/** \addtogroup vm */
/*@{*/

// useful math functions used by below

// table is 20 bins (one for each bit of value) of 8 values each + one for infinity
static const sint16 aseba_atan_table[20*8+1] = { 652, 735, 816, 896, 977, 1058, 1139, 1218, 1300, 1459, 1620, 1777, 1935, 2093, 2250, 2403, 2556, 2868, 3164, 3458, 3748, 4029, 4307, 4578, 4839, 5359, 5836, 6290, 6720, 7126, 7507, 7861, 8203, 8825, 9357, 9839, 10260, 10640, 10976, 11281, 11557, 12037, 12425, 12755, 13036, 13277, 13486, 13671, 13837, 14112, 14331, 14514, 14666, 14796, 14907, 15003, 15091, 15235, 15348, 15441, 15519, 15585, 15642, 15691, 15736, 15808, 15865, 15912, 15951, 15984, 16013, 16037, 16060, 16096, 16125, 16148, 16168, 16184, 16199, 16211, 16222, 16240, 16255, 16266, 16276, 16284, 16292, 16298, 16303, 16312, 16320, 16325, 16331, 16334, 16338, 16341, 16344, 16348, 16352, 16355, 16357, 16360, 16361, 16363, 16364, 16366, 16369, 16369, 16371, 16372, 16373, 16373, 16375, 16375, 16377, 16376, 16378, 16378, 16378, 16379, 16379, 16380, 16380, 16380, 16382, 16381, 16381, 16381, 16382, 16382, 16382, 16382, 16382, 16382, 16384, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16383, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384 };
/* Generation code:
for (int i = 0; i < 20; i++)
{
	double d = pow(2, (double)(i - 4));
	double dd = d / 8;
	for (int j = 0; j < 8; j++)
	{
		double v = atan(d + dd * j);
		aseba_atan_table[i*8+j] = (int)(((32768.*v) /  (M_PI))+0.5);
	}
	aseba_atan_table[20*8] = 16384
}
+ optimisation
*/


// atan2, do y/x and return an "aseba" angle that spans the whole 16 bits range
sint16 aseba_atan2(sint16 y, sint16 x)
{
	if (y == 0)
	{
		if (x >= 0)	// we return 0 on division by zero
			return 0;
		else if (x < 0)
			return -32768;
	}
	
	{
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
	#ifdef __C30__ 
			// ASM optimisation for 16bits PIC
			// Find first bit from left (MSB) on 32 bits word
			asm ("ff1l %[word], %[b]" : [b] "=r" (fb1) : [word] "r" ((int) (value >> 16)) : "cc");
			if(fb1) 
				fb1 = fb1 + 16 - 1; // Bit 0 is "1", fbl = 0 mean no 1 found for ff1l
			else {
				asm ("ff1l %[word], %[b]" : [b] "=r" (fb1) : [word] "r" ((int) value) : "cc");
				if(fb1)
					fb1--; // See above
			}
				
	#else		
			sint16 fb1_counter;
			for (fb1_counter = 0; fb1_counter < 32; fb1_counter++)
				if ((value >> (sint32)fb1_counter) != 0)
					fb1 = fb1_counter;
	#endif	
			{
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
		}
		
		if (y > 0)
			return res;
		else
			return -res;
	}
}

// 2 << 7 entries + 1, from 0 to 16384, being from 0 to PI
static const sint16 aseba_sin_table[128+1] = {0, 403, 804, 1207, 1608, 2010, 2411, 2812, 3212, 3612, 4011, 4411, 4808, 5206, 5603, 5998, 6393, 6787, 7180, 7572, 7962, 8352, 8740, 9127, 9513, 9896, 10279, 10660, 11040, 11417, 11794, 12167, 12540, 12911, 13279, 13646, 14010, 14373, 14733, 15091, 15447, 15801, 16151, 16500, 16846, 17190, 17531, 17869, 18205, 18538, 18868, 19196, 19520, 19842, 20160, 20476, 20788, 21097, 21403, 21706, 22006, 22302, 22595, 22884, 23171, 23453, 23732, 24008, 24279, 24548, 24812, 25073, 25330, 25583, 25833, 26078, 26320, 26557, 26791, 27020, 27246, 27467, 27684, 27897, 28106, 28311, 28511, 28707, 28899, 29086, 29269, 29448, 29622, 29792, 29957, 30117, 30274, 30425, 30572, 30715, 30852, 30985, 31114, 31238, 31357, 31471, 31581, 31686, 31786, 31881, 31972, 32057, 32138, 32215, 32285, 32352, 32413, 32470, 32521, 32569, 32610, 32647, 32679, 32706, 32728, 32746, 32758, 32766, 32767, };
/* Generation code:
int i;
for (i = 0; i <= 128; i++)
{
	sinTable[i] = (sint16)(32767. * sin( ((M_PI/2.)*(double)i)/128.) + 0.5);
}
+ optimisation
*/


// do the sinus of an "aseba" angle that spans the whole 16 bits range, and return a 1.15 fixed point value
sint16 aseba_sin(sint16 angle)
{
	sint16 index;
	sint16 subIndex;
	sint16 invert;
	sint16 lookupAngle;
	if (angle < 0)
	{
		if (angle < -16384)
			lookupAngle = 32768 + angle;
		else if (angle > -16384)
			lookupAngle = -angle;
		else
			return -32767;
		invert = 1;
	}
	else
	{
		if (angle > 16384)
			lookupAngle = 32767 - angle + 1;
		else if (angle < 16484)
			lookupAngle = angle;
		else
			return 32767;
		invert = 0;
	}
	
	index = lookupAngle >> 7;
	subIndex = lookupAngle & 0x7f;
	
	{
		sint16 result = (sint16)(((sint32)aseba_sin_table[index] * (sint32)(128-subIndex) + (sint32)aseba_sin_table[index+1] * (sint32)(subIndex)) >> 7);
		
		if (invert)
			return -result;
		else
			return result;
	}
}

// do the cos of an "aseba" angle that spans the whole 16 bits range, and return a 1.15 fixed point value
sint16 aseba_cos(sint16 angle)
{
	return aseba_sin(16384 + angle);
}

// standard natives functions

uint16 AsebaNative_vecfill(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativeGetArg(vm, 0);
	uint16 value = AsebaNativeGetArg(vm, 1);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 2);
	
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[value];
	}
	
	return 3;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecfill =
{
	"math.fill",
	"fill vector with constant",
	{
		{ -1, "dest" },
		{ 1, "value" },
		{ 0, 0 }
	}
};


uint16 AsebaNative_veccopy(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativeGetArg(vm, 0);
	uint16 src = AsebaNativeGetArg(vm, 1);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 2);
	
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src++];
	}
	
	return 3;
}

AsebaNativeFunctionDescription AsebaNativeDescription_veccopy =
{
	"math.copy",
	"copy vector content",
	{
		{ -1, "dest" },
		{ -1, "src" },
		{ 0, 0 }
	}
};

uint16 AsebaNative_vecadd(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativeGetArg(vm, 0);
	uint16 src1 = AsebaNativeGetArg(vm, 1);
	uint16 src2 = AsebaNativeGetArg(vm, 2);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 3);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src1++] + vm->variables[src2++];
	}
	
	return 4;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecadd =
{
	"math.add",
	"add two vectors",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};

uint16 AsebaNative_vecsub(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativeGetArg(vm, 0);
	uint16 src1 = AsebaNativeGetArg(vm, 1);
	uint16 src2 = AsebaNativeGetArg(vm, 2);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 3);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src1++] - vm->variables[src2++];
	}
	
	return 4;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecsub =
{
	"math.sub",
	"substract two vectors",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};


uint16 AsebaNative_vecmin(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativeGetArg(vm, 0);
	uint16 src1 = AsebaNativeGetArg(vm, 1);
	uint16 src2 = AsebaNativeGetArg(vm, 2);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 3);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 v1 = vm->variables[src1++];
		sint16 v2 = vm->variables[src2++];
		sint16 res = v1 < v2 ? v1 : v2;
		vm->variables[dest++] = res;
	}
	
	return 4;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecmin =
{
	"math.min",
	"element by element minimum",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};


uint16 AsebaNative_vecmax(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativeGetArg(vm, 0);
	uint16 src1 = AsebaNativeGetArg(vm, 1);
	uint16 src2 = AsebaNativeGetArg(vm, 2);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 3);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 v1 = vm->variables[src1++];
		sint16 v2 = vm->variables[src2++];
		sint16 res = v1 > v2 ? v1 : v2;
		vm->variables[dest++] = res;
	}
	
	return 4;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecmax =
{
	"math.max",
	"element by element maximum",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};



uint16 AsebaNative_vecdot(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativeGetArg(vm, 0);
	uint16 src1 = AsebaNativeGetArg(vm, 1);
	uint16 src2 = AsebaNativeGetArg(vm, 2);
	sint32 shift = AsebaNativeGetArg(vm, 3);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 4);
	
	sint32 res = 0;
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		res += (sint32)vm->variables[src1++] * (sint32)vm->variables[src2++];
	}
	res >>= shift;
	vm->variables[dest] = (sint16)res;
	
	return 5;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecdot =
{
	"math.dot",
	"scalar product between two vectors",
	{
		{ 1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 1, "shift" },
		{ 0, 0 }
	}
};


uint16 AsebaNative_vecstat(AsebaVMState *vm)
{
	// variable pos
	uint16 src = AsebaNativeGetArg(vm, 0);
	uint16 min = AsebaNativeGetArg(vm, 1);
	uint16 max = AsebaNativeGetArg(vm, 2);
	uint16 mean = AsebaNativeGetArg(vm, 3);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 4);
	sint16 val;
	sint32 acc;
	uint16 i;
	
	if (length)
	{	
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
	return 5;
}

AsebaNativeFunctionDescription AsebaNativeDescription_vecstat =
{
	"math.stat",
	"statistics on a vector",
	{
		{ -1, "src" },
		{ 1, "min" },
		{ 1, "max" },
		{ 1, "mean" },
		{ 0, 0 }
	}
};

uint16 AsebaNative_mathmuldiv(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativeGetArg(vm, 0);
	sint16 aIndex = AsebaNativeGetArg(vm, 1);
	sint16 bIndex = AsebaNativeGetArg(vm, 2);
	sint16 cIndex = AsebaNativeGetArg(vm, 3);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 4);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint32 a = (sint32)vm->variables[aIndex++];
		sint32 b = (sint32)vm->variables[bIndex++];
		sint32 c = (sint32)vm->variables[cIndex++];
		vm->variables[destIndex++] = (sint16)((a * b) / c);
	}
	
	return 5;
}

AsebaNativeFunctionDescription AsebaNativeDescription_mathmuldiv =
{
	"math.muldiv",
	"performs dest = (a*b)/c in 32 bits element by element",
	{
		{ -1, "dest" },
		{ -1, "a" },
		{ -1, "b" },
		{ -1, "c" },
		{ 0, 0 }
	}
};

uint16 AsebaNative_mathatan2(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativeGetArg(vm, 0);
	sint16 yIndex = AsebaNativeGetArg(vm, 1);
	sint16 xIndex = AsebaNativeGetArg(vm, 2);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 3);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 y = vm->variables[yIndex++];
		sint16 x = vm->variables[xIndex++];
		vm->variables[destIndex++] = aseba_atan2(y, x);
	}
	
	return 4;
}

AsebaNativeFunctionDescription AsebaNativeDescription_mathatan2 =
{
	"math.atan2",
	"performs atan2(y,x) element by element",
	{
		{ -1, "dest" },
		{ -1, "y" },
		{ -1, "x" },
		{ 0, 0 }
	}
};

uint16 AsebaNative_mathsin(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativeGetArg(vm, 0);
	sint16 xIndex = AsebaNativeGetArg(vm, 1);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 2);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 x = vm->variables[xIndex++];
		vm->variables[destIndex++] = aseba_sin(x);
	}
	
	return 3;
}

AsebaNativeFunctionDescription AsebaNativeDescription_mathsin =
{
	"math.sin",
	"performs sin(x) element by element",
	{
		{ -1, "dest" },
		{ -1, "x" },
		{ 0, 0 }
	}
};

uint16 AsebaNative_mathcos(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativeGetArg(vm, 0);
	sint16 xIndex = AsebaNativeGetArg(vm, 1);
	
	// variable size
	uint16 length = AsebaNativeGetArg(vm, 2);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 x = vm->variables[xIndex++];
		vm->variables[destIndex++] = aseba_cos(x);
	}
	
	return 3;
}

AsebaNativeFunctionDescription AsebaNativeDescription_mathcos =
{
	"math.cos",
	"performs cos(x) element by element",
	{
		{ -1, "dest" },
		{ -1, "x" },
		{ 0, 0 }
	}
};

uint16 AsebaNative_mathrot2(AsebaVMState *vm)
{
	// variable pos
	uint16 vectOutIndex = AsebaNativeGetArg(vm, 0);
	uint16 vecInIndex = AsebaNativeGetArg(vm, 1);
	uint16 angleIndex = AsebaNativeGetArg(vm, 2);
	
	// variables
	sint16 x = vm->variables[vecInIndex];
	sint16 y = vm->variables[vecInIndex+1];
	sint16 a = vm->variables[angleIndex];
	
	sint16 cos_a = aseba_cos(a);
	sint16 sin_a = aseba_sin(a);
	
	sint16 xp = (sint16)(((sint32)cos_a * (sint32)x + (sint32)sin_a * (sint32)y) >> (sint32)15);
	sint16 yp = (sint16)(((sint32)cos_a * (sint32)y - (sint32)sin_a * (sint32)x) >> (sint32)15);
	
	vm->variables[vectOutIndex] = xp;
	vm->variables[vectOutIndex+1] = yp;
	
	return 3;
}

AsebaNativeFunctionDescription AsebaNativeDescription_mathrot2 =
{
	"math.rot2",
	"rotates v of a",
	{
		{ 2, "dest" },
		{ 2, "v" },
		{ 1, "a" },
		{ 0, 0 }
	}
};

/*@}*/
