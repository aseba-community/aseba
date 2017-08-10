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

#include "../consts.h"
#include "../utils/utils.h"
#include "../utils/FormatableString.h"
#include "zeroconf-dashelhub.h"

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Constructor, taking a reference to a Dashel::Hub
	DashelhubZeroconf::DashelhubZeroconf(Hub& hub):
		hub(hub)
	{}

	// TODO: add destructor to delete all remaining references + browseServiceRef

	//! Set up function called after a discovery request has been made. The file
	//! descriptor associated with zdr.serviceref must be watched, to know when to
	//! call DNSServiceProcessResult, which in turn calls the callback that was
	//! registered with the discovery request.
	void DashelhubZeroconf::processServiceRef(DNSServiceRef serviceRef)
	{
		assert(serviceRef);

		int socket = DNSServiceRefSockFD(serviceRef);
		if (socket != -1)
		{
			string dashelTarget = FormatableString("tcppoll:sock=%0").arg(socket);
			auto stream = hub.connect(dashelTarget);
			assert(stream);
			zeroconfStreams.emplace(stream, serviceRef);
		}
	}

	void DashelhubZeroconf::releaseServiceRef(DNSServiceRef serviceRef)
	{
		if (!serviceRef)
			return;

		int socket = DNSServiceRefSockFD(serviceRef);
		assert (socket != -1);

		auto streamRequestIt(zeroconfStreams.begin());
		while (streamRequestIt != zeroconfStreams.end())
		{
			if (streamRequestIt->second == serviceRef)
			{
				pendingReleaseStreams[streamRequestIt->first] = streamRequestIt->second;
				streamRequestIt = zeroconfStreams.erase(streamRequestIt);
			}
			else
				++streamRequestIt;
		}
	}

	//! Check if data were coming on one of our streams, and if so process
	void DashelhubZeroconf::dashelIncomingData(Dashel::Stream * stream)
	{
		if (zeroconfStreams.find(stream) != zeroconfStreams.end())
		{
			auto serviceRef = zeroconfStreams.at(stream);
			DNSServiceErrorType err = DNSServiceProcessResult(serviceRef);
			if (err != kDNSServiceErr_NoError)
				throw Zeroconf::Error(FormatableString("DNSServiceProcessResult (service ref %1): error %0").arg(err).arg(serviceRef));
		}
	}

	//! Check if one of our streams was disconnected, and if so take note
	void DashelhubZeroconf::dashelConnectionClosed(Stream * stream)
	{
		zeroconfStreams.erase(stream);
	}

	bool DashelhubZeroconf::dashelStep(int timeout)
	{
		bool ret(hub.step(timeout));
		for (auto& streamKV: pendingReleaseStreams)
		{
			hub.closeStream(streamKV.first);
			DNSServiceRefDeallocate(streamKV.second);
		}
		pendingReleaseStreams.clear();
		return ret;
	}

} // namespace Aseba
