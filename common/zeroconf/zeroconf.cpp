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
		targets.push_back(Target(name, port, *this));
		return targets.back();
	}

	//! Create a target in the Zeroconf container, described by a Dashel stream
	Zeroconf::Target& Zeroconf::insert(const Dashel::Stream* dashelStream)
	{
		targets.push_back(Target(dashelStream, *this));
		return targets.back();
	}

	//! Search the Zeroconf container by human-readable target name
	Zeroconf::Targets::iterator Zeroconf::find(const std::string & name)
	{
		return find_if(
			targets.begin(),
			targets.end(),
			[&] (const Zeroconf::Target& t) { return t.name.find(name) == 0; }
		);
	}
	
	// Release the service reference
	Zeroconf::DiscoveryRequest::~DiscoveryRequest()
	{   
		if (serviceRef)
			DNSServiceRefDeallocate(serviceRef);
		serviceRef = 0;
	}
	
	//! Are the serviceRef equals?
	bool operator==(const Zeroconf::DiscoveryRequest& lhs, const Zeroconf::DiscoveryRequest& rhs)
	{
		return lhs.serviceRef == rhs.serviceRef;
	}

	//! A target can ask Zeroconf to register it in DNS, with additional information in a TXT record
	void Zeroconf::registerTarget(Zeroconf::Target & target, const TxtRecord& txtrec)
	{
		string txt{txtrec.record()};
		uint16_t len = txt.size();
		const char* record = txt.c_str();
		target.zdr.~DiscoveryRequest();
		auto err = DNSServiceRegister(&(target.zdr.serviceRef),
					      0, // no flags
					      0, // default all interfaces
					      target.name.c_str(),
					      "_aseba._tcp",
					      nullptr, // use default domain, usually "local."
					      nullptr, // use this host name
					      htons(target.port),
					      len, // TXT length
					      record, // TXT record
					      cb_Register,
					      &target); // context pointer is Zeroconf::Target object
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
			processDiscoveryRequest(target.zdr);
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
			target->regtype = regtype;
			target->registerCompleted();
		}
	}

	//! A target can ask Zeroconf to update its TXT record
	void Zeroconf::updateTarget(const Zeroconf::Target & target, const TxtRecord& txtrec)
	{
		const string rawdata{txtrec.record()};
		DNSServiceErrorType err = DNSServiceUpdateRecord(target.zdr.serviceRef,
								 nullptr, // update primary TXT record
								 0, // no flags
								 rawdata.length(),
								 rawdata.c_str(),
								 0);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceUpdateRecord: error %0").arg(err));
		target.updateCompleted();
	}

	//! A remote target can ask Zeroconf to resolve its host name and port
	void Zeroconf::resolveTarget(const std::string & name, const std::string & regtype, const std::string & domain)
	{
		auto target(new Target(name, regtype, domain, *this));
		auto err = DNSServiceResolve(&(target->zdr.serviceRef),
						 0, // no flags
						 0, // default all interfaces
						 name.c_str(),
						 regtype.c_str(),
						 domain.c_str(),
						 cb_Resolve,
						 target);
		if (err != kDNSServiceErr_NoError)
		{
			delete target; // because DNSServiceResolve will NOT call cb_Resolve
			throw Zeroconf::Error(FormatableString("DNSServiceQueryRecord: error %0").arg(err));
		}
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
		Zeroconf::Target *target = static_cast<Zeroconf::Target *>(context);
		if (errorCode != kDNSServiceErr_NoError)
		{
			delete target;
			throw Zeroconf::Error(FormatableString("DNSServiceRegisterReply: error %0").arg(errorCode));
		}
		else
		{
			target->host = hosttarget;
			target->port = ntohs(port);
			Aseba::Zeroconf::TxtRecord tnew{ txtRecord,txtLen };
			target->properties.clear();
			for (auto const & field: tnew)
				target->properties[field.first] = field.second;
			target->properties["fullname"] = string(fullname);
			target->resolveCompleted();
			delete target;
		}
	}

	//! Update the set of known targets with remote ones found by browsing DNS.
	void Zeroconf::browse()
	{
		releaseDiscoveryRequest(browseZDR);
		browseZDR.~DiscoveryRequest();
		auto err = DNSServiceBrowse(&browseZDR.serviceRef,
					    0, // no flags
					    0, // default all interfaces
					    "_aseba._tcp",
					    nullptr, // use default domain, usually "local."
					    cb_Browse,
					    this
		);
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
			Zeroconf *zeroconf = static_cast<Zeroconf *>(context);
			if (flags & kDNSServiceFlagsAdd)
			{
				zeroconf->resolveTarget(name, regtype, domain);
			}
		}
	}

} // namespace Aseba
