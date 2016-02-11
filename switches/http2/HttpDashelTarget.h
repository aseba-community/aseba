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

#ifndef ASEBA_HTTP_DASHEL_TARGET
#define ASEBA_HTTP_DASHEL_TARGET

#include <map>
#include <string>
#include <set>
#include <dashel/dashel.h>
#include "../../common/msg/NodesManager.h"
#include "AeslProgram.h"

namespace Aseba { namespace Http
{
	class HttpInterface; // foward declaration
	class HttpRequest; // foward declaration
	class DashelHttpRequest; // foward declaration

	/**
	 * A HTTP Dashel target represents a connection that was established to one of the specified peers
	 * on the Aseba network. It is typically identified by its address which also specifies the target
	 * for the corresponding Dashel stream.
	 *
	 * Every target stores a list of nodes as reported by the integrated descriptions manager over the
	 * Aseba network. Nodes also store a list of their defined variables for the currently compiled
	 * AESL program, as well as a list of pending variable values requested by HTTP requests.
	 */
	class HttpDashelTarget : public NodesManager
	{
		public:
			struct Node {
				unsigned localId;
				unsigned globalId;
				std::string name;
				VariablesMap variablesMap;
				std::map< unsigned, std::set< std::pair<Dashel::Stream *, DashelHttpRequest *> > > pendingVariables;
			};

			HttpDashelTarget(HttpInterface *interface, const std::string& address, Dashel::Stream *stream);
			virtual ~HttpDashelTarget();

			/**
			 * Send an event to all nodes registered by this target on the Aseba bus. The first element
			 * of the args array is expected to contain the name of the event, followed by its
			 * arguments.
			 *
			 * Returns true if the event was successfully sent to all nodes.
			 */
			virtual bool sendEvent(const std::vector<std::string>& args);

			/**
			 * Send a get variables request to one of this target's specified nodes. The args array
			 * contains the names of all variables to lookup. The supplied HTTP request will be
			 * notified as soon as an answer with the requested variable values arrives.
			 *
			 * Returns true if the request was successfully sent to the specified node.
			 */
			virtual bool sendGetVariables(unsigned globalNodeId, const std::vector<std::string>& args, HttpRequest *request);

			/**
			 * Send a set variables request to one of this target's specified nodes. The first element
			 * of the args array is expected to contain the name of the variable to set, followed by
			 * values for each of its (array) values.
			 *
			 * Returns true if the request was successfully sent to the specified node.
			 */
			virtual bool sendSetVariable(unsigned globalNodeId, const std::vector<std::string>& args);

			/**
			 * Compiles a specified code entry from an AESL program for one of this target's specified
			 * nodes and sends the resulting bytecode as well as a run request to the node. In case
			 * false is returned on failure, errorString will contain a description of the problem
			 * that has occurred.
			 */
			virtual bool compileAndRunCode(unsigned globalNodeId, const std::string& code, std::string& errorString);

			/**
			 * Remove a pending variable request from one of this target's specified nodes.
			 *
			 * Returns true on success.
			 */
			virtual bool removePendingVariable(unsigned globalNodeId, unsigned start);

			virtual std::set<const Node *> getNodesByName(const std::string& name) const;
			virtual const Node *getNodeById(unsigned globalNodeId) const;
			virtual const Node *getNodeByLocalId(unsigned localNodeId) const;

			virtual const std::string& getAddress() const { return address; }
			virtual Dashel::Stream *getStream() { return stream; }
			virtual const std::map<unsigned, Node>& getNodes() const { return nodes; }

		protected:
			virtual bool getVariableInfo(const Node& node, const std::string& variableName, unsigned& position, unsigned& size);
			virtual void sendMessage(const Message& message);
			virtual void nodeDescriptionReceived(unsigned localNodeId);
			

		private:
			HttpInterface *interface;
			std::string address;
			Dashel::Stream *stream;

			std::map<unsigned, Node> nodes;
			std::map<unsigned, unsigned> globalIds;
	};
} }

#endif
