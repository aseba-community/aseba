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

#ifndef __PLAYGROUND_DASHEL_ASEBA_GLUE_H
#define __PLAYGROUND_DASHEL_ASEBA_GLUE_H

#include "AsebaGlue.h"
#include "EnkiGlue.h"
#include <dashel/dashel.h>

#ifdef ZEROCONF_SUPPORT
#	include "../../common/zeroconf/zeroconf-qt.h"
#endif // ZEROCONF_SUPPORT

// Implementation of the connection using Dashel

namespace Aseba
{
	class SimpleDashelConnection : public RecvBufferNodeConnection, public Dashel::Hub
	{
	protected:
		Dashel::Stream* listenStream = nullptr;
		Dashel::Stream* stream = nullptr;
		std::vector<Dashel::Stream*> toDisconnect; // all streams that must be disconnected at next step

	public:
		SimpleDashelConnection(unsigned port);

		void sendBuffer(uint16_t nodeId, const uint8_t* data, uint16_t length) override;

		void connectionCreated(Dashel::Stream* stream) override;
		void incomingData(Dashel::Stream* stream) override;
		void connectionClosed(Dashel::Stream* stream, bool abnormal) override;

	protected:
		void clearBreakpoints();
		void closeOldStreams();
	};

} // namespace Aseba


// Implementations of robots using Dashel

namespace Enki
{
	template<typename AsebaRobot>
	class DashelConnected : public AsebaRobot, public Aseba::SimpleDashelConnection
	{
	public:
		template<typename... Params>
#ifdef ZEROCONF_SUPPORT
		DashelConnected(Aseba::Zeroconf& zeroconf, std::string robotTypeName, unsigned port, Params... parameters) :
			AsebaRobot(parameters...),
			Aseba::SimpleDashelConnection(port),
			zeroconf(zeroconf),
			robotTypeName(std::move(robotTypeName))
#else // ZEROCONF_SUPPORT
		DashelConnected(unsigned port, Params... parameters) :
			AsebaRobot(parameters...),
			Aseba::SimpleDashelConnection(port)
#endif // ZEROCONF_SUPPORT
		{
			Aseba::vmStateToEnvironment[&this->vm] =
				std::make_pair((Aseba::AbstractNodeGlue*)this, (Aseba::AbstractNodeConnection*)this);
#ifdef ZEROCONF_SUPPORT
			updateZeroconfStatus();
#endif // ZEROCONF_SUPPORT
		}

		virtual ~DashelConnected() { Aseba::vmStateToEnvironment.erase(&this->vm); }

	protected:
		// from AbstractNodeGlue

		virtual void externalInputStep(double dt) override
		{
			// do a network step, if there are some events from the network, they will be executed
			Hub::step();

			// disconnect old streams
			closeOldStreams();
		}

#ifdef ZEROCONF_SUPPORT
		Aseba::Zeroconf& zeroconf;
		std::string robotTypeName;

		void connectionCreated(Dashel::Stream* stream) override
		{
			Aseba::SimpleDashelConnection::connectionCreated(stream);
			updateZeroconfStatus();
		}

		void connectionClosed(Dashel::Stream* stream, bool abnormal) override
		{
			Aseba::SimpleDashelConnection::connectionClosed(stream, abnormal);
			updateZeroconfStatus();
		}

		void updateZeroconfStatus()
		{
			try
			{
				if (stream)
					zeroconf.forget(this->robotName, listenStream);
				else
					zeroconf.advertise(this->robotName,
						listenStream,
						{ ASEBA_PROTOCOL_VERSION, this->robotTypeName, stream != nullptr, { this->vm.nodeId }, {
							 static_cast<unsigned int>(this->variables.productId)
						 } });
			}
			catch (const std::runtime_error& e)
			{
				SEND_NOTIFICATION(LOG_ERROR,
					stream ? "Cannot de-advertise stream" : "Cannot advertise stream",
					listenStream->getTargetName(),
					e.what());
			}
		}
#endif // ZEROCONF_SUPPORT
	};

} // namespace Enki


#endif // __PLAYGROUND_DASHEL_ASEBA_GLUE_H
