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

#include "NodesManager.h"
#include "msg.h"
#include <iostream>

using namespace std;

namespace Aseba
{
	NodesManager::Node::Node() :
		namedVariablesReceptionCounter(0),
		localEventsReceptionCounter(0),
		nativeFunctionReceptionCounter(0),
		connected(true)
	{
	}
	
	NodesManager::Node::Node(const TargetDescription& targetDescription) :
		TargetDescription(targetDescription),
		namedVariablesReceptionCounter(0),
		localEventsReceptionCounter(0),
		nativeFunctionReceptionCounter(0),
		connected(true)
	{
	}
	
	bool NodesManager::Node::isComplete() const
	{
		return (namedVariablesReceptionCounter == namedVariables.size()) &&
			(localEventsReceptionCounter == localEvents.size()) &&
			(nativeFunctionReceptionCounter == nativeFunctions.size());
	}
	
	void NodesManager::pingNetwork()
	{
		// check whether there are new nodes in the network, for new targets (protocol >= 5)
		ListNodes listNodes;
		sendMessage(listNodes);
		
		// check nodes that have not been seen for long, mark them as disconnected
		const UnifiedTime now;
		const UnifiedTime delayToDisconnect(3000);
		bool isAnyConnected(false);
		for (NodesMap::iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
		{
			// if node supports listing, 
			if (nodeIt->second.protocolVersion >= 5 && (now - nodeIt->second.lastSeen) > delayToDisconnect && nodeIt->second.connected)
			{
				nodeIt->second.connected = false;
				nodeDisconnected(nodeIt->first);
			}
			// is this node connected?
			isAnyConnected = isAnyConnected || nodeIt->second.connected;
		}
		
		// if no node is connected, broadcast get description as well, for old targets (protocol 4)
		if (!isAnyConnected)
		{
			GetDescription getDescription;
			sendMessage(getDescription);
		}
	}
	
	void NodesManager::processMessage(const Message* message)
	{
		// check whether the node is known
		NodesMap::iterator nodeIt(nodes.find(message->source));
		if (nodeIt == nodes.end())
		{
			// node is not known, so ignore excepted if the message type 
			// is node present and it is not a known mismatch protocol,
			// in that case, request description...
			if ((message->type == ASEBA_MESSAGE_NODE_PRESENT) &&
				mismatchingNodes.find(message->source) == mismatchingNodes.end())
			{
				GetNodeDescription getNodeDescription(message->source);
				sendMessage(getNodeDescription);
			}
			// or if message type is description, in that case, proceed further
			if (message->type != ASEBA_MESSAGE_DESCRIPTION)
				return;
		}
		else
		{
			// node is known, check if connected...
			if (!nodeIt->second.connected)
			{
				// if not, build complete, set as connected and notify client
				nodeIt->second.connected = true;
				if (nodeIt->second.isComplete())
				{
					// only notify connections of completed known nodes
					nodeConnected(nodeIt->first);
				}
			}
			// update last seen time
			nodeIt->second.lastSeen = UnifiedTime();
		}
		
		// if we have a disconnection message
		{
			// FIXME: handle disconnected state
			const Disconnected *disconnected = dynamic_cast<const Disconnected *>(message);
			if (disconnected)
			{
				NodesMap::iterator nodeIt = nodes.find(disconnected->source);
				assert (nodeIt != nodes.end());
				nodes.erase(nodeIt);
			}
		}
		
		// if we have an initial description
		{
			const Description *description = dynamic_cast<const Description *>(message);
			if (description)
			{
				NodesMap::iterator nodeIt = nodes.find(description->source);
				
				// We can receive a description twice, for instance if there is another IDE connected
				if (nodeIt != nodes.end() || (mismatchingNodes.find(description->source) != mismatchingNodes.end()))
					return;
				
				// Call a user function when a node protocol version mismatches
				if ((description->protocolVersion < ASEBA_MIN_TARGET_PROTOCOL_VERSION) ||
					(description->protocolVersion > ASEBA_PROTOCOL_VERSION))
				{
					nodeProtocolVersionMismatch(description->source, description->name, description->protocolVersion);
					mismatchingNodes.insert(description->source);
					return;
				}
				
				// create node and copy description into it
				nodes[description->source] = Node(*description);
				checkIfNodeDescriptionComplete(description->source, nodes[description->source]);
			}
		}
		
		// if we have a named variable description
		{
			const NamedVariableDescription *description = dynamic_cast<const NamedVariableDescription *>(message);
			if (description)
			{
				NodesMap::iterator nodeIt = nodes.find(description->source);
				assert (nodeIt != nodes.end());
				
				// copy description into array if array is empty
				if (nodeIt->second.namedVariablesReceptionCounter < nodeIt->second.namedVariables.size())
				{
					nodeIt->second.namedVariables[nodeIt->second.namedVariablesReceptionCounter++] = *description;
					checkIfNodeDescriptionComplete(nodeIt->first, nodeIt->second);
				}
			}
		}
		
		// if we have a local event description
		{
			const LocalEventDescription *description = dynamic_cast<const LocalEventDescription *>(message);
			if (description)
			{
				NodesMap::iterator nodeIt = nodes.find(description->source);
				assert (nodeIt != nodes.end());
				
				// copy description into array if array is empty
				if (nodeIt->second.localEventsReceptionCounter < nodeIt->second.localEvents.size())
				{
					nodeIt->second.localEvents[nodeIt->second.localEventsReceptionCounter++] = *description;
					checkIfNodeDescriptionComplete(nodeIt->first, nodeIt->second);
				}
			}
		}
		
		// if we have a native function description
		{
			const NativeFunctionDescription *description = dynamic_cast<const NativeFunctionDescription *>(message);
			if (description)
			{
				NodesMap::iterator nodeIt = nodes.find(description->source);
				assert (nodeIt != nodes.end());
				
				// copy description into array
				if (nodeIt->second.nativeFunctionReceptionCounter < nodeIt->second.nativeFunctions.size())
				{
					nodeIt->second.nativeFunctions[nodeIt->second.nativeFunctionReceptionCounter++] = *description;
					checkIfNodeDescriptionComplete(nodeIt->first, nodeIt->second);
				}
			}
		}
	}

	void NodesManager::checkIfNodeDescriptionComplete(unsigned id, const Node& description)
	{
		// we will call the virtual function only when we have received all local events and native functions
		if (description.isComplete() && description.connected)
		{
			nodeDescriptionReceived(id);
			nodeConnected(id);
		}
	}
	
	std::wstring NodesManager::getNodeName(unsigned nodeId) const
	{
		NodesMap::const_iterator nodeIt = nodes.find(nodeId);
		if (nodeIt != nodes.end())
		{
			return nodeIt->second.name;
		}
		else
		{
			return L"";
		}
	}
	
	unsigned NodesManager::getNodeId(const std::wstring& name, unsigned preferedId, bool *ok) const
	{
		// search for the first node with a given name
		bool found(false);
		unsigned foundId(0);
		for (NodesMap::const_iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
		{
			if (nodeIt->second.name == name)
			{
				if (ok)
					*ok = true;
				
				if (nodeIt->first == preferedId)
					return nodeIt->first;
				else if (!found)
					foundId = nodeIt->first;
				
				found = true;
			}
		}
		
		// node found, but with another id than prefered
		if (found)
			return foundId;
		
		// node not found
		if (ok)
			*ok = false;
		return 0xFFFFFFFF;
	}
	
	const TargetDescription * NodesManager::getDescription(unsigned nodeId, bool *ok) const
	{
		NodesMap::const_iterator nodeIt = nodes.find(nodeId);
		
		// node not found
		if (nodeIt == nodes.end())
		{
			if (ok)
				*ok = false;
			return 0;
		}
		
		if (ok)
			*ok = true;
		return &(nodeIt->second);
	}
	
	unsigned NodesManager::getVariablePos(unsigned nodeId, const std::wstring& name, bool *ok) const
	{
		NodesMap::const_iterator nodeIt = nodes.find(nodeId);
		
		// node not found
		if (nodeIt != nodes.end())
		{
			size_t pos = 0;
			for (size_t i = 0; i < nodeIt->second.namedVariables.size(); ++i)
			{
				if (nodeIt->second.namedVariables[i].name == name)
				{
					if (ok)
						*ok = true;
					return pos;
				}
				pos += nodeIt->second.namedVariables[i].size;
			}
		}
		
		// node not found or variable not found
		if (ok)
			*ok = false;
		return 0xFFFFFFFF;
	}
	
	unsigned NodesManager::getVariableSize(unsigned nodeId, const std::wstring& name, bool *ok) const
	{
		NodesMap::const_iterator nodeIt = nodes.find(nodeId);
		
		// node not found
		if (nodeIt != nodes.end())
		{
			for (size_t i = 0; i < nodeIt->second.namedVariables.size(); ++i)
			{
				if (nodeIt->second.namedVariables[i].name == name)
				{
					if (ok)
						*ok = true;
					return nodeIt->second.namedVariables[i].size;
				}
			}
		}
		
		// node not found or variable not found
		if (ok)
			*ok = false;
		return 0xFFFFFFFF;
	}
	
	void NodesManager::reset()
	{
		nodes.clear();
	}
} // namespace Aseba
