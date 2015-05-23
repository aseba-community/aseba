/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

#include "../common/consts.h"
#include "../common/types.h"
#include "natives.h"
#include <string.h>

#include <assert.h>

#if defined(__dsPIC30F__)
#include <p30Fxxxx.h>
#define DSP_AVAILABLE
#elif defined(__dsPIC33F__)
#include <p33Fxxxx.h>
#define DSP_AVAILABLE
#elif defined(__PIC24H__)
#include <p24Hxxxx.h>
#endif 



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
#ifdef __C30__
			unsigned int q2, rem1;
			unsigned long q1;
			q1 = __builtin_divmodud(ay, ax, &rem1);
			q2 = __builtin_divud(((unsigned long) rem1) << 16, ax);
			sint32 value = (q1 << 16) | q2;
			
			sint16 fb1;
			
			// find first bit at one
			// ASM optimisation for 16bits PIC
			// Find first bit from left (MSB) on 32 bits word
			asm ("ff1l %[word], %[b]" : [b] "=r" (fb1) : [word] "r" ((int) (value >> 16)) : "cc");
			if(fb1) 
				fb1 = 17 - fb1 + 16 - 1; // Bit 0 is "16", fbl = 0 mean no 1 found for ff1l
			else {
				asm ("ff1l %[word], %[b]" : [b] "=r" (fb1) : [word] "r" ((int) value) : "cc");
				if(fb1)
					fb1 = 17 - fb1 - 1; // see above
			}
			{
				// we only keep 4 bits of precision below comma as atan(x) is like x near 0
				sint16 index = fb1 - 12;
				if (index < 0)
				{
					// value is smaller than 2e-4
					res = (value*aseba_atan_table[0]) >> 12;
				}
				else
				{
					sint32 subprecision_rest = value - (1L << fb1);
					sint16 to_shift = fb1 - 8; // fb1 >= 12 otherwise index would have been < 0
					sint16 subprecision_index = (sint16)(subprecision_rest >> to_shift);
					sint16 bin = subprecision_index >> 5;
					sint16 delta = subprecision_index & 0x1f;
					res = __builtin_divsd(__builtin_mulss(aseba_atan_table[index*8 + bin], 32 - delta) + __builtin_mulss(aseba_atan_table[index*8 + bin + 1], delta),32);
				}
				
#else		
			sint32 value = (((sint32)ay << 16)/(sint32)(ax));
			sint16 fb1 = 0;
			
			sint16 fb1_counter;
			for (fb1_counter = 0; fb1_counter < 32; fb1_counter++)
				if ((value >> (sint32)fb1_counter) != 0)
					fb1 = fb1_counter;
						
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
					sint32 subprecision_rest = value - (((sint32) 1) << fb1);
					sint16 to_shift = fb1 - 8; // fb1 >= 12 otherwise index would have been < 0
					sint16 subprecision_index = (sint16)(subprecision_rest >> to_shift);
					sint16 bin = subprecision_index >> 5;
					sint16 delta = subprecision_index & 0x1f;
					res = (sint16)(((sint32)aseba_atan_table[index*8 + bin] * (sint32)(32 - delta) + (sint32)aseba_atan_table[index*8 + bin + 1] * (sint32)delta) >> 5);
				}
#endif
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
		else if (angle < 16384)
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

// Do integer square root ( from Wikipedia )
sint16 aseba_sqrt(sint16 num)
{
	sint16 op = num;
	sint16 res = 0;
	sint16 one = 1 << 14;
	
	while(one > op)
		one >>= 2;
		
	while(one != 0) 
	{
		if (op >= res + one) 
		{
			op -= res + one;
			res = (res >> 1) + one;
		}
		else
		{
			res >>= 1;
		}
		one >>= 2;
	}
	return res;
}

