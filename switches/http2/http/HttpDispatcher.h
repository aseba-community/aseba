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

#ifndef ASEBA_SWITCH_DISPATCHER_HTTP
#define ASEBA_SWITCH_DISPATCHER_HTTP

#include <functional>
#include "../Module.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../../../common/utils/utils.h"

namespace Aseba
{
	class HttpDispatcher: public Module
	{
	public:
		typedef std::function<HttpResponse(Switch*, const HttpRequest&, const strings&)> Handler;
		// TODO: think about SSE handling architecture
		
	public:
		HttpDispatcher();
		
		virtual std::string name() const;
		virtual void dumpArgumentsDescription(std::ostream &stream) const;
		virtual ArgumentDescriptions describeArguments() const;
		virtual void processArguments(Switch* asebaSwitch, const Arguments& arguments);
		
		virtual bool connectionCreated(Switch* asebaSwitch, Dashel::Stream * stream);
		virtual void incomingData(Switch* asebaSwitch, Dashel::Stream * stream);
		virtual void connectionClosed(Switch* asebaSwitch, Dashel::Stream * stream);
		virtual void processMessage(Switch* asebaSwitch, const Message& message);
		
		void registerHandler(const strings& uriPath, Handler handler);
	
	protected:
		unsigned serverPort;
		
		std::map<strings, Handler> handlers;
	};
}

#endif // ASEBA_SWITCH_DISPATCHER_HTTP
