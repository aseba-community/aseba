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
	class DashelhubZeroconf : public Zeroconf, public Dashel::Hub
	{
	protected:
		// From Zeroconf
		virtual void processDiscoveryRequest(DiscoveryRequest & zdr) override;

		// From Dashel::Hub
		virtual void incomingData(Dashel::Stream *stream) override;
		virtual void connectionClosed(Dashel::Stream * stream, bool abnormal) override;

	private:
		//! Collection of (tcp:) SocketStreams that watch the serviceref file descriptors.
		std::map<Dashel::Stream *, std::reference_wrapper<DiscoveryRequest>> zeroconfStreams;
	};
}

#endif /* ASEBA_ZEROCONF_DASHELHUB */
