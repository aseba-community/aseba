/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
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

#ifndef __ENKI_ASEBA_MARXBOT_H
#define __ENKI_ASEBA_MARXBOT_H

#include <enki/robots/marxbot/Marxbot.h>
#ifndef ASEBA_ASSERT
#define ASEBA_ASSERT
#endif
#include "../vm/vm.h"
#include "../common/consts.h"
#include <dashel/streams.h>
#include <deque>

/*!	\file AsebaMarxbot.h
	\brief Header of the aseba-enabled marXbot robot
*/

namespace Enki
{
	
	//! The aseba-enabled version of the marXbot robot.
	/*! This robot provides a full simulation of event-based architecture
		as present on the real marXbot, using aseba.
		This robot provides a tcp server connection on port ASEBA_DEFAULT_PORT
		(currently 33333), or higher if other AsebaMarxbot already use it.
		This allows the connection of Aseba Studio to this robot.
		The feature is provided by inheriting from a Network server
		\ingroup robot
	*/
	class AsebaMarxbot : public Marxbot, public Streams::Client
	{
	private:
		struct Event
		{
			unsigned short id;
			std::vector<signed short> data;
		};
		
		struct Module
		{
			Module();
			
			AsebaVMState vm;
			std::valarray<unsigned short> bytecode;
			std::valarray<signed short> stack;
			std::deque<Event> eventsQueue;
			unsigned amountOfTimerEventInQueue;
		};
		
		struct BaseVariables
		{
			sint16 args[32];
		};
		
		struct MotorVariables
		{
			sint16 args[32];
			sint16 speed;
			sint16 odo[2];
			sint16 user[220];
		};
		
		struct ProximitySensorVariables
		{
			sint16 args[32];
			sint16 bumpers[24];
			sint16 ground[12];
			sint16 user[188];
		};
		
		struct DistanceSensorVariables
		{
			sint16 args[32];
			sint16 distances[180];
			sint16 user[44];
		};
		
		MotorVariables leftMotorVariables;
		MotorVariables rightMotorVariables;
		ProximitySensorVariables proximitySensorVariables;
		DistanceSensorVariables distanceSensorVariables;
		
		Module leftMotor;
		Module rightMotor;
		Module proximitySensors;
		Module distanceSensors;
		
		std::vector<Module *> modules;
		
	public:
		//! Constructor, connect to a host and register VMs
		AsebaMarxbot(const std::string &target = ASEBA_DEFAULT_TARGET);
		//! Destructor, unregister VMs
		virtual ~AsebaMarxbot();
		//! In addition to DifferentialWheeled::step(), update aseba variables and initiate periodic events.
		virtual void step(double dt);
		
		virtual void incomingData(Streams::Stream *stream);
		virtual void connectionClosed(Streams::Stream *stream);
	};

}
#endif

