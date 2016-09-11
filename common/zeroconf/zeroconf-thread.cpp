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
#include <dashel/dashel.h>
#include "zeroconf-thread.h"

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	ThreadZeroconf::~ThreadZeroconf()
	{
		running = false;
		watcher.join(); // tell watcher to stop, to avoid std::terminate
	}

	void ThreadZeroconf::processDiscoveryRequest(ZeroconfDiscoveryRequest & zdr)
	{
		zeroconfDRs.insert(&zdr);
	}

	void ThreadZeroconf::eraseDiscoveryRequest(ZeroconfDiscoveryRequest & zdr)
	{
		zeroconfDRs.erase(zeroconfDRs.find(&zdr));
	}

	void ThreadZeroconf::handleDnsServiceEvents()
	{
		struct timeval tv{1,0}; //!< maximum time to learn about a new service (5 sec)

		while (running)
		{
			if (zeroconfDRs.size() == 0)
			{
				continue;
			}
			fd_set fds;
			int max_fds(0);
			FD_ZERO(&fds);
			std::map<DNSServiceRef,int> serviceFd;

			int fd_count(0);

			for (auto const& zdr: zeroconfDRs)
			{
				int fd = DNSServiceRefSockFD(zdr->serviceref);
				if (fd != -1)
				{
					max_fds = max_fds > fd ? max_fds : fd;
					FD_SET(fd, &fds);
					serviceFd[zdr->serviceref] = fd;
					fd_count++;
				}
			}
			int result = select(max_fds+1, &fds, (fd_set*)NULL, (fd_set*)NULL, &tv);
			if (result > 0)
			{
				for (auto const& zdr: zeroconfDRs)
					if (FD_ISSET(serviceFd[zdr->serviceref], &fds))
						DNSServiceProcessResult(zdr->serviceref);
			}
			else if (result < 0)
				throw Zeroconf::Error(FormatableString("handleDnsServiceEvents: select returned %0 errno %1").arg(result).arg(errno));
			else
				; // timeout, check for new services
		}
	}
} // namespace Aseba
