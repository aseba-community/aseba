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
	QtZeroconf::QtZeroconf()
	{
		zeroconfMapper = new QSignalMapper();
		//QObject::connect(zeroconfMapper, SIGNAL(mapped(::Aseba::Zeroconf::ZeroconfDiscoveryRequest &)), this, SLOT(incomingData(::Aseba::Zeroconf::ZeroconfDiscoveryRequest &)));
	}
	
	void QtZeroconf::processDiscoveryRequest(ZeroconfDiscoveryRequest & zdr)
	{
		int socket = DNSServiceRefSockFD(zdr.serviceref);
		if (socket != -1)
		{
			auto qzdr = dynamic_cast<QtZeroconfDiscoveryRequest*>(&zdr);
			if (! qzdr->notifier)
				qzdr->notifier = new QSocketNotifier{socket, QSocketNotifier::Read, this};
			QObject::connect(qzdr->notifier, SIGNAL(activated(int)), this, SLOT(doIncoming(int)));
			zeroconfSockets[socket] = zdr;
		}
	}

	void QtZeroconf::browse()
	{
		Zeroconf::browse();
	}

	void QtZeroconf::run()
	{
	}

	void QtZeroconf::doIncoming(int socket)
	{
		incomingData(zeroconfSockets.at(socket));
	}

	void QtZeroconf::incomingData(ZeroconfDiscoveryRequest & zdr)
	{
		DNSServiceErrorType err = DNSServiceProcessResult(zdr.serviceref);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceProcessResult (service ref %1): error %0").arg(err).arg(zdr.serviceref));
	}


} // namespace Aseba
