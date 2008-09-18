/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "descriptions-manager.h"
#include "msg.h"

using namespace std;

namespace Aseba
{
	DescriptionsManager::NodeDescription::NodeDescription() :
		namedVariablesReceptionCounter(0),
		localEventsReceptionCounter(0),
		nativeFunctionReceptionCounter(0)
	{
	}
	
	DescriptionsManager::NodeDescription::NodeDescription(const TargetDescription& targetDescription) :
		TargetDescription(targetDescription),
		namedVariablesReceptionCounter(0),
		localEventsReceptionCounter(0),
		nativeFunctionReceptionCounter(0)
	{
	}
	
	void DescriptionsManager::processMessage(const Message* message)
	{
		// if we have an initial description
		{
			const Description *description = dynamic_cast<const Description *>(message);
			if (description)
			{
				NodesDescriptionsMap::iterator it = nodesDescriptions.find(description->source);
				
				// We can receive a description twice, for instance if there is another IDE connected
				if (it != nodesDescriptions.end())
					return;
				
				// Call a user function when a node protocol version mismatches
				if (description->protocolVersion != ASEBA_PROTOCOL_VERSION)
				{
					nodeProtocolVersionMismatch(description->name, description->protocolVersion);
					return;
				}
				
				// create node and copy description into it
				nodesDescriptions[description->source] = NodeDescription(*description);
				checkIfNodeDescriptionComplete(description->source, nodesDescriptions[description->source]);
			}
		}
		
		// if we have a named variabledescription
		{
			const NamedVariableDescription *description = dynamic_cast<const NamedVariableDescription *>(message);
			if (description)
			{
				NodesDescriptionsMap::iterator it = nodesDescriptions.find(description->source);
				
				// we must have received a description first
				if (it == nodesDescriptions.end())
					return;
				
				// copy description into array if array is empty
				if (it->second.namedVariablesReceptionCounter < it->second.namedVariables.size())
				{
					it->second.namedVariables[it->second.namedVariablesReceptionCounter++] = *description;
					checkIfNodeDescriptionComplete(it->first, it->second);
				}
			}
		}
		
		// if we have a local event description
		{
			const LocalEventDescription *description = dynamic_cast<const LocalEventDescription *>(message);
			if (description)
			{
				NodesDescriptionsMap::iterator it = nodesDescriptions.find(description->source);
				
				// we must have received a description first
				if (it == nodesDescriptions.end())
					return;
				
				// copy description into array if array is empty
				if (it->second.localEventsReceptionCounter < it->second.localEvents.size())
				{
					it->second.localEvents[it->second.localEventsReceptionCounter++] = *description;
					checkIfNodeDescriptionComplete(it->first, it->second);
				}
			}
		}
		
		// if we have a native function description
		{
			const NativeFunctionDescription *description = dynamic_cast<const NativeFunctionDescription *>(message);
			if (description)
			{
				NodesDescriptionsMap::iterator it = nodesDescriptions.find(description->source);
				
				// we must have received a description first
				if (it == nodesDescriptions.end())
					return;
				
				// copy description into array
				if (it->second.nativeFunctionReceptionCounter < it->second.nativeFunctions.size())
				{
					it->second.nativeFunctions[it->second.nativeFunctionReceptionCounter++] = *description;
					checkIfNodeDescriptionComplete(it->first, it->second);
				}
			}
		}
	}
	
	void DescriptionsManager::checkIfNodeDescriptionComplete(unsigned id, const NodeDescription& description)
	{
		// we will call the virtual function only when we have received all local events and native functions
		if ((description.namedVariablesReceptionCounter == description.namedVariables.size()) &&
			(description.localEventsReceptionCounter == description.localEvents.size()) &&
			(description.nativeFunctionReceptionCounter == description.nativeFunctions.size())
		)
			nodeDescriptionReceived(id);
	}
	
	unsigned DescriptionsManager::getNodeId(const std::string& name, bool *ok ) const
	{
		// search for the first node with a given name
		for (NodesDescriptionsMap::const_iterator it = nodesDescriptions.begin(); it != nodesDescriptions.end(); ++it)
		{
			if (it->second.name == name)
			{
				if (ok)
					*ok = true;
				return it->first;
			}
		}
		
		// node not found
		if (ok)
			*ok = false;
		return 0xFFFFFFFF;
	}
	
	const TargetDescription * const DescriptionsManager::getDescription(unsigned nodeId, bool *ok) const
	{
		NodesDescriptionsMap::const_iterator it = nodesDescriptions.find(nodeId);
		
		// node not found
		if (it == nodesDescriptions.end())
		{
			if (ok)
				*ok = false;
			return 0;
		}
		
		if (ok)
			*ok = true;
		return &(it->second);
	}
	
	unsigned DescriptionsManager::getVariablePos(unsigned nodeId, const std::string& name, bool *ok) const
	{
		NodesDescriptionsMap::const_iterator it = nodesDescriptions.find(nodeId);
		
		// node not found
		if (it != nodesDescriptions.end())
		{
			size_t pos = 0;
			for (size_t i = 0; i < it->second.namedVariables.size(); ++i)
			{
				if (it->second.namedVariables[i].name == name)
				{
					if (ok)
						*ok = true;
					return pos;
				}
				pos += it->second.namedVariables[i].size;
			}
		}
		
		// node not found or variable not found
		if (ok)
			*ok = false;
		return 0xFFFFFFFF;
	}
	
	unsigned DescriptionsManager::getVariableSize(unsigned nodeId, const std::string& name, bool *ok) const
	{
		NodesDescriptionsMap::const_iterator it = nodesDescriptions.find(nodeId);
		
		// node not found
		if (it != nodesDescriptions.end())
		{
			for (size_t i = 0; i < it->second.namedVariables.size(); ++i)
			{
				if (it->second.namedVariables[i].name == name)
				{
					if (ok)
						*ok = true;
					return it->second.namedVariables[i].size;
				}
			}
		}
		
		// node not found or variable not found
		if (ok)
			*ok = false;
		return 0xFFFFFFFF;
	}
}
