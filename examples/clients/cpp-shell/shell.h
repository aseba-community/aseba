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

#ifndef SHELL_H
#define SHELL_H

#include <vector>
#include <string>
#include <dashel/dashel.h>
#include "common/msg/NodesManager.h"

/*
	This is the main class of the Aseba client, called Shell
	because this example provides a simple command-line shell
	to interact with an Aseba network. Start this program
	and type "help" to see the list of commands.
	
	This class inherits from Dashel::Hub and Aseba::NodesManager.
	The first provides the synchronisation between the command-line
	inputs and the Aseba messages from an Aseba target, through the
	streams shellStream and targetStream.
	The second reconstruct target descriptions by filtering
	Aseba messages through NodesManager::processMessage().
*/
struct Shell: public Dashel::Hub, public Aseba::NodesManager
{
public:
	typedef std::vector<std::string> strings;
	typedef std::map<std::string, Aseba::VariablesMap> NodeNameToVariablesMap;
	
protected:
	// streams
	Dashel::Stream* shellStream;
	Dashel::Stream* targetStream;
	
	// current command line
	std::string curShellCmd;
	
	// result of last compilation and load used to interprete messages
	Aseba::CommonDefinitions commonDefinitions;
	NodeNameToVariablesMap allVariables;
	
public:
	// interface with main()
	Shell(const char* target);
	bool isValid() const;
	bool run1s();
	
protected:
	// reimplemented from parent classes
	virtual void sendMessage(const Aseba::Message& message);
	virtual void nodeDescriptionReceived(unsigned nodeId);
	virtual void incomingData(Dashel::Stream *stream);
	
	// main handlers for activites on streams
	void incomingTargetData(Dashel::Stream *stream);
	void incomingShellData(Dashel::Stream *stream);
	
	// process curShellCmd once a '\n' is detected on shellStream
	void processShellCmd();
	
	// implementation of all commands
	void listNodes();
	void listVariables(const strings& args);
	void getVariable(const strings& args);
	void setVariable(const strings& args);
	void emit(const strings& args);
	void load(const strings& args);
	void stop(const strings& args);
	void run(const strings& args);
	
	// helper functions
	bool getNodeAndVarPos(const std::string& nodeName, const std::string& variableName, unsigned& nodeId, unsigned& pos) const;
	bool compileAndSendCode(const std::wstring& source, unsigned nodeId, const std::string& nodeName);
};

#endif // SHELL_H
