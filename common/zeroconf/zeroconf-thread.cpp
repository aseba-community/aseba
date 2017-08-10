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

	//! Destructor, need to terminate the thread, clear all targets, and deallocate remaining service references
	ThreadZeroconf::~ThreadZeroconf()
	{
		running = false;
		watcher.join(); // tell watcher to stop, to avoid std::terminate
		// clear all targets
		targets.clear();
		// deallocate remaining service references
		for (auto serviceRef: serviceRefs)
			DNSServiceRefDeallocate(serviceRef);
		for (auto serviceRef: pendingReleaseServiceRefs)
			DNSServiceRefDeallocate(serviceRef);
	}

	//! Wait for the watcher thread to complete, rethrowing exceptions
	void ThreadZeroconf::run()
	{
		watcher.join();
		if (watcherException)
			std::rethrow_exception(watcherException);
	}

	// FIXME: updated doc
	//! Set up function called after a discovery request has been made. The file
	//! descriptor associated with zdr.serviceref must be watched, to know when to
	//! call DNSServiceProcessResult, which in turn calls the callback that was
	//! registered with the discovery request.
	void ThreadZeroconf::processServiceRef(DNSServiceRef serviceRef)
	{
		assert(serviceRef);

		std::lock_guard<std::recursive_mutex> locker(watcherLock);
		serviceRefs.insert(serviceRef);
	}

	void ThreadZeroconf::releaseServiceRef(DNSServiceRef serviceRef)
	{
		if (!serviceRef)
			return;

		std::lock_guard<std::recursive_mutex> locker(watcherLock);
		serviceRefs.erase(serviceRef);
		pendingReleaseServiceRefs.insert(serviceRef);
	}

	//! Run the handleDSEvents_thread
	void ThreadZeroconf::handleDnsServiceEvents()
	{
		struct timeval tv{1,0}; //!< maximum time to learn about a new service (1 sec)

		while (running)
		{
			{// lock
				std::lock_guard<std::recursive_mutex> locker(watcherLock);
				// FIXME: use condition variable to avoid infinite loops here
				if (serviceRefs.size() == 0)
					continue;
			}// unlock
			fd_set fds;
			int max_fds(0);
			FD_ZERO(&fds);
			std::map<DNSServiceRef,int> serviceFd;

			int fd_count(0);

			{// lock
				std::lock_guard<std::recursive_mutex> locker(watcherLock);
				for (auto serviceRef: serviceRefs)
				{
					int fd = DNSServiceRefSockFD(serviceRef);
					if (fd != -1)
					{
						max_fds = max_fds > fd ? max_fds : fd;
						FD_SET(fd, &fds);
						serviceFd[serviceRef] = fd;
						fd_count++;
					}
				}
			}// unlock
			int result = select(max_fds+1, &fds, (fd_set*)nullptr, (fd_set*)nullptr, &tv);
			try {
				if (result >= 0)
				{
					// lock
					std::lock_guard<std::recursive_mutex> locker(watcherLock);

					// release old service refs
					for (auto serviceRef: pendingReleaseServiceRefs)
						DNSServiceRefDeallocate(serviceRef);
					pendingReleaseServiceRefs.clear();

					// check for activity
					if (result > 0)
					{
						// no timeout
						for (auto serviceRef: serviceRefs)
						{
							auto fdIt(serviceFd.find(serviceRef));
							if (fdIt != serviceFd.end())
							{
								if (FD_ISSET(fdIt->second, &fds))
									DNSServiceProcessResult(serviceRef);
							}
						}
					}
					else
						; // timeout

					// unlock
				}
				else
					throw Zeroconf::Error(FormatableString("handleDnsServiceEvents: select returned %0 errno %1").arg(result).arg(errno));
			}
			catch (...) {
				watcherException = std::current_exception();
				break;
			}
		}
	}
} // namespace Aseba
