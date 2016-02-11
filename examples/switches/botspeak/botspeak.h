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

#ifndef BOTSPEAK_H
#define BOTSPEAK_H

#include <stdint.h>
#include <queue>
#include <dashel/dashel.h>
#include "common/msg/NodesManager.h"

namespace Aseba
{
	/**
	\defgroup botspeak Bridge with BotSpeak (sites.google.com/site/botspeak), using TinySpeak protocol
	*/
	/*@{*/
	
	class BotSpeakBridge:  public Dashel::Hub, public Aseba::NodesManager
	{
	protected:
		typedef std::vector<std::string> StringVector;
		typedef TargetDescription::NamedVariable NamedVariable;
		
		struct Value
		{
			enum
			{
				RESOLVED,
				PENDING_DIRECT_VALUE,
				PENDING_INDIRECT_VALUE
			} state;
			int value; //! resolved value
			unsigned address; //! address to which the value belong
			unsigned indirectAddress; //! address to which the index belongs
			
			Value(BotSpeakBridge* bridge, const std::string& arg);
		
			bool update(BotSpeakBridge* bridge, unsigned addr, int val);
		};
		
		struct Operation
		{
			std::string op;
			Value lhs;
			Value rhs;
			
			Operation(BotSpeakBridge* bridge, const std::string& op, const std::string& lhs, const std::string& rhs);
			bool update(BotSpeakBridge* bridge, unsigned addr, int val);
		
		protected:
			bool isReady();
			void exec(BotSpeakBridge* bridge);
		};
		
	protected:
		// streams
		Dashel::Stream* botSpeakStream;
		Dashel::Stream* asebaStream;
		unsigned nodeId;
		
		// events used for running botspeak code
		CommonDefinitions commonDefinitions;
		
		// known variables until now
		unsigned freeVariableIndex;
		VariablesMap variablesMap;
		std::vector<NamedVariable> botspeakVariables;
		
		// script being recorded
		StringVector script; 
		bool recordScript;
		
		// is in run&wait mode?
		bool runAndWait;
		
		// current operation
		bool getInProgress;
		std::auto_ptr<Value> getValue;
		std::auto_ptr<Operation> currentOperation;
		// and pending ones
		std::queue<std::string> nextOperationsOp;
		std::queue<std::string> nextOperationsArg0;
		std::queue<std::string> nextOperationsArg1;
		
		// debug variables
		bool verbose;
		
	public:
		BotSpeakBridge(unsigned botSpeakPort, const char* asebaTarget);
		
		bool run1s();
		
	protected:
		// reimplemented from parent classes
		virtual void connectionCreated(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream * stream, bool abnormal);
		virtual void incomingData(Dashel::Stream *stream);
		virtual void sendMessage(const Message& message);
		virtual void nodeDescriptionReceived(unsigned nodeId);
		
		// main handlers for activites on streams
		void incomingAsebaData(Dashel::Stream *stream);
		void incomingBotspeakData(Dashel::Stream *stream);
		
		// commands implementation
		void processCommand(const std::string& command);
		void interpreteCommand(const std::string& command);
		void compileAndRunScript();
		
		// helper functions
		void scheduleGet(const std::string& arg);
		void scheduleOperation(const std::string& op, const std::string& arg0, const std::string& arg1);
		void startNextOperationIfPossible();
		std::wstring asebaCodeHeader() const;
		std::wstring asebaCodeFooter() const;
		void defineVar(const std::wstring& varName, unsigned varSize);
		void createBotspeakVarIfUndefined(const std::wstring& varName, unsigned varSize);
		unsigned getVarAddress(const std::string& varName);
		unsigned getVarSize(const std::string& varName);
		unsigned eventId(const std::wstring& eventName) const;
		void outputBotspeak(int value);
		void outputBotspeak(const std::string& message, bool noEol=false);
	};
	
	/*@}*/
};

#endif // BOTSPEAK_H

