/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
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

#ifndef __ASEBA_NATIVES_H
#define __ASEBA_NATIVES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../common/types.h"
#include "vm.h"

/**
	\file natives.h
	Definition of standard natives functions for Aseba Virtual Machine
*/

/** \addtogroup vm */
/*@{*/


/*! Description of a variable */
typedef struct
{
	uint16 size;			/*!< size of the variable */
	const char* name;		/*!< name of the variable */
} AsebaVariableDescription;

/*! Description of all variable */
typedef struct
{
	const char* name;		/*!< name of the microcontroller */	
	AsebaVariableDescription variables[];	/*!< named variables, terminated by a variable of size 0 */
} AsebaVMDescription;

/*! Description of a local event */
typedef struct
{
	const char* name;	/*!< name of the function */
	const char* doc;	/*!< documentation of the local event */
} AsebaLocalEventDescription;

/*! Signature of a native function */
typedef void (*AsebaNativeFunctionPointer)(AsebaVMState *vm);

/*! Description of an argument of a native function */
typedef struct
{
	sint16 size;		/*!< size of the argument in number of values; if negative, template parameter */
	const char* name;	/*!< name of the argument */
} AsebaNativeFunctionArgumentDescription;

/*! Description of a native function */
typedef struct 
{
	const char* name;	/*!< name of the function */
	const char* doc;	/*!< documentation of the function */
	AsebaNativeFunctionArgumentDescription arguments[];	/*!< arguments, terminated by an element of size 0 */
} AsebaNativeFunctionDescription;

// standard natives functions

/*! Function to fill all the elements of a vector to a specific value*/
void AsebaNative_vecfill(AsebaVMState *vm);
/*! Description of AsebaNative_vecfill */
extern AsebaNativeFunctionDescription AsebaNativeDescription_vecfill;

/*! Function to copy a vector */
void AsebaNative_veccopy(AsebaVMState *vm);
/*! Description of AsebaNative_veccopy */
extern AsebaNativeFunctionDescription AsebaNativeDescription_veccopy;

/*! Function to add two vectors */
void AsebaNative_vecadd(AsebaVMState *vm);
/*! Description of AsebaNative_vecadd */
extern AsebaNativeFunctionDescription AsebaNativeDescription_vecadd;

/*! Function to substract two vectors */
void AsebaNative_vecsub(AsebaVMState *vm);
/*! Description of AsebaNative_vecsub */
extern AsebaNativeFunctionDescription AsebaNativeDescription_vecsub;

/*! Function to perform a dot product on a vector */
void AsebaNative_vecdot(AsebaVMState *vm);
/*! Description of AsebaNative_vecdot */
extern AsebaNativeFunctionDescription AsebaNativeDescription_vecdot;

/*! Function to perform statistics on a vector */
void AsebaNative_vecstat(AsebaVMState *vm);
/*! Description of AsebaNative_vecstat */
extern AsebaNativeFunctionDescription AsebaNativeDescription_vecstat;

/*@}*/

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
