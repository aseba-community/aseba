/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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

#ifndef BOTSPEAK_H
#define BOTSPEAK_H

#include <stdint.h>
#include <dashel/dashel.h>
#include "../../common/msg/descriptions-manager.h"

namespace Aseba
{
	/**
	\defgroup botspeak Bridge with BotSpeak (sites.google.com/site/botspeak), using TinySpeak protocol
	*/
	/*@{*/
	
	class BotSpeakBridge:  public Dashel::Hub, public Aseba::DescriptionsManager
	{
	protected:
		typedef std::vector<std::string> StringVector;
		
	protected:
		// streams
		Dashel::Stream* botSpeakStream;
		Dashel::Stream* asebaStream;
		unsigned nodeId;
		
		// known variables until now
		unsigned freeVariableIndex;
		VariablesMap variablesMap;
		
		// script being recorded
		StringVector script; 
		bool recordScript;
		
		// debug variables
		bool verbose;
		
	public:
		BotSpeakBridge(unsigned botSpeakPort, const char* asebaTarget);
		
	protected:
		// reimplemented from parent classes
		virtual void connectionCreated(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream * stream, bool abnormal);
		virtual void incomingData(Dashel::Stream *stream);
		virtual void nodeDescriptionReceived(unsigned nodeId);
		
		// main handlers for activites on streams
		void incomingAsebaData(Dashel::Stream *stream);
		void incomingBotspeakData(Dashel::Stream *stream);
		
		// commands implementation
		void processCommand(const std::string& command);
		void interpreteCommand(const std::string& command);
		void compileAndRunScript();
		
		// helper functions
		unsigned getVarAddress(const std::wstring& varName);
	};
	
	/*@}*/
};

#endif // BOTSPEAK_H

