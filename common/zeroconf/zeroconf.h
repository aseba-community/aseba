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

#ifndef ASEBA_ZEROCONF
#define ASEBA_ZEROCONF

#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <functional>
#include <cmath>
#include "dns_sd.h"
#include "../utils/utils.h"

namespace Dashel
{
	class Stream;
}

namespace Aseba
{
	/**
	 \defgroup zeroconf Zeroconf (mDNS-SD multicast DNS Service Discovery)

		Using Zeroconf, an Aseba target announces itself on the TCP/IP network
		as a service of type _aseba._tcp, and can be discovered by clients.
		A target might be an individual node or an Aseba switch.
		The zeroconf service provides minimal information about a target:
		Dashel target (host name, IP port), target name, target type, Aseba
		protocol version, a list of node ids, and a list of node product ids.
		A client may obtain complete node descriptions by connecting to the
		Dashel target and requesting them using the Aseba protocol
	 */
	/*@{*/

	//! Aseba::Zeroconf provides methods for registering targets, updating target
	//! descriptions, and browsing for targets. It runs a thread that watches the
	//! DNS Service for updates, and triggers callback processing as necessary.
	class Zeroconf
	{
	public:
		struct Error;
		struct TargetInformation;
		class Target;
		class TxtRecord;
		using Targets = std::vector<Target>;

	public:
		//! Default virtual destructor
		virtual ~Zeroconf() = default;

		// Aseba::Zeroconf can advertise local targets
		void advertise(const std::string & name, const int & port, const TxtRecord & txtrec);
		void advertise(const Dashel::Stream * stream, const TxtRecord & txtrec);
		void forget(const std::string & name, const int & port);
		void forget(const Dashel::Stream * stream);

		// Aseba::Zeroconf can request the listing of non-local targets by browsing the network
		void browse();

	protected:
		// helper functions
		void registerTarget(Target & target, const TxtRecord & txtrec);
		void updateTarget(Target & target, const TxtRecord & txtrec);
		void resolveTarget(const std::string & name, const std::string & regtype, const std::string & domain);
		Targets::iterator getTarget(DNSServiceRef serviceRef);
		Targets::iterator getTarget(const std::string & name, const int & port);
		Targets::iterator getTarget(const Dashel::Stream * stream);

	protected:
		// information callback for sub-class
		virtual void registerCompleted(const Aseba::Zeroconf::TargetInformation &) {} //!< Called when a register is completed
		virtual void updateCompleted(const Aseba::Zeroconf::TargetInformation &) {}
		virtual void targetFound(const Aseba::Zeroconf::TargetInformation &) {} //!< Called for each resolved target

		// serviceRef registering/de-registering, to be implemented by subclasses
		//! Watch the file description associated with the service reference and call DNSServiceProcessResult when data are available
		virtual void processServiceRef(DNSServiceRef serviceRef) = 0;
		//! Stop watching the file description associated with the service reference, and deallocate it afterwards (calling DNSServiceRefDeallocate)
		virtual void releaseServiceRef(DNSServiceRef serviceRef) = 0;

	protected:
		//! A list of all targets currently being processed, i.e. whose serviceRefs are handled by subclasses
		Targets targets;
		//! The serviceRef for browse requests isn't attached to a target
		DNSServiceRef browseServiceRef{nullptr};

	protected:
		// callbacks for the DNS Service Discovery API
		static void DNSSD_API cb_Register(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context);
		static void DNSSD_API cb_Browse(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context);
		static void DNSSD_API cb_Resolve(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, /* In network byte order */ uint16_t txtLen, const unsigned char *txtRecord, void *context);
	};

	/**
	 \addtogroup zeroconf
		An error in registering or browsing Zeroconf
	*/
	struct Zeroconf::Error: public std::runtime_error
	{
		Error(const std::string & what): std::runtime_error(what) {}
	};

	/**
	 \addtogroup zeroconf
		A Zeroconf::TargetInformation allows client classes to choose and access Aseba targets.
	*/
	struct Zeroconf::TargetInformation
	{
		// informations from discovery
		std::string name; //!< the full name of this target
		std::string regtype{"_aseba._tcp"}; //!< the mDNS registration type
		std::string domain{"local."}; //!< the domain of the host

		// additional metadata about this target after resolution
		std::string host; //!< the Dashel target connection ports
		int port; //!< the Dashel target connection port

		std::map<std::string, std::string> properties; //!< User-modifiable metadata about this target

