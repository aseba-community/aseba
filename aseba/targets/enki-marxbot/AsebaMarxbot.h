/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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

#ifndef __ENKI_ASEBA_MARXBOT_H
#define __ENKI_ASEBA_MARXBOT_H

#include <enki/robots/marxbot/Marxbot.h>
#ifndef ASEBA_ASSERT
#define ASEBA_ASSERT
#endif
#include "vm/vm.h"
#include "common/consts.h"
#include <dashel/dashel.h>
#include <deque>
#include <string.h>

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
	class AsebaMarxbot : public Marxbot, public Dashel::Hub
	{
	public:
		struct Event
		{
			unsigned short source;
			std::valarray<unsigned char> data;

			Event(unsigned short source, const uint8_t* data, uint16_t length) :
				source(source),
				data(length)
			{
				memcpy(&this->data[0], data, length);
			}

			Event(Dashel::Stream* stream)
			{
				uint16_t temp;
				uint16_t len;
				stream->read(&temp, 2);
				len = bswap16(temp);
				stream->read(&temp, 2);
				source = bswap16(temp);
				data.resize(len + 2);
				stream->read(&data[0], data.size());
			}
		};

		struct Module
		{
			Module();

			AsebaVMState vm;
			std::valarray<unsigned short> bytecode;
			std::valarray<signed short> stack;
			//std::deque<Event> events;

			std::deque<Event> events;
		};

	private:
		struct BaseVariables
		{
			int16_t id;
			int16_t source;
			int16_t args[32];
		};

		struct MotorVariables
		{
			int16_t id;
			int16_t source;
			int16_t args[32];
			int16_t speed;
			int16_t odo[2];
			int16_t user[220];
		};

		struct ProximitySensorVariables
		{
			int16_t id;
			int16_t source;
			int16_t args[32];
			int16_t bumpers[24];
			int16_t ground[12];
			int16_t user[188];
		};

		struct DistanceSensorVariables
		{
			int16_t id;
			int16_t source;
			int16_t args[32];
			int16_t distances[180];
			int16_t user[44];
		};

		MotorVariables leftMotorVariables;
		MotorVariables rightMotorVariables;
		ProximitySensorVariables proximitySensorVariables;
		DistanceSensorVariables distanceSensorVariables;

		Module leftMotor;
		Module rightMotor;
		Module proximitySensors;
		Module distanceSensors;

	public:
		// public because accessed from a glue function
		std::vector<Module *> modules;
		static int marxbotNumber;

	public:

		//! Constructor, connect to a host and register VMs
		AsebaMarxbot();
		//! Destructor, unregister VMs
		virtual ~AsebaMarxbot();
		//! In addition to DifferentialWheeled::step(), update aseba variables and initiate periodic events.
		virtual void controlStep(double dt);

		virtual void connectionCreated(Dashel::Stream *stream);
		virtual void incomingData(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);

		// this must be public because of bindings to C functions
		Dashel::Stream* stream;
		// all streams that must be disconnected at next step
		std::vector<Dashel::Stream*> toDisconnect;
	};

}
#endif