// comb sort ( from Wikipedia )
void aseba_comb_sort(sint16* input, uint16 size)
{
	uint16 gap = size;
	uint16 swapped = 0;
	uint16 i;

	while ((gap > 1) || swapped)
	{
		if (gap > 1)
		{
#ifdef __C30__
			gap = __builtin_divud(__builtin_muluu(gap,4),5);
#else
			gap = (uint16)(((uint32)gap * 4) / 5);
#endif
		}

		swapped = 0;

		for (i = 0; gap + i < size; i++)
		{
			if (input[i] - input[i + gap] > 0)
			{
				sint16 swap = input[i];
				input[i] = input[i + gap];
				input[i + gap] = swap;
				swapped = 1;
			}
		}
	}
}


// standard natives functions

void AsebaNative_veccopy(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src++];
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_veccopy =
{
	"math.copy",
	"copies src to dest element by element",
	{
		{ -1, "dest" },
		{ -1, "src" },
		{ 0, 0 }
	}
};


void AsebaNative_vecfill(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 value = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[value];
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecfill =
{
	"math.fill",
	"fills dest with constant value",
	{
		{ -1, "dest" },
		{ 1, "value" },
		{ 0, 0 }
	}
};


void AsebaNative_vecaddscalar(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src = AsebaNativePopArg(vm);
	uint16 scalar = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	const sint16 scalarValue = vm->variables[scalar];
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src++] + scalarValue;
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecaddscalar =
{
	"math.addscalar",
	"add scalar to each element of src and store result to dest",
	{
		{ -1, "dest" },
		{ -1, "src" },
		{ 1, "scalar" },
		{ 0, 0 }
	}
};


void AsebaNative_vecadd(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src1 = AsebaNativePopArg(vm);
	uint16 src2 = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src1++] + vm->variables[src2++];
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecadd =
{
	"math.add",
	"adds src1 and src2 to dest, element by element",
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
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src1 = AsebaNativePopArg(vm);
	uint16 src2 = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src1++] - vm->variables[src2++];
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecsub =
{
	"math.sub",
	"substracts src2 from src1 to dest, element by element",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};


void AsebaNative_vecmul(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src1 = AsebaNativePopArg(vm);
	uint16 src2 = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[dest++] = vm->variables[src1++] * vm->variables[src2++];
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecmul =
{
	"math.mul",
	"multiplies src1 and src2 to dest, element by element",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};


void AsebaNative_vecdiv(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src1 = AsebaNativePopArg(vm);
	uint16 src2 = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint32 dividend = (sint32)vm->variables[src1++];
		sint32 divisor = (sint32)vm->variables[src2++];
		
		if (divisor != 0)
		{
			vm->variables[dest++] = (sint16)(dividend / divisor);
		}
		else
		{
			vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
			AsebaSendMessage(vm, ASEBA_MESSAGE_DIVISION_BY_ZERO, &(vm->pc), sizeof(vm->pc));
			return;
		}
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecdiv =
{
	"math.div",
	"divides src1 by src2 to dest, element by element",
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
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src1 = AsebaNativePopArg(vm);
	uint16 src2 = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 v1 = vm->variables[src1++];
		sint16 v2 = vm->variables[src2++];
		sint16 res = v1 < v2 ? v1 : v2;
		vm->variables[dest++] = res;
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecmin =
{
	"math.min",
	"writes the minimum of src1 and src2 to dest, element by element",
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
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src1 = AsebaNativePopArg(vm);
	uint16 src2 = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 v1 = vm->variables[src1++];
		sint16 v2 = vm->variables[src2++];
		sint16 res = v1 > v2 ? v1 : v2;
		vm->variables[dest++] = res;
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecmax =
{
	"math.max",
	"writes the maximum of src1 and src2 to dest, element by element",
	{
		{ -1, "dest" },
		{ -1, "src1" },
		{ -1, "src2" },
		{ 0, 0 }
	}
};

void AsebaNative_vecclamp(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src = AsebaNativePopArg(vm);
	uint16 low = AsebaNativePopArg(vm);
	uint16 high = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 v = vm->variables[src++];
		sint16 l = vm->variables[low++];
		sint16 h = vm->variables[high++];
		sint16 res = v > h ? h : (v < l ? l : v);
		vm->variables[dest++] = res;
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecclamp =
{
	"math.clamp",
	"clamp src to low/high bounds into dest, element by element",
	{
		{ -1, "dest" },
		{ -1, "src" },
		{ -1, "low" },
		{ -1, "high" },
		{ 0, 0 }
	}
};

void AsebaNative_vecdot(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm);
	uint16 src1 = AsebaNativePopArg(vm);
	uint16 src2 = AsebaNativePopArg(vm);
	sint16 shift = vm->variables[AsebaNativePopArg(vm)];
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	sint32 res = 0;
	uint16 i;
	
	if(shift > 32) {
		vm->variables[dest] = 0;
		return;
	}

#ifdef DSP_AVAILABLE
	length--;
	
	CORCONbits.US = 0; // Signed mode
	CORCON |= 0b11110001; // 40 bits mode, saturation enable, integer mode.
	// Do NOT save the accumulator values, so do NOT USE THIS FUNCTION IN INTERRUPT ! 	
	asm __volatile__ (
	"push %[ptr1]		\r\n"
	"push %[ptr2]		\r\n"
	"clr A\r\n"									//	A = 0
	"mov [%[ptr1]++], w4 \r\n"					// Preload ptr
	"do %[loop_cnt], 1f	\r\n"					//	Iterate loop_cnt time the two following instructions
	"mov [%[ptr2]++], w5 \r\n"					// 	Load w5
	"1: mac w4*w5, A, [%[ptr1]]+=2, w4 \r\n"	//	A += w4 * w5, prefetch ptr1 into w4
	"pop %[ptr2]		\r\n"
	"pop %[ptr1]		\r\n"
	: /* No output */
	: [loop_cnt] "r" (length), [ptr1] "x" (&vm->variables[src1]), [ptr2] "r" (&vm->variables[src2])
	: "cc", "w4", "w5" );
	
	if(shift > 16) {
		shift -= 16;
		asm __volatile__ ("sftac	A, #16\r\n" : /* No output */ : /* No input */ : "cc");
	}
	
	// Shift and get the output
	asm __volatile__ ("sftac A, %[sft]\r\n" : /* No output */ : [sft] "r" (shift) : "cc");
	
	vm->variables[dest] = ACCAL; // Get the Accumulator low word
#elif defined(__C30__)
	for (i= 0; i < length; i++)
		res += __builtin_mulss(vm->variables[src1++], vm->variables[src2++]);
	res >>= shift;
	vm->variables[dest] = (sint16) res;
#else
	for (i = 0; i < length; i++)
	{
		res += (sint32)vm->variables[src1++] * (sint32)vm->variables[src2++];
	}
	res >>= shift;
	vm->variables[dest] = (sint16)res;
#endif
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecdot =
{
	"math.dot",
	"writes the dot product of src1 and src2 to dest, after a shift",
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
	uint16 src = AsebaNativePopArg(vm);
	uint16 min = AsebaNativePopArg(vm);
	uint16 max = AsebaNativePopArg(vm);
	uint16 mean = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
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
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecstat =
{
	"math.stat",
	"performs statistics on src",
	{
		{ -1, "src" },
		{ 1, "min" },
		{ 1, "max" },
		{ 1, "mean" },
		{ 0, 0 }
	}
};


void AsebaNative_vecargbounds(AsebaVMState *vm)
{
	// variable pos
	uint16 src = AsebaNativePopArg(vm);
	uint16 argmin = AsebaNativePopArg(vm);
	uint16 argmax = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	sint16 min = 32767;
	sint16 max = -32768;
	sint16 val;
	uint16 i;
	
	if (length)
	{
		for (i = 0; i < length; i++)
		{
			val = vm->variables[src++];
			if (val < min)
			{
				min = val;
				vm->variables[argmin] = i;
			}
			if (val > max)
			{
				max = val;
				vm->variables[argmax] = i;
			}
		}
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecargbounds =
{
	"math.argbounds",
	"get the indices (argmin, argmax) of the limit values of src",
	{
		{ -1, "src" },
		{ 1, "argmin" },
		{ 1, "argmax" },
		{ 0, 0 }
	}
};


void AsebaNative_vecsort(AsebaVMState *vm)
{
	// variable pos
	uint16 src = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	aseba_comb_sort(&vm->variables[src], length);
}

const AsebaNativeFunctionDescription AsebaNativeDescription_vecsort =
{
	"math.sort",
	"sort array in place",
	{
		{ -1, "array" },
		{ 0, 0 }
	}
};


void AsebaNative_mathmuldiv(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativePopArg(vm);
	uint16 aIndex = AsebaNativePopArg(vm);
	uint16 bIndex = AsebaNativePopArg(vm);
	uint16 cIndex = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint32 a = (sint32)vm->variables[aIndex++];
		sint32 b = (sint32)vm->variables[bIndex++];
		sint32 c = (sint32)vm->variables[cIndex++];
		if (c != 0)
		{
			vm->variables[destIndex++] = (sint16)((a * b) / c);
		}
		else
		{
			vm->flags = ASEBA_VM_STEP_BY_STEP_MASK;
			AsebaSendMessage(vm, ASEBA_MESSAGE_DIVISION_BY_ZERO, &(vm->pc), sizeof(vm->pc));
			return;
		}
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_mathmuldiv =
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

void AsebaNative_mathatan2(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativePopArg(vm);
	sint16 yIndex = AsebaNativePopArg(vm);
	sint16 xIndex = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 y = vm->variables[yIndex++];
		sint16 x = vm->variables[xIndex++];
		vm->variables[destIndex++] = aseba_atan2(y, x);
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_mathatan2 =
{
	"math.atan2",
	"performs dest = atan2(y,x) element by element",
	{
		{ -1, "dest" },
		{ -1, "y" },
		{ -1, "x" },
		{ 0, 0 }
	}
};

void AsebaNative_mathsin(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativePopArg(vm);
	sint16 xIndex = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 x = vm->variables[xIndex++];
		vm->variables[destIndex++] = aseba_sin(x);
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_mathsin =
{
	"math.sin",
	"performs dest = sin(x) element by element",
	{
		{ -1, "dest" },
		{ -1, "x" },
		{ 0, 0 }
	}
};

void AsebaNative_mathcos(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativePopArg(vm);
	sint16 xIndex = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 x = vm->variables[xIndex++];
		vm->variables[destIndex++] = aseba_cos(x);
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_mathcos =
{
	"math.cos",
	"performs dest = cos(x) element by element",
	{
		{ -1, "dest" },
		{ -1, "x" },
		{ 0, 0 }
	}
};

void AsebaNative_mathrot2(AsebaVMState *vm)
{
	// variable pos
	uint16 vectOutIndex = AsebaNativePopArg(vm);
	uint16 vecInIndex = AsebaNativePopArg(vm);
	uint16 angleIndex = AsebaNativePopArg(vm);
	
	// variables
	sint16 x = vm->variables[vecInIndex];
	sint16 y = vm->variables[vecInIndex+1];
	sint16 a = vm->variables[angleIndex];
	
	sint16 cos_a = aseba_cos(a);
	sint16 sin_a = aseba_sin(a);
	
	sint16 xp = (sint16)(((sint32)cos_a * (sint32)x - (sint32)sin_a * (sint32)y) >> (sint32)15);
	sint16 yp = (sint16)(((sint32)cos_a * (sint32)y + (sint32)sin_a * (sint32)x) >> (sint32)15);
	
	vm->variables[vectOutIndex] = xp;
	vm->variables[vectOutIndex+1] = yp;
}

const AsebaNativeFunctionDescription AsebaNativeDescription_mathrot2 =
{
	"math.rot2",
	"rotates v of angle a to dest",
	{
		{ 2, "dest" },
		{ 2, "v" },
		{ 1, "a" },
		{ 0, 0 }
	}
};

void AsebaNative_mathsqrt(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativePopArg(vm);
	sint16 xIndex = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		sint16 x = vm->variables[xIndex++];
		vm->variables[destIndex++] = aseba_sqrt(x);
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_mathsqrt =
{
	"math.sqrt",
	"performs dest = sqrt(x) element by element",
	{
		{ -1, "dest" },
		{ -1, "x" },
		{ 0, 0 }
	}
};

void AsebaNative_vecnonzerosequence(AsebaVMState *vm)
{
	// variable pos
	uint16 dest = AsebaNativePopArg(vm); // output value index
	uint16 src = AsebaNativePopArg(vm); // input vector index
	uint16 length = AsebaNativePopArg(vm); // length threshold index
	
	// variable size
	uint16 inputLength = AsebaNativePopArg(vm);
	sint16 minLength = vm->variables[length]; // length threshold 
	
	// work variables
	uint16 nzFirstZero;
	sint16 index;
	sint16 seqIndex;
	sint16 bestSeqLength;
	sint16 bestSeqIndex;
	sint16 seqLength;
	
	// search for a zero, then non-zero
	uint16 nzFirstIndex = 0;
	while (vm->variables[src + nzFirstIndex] != 0)
	{
		++nzFirstIndex;
		if (nzFirstIndex > inputLength)
		{
			vm->variables[dest] = 0;
			return;
		}
	}
	nzFirstZero = nzFirstIndex;
	while (vm->variables[src + nzFirstIndex] == 0)
	{
		++nzFirstIndex;
		if (nzFirstIndex > inputLength)
		{
			if (nzFirstZero == 0)
			{
				vm->variables[dest] = -1;
			}
			else
			{
				uint16 seqLength = nzFirstZero - 1;
				if (seqLength < minLength)
					vm->variables[dest] = -1;
				else
					vm->variables[dest] = (nzFirstZero - 1) / 2;
			}
			return;
		}
	}
	
	index = 0;
	bestSeqLength = 0;
	bestSeqIndex = 0;
	
	while (1)
	{
		// scan non-zero sequence
		seqIndex = index;
		while (vm->variables[src + (nzFirstIndex + index) % inputLength] != 0)
		{
			++index;
			if (index >= inputLength)
				break;
		}
		// check size
		seqLength = index - seqIndex;
		if (seqLength > bestSeqLength)
		{
			bestSeqLength = seqLength;
			bestSeqIndex = (index + seqIndex) / 2;
		}
		if (index >= inputLength)
			break;
		
		// scan zero sequence
		while (vm->variables[src + (nzFirstIndex + index) % inputLength] == 0)
		{
			++index;
			if (index >= inputLength)
				goto doubleBreak;
		}
	}
	
doubleBreak:
	if (bestSeqLength < minLength)
	{
		vm->variables[dest] = -1;
	}
	else
	{
		vm->variables[dest] = (nzFirstIndex + bestSeqIndex) % inputLength;
	}
}


const AsebaNativeFunctionDescription AsebaNativeDescription_vecnonzerosequence =
{
	"math.nzseq",
	"write to dest the middle index of the largest sequence of non-zero elements from src, -1 if not found or if smaller than minLength",
	{
		{ 1, "dest" },
		{ -1, "src" },
		{ 1, "minLength" },
		{ 0, 0 }
	}
};

static uint16 rnd_state;

void AsebaSetRandomSeed(uint16 seed)
{
	rnd_state = seed;
}

uint16 AsebaGetRandom()
{
	rnd_state = 25173 * rnd_state + 13849;
	return rnd_state;
}

void AsebaNative_rand(AsebaVMState *vm)
{
	// variable pos
	uint16 destIndex = AsebaNativePopArg(vm);
	
	// variable size
	uint16 length = AsebaNativePopArg(vm);
	
	uint16 i;
	for (i = 0; i < length; i++)
	{
		vm->variables[destIndex++] = (sint16)AsebaGetRandom();
	}
}

const AsebaNativeFunctionDescription AsebaNativeDescription_rand =
{
	"math.rand",
	"fill array with random values",
	{
		{ -1, "dest" },
		{ 0, 0 }
	}
};


/*@}*/
