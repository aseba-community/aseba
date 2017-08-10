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
	Zeroconf::TargetInformation::TargetInformation(const std::string & name, const std::string & regtype, const std::string & domain):
		name(name),
		regtype(regtype),
		domain(domain)
	{}

	//! This target is described by a human-readable name and a port.
	//! If a port is nonzero at creation then this is a local target to be remembered
	//! otherwise it is a remote one discovered through browsing and may be refreshed.
	//! Note that later the port will become nonzero if the target is resolved.
	Zeroconf::TargetInformation::TargetInformation(const std::string & name, const int port) :
		name(name),
		port(port)
	{}

	//! This target describes an existing Dashel stream
	//! Raises Dashel::DashelException(Parameter missing: port) if not a tcp target
	Zeroconf::TargetInformation::TargetInformation(const Dashel::Stream* stream) :
		name("Aseba Local " + stream->getTargetParameter("port")),
		port(atoi(stream->getTargetParameter("port").c_str()))
	{}

	//! Are all fields equal?
	bool operator==(const Zeroconf::TargetInformation& lhs, const Zeroconf::TargetInformation& rhs)
	{
		return
			lhs.name == rhs.name &&
			lhs.domain == rhs.domain &&
			lhs.regtype == rhs.regtype
		;
	}

	//! Are all fields of this lower than fileds of that, in order?
	bool operator<(const Zeroconf::TargetInformation& lhs, const Zeroconf::TargetInformation& rhs)
	{
		return
			lhs.name < rhs.name &&
			lhs.domain < rhs.domain &&
			lhs.regtype < rhs.regtype
		;
	}

	//! This target is described by a human-readable name, regtype and domain.
	//! It corresponds to a remote target on which that we want to resolve
	Zeroconf::Target::Target(const std::string & name, const std::string & regtype, const std::string & domain, Zeroconf & container):
		Zeroconf::TargetInformation(name, regtype, domain),
		container(container)
	{}

	//! This target is described by a human-readable name and a port.
	//! It corresponds to a local target being advertised.
	Zeroconf::Target::Target(const std::string & name, const int port, Zeroconf & container):
		Zeroconf::TargetInformation(name, port),
		container(container)
	{}

	//! This target describes an existing Dashel stream
	//! It corresponds to a local target being advertised.
	//! Raises Dashel::DashelException(Parameter missing: port) if not a tcp target
	Zeroconf::Target::Target(const Dashel::Stream* dashelStream, Zeroconf & container):
		Zeroconf::TargetInformation(dashelStream),
		container(container)
	{}

	//! Destructor, release the serviceRef through the container
	Zeroconf::Target::~Target()
	{
		container.get().releaseServiceRef(serviceRef);
	}

	//! Ask the containing Zeroconf to indicate that this register is completed
	void Zeroconf::Target::registerCompleted() const
	{
		container.get().registerCompleted(*this);
	}

	//! Ask the containing Zeroconf to indicate this resolve is completed
	void Zeroconf::Target::updateCompleted() const
	{
		container.get().updateCompleted(*this);
	}

	//! Ask the containing Zeroconf to indicate that this target has been found
	void Zeroconf::Target::targetFound() const
	{
		container.get().targetFound(*this);
	}

	//! Are the target information and the container equals?
	bool operator==(const Zeroconf::Target& lhs, const Zeroconf::Target& rhs)
	{
		return static_cast<const Zeroconf::TargetInformation>(lhs) == static_cast<const Zeroconf::TargetInformation>(rhs) &&
			&lhs.container == &rhs.container;
	}

} // namespace Aseba
