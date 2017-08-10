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

#ifndef ASEBA_ZEROCONF_DASHELHUB
#define ASEBA_ZEROCONF_DASHELHUB

#include <dashel/dashel.h>
#include "zeroconf.h"

namespace Aseba
{
	//! Use the Dashel run loop to watch the DNS Service for updates, and trigger callback
	//! processing as necessary.
	//! Call dashelIncomingData and dashelConnectionClosed from the equivalent Dashel functions,
	//! and call dashelStep instead of Dashel::Hub::step(), in order to close the pendingReleaseStreams
	class DashelhubZeroconf : public Zeroconf
	{
	public:
		DashelhubZeroconf(Dashel::Hub& hub);

		// To be called from similar functions in Dashel::Hub
		void dashelIncomingData(Dashel::Stream * stream);
		void dashelConnectionClosed(Dashel::Stream * stream);
		// To be called instead the functions in Dashel::Hub
		bool dashelStep(int timeout = 0);

	protected:
		// From Zeroconf
		virtual void processServiceRef(DNSServiceRef serviceRef) override;
		virtual void releaseServiceRef(DNSServiceRef serviceRef) override;

	private:
		//! Collection of (tcp:) SocketStreams that watch the serviceref file descriptors.
		std::map<Dashel::Stream *, DNSServiceRef> zeroconfStreams;
		//! Reference to the hub to create connections from processDiscoveryRequest
		Dashel::Hub& hub;
		//! Streams that dashelStep() will call closeStream() on, after the current Hub::step() is finished
		std::map<Dashel::Stream *, DNSServiceRef> pendingReleaseStreams;
	};
}

#endif /* ASEBA_ZEROCONF_DASHELHUB */
