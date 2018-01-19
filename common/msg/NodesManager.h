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

#ifndef ASEBA_DESCRIPTIONS_MANAGER_H
#define ASEBA_DESCRIPTIONS_MANAGER_H

#include "msg.h"
#include "../utils/utils.h"
#include <string>
#include <set>

namespace Aseba
{
	/** \addtogroup msg */
	/*@{*/

	//! This helper class builds complete descriptions out of multiple message parts.
	//! For now, it does not support the disconnection of a whole network nor the update of the description of any node
	class NodesManager
	{
	protected:
		//! Potentially partial Descriptions of nodes along with their reception status
		struct Node : public TargetDescription
		{
			Node() = default;
			Node(const TargetDescription& targetDescription);

			unsigned namedVariablesReceptionCounter{ 0 }; //!< what is the status of the reception of named variables
			unsigned localEventsReceptionCounter{ 0 }; //!< what is the status of the reception of local events
			unsigned nativeFunctionReceptionCounter{ 0 }; //!< what is the status of the reception of native functions

			bool connected{ true }; //!< whether this node is considered connected, on a "physical" level
			UnifiedTime lastSeen; //!< when this node was last seen?

			bool isComplete() const;
		};
		//! Map from nodes id to nodes descriptions
		typedef std::map<unsigned, Node> NodesMap;
		NodesMap nodes; //!< all known nodes descriptions and connection status
		std::set<unsigned> mismatchingNodes; //<! seen nodes with mismatching protocol versions

	public:
		//! Virtual destructor
		virtual ~NodesManager() = default;

		//! Ping the network to detect new nodes and ensure existing nodes are still here
		void pingNetwork();
		//! Process a message and request and reconstruct descriptions if relevant
		void processMessage(const Message* message);

		//! Return the name corresponding to a node identifier; if invalid, return the empty string
		std::wstring getNodeName(unsigned nodeId) const;
		//! Return the id of the node corresponding to name and set ok to true, if provided; if invalid, return 0xFFFFFFFF and set ok to false. If several nodes have this name, either return preferedId if it exists otherwise return the first found.
		unsigned getNodeId(const std::wstring& name, unsigned preferedId = 0, bool* ok = nullptr) const;
		//! Return the description of a node and set ok to true, if provided; if invalid, return 0 and set ok to false
		const TargetDescription* getDescription(unsigned nodeId, bool* ok = nullptr) const;
		//! Return the position of a variable and set ok to true, if provided; if invalid, return 0xFFFFFFFF and set ok to false
		unsigned getVariablePos(unsigned nodeId, const std::wstring& name, bool* ok = nullptr) const;
		//! Return the length of a variable and set ok to true, if provided; if invalid, return 0xFFFFFFFF and set ok to false
		unsigned getVariableSize(unsigned nodeId, const std::wstring& name, bool* ok = nullptr) const;
		//! Reset all descriptions, for instance when a network was disconnected and is reconnected
		void reset();

	protected:
		//! Check if a node description has been fully received, and if so, call the nodeDescriptionReceived() virtual function
		void checkIfNodeDescriptionComplete(unsigned id, const Node& description);

		//! Virtual function that is called when a message must be sent
		virtual void sendMessage(const Message& message) = 0;

		//! Virtual function that is called when a version mismatches, called once per node
		virtual void nodeProtocolVersionMismatch(
			unsigned nodeId, const std::wstring& nodeName, uint16_t protocolVersion)
		{}

		//! Virtual function that is called when a node is first seen, meaning its description has been fully received
		virtual void nodeDescriptionReceived(unsigned nodeId) {}

		//! Virtual function that is called when a node is connected, we know its description has been fully received
		virtual void nodeConnected(unsigned nodeId) {}

		//! Virtual function that is called when a node is explicitly connected or has not been seen for a while
		virtual void nodeDisconnected(unsigned nodeId) {}
	};

	/*@}*/
} // namespace Aseba

#endif
