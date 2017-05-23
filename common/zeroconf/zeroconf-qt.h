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

#ifndef ASEBA_ZEROCONF_QT
#define ASEBA_ZEROCONF_QT

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

	signals:
		void zeroconfBrowseCompleted(); //!< emitted when browsing is completed
		void zeroconfRegisterCompleted(const Aseba::Zeroconf::TargetInformation&); //!< emitted when a register is completed
		void zeroconfResolveCompleted(const Aseba::Zeroconf::TargetInformation&); //!< emitted when a resolve is completed
		void zeroconfUpdateCompleted(const Aseba::Zeroconf::TargetInformation&); //!< emitted when an update is completed

	protected:
		//! Set up function called after a discovery request has been made. The file
		//! descriptor associated with zdr.serviceref must be watched, to know when to
		//! call DNSServiceProcessResult, which in turn calls the callback that was
		//! registered with the discovery request.
		virtual void processDiscoveryRequest(DiscoveryRequest & zdr);
		virtual void incomingData(DiscoveryRequest & zdr);

		//! Emit signal when register completed. If you override this method you take responsibility for emitting signals as you see fit.
		void registerCompleted(const Aseba::Zeroconf::Target * target)
		{
			emit zeroconfRegisterCompleted(*target);
		}
		//! Emit signal when resolve completed. If you override this method you take responsibility for emitting signals as you see fit.
		void resolveCompleted(const Aseba::Zeroconf::Target * target)
		{
			emit zeroconfResolveCompleted(*target);
		}
		//! Emit signal when update completed. If you override this method you take responsibility for emitting signals as you see fit.
		void updateCompleted(const Aseba::Zeroconf::Target * target)
		{
			emit zeroconfUpdateCompleted(*target);
		}
		//! Emit signal when browse completed. If you override this method you take responsibility for emitting signals as you see fit.
		void browseCompleted()
		{
			emit zeroconfBrowseCompleted();
		}

	protected slots:
		void doIncoming(int socket);

	private:
		//! Collection of QSocketNotifier that watch the serviceref file descriptors.
		typedef std::pair<DiscoveryRequest &, QSocketNotifier*> ZdrQSocketNotifierPair;
		std::map<int, ZdrQSocketNotifierPair> zeroconfSockets;

	};

}

#endif /* ASEBA_ZEROCONF_QT */
