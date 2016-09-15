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

//#include <QCoreApplication>
#include <QSocketNotifier>
#include <QSignalMapper>
#include "zeroconf.h"

namespace Aseba
{
	//! Use the QT event loop to watch the DNS Service for updates, and triggers callback
	//! processing as necessary.
	class QtZeroconf : public QObject, public Zeroconf
	{
		Q_OBJECT

	public:
		void browse();
		void run();
		QtZeroconf();

	protected:
		//! Encapsulate ZeroconfDiscoveryRequest in a QObject
		class QtZeroconfDiscoveryRequest : public QObject, protected ZeroconfDiscoveryRequest
		{
			friend QtZeroconf;
			QSocketNotifier * notifier;
		};

		//! Set up function called after a discovery request has been made. The file
		//! descriptor associated with zdr.serviceref must be watched, to know when to
		//! call DNSServiceProcessResult, which in turn calls the callback that was
		//! registered with the discovery request.
		virtual void processDiscoveryRequest(ZeroconfDiscoveryRequest & zdr);
		virtual void incomingData(::Aseba::Zeroconf::ZeroconfDiscoveryRequest & zdr);

	protected slots:
		void doIncoming(int socket);

	private:
		//! Collection of QSocketNotifier that watch the serviceref file descriptors.
		//std::map<ZeroconfDiscoveryRequest &, std::unique_ptr<QSocketNotifier>> zeroconfSockets;
		std::map<int, ZeroconfDiscoveryRequest &> zeroconfSockets;
		QSignalMapper * zeroconfMapper;
	};
}
