/*
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

#ifndef __PLAYGROUND_DIRECT_ASEBA_GLUE_H
#define __PLAYGROUND_DIRECT_ASEBA_GLUE_H

#include <memory>
#include <queue>
#include "../../common/msg/msg.h"
#include "../../transport/buffer/vm-buffer.h"
#include "AsebaGlue.h"

// Implementation of the connection using direct connection

namespace Aseba
{
	class DirectConnection : public RecvBufferNodeConnection
	{
	public:
		typedef std::queue<std::unique_ptr<Message>> MessageQueue;
		MessageQueue inQueue;
		MessageQueue outQueue;

	public:
		virtual void sendBuffer(uint16_t nodeId, const uint8_t* data, uint16_t length);
	};

} // namespace Aseba


// Implementations of robots using Dashel

namespace Enki
{
	template<typename AsebaRobot>
	class DirectlyConnected : public AsebaRobot, public Aseba::DirectConnection
	{
	public:
		template<typename... Params>
		DirectlyConnected(Params... parameters) : AsebaRobot(parameters...)
		{
			Aseba::vmStateToEnvironment[&this->vm] =
				std::make_pair((Aseba::AbstractNodeGlue*)this, (Aseba::AbstractNodeConnection*)this);
		}

		virtual ~DirectlyConnected() { Aseba::vmStateToEnvironment.erase(&this->vm); }

	protected:
		// from AbstractNodeGlue

		virtual void externalInputStep(double dt)
		{
			while (!inQueue.empty())
			{
				// serialize message into reception buffer
				const auto message(inQueue.front().get());
				lastMessageSource = message->source;
				Aseba::Message::SerializationBuffer content;
				message->serializeSpecific(content);
				lastMessageData.resize(content.rawData.size() + 2);
				*reinterpret_cast<uint16_t*>(&lastMessageData[0]) = message->type;
				std::copy(&content.rawData[0], &content.rawData[content.rawData.size()], &lastMessageData[2]);

				// execute event on all VM that are linked to this connection
				for (auto vmStateToEnvironmentKV : Aseba::vmStateToEnvironment)
				{
					if (vmStateToEnvironmentKV.second.second == this)
					{
						AsebaProcessIncomingEvents(vmStateToEnvironmentKV.first);
						AsebaVMRun(vmStateToEnvironmentKV.first, 1000);
					}
				}

				// delete message
				inQueue.pop();
			}
		}
	};
} // namespace Enki


#endif // __PLAYGROUND_DIRECT_ASEBA_GLUE_H
