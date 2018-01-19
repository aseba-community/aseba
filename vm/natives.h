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

#ifndef __ASEBA_NATIVES_H
#define __ASEBA_NATIVES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../common/types.h"
#include "vm.h"

#ifdef _MSC_VER
#	pragma warning(disable : 4200) // disable 0 size arrays warning
#	define inline __inline // msvc does not support the standart C99/C++ static inline construct
#endif

	/**
	\file natives.h
	Definition of standard natives functions for Aseba Virtual Machine
*/

	/** \addtogroup vm */
	/*@{*/

	// data structures

	/*! Description of a variable */
	typedef struct
	{
		uint16_t size; /*!< size of the variable */
		const char* name; /*!< name of the variable */
	} AsebaVariableDescription;

	/*! Description of all variable */
	typedef struct
	{
		const char* name; /*!< name of the microcontroller */
		AsebaVariableDescription variables[]; /*!< named variables, terminated by a variable of size 0 */
	} AsebaVMDescription;

	/*! Description of a local event */
	typedef struct
	{
		const char* name; /*!< name of the function */
		const char* doc; /*!< documentation of the local event */
	} AsebaLocalEventDescription;

	/*! Signature of a native function */
	typedef void (*AsebaNativeFunctionPointer)(AsebaVMState* vm);

	/*! Description of an argument of a native function */
	typedef struct
	{
		int16_t size; /*!< size of the argument in number of values; if negative, template parameter */
		const char* name; /*!< name of the argument */
	} AsebaNativeFunctionArgumentDescription;

	/*! Description of a native function */
	typedef struct
	{
		const char* name; /*!< name of the function */
		const char* doc; /*!< documentation of the function */
		AsebaNativeFunctionArgumentDescription arguments[]; /*!< arguments, terminated by an element of size 0 */
	} AsebaNativeFunctionDescription;

	// support functions

	/*! Return an argument on the stack, including the value of template parameters */
	static inline int16_t AsebaNativePopArg(AsebaVMState* vm) { return vm->stack[vm->sp--]; }

	// standard natives functions

	/*! Function to copy a vector */
	void AsebaNative_veccopy(AsebaVMState* vm);
	/*! Description of AsebaNative_veccopy */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_veccopy;

	/*! Function to fill all the elements of a vector to a specific value*/
	void AsebaNative_vecfill(AsebaVMState* vm);
	/*! Description of AsebaNative_vecfill */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecfill;

	/*! Function to add a scalar to each element of a vector */
	void AsebaNative_vecaddscalar(AsebaVMState* vm);
	/*! Description of AsebaNative_vecaddscalar */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecaddscalar;

	/*! Function to add two vectors */
	void AsebaNative_vecadd(AsebaVMState* vm);
	/*! Description of AsebaNative_vecadd */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecadd;

	/*! Function to substract two vectors */
	void AsebaNative_vecsub(AsebaVMState* vm);
	/*! Description of AsebaNative_vecsub */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecsub;

	/*! Function to multiply two vectors elements by elements */
	void AsebaNative_vecmul(AsebaVMState* vm);
	/*! Description of AsebaNative_vecadd */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecmul;

	/*! Function to divide two vectors elements by elements  */
	void AsebaNative_vecdiv(AsebaVMState* vm);
	/*! Description of AsebaNative_vecsub */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecdiv;

	/*! Function to take the element by element minimum */
	void AsebaNative_vecmin(AsebaVMState* vm);
	/*! Description of AsebaNative_vecmin */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecmin;

	/*! Function to take the element by element maximum */
	void AsebaNative_vecmax(AsebaVMState* vm);
	/*! Description of AsebaNative_vecsmax */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecmax;

	/*! Function to clamp a vector of values element by element */
	void AsebaNative_vecclamp(AsebaVMState* vm);
	/*! Description of AsebaNative_vecclamp*/
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecclamp;

	/*! Function to perform a dot product on a vector */
	void AsebaNative_vecdot(AsebaVMState* vm);
	/*! Description of AsebaNative_vecdot */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecdot;

	/*! Function to perform statistics on a vector */
	void AsebaNative_vecstat(AsebaVMState* vm);
	/*! Description of AsebaNative_vecstat */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecstat;

	/*! Function to get indices of the bounds of a vector */
	void AsebaNative_vecargbounds(AsebaVMState* vm);
	/*! Description of AsebaNative_vecargbounds */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecargbounds;

	/*! Function to sort a vector */
	void AsebaNative_vecsort(AsebaVMState* vm);
	/*! Description of AsebaNative_vecsort */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecsort;

	/*! Function to perform dest = (a*b)/c in 32 bits */
	void AsebaNative_mathmuldiv(AsebaVMState* vm);
	/*! Description of AsebaNative_mathmuldiv */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_mathmuldiv;

	/*! Function to perform atan2 */
	void AsebaNative_mathatan2(AsebaVMState* vm);
	/*! Description of AsebaNative_mathatan2 */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_mathatan2;

	/*! Function to perform sin */
	void AsebaNative_mathsin(AsebaVMState* vm);
	/*! Description of AsebaNative_mathsin */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_mathsin;

	/*! Function to perform cos */
	void AsebaNative_mathcos(AsebaVMState* vm);
	/*! Description of AsebaNative_mathcos */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_mathcos;

	/*! Function to perform the rotation of a vector */
	void AsebaNative_mathrot2(AsebaVMState* vm);
	/*! Description of AsebaNative_mathrot2 */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_mathrot2;

	/*! Function to perform sqrt */
	void AsebaNative_mathsqrt(AsebaVMState* vm);
	/*! Description of AsebaNative_mathsqrt */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_mathsqrt;

	/*! Function to get the middle index of the largest sequence of non-zero elements */
	void AsebaNative_vecnonzerosequence(AsebaVMState* vm);
	/*! Description of AsebaNative_vecnonzerosequence */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_vecnonzerosequence;

	/*! Functon to set the seed of random generator */
	void AsebaSetRandomSeed(uint16_t seed);
	/*! Functon to get a random number */
	uint16_t AsebaGetRandom(void);
	/*! Function to get a 16-bit signed random number */
	void AsebaNative_rand(AsebaVMState* vm);
	/*! Description of AsebaNative_rand */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_rand;

	/*! Function that reports size of deque dest in size */
	void AsebaNative_deqsize(AsebaVMState* vm);
	/*! Description of AsebaNative_deqsize */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqsize;
	/*! Function that copies index-th len-dest elements of deque src to dest */
	void AsebaNative_deqget(AsebaVMState* vm);
	/*! Description of AsebaNative_deqget */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqget;
	/*! Function that copies len-src elements of src to index-th position of deque dest */
	void AsebaNative_deqset(AsebaVMState* vm);
	/*! Description of AsebaNative_deqset */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqset;
	/*! Function that copies len-src elements of src before index-th position of deque dest */
	void AsebaNative_deqinsert(AsebaVMState* vm);
	/*! Description of AsebaNative_deqinsert */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqinsert;
	/*! Function that erases len elements from deque dest starting from index-th position */
	void AsebaNative_deqerase(AsebaVMState* vm);
	/*! Description of AsebaNative_deqerase */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqerase;
	/*! Function that copies len-src elements of src before the front of deque dest */
	void AsebaNative_deqpushfront(AsebaVMState* vm);
	/*! Description of AsebaNative_deqpushfront */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqpushfront;
	/*! Function that copies len-src elements of src after the end of deque dest */
	void AsebaNative_deqpushback(AsebaVMState* vm);
	/*! Description of AsebaNative_deqpushback */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqpushback;
	/*! Function that erases len elements from deque dest starting at the front */
	void AsebaNative_deqpopfront(AsebaVMState* vm);
	/*! Description of AsebaNative_deqpopfront */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqpopfront;
	/*! Function that erases len elements from deque dest starting from the end */
	void AsebaNative_deqpopback(AsebaVMState* vm);
	/*! Description of AsebaNative_deqpopback */
	extern const AsebaNativeFunctionDescription AsebaNativeDescription_deqpopback;

