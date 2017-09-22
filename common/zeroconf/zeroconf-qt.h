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
		void zeroconfRegisterCompleted(const Aseba::Zeroconf::TargetInformation &); //!< emitted when a register is completed
		void zeroconfUpdateCompleted(const Aseba::Zeroconf::TargetInformation &); //!< emitted when an update is completed
		void zeroconfTargetFound(const Aseba::Zeroconf::TargetInformation &); //!< emitted when a target is resolved

	public:
		~QtZeroconf() override;

	protected slots:
		void doIncoming(int socket);

	protected:
		// From Zeroconf
		void registerCompleted(const Aseba::Zeroconf::TargetInformation & target) override;
		void updateCompleted(const Aseba::Zeroconf::TargetInformation & target) override;
		void targetFound(const Aseba::Zeroconf::TargetInformation & target) override;

		void processServiceRef(DNSServiceRef serviceRef) override;
		void releaseServiceRef(DNSServiceRef serviceRef) override;

	protected:
		void incomingData(DNSServiceRef serviceRef);

	private:
		struct ServiceRefSocketNotifier;
		//! A collection of QSocketNotifier that watch the serviceref file descriptors.
		std::map<int, ServiceRefSocketNotifier*> zeroconfSockets;
	};

	//! Extends a Qt socket notifier with the associated service reference
	struct QtZeroconf::ServiceRefSocketNotifier: QSocketNotifier
	{
		ServiceRefSocketNotifier(DNSServiceRef serviceRef, QObject *parent);
		~ServiceRefSocketNotifier() override;
		DNSServiceRef serviceRef; //!< the associated service reference
	};
}

#endif /* ASEBA_ZEROCONF_QT */
