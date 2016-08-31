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

#include "HttpDispatcher.h"
#include "../Globals.h"
#include "../Switch.h"


namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	void HttpDispatcher::getEventsHandler(HandlerContext& context)
	{
		Switch* asebaSwitch(context.asebaSwitch);
		Stream* stream(context.stream);
		const PathTemplateMap &filledPathTemplates(context.filledPathTemplates);
		
		const auto nameIt(filledPathTemplates.find("name"));
		if (nameIt != filledPathTemplates.end())
		{
			// check if name exists
			const wstring name(UTF8ToWString(nameIt->second));
			if (asebaSwitch->commonDefinitions.events.contains(name))
			{
				// if name exists, subscribe
				LOG_VERBOSE << "HTTP | Registered stream " << stream->getTargetName() << " for SSE on specific Aseba event " << nameIt->second << endl;
				eventStreams.emplace(nameIt->second, stream);
				HttpResponse::createSSE().send(stream);
			}
			else
			{
				// otherwise produce an error
				LOG_VERBOSE << "HTTP | Stream " << stream->getTargetName() << " requested SSE registration on Aseba event " << nameIt->second << ", which does not exist" << endl;
				HttpResponse::fromPlainString(FormatableString("Event %0 does not exist").arg(nameIt->second), HttpStatus::NOT_FOUND).send(stream);
			}
		}
		else
		{
			LOG_VERBOSE << "HTTP | Registered stream " << stream->getTargetName() << " for SSE on all Aseba events" << endl;
			eventStreams.emplace("", stream);
			HttpResponse::createSSE().send(stream);
		}
	}
	
} // namespace Aseba