/*! Embedded targets must know the size of ASEBA_NATIVES_STD_FUNCTIONS without having to compute them by hand, please update this when adding a new function */
#define ASEBA_NATIVES_STD_COUNT 30

/*! snippet to include standard native functions */
#define ASEBA_NATIVES_STD_FUNCTIONS                                                                                    \
	AsebaNative_veccopy, AsebaNative_vecfill, AsebaNative_vecaddscalar, AsebaNative_vecadd, AsebaNative_vecsub,        \
		AsebaNative_vecmul, AsebaNative_vecdiv, AsebaNative_vecmin, AsebaNative_vecmax, AsebaNative_vecclamp,          \
		AsebaNative_vecdot, AsebaNative_vecstat, AsebaNative_vecargbounds, AsebaNative_vecsort,                        \
		AsebaNative_mathmuldiv, AsebaNative_mathatan2, AsebaNative_mathsin, AsebaNative_mathcos, AsebaNative_mathrot2, \
		AsebaNative_mathsqrt, AsebaNative_rand, AsebaNative_deqsize, AsebaNative_deqget, AsebaNative_deqset,           \
		AsebaNative_deqinsert, AsebaNative_deqerase, AsebaNative_deqpushfront, AsebaNative_deqpushback,                \
		AsebaNative_deqpopfront, AsebaNative_deqpopback

/*! snippet to include descriptions of standard native functions */
#define ASEBA_NATIVES_STD_DESCRIPTIONS                                                                             \
	&AsebaNativeDescription_veccopy, &AsebaNativeDescription_vecfill, &AsebaNativeDescription_vecaddscalar,        \
		&AsebaNativeDescription_vecadd, &AsebaNativeDescription_vecsub, &AsebaNativeDescription_vecmul,            \
		&AsebaNativeDescription_vecdiv, &AsebaNativeDescription_vecmin, &AsebaNativeDescription_vecmax,            \
		&AsebaNativeDescription_vecclamp, &AsebaNativeDescription_vecdot, &AsebaNativeDescription_vecstat,         \
		&AsebaNativeDescription_vecargbounds, &AsebaNativeDescription_vecsort, &AsebaNativeDescription_mathmuldiv, \
		&AsebaNativeDescription_mathatan2, &AsebaNativeDescription_mathsin, &AsebaNativeDescription_mathcos,       \
		&AsebaNativeDescription_mathrot2, &AsebaNativeDescription_mathsqrt, &AsebaNativeDescription_rand,          \
		&AsebaNativeDescription_deqsize, &AsebaNativeDescription_deqget, &AsebaNativeDescription_deqset,           \
		&AsebaNativeDescription_deqinsert, &AsebaNativeDescription_deqerase, &AsebaNativeDescription_deqpushfront, \
		&AsebaNativeDescription_deqpushback, &AsebaNativeDescription_deqpopfront, &AsebaNativeDescription_deqpopback

	/*@}*/

#ifdef __cplusplus
} /* closing brace for extern "C" */
#	endif

#endif
