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

#ifndef WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#include "../utils/FormatableString.h"
#include <dashel/dashel.h>
#include "zeroconf.h"

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Create a target in the Zeroconf container, described by a name and a port
	Zeroconf::Target& Zeroconf::insert(const std::string & name, const int & port)
	{
		targets.emplace_back(name, port);
		targets.back().container = this;
		return targets.back();
	}

	//! Create a target in the Zeroconf container, described by a Dashel stream
	Zeroconf::Target& Zeroconf::insert(const Dashel::Stream* dashelStream)
	{
		targets.emplace_back(dashelStream);
		targets.back().container = this;
		return targets.back();
	}

	//! Search the Zeroconf container by human-readable target name
	std::vector<Zeroconf::Target>::iterator Zeroconf::find(const std::string & name)
	{
		return find_if(
			targets.begin(),
			targets.end(),
			[&] (const Zeroconf::Target& t) { return t.name.find(name) == 0; }
		);
	}

	// Overideable method to wait for responses from the DNS service.
	// Basic behavior is to block until daemon replies.
	// The serviceref is released by callbacks.
	void Zeroconf::processDiscoveryRequest(DiscoveryRequest & zdr)
	{
		while (zdr.in_process) {
			DNSServiceErrorType err = DNSServiceProcessResult(zdr.serviceref);
			if (err != kDNSServiceErr_NoError)
				throw Zeroconf::Error(FormatableString("DNSServiceProcessResult: error %0").arg(err));
		}
	}

	//! A target can ask Zeroconf to register it in DNS, with additional information in a TXT record
	void Zeroconf::registerTarget(Zeroconf::Target * target, const TxtRecord& txtrec)
	{
		string txt{txtrec.record()};
		uint16_t len = txt.size();
		const char* record = txt.c_str();
		target->zdr.~DiscoveryRequest();
		auto err = DNSServiceRegister(&(target->zdr.serviceref),
					      0, // no flags
					      0, // default all interfaces
					      target->name.c_str(),
					      "_aseba._tcp",
					      nullptr, // use default domain, usually "local."
					      nullptr, // use this host name
					      htons(target->port),
					      len, // TXT length
					      record, // TXT record
					      cb_Register,
					      target); // context pointer is Zeroconf::Target object
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
			processDiscoveryRequest(target->zdr);
	}

	//! DNSSD callback for registerTarget, update Zeroconf::Target record with results of registration
	void DNSSD_API Zeroconf::cb_Register(DNSServiceRef sdRef,
					     DNSServiceFlags flags,
					     DNSServiceErrorType errorCode,
					     const char *name,
					     const char *regtype,
					     const char *domain,
					     void *context)
	{
		if (errorCode != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegisterReply: error %0").arg(errorCode));
		else
		{
			Zeroconf::Target *target = static_cast<Zeroconf::Target *>(context);

			target->name = name;
			target->domain = domain;
			target->zdr.in_process = false;
			target->registerCompleted();
		}
	}

	//! A target can ask Zeroconf to update its TXT record
	void Zeroconf::updateTarget(const Zeroconf::Target * target, const TxtRecord& txtrec)
	{
		const string rawdata{txtrec.record()};
		DNSServiceErrorType err = DNSServiceUpdateRecord(target->zdr.serviceref,
								 nullptr, // update primary TXT record
								 0, // no flags
								 rawdata.length(),
								 rawdata.c_str(),
								 0);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceUpdateRecord: error %0").arg(err));
		target->updateCompleted();
	}

	//! A remote target can ask Zeroconf to resolve its host name and port
	void Zeroconf::resolveTarget(Zeroconf::Target * target)
	{
		target->zdr.~DiscoveryRequest();
		auto err = DNSServiceResolve(&(target->zdr.serviceref),
						 0, // no flags
						 0, // default all interfaces
						 target->name.c_str(),
						 target->regtype.c_str(),
						 target->domain.c_str(),
						 cb_Resolve,
						 target);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceQueryRecord: error %0").arg(err));
		else
			processDiscoveryRequest(target->zdr);
	}

	//! DNSSD callback for resolveTarget, update Zeroconf::Target record with results of lookup
	void DNSSD_API Zeroconf::cb_Resolve(DNSServiceRef sdRef,
					    DNSServiceFlags flags,
					    uint32_t interfaceIndex,
					    DNSServiceErrorType errorCode,
					    const char * fullname,
					    const char * hosttarget,
					    uint16_t port,
					    uint16_t txtLen,
					    const unsigned char * txtRecord,
					    void *context)
	{
		if (errorCode != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegisterReply: error %0").arg(errorCode));
		else
		{
			Zeroconf::Target *target = static_cast<Zeroconf::Target *>(context);

			target->host = hosttarget;
			target->port = ntohs(port);
			Aseba::Zeroconf::TxtRecord tnew{ txtRecord,txtLen };
			target->properties.clear();
			for (auto const & field: tnew)
				target->properties[field.first] = field.second;
			target->properties["fullname"] = string(fullname);
			target->zdr.in_process = false;
			target->resolveCompleted();
		}
	}

	//! Update the set of known targets with remote ones found by browsing DNS
	void Zeroconf::browse()
	{
		browseAlreadyCompleted = false;
		// remove previously discovered targets
		targets.erase(std::remove_if(targets.begin(),targets.end(),
					     [](const Target& t) { return ! t.local; }),
			      targets.end());
		browseZDR.~DiscoveryRequest();
		auto err = DNSServiceBrowse(&browseZDR.serviceref,
					    0, // no flags
					    0, // default all interfaces
					    "_aseba._tcp",
					    nullptr, // use default domain, usually "local."
					    cb_Browse,
					    this); // context pointer is this Zeroconf object
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
			processDiscoveryRequest(browseZDR);
	}

	//! DNSSD callback for browse, update DiscoveryRequest record with results of browse
	void DNSSD_API Zeroconf::cb_Browse(DNSServiceRef sdRef,
					   DNSServiceFlags flags,
					   uint32_t interfaceIndex,
					   DNSServiceErrorType errorCode,
					   const char *name,
					   const char *regtype,
					   const char *domain,
					   void *context)
	{
		if (errorCode != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceBrowseReply: error %0").arg(errorCode));
		else
		{
			Zeroconf *zref = static_cast<Zeroconf *>(context);
			if (zref->browseAlreadyCompleted)
			{
				throw Zeroconf::Error(FormatableString("DNSServiceBrowseReply: %0 arrived after browseCompleted()").arg(zref));
			}
			if (flags & kDNSServiceFlagsAdd)
			{
				auto it = zref->find(name);
				auto& target = (it == zref->targets.end()) ? zref->insert(string(name), 0) : *it; // since port==0 at creation it will be marked as nonlocal
				target.properties = { {"name",string(name)}, {"domain",string(domain)} };
			}
			if ( ! (flags & kDNSServiceFlagsMoreComing))
			{
				zref->browseZDR.in_process = false;
				zref->browseAlreadyCompleted = true;
				zref->browseCompleted();
			}
		}
	}

} // namespace Aseba
