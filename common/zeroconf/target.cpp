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

#include <dashel/dashel.h>
#include "zeroconf.h"

using namespace std;

namespace Aseba
{
	//! This target is described by a human-readable name and a port.
	//! If a port is nonzero at creation then this is a local target to be remembered
	//! otherwise it is a remote one discovered through browsing and may be refreshed.
	//! Note that later the port will become nonzero if the target is resolved.
	Zeroconf::TargetInformation::TargetInformation(const std::string & name, const int port) :
	name(name),
	port(port),
	local(port != 0)
	{}

	//! This target describes an existing Dashel stream
	//! Raises Dashel::DashelException(Parameter missing: port) if not a tcp target
	Zeroconf::TargetInformation::TargetInformation(const Dashel::Stream* stream) :
	name("Aseba Local " + stream->getTargetParameter("port")),
	port(atoi(stream->getTargetParameter("port").c_str()))
	{}

	//! Ask the containing Zeroconf to register a target with the DNS service, now that its description is complete
	void Zeroconf::Target::advertise(const TxtRecord& txtrec)
	{
		container->registerTarget(this, txtrec);
	}

	//! Ask the containing Zeroconf to replace this target's TXT record with a new one, typically when node, pid lists change
	void Zeroconf::Target::updateTxtRecord(const TxtRecord& txtrec)
	{
		container->updateTarget(this, txtrec);
	}

	//! Ask the containing Zeroconf to resolve this target's actual host name and port, using mDNS-SD
	void Zeroconf::Target::resolve()
	{
		container->resolveTarget(this);
	}

} // namespace Aseba
