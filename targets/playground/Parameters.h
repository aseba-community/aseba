/*
	Playground - An active arena to learn multi-robots programming
	Copyright (C) 1999--2013:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
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

#ifndef __PLAYGROUND_PARAMETERS_H
#define __PLAYGROUND_PARAMETERS_H

// Parameters for Playground

#define LEGO_RED						Enki::Color(0.77, 0.2, 0.15)
#define LEGO_GREEN						Enki::Color(0, 0.5, 0.17)
#define LEGO_BLUE						Enki::Color(0, 0.38 ,0.61)
#define LEGO_WHITE						Enki::Color(0.9, 0.9, 0.9)

#define EPUCK_FEEDER_INITIAL_ENERGY		10
#define EPUCK_FEEDER_THRESHOLD_HIDE		2
#define EPUCK_FEEDER_THRESHOLD_SHOW		4
#define EPUCK_FEEDER_RADIUS				5
#define EPUCK_FEEDER_RANGE				10

#define EPUCK_FEEDER_COLOR_ACTIVE		LEGO_GREEN
#define EPUCK_FEEDER_COLOR_INACTIVE		LEGO_WHITE

#define EPUCK_FEEDER_D_ENERGY			4
#define EPUCK_FEEDER_RECHARGE_RATE		0.5
#define EPUCK_FEEDER_MAX_ENERGY			100

#define EPUCK_FEEDER_HEIGHT				5

#define EPUCK_INITIAL_ENERGY			10
#define EPUCK_ENERGY_CONSUMPTION_RATE	1

#define SCORE_MODIFIER_COEFFICIENT		0.2

#define INITIAL_POOL_ENERGY				10

#define ACTIVATION_OBJECT_COLOR			LEGO_RED
#define ACTIVATION_OBJECT_HEIGHT		6

#define DEATH_ANIMATION_STEPS			30

#endif // __PLAYGROUND_PARAMETERS_H
