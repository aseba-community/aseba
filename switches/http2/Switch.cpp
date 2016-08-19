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
	
	//! Dump a short description of this module and the list of arguments it eats
	void Switch::dumpArgumentsDescription(std::ostream &stream) const
	{
		stream << "  Core features of the switch\n";
		stream << "    -v, --verbose   : makes the switch verbose (default: silent)\n";
		stream << "    -d, --dump      : makes the switch dump the content of messages (default: do not dump)\n";
		stream << "    -p, --port port : listens to incoming Aseba connection on this port (default: 33333)\n";
		stream << "    --rawtime       : shows time in the form of sec:usec since 1970 (default: user readable)\n";
		stream << "    --duration sec  : run the switch only for a given duration (default: run forever)\n";
	}
	
	//! Give the list of arguments this module can understand
	ArgumentDescriptions Switch::describeArguments() const
	{
		return {
			{ "-v", 0 },
			{ "--verbose", 0 },
			{ "-d", 0 },
			{ "--dump", 0 },
			{ "-p", 1 },
			{ "--port", 1 },
			{ "--rawtime", 0 },
			{ "--duration", 1 }
		};
	}
	
	//! Pass all parsed arguments to this module
	void processArguments(const Arguments& arguments)
	{
		// TODO
	}
	
	// TODO: document
	void Switch::handleAutomaticReconnection(Dashel::Stream* stream)
	{
		automaticReconnectionStreams.insert(stream);
	}
	
	// TODO: document
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
