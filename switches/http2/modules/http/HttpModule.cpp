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

#include "HttpModule.h"
#include "../../Switch.h"

namespace Aseba
{
	HttpModule::HttpModule(Switch* asebaSwitch)
	{
		asebaSwitch->registerModuleNotification(this);
	}
		
	void HttpModule::dumpArgumentsDescription(std::ostream &stream) const
	{
		stream << "  HTTP module, provides access to the network from a web application\n";
		stream << "    -w, --http      : listens to incoming HTTP connections on this port (default: 3000)\n";
	}
	
	ArgumentDescriptions HttpModule::describeArguments() const
	{
		return {
			{ "-w", 1 },
			{ "--http", 1 }
		};
	}
	
	void HttpModule::incomingData(Dashel::Stream * stream)
	{
		
	}
	
	void HttpModule::connectionClosed(Dashel::Stream * stream)
	{
		
	}
	
	void HttpModule::processMessage(const Message& message)
	{
		
	}
	
} // namespace Aseba
