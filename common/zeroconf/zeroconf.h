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
#include "dns_sd.h"

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
		class Target;
//		typedef std::map<std::string, Zeroconf::Target*> NameTargetMap;

	public:
		//! Aseba::Zeroconf is a container of targets
		virtual void erase(std::vector<Target>::iterator position) { targets.erase(position); }
		virtual void clear() { targets.clear(); }
		virtual bool empty() { return targets.empty(); }
		virtual int size() { return targets.size(); }
		virtual Target & front() { return targets.front(); }
		virtual std::vector<Target>::iterator begin() { return targets.begin(); }
		virtual std::vector<Target>::iterator end() { return targets.end(); }
		virtual std::vector<Target>::iterator find(const std::string & name);

		//! Aseba::Zeroconf is a factory for creating and inserting targets
		virtual Target& insert(const std::string & name, const int & port);
		virtual Target& insert(const Dashel::Stream* dashel_stream);

		//! Aseba::Zeroconf can update its knowledge of non-local targets by browsing the network
		virtual void browse();

	protected:
		std::vector<Target> targets; //!< the targets in this container
		void registerTarget(Target * target, const TxtRecord& txtrec); //!< requested through target
		void updateTarget(const Target * target, const TxtRecord& txtrec); //!< requested through target
		void resolveTarget(Target * target); //!< requested through target

	public:
		//! An error in registering or browsing Zeroconf
		struct Error:public std::runtime_error
		{
			Error(const std::string& what): std::runtime_error(what) {}
		};

	protected:
		//! Private class that encapsulates an active service request to the mDNS-SD daemon
		class ZeroconfDiscoveryRequest
		{
		public:
			DNSServiceRef serviceref{nullptr};
			bool in_process{true}; //!< flag to signal when this request is no longer active
		public:
			virtual ~ZeroconfDiscoveryRequest()
			{   // release the service reference
				if (serviceref)
					DNSServiceRefDeallocate(serviceref);
				serviceref = 0;
			}
			virtual bool operator==(const ZeroconfDiscoveryRequest &other) const
			{
				return serviceref == other.serviceref;
			}
		};

		ZeroconfDiscoveryRequest browseZDR; //! the zdr for browse requests isn't attached to a target

	protected:
		//! Called when the browsing process is complete.
		//! Can be overridden in derived classes to schedule an update to the UI.
		virtual void browseComplete() {}

		//! The discovery request can be processed immediately, or can be registered with
		//! an event loop for asynchronous processing.
		//! Can be overridden in derived classes to set up asynchronous processing.
		virtual void processDiscoveryRequest(ZeroconfDiscoveryRequest & zdr);

	private:
		//! callbacks for the DNS Service Discovery API
		static void DNSSD_API cb_Register(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context);
		static void DNSSD_API cb_Browse(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context);
		static void DNSSD_API cb_Resolve(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, /* In network byte order */ uint16_t txtLen, const unsigned char *txtRecord, void *context);
	};

	/**
	 \addtogroup zeroconf

		A Zeroconf::Target allows client classes to choose and access Aseba targets.
	 */
	class Zeroconf::Target
	{
	public:
		std::string name;
		int port;
		std::string domain{"local."};
		std::string regtype{"_aseba._tcp"};
		std::string host{"localhost"};
		bool local{true};
		ZeroconfDiscoveryRequest zdr;

	public:
		Target(const std::string & name, const int port);
		Target(const Dashel::Stream* dashel_stream);
		virtual void advertise(const TxtRecord& txtrec); //!< Inform the DNS service about this target
		virtual void updateTxtRecord(const TxtRecord& txtrec); //!< Update this target's description in the DNS service
		virtual void resolve(); //!< Ask the DNS service for the host name and port of this target

		std::map<std::string, std::string> properties; //!< User-modifiable metadata about this target

		virtual bool operator==(const Target &other) const
		{
			return name == other.name && domain == other.domain;
		}

	private:
		friend Zeroconf;
		Zeroconf * container; //!< Back reference to containing Aseba::Zeroconf object
	};

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
		TxtRecord(const unsigned int protovers, const std::string& type, const std::vector<unsigned int>& ids, const std::vector<unsigned int>& pids);
		TxtRecord(const std::string & txtRecord);
		TxtRecord(const unsigned char * txtRecord, uint16_t txtLen);
		//! Different kinds of keys can be encoded into TXT records
		virtual void assign(const std::string& key, const std::string& value);
		virtual void assign(const std::string& key, const int value);
		virtual void assign(const std::string& key, const std::vector<unsigned int>& values);
		virtual void assign(const std::string& key, const bool value);
		//! Retrieve the encoded TXT record
		virtual std::string record() const;

		virtual const std::string at (const std::string& k) const { return fields.at(k); }
		virtual std::map<std::string, std::string>::iterator begin() { return fields.begin(); }
		virtual std::map<std::string, std::string>::iterator end() { return fields.end(); }

	private:
		std::map<std::string, std::string> fields;
		virtual void serializeField(std::ostringstream& txt, const std::string& key) const; //!< serialize a field into the encoded TXT record
	};

	/*@}*/
} // namespace Aseba

#endif /* ASEBA_ZEROCONF */
