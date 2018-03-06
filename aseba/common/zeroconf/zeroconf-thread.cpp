/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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
#include "dns_sd.h"
#ifdef WIN32
#include <winsock2.h>
#endif

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Destructor, need to terminate the thread, clear all targets, and deallocate remaining service references
	ThreadZeroconf::~ThreadZeroconf()
	{
		running = false;
		threadWait.notify_one();
		watcher.join(); // tell watcher to stop
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

	//! With the lock held, insert the provided service reference to serviceRefs
	void ThreadZeroconf::processServiceRef(DNSServiceRef serviceRef)
	{
		assert(serviceRef);
		{
			std::lock_guard<std::recursive_mutex> locker(watcherLock);
			serviceRefs.insert(serviceRef);
		}
		threadWait.notify_one();
	}

	//! With the lock held, move the provided service reference from serviceRefs to
	//! pendingReleaseServiceRefs, so that it will be released after the current call to select has completed.
	void ThreadZeroconf::releaseServiceRef(DNSServiceRef serviceRef)
	{
		if (!serviceRef)
			return;

		std::lock_guard<std::recursive_mutex> locker(watcherLock);
		serviceRefs.erase(serviceRef);
		pendingReleaseServiceRefs.insert(serviceRef);
	}

	//! When serviceRefs is not empty, for each service reference collect their associated
	//! file descriptor and wait on them for activity using select.
	void ThreadZeroconf::handleDnsServiceEvents()
	{
		struct timeval tv{1,0}; //!< maximum time to learn about a new service (1 sec)

		while (true)
		{
			{// lock
				std::unique_lock<std::recursive_mutex> locker(watcherLock);
				// we use a condition variable to avoid infinite loops here
				threadWait.wait(locker, [&] { return !serviceRefs.empty() || !running; });
			}// unlock
			if (!running)
				break;
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
