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
		class TxtRecord;
		class TargetInformation;
		class Target;
		//! An error in registering or browsing Zeroconf
		struct Error: public std::runtime_error
		{
			Error(const std::string & what): std::runtime_error(what) {}
		};
		//! A vector of Target
		using Targets = std::vector<Target>;

	public:
		//! Default virtual destructor
		virtual ~Zeroconf() = default;

		// Aseba::Zeroconf is a factory for creating and inserting targets
		Target& insert(const std::string & name, const int & port);
		Target& insert(const Dashel::Stream * dashelStream);

		// Support methods to access targets
		Targets::iterator find(const std::string & name);

		// Aseba::Zeroconf can update its knowledge of non-local targets by browsing the network
		virtual void browse();

	public:
		Targets targets; //!< the known targets, local or remote

	protected:
		// Requested through target
		virtual void registerTarget(Target & target, const TxtRecord & txtrec);
		virtual void updateTarget(const Target & target, const TxtRecord & txtrec);
		virtual void resolveTarget(Target & target);
		
		virtual void registerCompleted(const Aseba::Zeroconf::Target &) {} //!< Called when a register is completed
		virtual void resolveCompleted(const Aseba::Zeroconf::Target &) {} //!< Called when a register is completed
		virtual void updateCompleted(const Aseba::Zeroconf::Target &) {} //!< Called when a register is completed

		virtual void browseCompleted() {} //!< Called when browsing is completed

		bool browseAlreadyCompleted; //!< true if browse() was called and the callback browseCompleted() was called as well

	protected:
		//! Private class that encapsulates an active service request to the mDNS-SD daemon
		class DiscoveryRequest
		{
		public:
			DNSServiceRef serviceRef{nullptr};
			bool inProcess{true}; //!< flag to signal when this request is no longer active
		
		public:
			~DiscoveryRequest();
			bool operator==(const DiscoveryRequest &other) const;
		};

		DiscoveryRequest browseZDR; //! the zdr for browse requests isn't attached to a target

	protected:
		//! The discovery request can be processed immediately, or can be registered with
		//! an event loop for asynchronous processing.
		//! Can be overridden in derived classes to set up asynchronous processing.
		virtual void processDiscoveryRequest(DiscoveryRequest & zdr);

	protected:
		// callbacks for the DNS Service Discovery API
		static void DNSSD_API cb_Register(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context);
		static void DNSSD_API cb_Browse(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context);
		static void DNSSD_API cb_Resolve(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, /* In network byte order */ uint16_t txtLen, const unsigned char *txtRecord, void *context);
	};
	
	/**
	 \addtogroup zeroconf

		A Zeroconf::TargetInformation allows client classes to choose and access Aseba targets.
	*/
	class Zeroconf::TargetInformation
	{
	public:
		// unique identification of a target information
		std::string name; //!< the full name of this target
		std::string domain{"local."}; //!< the domain of the host
		std::string regtype{"_aseba._tcp"}; //!< the mDNS registration type

		// additional metadata about this target after resolution
		std::string host; //!< the Dashel target connection port
		int port; //!< the Dashel target connection port
		bool local{true}; //!< whether this host is local

		std::map<std::string, std::string> properties; //!< User-modifiable metadata about this target

	public:
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
	private:
		friend class Zeroconf;
		Target(const std::string & name, const int port, Zeroconf & container);
		Target(const Dashel::Stream* dashelStream, Zeroconf & container);

	public:
		void advertise(const TxtRecord & txtrec);
		void updateTxtRecord(const TxtRecord & txtrec);
		void resolve();
		void registerCompleted() const;
		void resolveCompleted() const;
		void updateCompleted() const;

		friend bool operator==(const Zeroconf::Target& lhs, const Zeroconf::Target& rhs);

	public:
		DiscoveryRequest zdr; //!< Attached discovery request

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
