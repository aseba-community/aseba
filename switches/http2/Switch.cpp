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

#include "Switch.h"

namespace Aseba
{
	// Switch::NodeWithProgram
	
	Switch::NodeWithProgram::NodeWithProgram(const TargetDescription& targetDescription):
		Node(targetDescription)
	{
	}
	
	void Switch::NodeWithProgram::compile(const CommonDefinitions& commonDefinitions)
	{
		Compiler compiler;
		compiler.setTargetDescription(this);
		compiler.setCommonDefinitions(&commonDefinitions);
		
		std::wistringstream is(code);
		
		success = compiler.compile(is, bytecode, allocatedVariablesCount, error);
		
		if (success)
		{
			variables = compiler.getVariablesMap();
			subroutinesNames = compiler.getSubroutinesNames();
		}
	}
	
	
	// Switch
	
	void Switch::handleAutomaticReconnection(Dashel::Stream* stream)
	{
		automaticReconnectionStreams.insert(stream);
	}
	
	void Switch::delegateHandlingToModule(Dashel::Stream* stream, Module* owner)
	{
		moduleSpecificStreams[stream] = owner;
	}
	
	Switch::Node* Switch::createNode(const TargetDescription& targetDescription)
	{
		return new NodeWithProgram(targetDescription);
	}
	
	void Switch::sendMessage(const Message& message)
	{
		// TODO: implement
	}
	
} // namespace Aseba
