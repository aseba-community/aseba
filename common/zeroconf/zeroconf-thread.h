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

#ifndef ASEBA_ZEROCONF_THREAD
#define ASEBA_ZEROCONF_THREAD

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>
#include "zeroconf.h"

namespace Aseba
{
	//! Run a thread to watch the DNS Service for updates, and trigger callback
	//! processing as necessary.
	//! This class creates a thread processing zeroconf requests in background
	//! until this object is destroyed.
	//! Completion callbacks such as browseCompleted are called within the background
	//! thread with the watcherLock held.
	class ThreadZeroconf : public Zeroconf
	{
	public:
		virtual ~ThreadZeroconf() override;

		void run();

	protected:
		// From Zeroconf
		virtual void processDiscoveryRequest(DiscoveryRequest & zdr) override;
		virtual void releaseDiscoveryRequest(DiscoveryRequest & zdr) override;

	protected:
		void handleDnsServiceEvents();

	private:
		//! all the requests we are handling
		std::unordered_set<std::reference_wrapper<DiscoveryRequest>> zeroconfDRs;
		// threading support
		std::atomic_bool running{true}; //!< are we watching for DNS service updates?
		std::recursive_mutex watcherLock; //!< the lock for accessing zeroconfDRs
		std::exception_ptr watcherException{nullptr}; //!< pointer to rethrow exceptions in the outer thread
		std::thread watcher{&ThreadZeroconf::handleDnsServiceEvents, this}; //!< thread in which select loop occurs
	};
}

#endif /* ASEBA_ZEROCONF_THREAD */
