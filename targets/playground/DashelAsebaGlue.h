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
#include "Thymio2.h"
#include "EPuck.h"
#include <dashel/dashel.h>

// Implementation of the connection using Dashel

namespace Aseba
{
	class SimpleDashelConnection: public RecvBufferNodeConnection, public Dashel::Hub
	{
		Dashel::Stream* stream;
		std::vector<Dashel::Stream*> toDisconnect; // all streams that must be disconnected at next step

	public:
		SimpleDashelConnection(unsigned port);
		
		virtual void sendBuffer(uint16 nodeId, const uint8* data, uint16 length);
		
		virtual void connectionCreated(Dashel::Stream *stream);
		virtual void incomingData(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
		
		void closeOldStreams();
	};
	
} // namespace Aseba


// Implementations of robots using Dashel

namespace Enki
{
	template<typename AsebaRobot>
	class DashelConnected: public AsebaRobot, public Aseba::SimpleDashelConnection
	{
	public:
		template<typename... Params>
		DashelConnected(unsigned port, Params... parameters):
			AsebaRobot(parameters...),
			Aseba::SimpleDashelConnection(port)
		{
			Aseba::vmStateToEnvironment[&this->vm] = std::make_pair((Aseba::AbstractNodeGlue*)this, (Aseba::AbstractNodeConnection *)this);
		}
		
		virtual ~DashelConnected()
		{
			Aseba::vmStateToEnvironment.erase(&this->vm);
		}
		
	protected:
		
		// from Aseba::SimpleDashelConnection
		
		virtual void externalInputStep(double dt)
		{
			// do a network step, if there are some events from the network, they will be executed
			Hub::step();
			
			// disconnect old streams
			closeOldStreams();
		}
	};
	
	typedef DashelConnected<AsebaThymio2> DashelAsebaThymio2;
	typedef DashelConnected<AsebaFeedableEPuck> DashelAsebaFeedableEPuck;
	
} // namespace Enki


#endif // __PLAYGROUND_DASHEL_ASEBA_GLUE_H
