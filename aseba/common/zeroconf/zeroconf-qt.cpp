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

#include <QSocketNotifier>
#include <QSignalMapper>
#include "../utils/FormatableString.h"
#include "zeroconf-qt.h"
#include "dns_sd.h"

using namespace std;

namespace Aseba
{
	//! Destructor, clear all targets
	QtZeroconf::~QtZeroconf()
	{
		// clear all targets
		targets.clear();
		// release browse serviceRef
		releaseServiceRef(browseServiceRef);
	}

	//! Create a socket notifier for the provided service reference, and start watching
	//! its file descriptor for activity.
	void QtZeroconf::processServiceRef(DNSServiceRef serviceRef)
	{
		assert(serviceRef);

		auto notifier = new ServiceRefSocketNotifier(serviceRef, this);
		QObject::connect(notifier, SIGNAL(activated(int)), this, SLOT(doIncoming(int)));
		zeroconfSockets.emplace(notifier->socket(), notifier);
	}

	//! Remove the socket notifier associated with the provided service reference,
	//! and later deallocate it (through Qt's deleteLater mechanism).
	void QtZeroconf::releaseServiceRef(DNSServiceRef serviceRef)
	{
		if (!serviceRef)
			return;

		// safety check
		int socket = DNSServiceRefSockFD(serviceRef);
		assert(socket != -1);

		// remove the notifier and delete it later
		auto socketNotifierIt(zeroconfSockets.find(socket));
		if (socketNotifierIt != zeroconfSockets.end())
		{
			socketNotifierIt->second->deleteLater();
			zeroconfSockets.erase(socketNotifierIt);
		}
	}

	//! Emit signal when register completed.
	//! If you override this method you take responsibility for emitting signals as you see fit.
	void QtZeroconf::registerCompleted(const Aseba::Zeroconf::TargetInformation & target)
	{
		emit zeroconfRegisterCompleted(target);
	}
	//! Emit signal when update completed.
	//! If you override this method you take responsibility for emitting signals as you see fit.
	void QtZeroconf::updateCompleted(const Aseba::Zeroconf::TargetInformation & target)
	{
		emit zeroconfUpdateCompleted(target);
	}

	//! Emit signal when resolve completed.
	//! If you override this method you take responsibility for emitting signals as you see fit.
	void QtZeroconf::targetFound(const Aseba::Zeroconf::TargetInformation & target)
	{
		emit zeroconfTargetFound(target);
	}

	//! Emit signal when target is removed.
	//! If you override this method you take responsibility for emitting signals as you see fit.
	void QtZeroconf::targetRemoved(const Aseba::Zeroconf::TargetInformation & target)
	{
		emit zeroconfTargetRemoved(target);
	}

	//! Data are available on a zeroconf socket
	void QtZeroconf::doIncoming(int socket)
	{
		auto streamIt(zeroconfSockets.find(socket));
		if (streamIt != zeroconfSockets.end())
			incomingData(streamIt->second->serviceRef);
	}

	//! Process incoming data associated to a service reference
	void QtZeroconf::incomingData(DNSServiceRef serviceRef)
	{
		DNSServiceErrorType err = DNSServiceProcessResult(serviceRef);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceProcessResult (service ref %1): error %0").arg(err).arg(serviceRef));
	}

	//! Create a Qt socket notifier in read mode for the file descriptor of the provided service reference
	QtZeroconf::ServiceRefSocketNotifier::ServiceRefSocketNotifier(DNSServiceRef serviceRef, QObject *parent):
		QSocketNotifier(DNSServiceRefSockFD(serviceRef), QSocketNotifier::Read, parent),
		serviceRef(serviceRef)
	{ }

	//! Deallocate the service reference
	QtZeroconf::ServiceRefSocketNotifier::~ServiceRefSocketNotifier()
	{
		DNSServiceRefDeallocate(serviceRef);
	}

} // namespace Aseba
