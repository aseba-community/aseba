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

#ifndef ASEBA_DESCRIPTIONS_MANAGER_H
#define ASEBA_DESCRIPTIONS_MANAGER_H

#include "msg.h"
#include <string>

namespace Aseba
{
	/** \addtogroup msg */
	/*@{*/
	
	//! This helper class builds complete descriptions out of multiple message parts.
	//! For now, it does not support the disconnection of a whole network nor the update of the description of any node
	class DescriptionsManager
	{
	protected:
		//! Potentially partial Descriptions of nodes along with their reception status
		struct NodeDescription: public TargetDescription
		{
			NodeDescription();
			NodeDescription(const TargetDescription& targetDescription);
			
			unsigned namedVariablesReceptionCounter; //!< what is the status of the reception of named variables
			unsigned localEventsReceptionCounter; //!< what is the status of the reception of local events
			unsigned nativeFunctionReceptionCounter; //!< what is the status of the reception of native functions
		};
		//! Map from nodes id to nodes descriptions
		typedef std::map<unsigned, NodeDescription> NodesDescriptionsMap;
		NodesDescriptionsMap nodesDescriptions; //!< all known nodes descriptions
		
	public:
		//! Virtual destructor
		virtual ~DescriptionsManager() {}
		
		//! Process a message and reconstruct descriptions if relevant
		void processMessage(const Message* message);
		
		//! Return the id of the node corresponding to name and set ok to true, if provided; if invalid, return 0xFFFFFFFF and set ok to false
		unsigned getNodeId(const std::string& name, bool *ok = 0) const;
		//! Return the description of a node and set ok to true, if provided; if invalid, return 0 and set ok to false
		const TargetDescription * const getDescription(unsigned nodeId, bool *ok = 0) const;
		//! Return the position of a variable and set ok to true, if provided; if invalid, return 0xFFFFFFFF and set ok to false
		unsigned getVariablePos(unsigned nodeId, const std::string& name, bool *ok = 0) const;
		//! Return the length of a variable and set ok to true, if provided; if invalid, return 0xFFFFFFFF and set ok to false
		unsigned getVariableSize(unsigned nodeId, const std::string& name, bool *ok = 0) const;
		// TODO: reverse lookup?
		// TODO: move bytecode sender manager here, rename class?
		
	protected:
		//! Check if a node description has been fully received, and if so, call the nodeDescriptionReceived() virtual function
		void checkIfNodeDescriptionComplete(unsigned id, const NodeDescription& description);
		
		//! Virtual function that is called when a version mismatches
		virtual void nodeProtocolVersionMismatch(const std::string &nodeName, uint16 protocolVersion) { }
		
		//! Virtual function that is called when a node description has been fully received
		virtual void nodeDescriptionReceived(unsigned nodeId) { }
	};
	
	/*@}*/
}

#endif
