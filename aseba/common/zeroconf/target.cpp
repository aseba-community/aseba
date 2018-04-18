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

#include <dashel/dashel.h>

#include <utility>
#include "zeroconf.h"
#include "common/utils/FormatableString.h"

using namespace std;

namespace Aseba {
//! Return the hash of the key of target
std::size_t Zeroconf::TargetInformationKeyHash::operator()(TargetInformation const& target) const noexcept {
    auto nameHash = std::hash<std::string>{}(target.name);
    auto regTypeHash = std::hash<std::string>{}(target.regtype);
    auto domainHash = std::hash<std::string>{}(target.domain);
    return nameHash ^ regTypeHash ^ domainHash;
}

//! Return whether the keys of lhs and rhs are equal
bool Zeroconf::TargetInformationKeyCompareEqual::operator()(const TargetInformation& lhs,
                                                            const TargetInformation& rhs) const {
    return static_cast<const TargetKey&>(lhs) == static_cast<const TargetKey&>(rhs);
};

//! Two target keys are equal if all their members are equal
bool operator==(const Zeroconf::TargetKey& lhs, const Zeroconf::TargetKey& rhs) {
    return lhs.name == rhs.name && lhs.regtype == rhs.regtype && lhs.domain == rhs.domain;
}

//! This target is described by a human-readable name, regtype, and domain.
//! It corresponds to a remote target on which that we want to resolve
Zeroconf::TargetInformation::TargetInformation(std::string name, std::string regtype, std::string domain)
    : Zeroconf::TargetKey{std::move(name), std::move(regtype), std::move(domain)} {}

//! This target is described by a human-readable name and a port.
//! It corresponds to a local target being advertised.
Zeroconf::TargetInformation::TargetInformation(std::string name, const int port)
    : Zeroconf::TargetKey{std::move(name)}, port(port) {}

//! This target describes an existing Dashel stream.
//! It corresponds to a local target being advertised.
//! Raises Dashel::DashelException(Parameter missing: port) if not a tcp target.
Zeroconf::TargetInformation::TargetInformation(std::string name, const Dashel::Stream* stream)
    : Zeroconf::TargetKey{std::move(name)}, port(atoi(stream->getTargetParameter("port").c_str())) {}

//! Return a valid Dashel target string
std::string Zeroconf::TargetInformation::dashel() const {
    return FormatableString("tcp:%0;port=%1").arg(host).arg(port);
}

//! Assign this->serviceRef to rhs.serviceRef and set the later to nullptr, and move other fields.
Zeroconf::Target::Target(Target&& rhs)
    : TargetInformation(std::move(rhs)), state(std::move(rhs.state)), container(std::move(rhs.container)) {
    serviceRef = rhs.serviceRef;
    rhs.serviceRef = nullptr;
}

//! Assign this->serviceRef to rhs.serviceRef and set the later to nullptr, and move other fields.
Zeroconf::Target& Zeroconf::Target::operator=(Target&& rhs) {
    if(serviceRef)
        container.get().releaseServiceRef(serviceRef);
    TargetInformation::operator=(std::move(rhs));
    state = std::move(rhs.state);
    container = std::move(rhs.container);
    serviceRef = rhs.serviceRef;
    rhs.serviceRef = nullptr;
    return *this;
}

//! This target is described by a human-readable name, regtype and domain.
//! It corresponds to a remote target on which that we want to resolve.
Zeroconf::Target::Target(std::string name, std::string regtype, std::string domain, Zeroconf& container)
    : Zeroconf::TargetInformation(std::move(name), std::move(regtype), std::move(domain)), container(container) {}

//! This target is described by a human-readable name and a port.
//! It corresponds to a local target being advertised.
Zeroconf::Target::Target(std::string name, const int port, Zeroconf& container)
    : Zeroconf::TargetInformation(std::move(name), port), container(container) {}

//! This target describes an existing Dashel stream
//! It corresponds to a local target being advertised.
//! Raises Dashel::DashelException(Parameter missing: port) if not a tcp target
Zeroconf::Target::Target(std::string name, const Dashel::Stream* dashelStream, Zeroconf& container)
    : Zeroconf::TargetInformation(std::move(name), dashelStream), container(container) {}

//! Destructor, release the serviceRef through the container,
//! which thus must be a valid object at that time.
Zeroconf::Target::~Target() {
    container.get().releaseServiceRef(serviceRef);
}

//! Ask the containing Zeroconf to indicate that this register is completed
void Zeroconf::Target::registerCompleted() const {
    container.get().registerCompleted(*this);
}

//! Ask the containing Zeroconf to indicate this resolve is completed
void Zeroconf::Target::updateCompleted() const {
    container.get().updateCompleted(*this);
}

//! Ask the containing Zeroconf to indicate that this target has been found
void Zeroconf::Target::targetFound() const {
    container.get().targetFound(*this);
}

}  // namespace Aseba
