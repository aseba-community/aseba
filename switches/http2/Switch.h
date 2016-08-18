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

#ifndef ASEBA_SWITCH_SWITCH
#define ASEBA_SWITCH_SWITCH

#include <dashel/dashel.h>
#include "../../common/msg/NodesManager.h"
#include "Module.h"

namespace Aseba
{
	class Switch: public Dashel::Hub, public NodesManager
	{
	protected:
		//! An association of stream to module
		typedef std::map<Dashel::Stream*, Module*> StreamModuleMap;
		//! A set of streams
		typedef std::set<Dashel::Stream*> StreamSet;
		
		//! A node that has a program and the result of compilation attached
		struct NodeWithProgram: public Node, public CompilationResult
		{
			NodeWithProgram(const TargetDescription& targetDescription);
			
			void compile(const CommonDefinitions& commonDefinitions);
			
			std::wstring code; //!< the source code of the program
		};
		
	public:
		// public API for main
		ArgumentDescriptions describeArguments() const;
		void processArguments(const Arguments& arguments);
		
		// public API for modules
		void handleAutomaticReconnection(Dashel::Stream* stream);
		void delegateHandlingToModule(Dashel::Stream* stream, Module* owner);
		
	protected:
		// from NodesManager
		
		virtual Node* createNode(const TargetDescription& targetDescription);
		virtual void sendMessage(const Message& message);
		
	protected:
		CommonDefinitions commonDefinitions; //!< global events and constants, user-definable
		StreamModuleMap moduleSpecificStreams; //!< streams whose data processing responsibility are delegated to a module
		StreamSet automaticReconnectionStreams; //!< streams that must be reconnected automatically if disconnected
	};
	
} // namespace Aseba

#endif // ASEBA_SWITCH_SWITCH
