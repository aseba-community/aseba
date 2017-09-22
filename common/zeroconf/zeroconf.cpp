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
#include "dns_sd.h"

using namespace std;

namespace Aseba
{
	using namespace Dashel;

	//! Callbacks for the DNS Service Discovery API, in a class to make it friend with Zeroconf
	//! while not exposing the #include "dns_sd.h" in zeroconf.h
	struct ZeroconfCallbacks
	{
		static void DNSSD_API cb_Register(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context);
		static void DNSSD_API cb_Browse(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context);
		static void DNSSD_API cb_Resolve(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, /* In network byte order */ uint16_t txtLen, const unsigned char *txtRecord, void *context);
	};

	//! Advertise a target with a given name and port, with associated txtrec.
	//! If the target already exists, its txtrec is updated.
	void Zeroconf::advertise(const std::string & name, const int & port, const TxtRecord & txtrec)
	{
		auto targetIt(getTarget(name, port));
		if (targetIt == targets.end())
		{
			// Target does not exist, create
			targets.emplace_back(name, port, *this);
			registerTarget(targets.back(), txtrec);
		}
		else
		{
			// Target does exist, update
			updateTarget(*targetIt, txtrec);
		}
	}

	//! Advertise a target for a given Dashel stream, with associated txtrec.
	//! If the target already exists, its txtrec is updated.
	void Zeroconf::advertise(const std::string & name, const Dashel::Stream * stream, const TxtRecord & txtrec)
	{
		auto targetIt(getTarget(stream));
		if (targetIt == targets.end())
		{
			// Target does not exist, create
			targets.emplace_back(name, stream, *this);
			registerTarget(targets.back(), txtrec);
		}
		else
		{
			// Target does exist, update
			updateTarget(*targetIt, txtrec);
		}
	}

	//! Forget a target with a given name and port, if the target is unknown, do nothing.
	void Zeroconf::forget(const std::string & name, const int & port)
	{
		auto targetIt(getTarget(name, port));
		if (targetIt != targets.end())
			targets.erase(targetIt);
	}

	//! Forget a target for a given Dashel stream, if the target is unknown, do nothing.
	void Zeroconf::forget(const Dashel::Stream * stream)
	{
		auto targetIt(getTarget(stream));
		if (targetIt != targets.end())
			targets.erase(targetIt);
	}

	//! A target can ask Zeroconf to register it in DNS, with additional information in a TXT record
	void Zeroconf::registerTarget(Zeroconf::Target & target, const TxtRecord& txtrec)
	{
		string txt{txtrec.record()};
		uint16_t len = txt.size();
		const char* record = txt.c_str();
		auto err = DNSServiceRegister(&(target.serviceRef),
					      0, // no flags
					      0, // default all interfaces
					      target.name.c_str(),
					      "_aseba._tcp",
					      nullptr, // use default domain, usually "local."
					      nullptr, // use this host name
					      htons(target.port),
					      len, // TXT length
					      record, // TXT record
					      ZeroconfCallbacks::cb_Register,
					      this); // context pointer is Zeroconf
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceRegister: error %0").arg(err));
		else
			processServiceRef(target.serviceRef);
	}

