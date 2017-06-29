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

	//! Destructor, need to terminate the thread
	ThreadZeroconf::~ThreadZeroconf()
	{
		running = false;
		watcher.join(); // tell watcher to stop, to avoid std::terminate
	}

	//! Wait for the watcher thread to complete, rethrowing exceptions
	void ThreadZeroconf::run()
	{
		watcher.join();
		if (watcherException)
			std::rethrow_exception(watcherException);
	}

	//! Set up function called after a discovery request has been made. The file
	//! descriptor associated with zdr.serviceref must be watched, to know when to
	//! call DNSServiceProcessResult, which in turn calls the callback that was
	//! registered with the discovery request.
	void ThreadZeroconf::processDiscoveryRequest(DiscoveryRequest & zdr)
	{
		std::lock_guard<std::recursive_mutex> locker(watcherLock);
		zeroconfDRs.insert(zdr);
	}

	//! Run the handleDSEvents_thread
	void ThreadZeroconf::handleDnsServiceEvents()
	{
		struct timeval tv{1,0}; //!< maximum time to learn about a new service (1 sec)

		while (running)
		{
			{// lock
				std::lock_guard<std::recursive_mutex> locker(watcherLock);
				if (zeroconfDRs.size() == 0)
					continue;
			}// unlock
			fd_set fds;
			int max_fds(0);
			FD_ZERO(&fds);
			std::map<DNSServiceRef,int> serviceFd;

			int fd_count(0);

			{// lock
				std::lock_guard<std::recursive_mutex> locker(watcherLock);
				for (auto const& zdrRef: zeroconfDRs)
				{
					DiscoveryRequest& zdr(zdrRef.get());
					int fd = DNSServiceRefSockFD(zdr.serviceRef);
					if (fd != -1)
					{
						max_fds = max_fds > fd ? max_fds : fd;
						FD_SET(fd, &fds);
						serviceFd[zdr.serviceRef] = fd;
						fd_count++;
					}
				}
			}// unlock
			int result = select(max_fds+1, &fds, (fd_set*)nullptr, (fd_set*)nullptr, &tv);
			try {
				if (result > 0)
				{
					// lock
					std::lock_guard<std::recursive_mutex> locker(watcherLock);

					for (auto const& zdrRef: zeroconfDRs)
					{
						DiscoveryRequest& zdr(zdrRef.get());
						if (FD_ISSET(serviceFd[zdr.serviceRef], &fds))
							DNSServiceProcessResult(zdr.serviceRef);
					}
					// unlock
				}
				else if (result < 0)
					throw Zeroconf::Error(FormatableString("handleDnsServiceEvents: select returned %0 errno %1").arg(result).arg(errno));

				else
					; // timeout, check for new services
			}
			catch (...) {
				watcherException = std::current_exception();
				break;
			}
		}
	}
} // namespace Aseba
