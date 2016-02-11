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

#ifndef ASEBA_HTTP_INTERFACE
#define ASEBA_HTTP_INTERFACE

#include <deque>
#include <string>
#include <map>
#include <set>
#include <dashel/dashel.h>
#include "../../common/msg/NodesManager.h"
#include "../../common/utils/utils.h"

#include "AeslProgram.h"
#include "HttpDashelTarget.h"
#include "HttpHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace Aseba { namespace Http
{
	/**
	 * The HTTP interface is the central class implementing the asebahttp switch. It manages three kinds of Dashel streams:
	 *  - httpStream: a listening server socket waiting for incoming HTTP connections
	 *  - httpConnections: incoming HTTP client connections making requests to the interface
	 *  - targets: connected Dashel targets to this Aseba network hub, where messages are switched between
	 *
	 * The interface is able to handle disconnections and reconnections of both HTTP client connections and Dashel targets.
	 *
	 * On the HTTP side, it reacts to several kinds of requests provided by HTTP handlers registered with the interface.
	 * There are broadly three different ways of handling a request:
	 *  - immediate response: if the request can be answered as it arrives, it will be
	 *  - delayed response: some requests require sending and receiving messages on the aseba bus. In this case, the
	 *    originating HTTP request is stalled until it can be answered
	 *  - event streams: HTTP requests can subscribe to event streams, in which case they will block the Dashel stream for
	 *    other requests, but will instead be provided a live feed of incoming events on the Aseba bus
	 *
	 * On the Dashel side, node ids are automatically remapped if a collision occurs between several targets to make them
	 * unique. Further, the interface reacts to several kinds of messages observed on the Aseba bus:
	 *  - node descriptions: are parsed and added to a list of nodes for each Dashel target
	 *  - variables: are reacted to if there were incoming HTTP requests for variables
	 *  - user messages: are reacted to if there were incoming HTTP requests for events
	 *  - error messages: are reacted to if there were incoming HTTP requests for execution events (divison by zero, etc.)
	 *
	 * The HTTP interface is also able to distribute an AESL program received over HTTP to its network. In that case, it
	 * is assumed that all nodes share the AESL file and different code entries are provided for the different nodes to
	 * be programmed.
	 */
	class HttpInterface : public Dashel::Hub, public RootHttpHandler
	{
		public:
			struct HttpConnection {
				Dashel::Stream *stream;
				std::deque<DashelHttpRequest *> queue;
				std::set<std::string> eventSubscriptions;
			};

			HttpInterface(const std::string& httpPort = "3000");
			virtual ~HttpInterface();

			/**
			 * Adds a Dashel target to the HTTP interface by its address (e.g. use "ser:device=/dev/ttyACM0" on Linux to
			 * connect to a Thymio. It is fine to call this method while the interface is already running, it will simply
			 * treat the new target as disconnected and attempt to automatically reconnect it.
			 *
			 * Returns true if the target didn't exist yet.
			 */
			virtual bool addTarget(const std::string& address);

			/**
			 * Steps the HTTP interface forward by switching messages on the Aseba bus and reacting to Http requests
			 */
			virtual void step();

			/**
			 * Send an event to all connected targets on the Aseba bus. The first element of the args array is expected to
			 * contain the name of the event, followed by its arguments.
			 *
			 * Returns true if the event was successfully sent to all targets.
			 */
			virtual bool sendEvent(const std::vector<std::string>& args);

			/**
			 * Notifies all HTTP event subscribers about an SSE event with a given name and a vector of argument data
			 */
			virtual void notifyEventSubscribers(const std::string& event, const std::vector<sint16>& data = std::vector<sint16>());

			/**
			 * Adds an event subscription to a HTTP connection specified by one of its requests that should receive the
			 * streamed SSE updates. If subscription is "*", the connection will subscribe to all events.
			 */
			virtual void addEventSubscription(HttpRequest *request, const std::string& subscription);

			/**
			 * Registers a node on the HTTP interface with its target and its local node id. This method will remap the id
			 * to a non-colliding global node id and return it.
			 */
			virtual unsigned registerNode(HttpDashelTarget *target, unsigned localNodeId);

			virtual void setVerbose(bool verbose) { this->verbose = verbose; }
			virtual bool isVerbose() const { return verbose; }

			/**
			 * Runs the current AESL program for the interface by compiling all the code entries for matching nodes on the
			 * network and running them. In case false is returned on failure, errorString will contain a description of the
			 * problem that has occurred.
			 */
			virtual bool runProgram(std::string& errorString);
			virtual void setProgram(const AeslProgram& program) { this->program = program; }
			virtual const AeslProgram& getProgram() const { return program; }

			virtual std::set< std::pair<HttpDashelTarget *, const HttpDashelTarget::Node *> > getNodesByName(const std::string& name);
			virtual std::pair<HttpDashelTarget *, const HttpDashelTarget::Node *> getNodeByIdString(const std::string& nodeIdString);
			virtual std::pair<HttpDashelTarget *, const HttpDashelTarget::Node *> getNodeById(unsigned nodeId);
			virtual std::set< std::pair<HttpDashelTarget *, const HttpDashelTarget::Node *> > getNodesByNameOrId(const std::string& nameOrId);
			virtual std::map<Dashel::Stream *, HttpDashelTarget *>& getTargets() { return targets; }

		protected:
			virtual void connectionCreated(Dashel::Stream *stream);
			virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
			virtual void incomingData(Dashel::Stream *stream);

			/**
			 * Sends all queued responses for HTTP requests. This method does not actually handle the requests, it simply
			 * sends out the responses that were queued from elsewhere.
			 */
			virtual void sendHttpResponses();

			virtual void closeClosingHttpConnections();
			virtual void closeHttpConnection(HttpConnection& connection);

			virtual void incomingVariables(HttpDashelTarget *target, const Variables *variables);
			virtual void incomingUserMessage(HttpDashelTarget *target, const UserMessage *userMessage);
			virtual void incomingErrorMessage(HttpDashelTarget *target, const Message *message);

		private:
			bool verbose;
			AeslProgram program;
			Dashel::Stream *httpStream;
			std::map<Dashel::Stream *, HttpConnection> httpConnections;
			std::set<Dashel::Stream *> closingHttpConnections;
			std::map<std::string, Dashel::Stream *> targetAddressStreams;
			std::map<std::string, UnifiedTime> targetAddressReconnectionTime;
			std::map<Dashel::Stream *, HttpDashelTarget *> targets;
			std::map< unsigned, std::pair<HttpDashelTarget *, unsigned> > nodeIds;

			static const std::string defaultProgram;

	};
} }

#endif
