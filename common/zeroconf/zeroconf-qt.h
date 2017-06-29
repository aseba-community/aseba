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

	signals:
		void zeroconfBrowseCompleted(); //!< emitted when browsing is completed
		void zeroconfRegisterCompleted(const Aseba::Zeroconf::TargetInformation &); //!< emitted when a register is completed
		void zeroconfResolveCompleted(const Aseba::Zeroconf::TargetInformation &); //!< emitted when a resolve is completed
		void zeroconfUpdateCompleted(const Aseba::Zeroconf::TargetInformation &); //!< emitted when an update is completed

	protected slots:
		void doIncoming(int socket);

	protected:
		// From Zeroconf
		virtual void registerCompleted(const Aseba::Zeroconf::Target & target) override;
		virtual void resolveCompleted(const Aseba::Zeroconf::Target & target) override;
		virtual void updateCompleted(const Aseba::Zeroconf::Target & target) override;
		virtual void browseCompleted() override;
		virtual void processDiscoveryRequest(DiscoveryRequest & zdr) override;

	protected:
		void incomingData(DiscoveryRequest & zdr);

	private:
		//! A discovery request and its associated QSocketNotifier to connect to Qt's event loop
		typedef std::pair<DiscoveryRequest &, QSocketNotifier*> ZdrQSocketNotifierPair;
		//! A collection of QSocketNotifier that watch the serviceref file descriptors.
		std::map<int, ZdrQSocketNotifierPair> zeroconfSockets;
	};

}

#endif /* ASEBA_ZEROCONF_QT */