	//! DNSSD callback for registerTarget, update Zeroconf::Target record with results of registration
	void DNSSD_API ZeroconfCallbacks::cb_Register(DNSServiceRef sdRef,
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
			// retrieve target
			auto zeroconf(static_cast<Zeroconf *>(context));
			auto targetIt(zeroconf->getTarget(sdRef));
			if (targetIt == zeroconf->targets.end())
				return;
			Zeroconf::Target& target(*targetIt);
			// fill informations
			target.name = name;
			target.domain = domain;
			target.regtype = regtype;
			target.registerCompleted();
		}
	}

	//! A target can ask Zeroconf to update its TXT record
	void Zeroconf::updateTarget(Zeroconf::Target & target, const TxtRecord& txtrec)
	{
		assert(target.serviceRef);
		const string rawdata{txtrec.record()};
		DNSServiceErrorType err = DNSServiceUpdateRecord(target.serviceRef,
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
		targets.emplace_back(name, regtype, domain, *this);
		Target& target(targets.back());
		auto err = DNSServiceResolve(&target.serviceRef,
						 0, // no flags
						 0, // default all interfaces
						 name.c_str(),
						 regtype.c_str(),
						 domain.c_str(),
						 ZeroconfCallbacks::cb_Resolve,
						 this);
		if (err != kDNSServiceErr_NoError)
		{
			targets.pop_back();
			throw Zeroconf::Error(FormatableString("DNSServiceResolve: error %0").arg(err));
		}
		else
			processServiceRef(target.serviceRef);
	}

	//! DNSSD callback for resolveTarget, update Zeroconf::Target record with results of lookup
	void DNSSD_API ZeroconfCallbacks::cb_Resolve(DNSServiceRef sdRef,
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
		auto zeroconf(static_cast<Zeroconf *>(context));
		auto targetIt(zeroconf->getTarget(sdRef));
		if (targetIt == zeroconf->targets.end())
			return;
		if (errorCode != kDNSServiceErr_NoError)
		{
			zeroconf->targets.erase(targetIt);
			throw Zeroconf::Error(FormatableString("DNSServiceResolveReply: error %0").arg(errorCode));
		}
		else
		{
			Zeroconf::Target& target(*targetIt);
			target.host = hosttarget;
			target.port = ntohs(port);
			Aseba::Zeroconf::TxtRecord tnew{ txtRecord,txtLen };
			target.properties.clear();
			for (auto const & field: tnew)
				target.properties[field.first] = field.second;
			target.properties["fullname"] = string(fullname);
			target.targetFound();
			zeroconf->targets.erase(targetIt);
		}
	}

	//! Update the set of known targets with remote ones found by browsing DNS.
	void Zeroconf::browse()
	{
		releaseServiceRef(browseServiceRef);
		auto err = DNSServiceBrowse(&browseServiceRef,
					    0, // no flags
					    0, // default all interfaces
					    "_aseba._tcp",
					    nullptr, // use default domain, usually "local."
					    ZeroconfCallbacks::cb_Browse,
					    this
		);
		if (err != kDNSServiceErr_NoError)
			throw Zeroconf::Error(FormatableString("DNSServiceBrowse: error %0").arg(err));
		else
			processServiceRef(browseServiceRef);
	}

	//! DNSSD callback for browse, update DiscoveryRequest record with results of browse
	void DNSSD_API ZeroconfCallbacks::cb_Browse(DNSServiceRef sdRef,
					   DNSServiceFlags flags,
					   uint32_t interfaceIndex,
					   DNSServiceErrorType errorCode,
					   const char *name,
					   const char *regtype,
					   const char *domain,
					   void *context)
	{
		if (errorCode != kDNSServiceErr_NoError)
		{
			throw Zeroconf::Error(FormatableString("DNSServiceBrowseReply: error %0").arg(errorCode));
		}
		else
		{
			auto zeroconf(static_cast<Zeroconf *>(context));
			if (flags & kDNSServiceFlagsAdd)
				zeroconf->resolveTarget(name, regtype, domain);
		}
	}

	//! Return an iterator to the target holding a given service reference, return targets.end() if not found.
	Zeroconf::Targets::iterator Zeroconf::getTarget(DNSServiceRef serviceRef)
	{
		return find_if(targets.begin(), targets.end(),
			[=] (const Target& target) { return target.serviceRef == serviceRef; }
		);
	}

	//! Return an iterator to the target with a name and port, return targets.end() if not found.
	Zeroconf::Targets::iterator Zeroconf::getTarget(const std::string & name, const int & port)
	{
		return find_if(targets.begin(), targets.end(),
			[=] (const Target& target) { return target.name == name && target.port == port; }
		);
	}

	//! Return an iterator to the target for a given Dashel stream, return targets.end() if not found.
	Zeroconf::Targets::iterator Zeroconf::getTarget(const Dashel::Stream * stream)
	{
		return getTarget(stream->getTargetParameter("port"), atoi(stream->getTargetParameter("port").c_str()));
	}

} // namespace Aseba
