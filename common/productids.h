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

#ifndef __ASEBA_PIDS_H
#define __ASEBA_PIDS_H

/** \addtogroup common */
/*@{*/

/*! List of product identifiers */
/**
	Used for instance by studio for device-specific functions
	such as custom flasher protocol.
*/
typedef enum
{
	ASEBA_PID_UNDEFINED = 0,
	ASEBA_PID_CHALLENGE,
	ASEBA_PID_PLAYGROUND_EPUCK,
	ASEBA_PID_MARXBOT,
	ASEBA_PID_HANDBOT,
	ASEBA_PID_EPUCK,
	ASEBA_PID_SMARTROB,
	ASEBA_PID_SMARTROBASL,
	ASEBA_PID_THYMIO2
} AsebaProductIds;

/*! name of the product-id variable */
#define ASEBA_PID_VAR_NAME "_productId"

/*@}*/

#endif
