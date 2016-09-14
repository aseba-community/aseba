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

#include "../utils/utils.h"
#include "../utils/FormatableString.h"
#include "zeroconf-dashelhub.h"

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	void DashelhubZeroconf::processDiscoveryRequest(ZeroconfDiscoveryRequest & zdr)
	{
		int socket = DNSServiceRefSockFD(zdr.serviceref);
		if (socket != -1)
		{
			string dashelTarget = FormatableString("tcp:sock=%0").arg(socket);
			if (auto stream = connect(dashelTarget))
				zeroconfStreams[stream] = zdr.serviceref;
		}
	}

	void DashelhubZeroconf::browse()
	{
		Zeroconf::browse();
	}

	void DashelhubZeroconf::run()
	{
		Dashel::Hub::run();
	}

	void DashelhubZeroconf::run2s()
	{
		int timeout{2000};
		UnifiedTime startTime;
		while (timeout > 0)
		{
			if (!step(100))
				break;
			const UnifiedTime now;
			timeout -= (now - startTime).value;
			startTime = now;
		}
	}

	void DashelhubZeroconf::incomingData(Dashel::Stream *stream)
	{
		if (zeroconfStreams.find(stream) != zeroconfStreams.end())
		{
			auto serviceref = zeroconfStreams[stream];
			DNSServiceErrorType err = DNSServiceProcessResult(serviceref);
			if (err != kDNSServiceErr_NoError)
				throw Zeroconf::Error(FormatableString("DNSServiceProcessResult (service ref %1): error %0").arg(err).arg(serviceref));
		}
	}

} // namespace Aseba
