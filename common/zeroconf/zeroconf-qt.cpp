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

#include <QSocketNotifier>
#include <QSignalMapper>
#include "../utils/FormatableString.h"
#include "zeroconf-qt.h"

using namespace std;

namespace Aseba
{
	//! Set up function called after a discovery request has been made. The file
	//! descriptor associated with zdr.serviceref must be watched, to know when to
	//! call DNSServiceProcessResult, which in turn calls the callback that was
	//! registered with the discovery request.
	void QtZeroconf::processDiscoveryRequest(DiscoveryRequest & zdr)
	{
		int socket = DNSServiceRefSockFD(zdr.serviceRef);
		if (socket != -1)
		{
			auto notifier = new QSocketNotifier(socket, QSocketNotifier::Read, this);
			QObject::connect(notifier, SIGNAL(activated(int)), this, SLOT(doIncoming(int)));
			zeroconfSockets.emplace(socket, ZdrQSocketNotifierPair{zdr, notifier});
		}
	}

	void QtZeroconf::releaseDiscoveryRequest(DiscoveryRequest & zdr)
	{
		if (!zdr.serviceRef)
			return;
		int socket = DNSServiceRefSockFD(zdr.serviceRef);
		if (socket != -1)
		{
			auto socketNotifierIt(zeroconfSockets.find(socket));
			if (socketNotifierIt != zeroconfSockets.end())
			{
				socketNotifierIt->second.second->deleteLater();
				zeroconfSockets.erase(socketNotifierIt);
			}
		}
	}

	//! Emit signal when register completed.
	//! If you override this method you take responsibility for emitting signals as you see fit.
	void QtZeroconf::registerCompleted(const Aseba::Zeroconf::Target & target)
	{
		emit zeroconfRegisterCompleted(target);
	}

	//! Emit signal when resolve completed.
	//! If you override this method you take responsibility for emitting signals as you see fit.
	void QtZeroconf::targetFound(const Aseba::Zeroconf::Target & target)
	{
		emit zeroconfTargetFound(target);
	}

	//! Emit signal when update completed.
	//! If you override this method you take responsibility for emitting signals as you see fit.
	void QtZeroconf::updateCompleted(const Aseba::Zeroconf::Target & target)
	{
		emit zeroconfUpdateCompleted(target);
	}

	//! Data are available on a zeroconf socket
	void QtZeroconf::doIncoming(int socket)
	{
		incomingData(zeroconfSockets.at(socket).first);
	}

	//! Process incoming data associated to a discovery request
	void QtZeroconf::incomingData(DiscoveryRequest & zdr)
	{
		DNSServiceErrorType err = DNSServiceProcessResult(zdr.serviceRef);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceProcessResult (service ref %1): error %0").arg(err).arg(zdr.serviceRef));
	}

} // namespace Aseba
