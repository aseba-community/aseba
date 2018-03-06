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

#ifndef ASEBA_ZEROCONF_THREAD
#define ASEBA_ZEROCONF_THREAD

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unordered_set>
#include "zeroconf.h"

namespace Aseba
{
	//! Run a thread to watch the DNS Service for updates, and trigger callback
	//! processing as necessary.
	//! This class creates a thread processing zeroconf requests in background
	//! until this object is destroyed.
	//! Completion callbacks such as registeredCompleted, updateCompleted and
	//! targetFound are called within the background thread with the watcherLock held.
	class ThreadZeroconf : public Zeroconf
	{
	public:
		~ThreadZeroconf() override;

		void run();

	protected:
		// From Zeroconf
		void processServiceRef(DNSServiceRef serviceRef) override;
		void releaseServiceRef(DNSServiceRef serviceRef) override;

	protected:
		void handleDnsServiceEvents();

	private:
		// all the requests we are handling
		std::unordered_set<DNSServiceRef> serviceRefs; //!< service references to wait for activity using select
		std::unordered_set<DNSServiceRef> pendingReleaseServiceRefs; //!< service references to be released once select exits
		// threading support
		std::atomic_bool running{true}; //!< are we watching for DNS service updates?
		std::recursive_mutex watcherLock; //!< the lock for accessing zeroconfDRs
		std::exception_ptr watcherException{nullptr}; //!< pointer to rethrow exceptions in the outer thread
		std::thread watcher{&ThreadZeroconf::handleDnsServiceEvents, this}; //!< thread in which select loop occurs
		std::condition_variable_any threadWait; //!< notify that either the serviceRefs array has at least one element or that we should stop
	};
}

#endif /* ASEBA_ZEROCONF_THREAD */
