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

#ifndef ASEBA_SWITCH_MODULE
#define ASEBA_SWITCH_MODULE

#include <iostream>
#include "Arguments.h"

namespace Dashel
{
	class Stream;
}
namespace Aseba
{
	class Message;
	class Switch;
	
	class Module
	{
	public:
		//! return the name of this module
		virtual std::string name() const = 0;
		//! Dump a short description of this module and the list of arguments it eats
		virtual void dumpArgumentsDescription(std::ostream &stream) const = 0;
		//! Give the list of arguments this module can understand
		virtual ArgumentDescriptions describeArguments() const { return ArgumentDescriptions(); }
		//! Pass all parsed arguments to this module
		virtual void processArguments(Switch* asebaSwitch, const Arguments& arguments) {}
		
		// main features
		
		//! Notify the module of the creation of a new stream.
		//! Give this module the opportunity to take ownership of the newly-created stream by returning true
		virtual bool connectionCreated(Switch* asebaSwitch, Dashel::Stream * stream) { return false; }
		//! Read data on a stream that is handled by this module (was registered by Switch::delegateHandlingToModule)
		virtual void incomingData(Switch* asebaSwitch, Dashel::Stream * stream) {}
		//! A stream handled by this module has been closed
		virtual void connectionClosed(Switch* asebaSwitch, Dashel::Stream * stream) {}
		
		//! Give the module the possibility to handle an Aseba message, whose id is global
		virtual void processMessage(Switch* asebaSwitch, const Message& message) = 0;
	};
	
} // namespace Aseba

#endif // ASEBA_SWITCH_MODULE
