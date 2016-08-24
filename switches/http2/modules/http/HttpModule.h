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

#ifndef ASEBA_SWITCH_MODULE_HTTP
#define ASEBA_SWITCH_MODULE_HTTP

#include "../../Module.h"

namespace Aseba
{
	class HttpModule: public Module
	{
	public:
		HttpModule();
		
		virtual std::string name() const;
		virtual void dumpArgumentsDescription(std::ostream &stream) const;
		virtual ArgumentDescriptions describeArguments() const;
		virtual void processArguments(Switch* asebaSwitch, const Arguments& arguments);
		
		virtual bool connectionCreated(Dashel::Stream * stream);
		virtual void incomingData(Dashel::Stream * stream);
		virtual void connectionClosed(Dashel::Stream * stream);
		virtual void processMessage(const Message& message);
	
	protected:
		unsigned serverPort;
	};
}

#endif // ASEBA_SWITCH_MODULE_HTTP