		TargetInformation(const std::string & name, const std::string & regtype, const std::string & domain);
		TargetInformation(const std::string & name, const int port);
		TargetInformation(const Dashel::Stream* dashelStream);

		friend bool operator==(const Zeroconf::TargetInformation& lhs, const Zeroconf::TargetInformation& rhs);
		friend bool operator<(const Zeroconf::TargetInformation& lhs, const Zeroconf::TargetInformation& rhs);
	};

	bool operator==(const Zeroconf::TargetInformation& lhs, const Zeroconf::TargetInformation& rhs);
	bool operator<(const Zeroconf::TargetInformation& lhs, const Zeroconf::TargetInformation& rhs);

	/**
	 \addtogroup zeroconf

		A Zeroconf::Target allows client classes to choose and access Aseba targets.
	 */
	class Zeroconf::Target: public Zeroconf::TargetInformation
	{
	public:
		Target(const std::string & name, const std::string & regtype, const std::string & domain, Zeroconf & container);
		Target(const std::string & name, const int port, Zeroconf & container);
		Target(const Dashel::Stream* dashelStream, Zeroconf & container);

		// disable copy constructor and copy assigment operator
		Target(const Target &) = delete;
		Target& operator=(const Target&) = delete;
		// implement move versions instead
		Target(Target && rhs);
		Target& operator=(Target&& rhs);

		~Target();

		void registerCompleted() const;
		void updateCompleted() const;
		void targetFound() const;

		friend bool operator==(const Zeroconf::Target& lhs, const Zeroconf::Target& rhs);

	public:
		DNSServiceRef serviceRef{nullptr}; //!< Attached serviceRef

	protected:
		std::reference_wrapper<Zeroconf> container; //!< Back reference to containing Aseba::Zeroconf object
	};

	bool operator==(const Zeroconf::Target& lhs, const Zeroconf::Target& rhs);

	/**
	 \addtogroup zeroconf

		Zeroconf records three DNS records for each service: a PTR for the
		symbolic name, an SRV for the actual host and port number, and a TXT
		for extra information. The TXT record is used by clients to help choose
		the hosts it wishes to connect to. The syntax of TXT records is defined
		in https://tools.ietf.org/html/rfc6763#section-6.
		The TXT record for Aseba targets specifies the target type, the Aseba
		protocol version, a list of node ids, and a list of node product ids.
	 */
	class Zeroconf::TxtRecord
	{
	public:
		//! The fields are contained as a map of strings to strings, the constraints are enforced through the assign functions.
		using Fields = std::map<std::string, std::string>;
		
	public:
		TxtRecord(const unsigned int protovers, const std::string& type, const std::vector<unsigned int>& ids, const std::vector<unsigned int>& pids);
		TxtRecord(const std::string & txtRecord);
		TxtRecord(const unsigned char * txtRecord, uint16_t txtLen);

		// Different kinds of keys can be encoded into TXT records
		void assign(const std::string& key, const std::string& value);
		void assign(const std::string& key, const int value);
		void assign(const std::string& key, const std::vector<unsigned int>& values);
		void assign(const std::string& key, const bool value);

		const std::string at (const std::string& k) const { return fields.at(k); }
		Fields::const_iterator begin() const { return fields.begin(); }
		Fields::const_iterator end() const { return fields.end(); }

		std::string record() const;

	private:
		void serializeField(std::ostringstream& txt, const std::string& key) const;

	private:
		Fields fields;
	};

	/*@}*/
} // namespace Aseba

namespace std {
	//! Hash for a zeroconf target information, use the generic point-based comparison
	template <>
	struct hash<Aseba::Zeroconf::TargetInformation>: public Aseba::PointerHash<Aseba::Zeroconf::TargetInformation> {};
	//! Hash for a const zeroconf target information, use the generic point-based comparison
	template <>
	struct hash<const Aseba::Zeroconf::TargetInformation>: public Aseba::PointerHash<const Aseba::Zeroconf::TargetInformation> {};
	//! Hash for a zeroconf target, use the generic point-based comparison
	template <>
	struct hash<Aseba::Zeroconf::Target>: public Aseba::PointerHash<Aseba::Zeroconf::Target> {};
	//! Hash for a const zeroconf target, use the generic point-based comparison
	template <>
	struct hash<const Aseba::Zeroconf::Target>: public Aseba::PointerHash<const Aseba::Zeroconf::Target> {};
} // namespace std

#endif /* ASEBA_ZEROCONF */
