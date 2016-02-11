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

#include <iostream>
#include <sstream>
#include <cassert>
#include <memory>
#include <functional>
#include <cstdlib>
#include "botspeak.h"
#include "common/utils/utils.h"
#include "transport/dashel_plugins/dashel-plugins.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	static bool isNumber(const std::string& s)
	{
		std::string::const_iterator it(s.begin());
		if (it != s.end() && *it == '-')
			++it;
		while (it != s.end() && std::isdigit(*it))
			++it;
		return !s.empty() && (it == s.end());
	}
	
	static struct OperationMap: public map<string, string>
	{
		OperationMap()
		{
			(*this)["ADD"] = "+=";
			(*this)["SUB"] = "-=";
			(*this)["MUL"] = "*=";
			(*this)["DIV"] = "/=";
			(*this)["MOD"] = "%=";
			(*this)["SHR"] = ">>=";
			(*this)["SHL"] = "<<=";
		}
		int exec(const string& op, int a, int b)
		{
			if (op == "ADD")
				return a+b;
			else if (op == "SUB")
				return a-b;
			else if (op == "MUL")
				return a*b;
			else if (op == "DIV")
				return b ? a/b : 0;
			else if (op == "MOD")
				return b ? a%b : 0;
			else if (op == "SHR")
				return a>>b;
			else if (op == "SHL")
				return a<<b;
			else
				return b;
		}
	} operationMap;
	
	
	BotSpeakBridge::Value::Value(BotSpeakBridge* bridge, const std::string& arg)
	{
		const StringVector parts(split<string>(arg, "[] "));
		assert(parts.size() >= 1);
		
		if (parts.size() == 2)
		{
			if (isNumber(parts[1]))
			{
				unsigned index(atoi(parts[1].c_str()));
				const unsigned varSize(bridge->getVarSize(parts[0]));
				assert(varSize != 0);
				if (index >= varSize)
				{
					cerr << FormatableString("Attempting to access variable %0 at index %1 past its size %2, accessing last element %3 instead").arg(parts[0]).arg(index).arg(varSize).arg(varSize-1) << endl;
					index = varSize-1;
				}
				address = bridge->getVarAddress(parts[0]) + index;
				GetVariables(bridge->nodeId, address, 1).serialize(bridge->asebaStream);
				bridge->asebaStream->flush();
				state = PENDING_DIRECT_VALUE;
			}
			else
			{
				address = bridge->getVarAddress(parts[0]);
				indirectAddress = bridge->getVarAddress(parts[1]);
				GetVariables(bridge->nodeId, indirectAddress, 1).serialize(bridge->asebaStream);
				bridge->asebaStream->flush();
				state = PENDING_INDIRECT_VALUE;
			}
		}
		else
		{
			indirectAddress = -1;
			if (isNumber(parts[0]))
			{
				address = -1;
				value = atoi(parts[0].c_str());
				state = RESOLVED;
			}
			else
			{
				address = bridge->getVarAddress(parts[0]);
				GetVariables(bridge->nodeId, address, 1).serialize(bridge->asebaStream);
				bridge->asebaStream->flush();
				state = PENDING_DIRECT_VALUE;
			}
		}
	}
	
	bool BotSpeakBridge::Value::update(BotSpeakBridge* bridge, unsigned addr, int val)
	{
		if (state == PENDING_INDIRECT_VALUE && addr == indirectAddress)
		{
			address += val;
			GetVariables(bridge->nodeId, address, 1).serialize(bridge->asebaStream);
			bridge->asebaStream->flush();
			state = PENDING_DIRECT_VALUE;
			return true;
		}
		else if (state == PENDING_DIRECT_VALUE && addr == address)
		{
			value = val;
			state = RESOLVED;
			return true;
		}
		return false;
	}
	
	BotSpeakBridge::Operation::Operation(BotSpeakBridge* bridge, const std::string& op, const std::string& lhs, const std::string& rhs):
		op(op),
		lhs(bridge, lhs),
		rhs(bridge, rhs)
	{}
	
	//! Update operation, exec if ready, return whether was executed
	bool BotSpeakBridge::Operation::update(BotSpeakBridge* bridge, unsigned addr, int val)
	{
		if (!lhs.update(bridge, addr, val))
			rhs.update(bridge, addr, val);
		if (isReady())
		{
			exec(bridge);
			return true;
		}
		return false;
	}
	
	bool BotSpeakBridge::Operation::isReady()
	{
		return (lhs.state == Value::RESOLVED) && (rhs.state == Value::RESOLVED);
	}
	
	void BotSpeakBridge::Operation::exec(BotSpeakBridge* bridge)
	{
		assert(isReady());
		const int result(operationMap.exec(op, lhs.value, rhs.value));
		// output result
		bridge->outputBotspeak(result);
		// write back result
		SetVariables(bridge->nodeId, lhs.address, SetVariables::VariablesVector(1, result)).serialize(bridge->asebaStream);
		// update outputs
		UserMessage(bridge->eventId(L"update_outputs")).serialize(bridge->asebaStream);
		bridge->asebaStream->flush();
		if (bridge->verbose) cout << "Set address " << lhs.address << " to value " << result << endl;
	}
	
	
	BotSpeakBridge::BotSpeakBridge(unsigned botSpeakPort, const char* asebaTarget):
		botSpeakStream(0),
		asebaStream(0),
		nodeId(0),
		freeVariableIndex(0),
		recordScript(false),
		runAndWait(false),
		verbose(true)
	{
		// fill common definitions
		commonDefinitions.events.push_back(NamedValue(L"start", 0));
		commonDefinitions.events.push_back(NamedValue(L"stopped", 0));
		commonDefinitions.events.push_back(NamedValue(L"running_ping", 0));
		commonDefinitions.events.push_back(NamedValue(L"update_inputs", 0));
		commonDefinitions.events.push_back(NamedValue(L"update_outputs", 0));
		
		// connect to the Aseba target
		connect(asebaTarget);
		
		// connect a server stream for BotSpeak
		ostringstream oss;
		oss << "tcpin:port=" << botSpeakPort;
		connect(oss.str());
		
		// request a description for aseba target
		GetDescription getDescription;
		getDescription.serialize(asebaStream);
		asebaStream->flush();
	}
	
	bool BotSpeakBridge::run1s()
	{
		int timeout(1000);
		UnifiedTime startTime;
		while (timeout > 0)
		{
			if (!step(timeout))
				return false;
			timeout -= (Aseba::UnifiedTime() - startTime).value;
		}
		return true;
	}
	
	void BotSpeakBridge::connectionCreated(Dashel::Stream *stream)
	{
		if (!asebaStream)
		{
			if (verbose)
				cout << "Incoming Aseba connection from " << stream->getTargetName() << endl;
			asebaStream = stream;
		}
		else
		{
			if (nodeId == 0)
				throw runtime_error("Got BotSpeak connection before Aseba target description");
			if (botSpeakStream)
				closeStream(botSpeakStream);
			if (verbose)
				cout << "Incoming BotSpeak connection from " << stream->getTargetName() << endl;
			botSpeakStream = stream;
		}
	}
	
	void BotSpeakBridge::connectionClosed(Stream * stream, bool abnormal)
	{
		if (stream == asebaStream)
		{
			asebaStream = 0;
			cout << "Connection closed to Aseba target" << endl;
			stop();
		}
		else if (stream == botSpeakStream)
		{
			botSpeakStream = 0;
			cout << "Connection closed to botspeak client" << endl;
		}
		else
			abort();
	}
	
	void BotSpeakBridge::sendMessage(const Message& message)
	{
		message.serialize(asebaStream);
		asebaStream->flush();
	}
	
	void BotSpeakBridge::nodeDescriptionReceived(unsigned nodeId)
	{
		if (verbose)
			wcout << L"Received description for " << getNodeName(nodeId) << endl;
		this->nodeId = nodeId;
		// getting variables from target, and allocate variables for BotSpeak runtime
		variablesMap = getDescription(nodeId)->getVariablesMap(freeVariableIndex);
		botspeakVariables.clear();
		// Botspeak
		defineVar(L"_currentBasicBlock", 1);
		// Thymio
		defineVar(L"DIO", 13);
		defineVar(L"AI", 16);
		defineVar(L"AO", 2);
		defineVar(L"PWM", 9);
		defineVar(L"TMR", 1);
		// Generate an empty script
		compileAndRunScript();
	}
	
	void BotSpeakBridge::incomingData(Stream *stream)
	{
		// dispatch to correct method in function of stream
		if (stream == asebaStream)
			incomingAsebaData(stream);
		else if (stream == botSpeakStream)
			incomingBotspeakData(stream);
		else
			abort();
	}
	
	void BotSpeakBridge::incomingAsebaData(Stream *stream)
	{
		// receive message
		Message *message(Message::receive(stream));
		
		// pass message to description manager, which builds
		// the node descriptions in background
		NodesManager::processMessage(message);
		
		// if variables, check for pending requests
		const Variables *variables(dynamic_cast<Variables *>(message));
		if (variables)
		{
			if (getValue.get() && (variables->variables.size() == 1))
			{
				if (verbose) cout << "Received variable value, updating pending get" << endl;
				getValue->update(this, variables->start, variables->variables[0]);
				if (getValue->state == Value::RESOLVED)
				{
					outputBotspeak(getValue->value);
					if (verbose) cout << "Received result of get: " << getValue->value << endl;
					getValue.reset();
					startNextOperationIfPossible();
				}
			}
			else if ((currentOperation.get()) && (variables->variables.size() == 1))
			{
				if (verbose) cout << "Received variable value, updating pending operation" << endl;
				if (currentOperation->update(this, variables->start, variables->variables[0]))
				{
					if (verbose) cout << "Operation completed" << endl;
					currentOperation.reset();
					startNextOperationIfPossible();
				}
			}
		}
		
		// if event
		const UserMessage *userMsg(dynamic_cast<UserMessage *>(message));
		if (userMsg)
		{
			if (userMsg->type == eventId(L"stopped"))
			{
				if (verbose) cout << "Execution completed" << endl;
				if (runAndWait)
					outputBotspeak("Done");
				runAndWait = 0;
			}
			else if (userMsg->type == eventId(L"running_ping"))
			{
				if (verbose) cout << "." << endl;
				outputBotspeak(".", true);
			}
		}
		
		delete message;
	}
	
	void BotSpeakBridge::incomingBotspeakData(Stream *stream)
	{
		uint8_t c;
		string command;
		while ((c = stream->read<uint8_t>()) != '\r')
		{
			if (c == '\n')
			{
				processCommand(command);
				command.clear();
			}
			else
				command += c;
		}
		processCommand(command);
		c = stream->read<uint8_t>();
		if (c != '\n')
			throw runtime_error(
				FormatableString("Invalid BotSpeak protocol, expecting '\\n' but got character 0x%0 instead").arg(unsigned(c),2,16,'0')
			);
	}
	
	void BotSpeakBridge::processCommand(const string& command)
	{
		if (command.empty())
			return;
		if (command == "ENDSCRIPT")
		{
			if (verbose) cout << "stopping recording script" << endl;
			recordScript = false;
			outputBotspeak("end script");
			return;
		}
		if (recordScript)
		{
			if (verbose) cout << "recording command: " << script.size() << " " << command << endl;
			script.push_back(command);
		}
		else
		{
			if (verbose) cout << "processing command: " << command << endl;
			interpreteCommand(command);
		}
	}
	
	template<typename T>
	static bool isSize(const T& name);
	template<>
	bool isSize(const string& name)
	{
		return (name.length() > 5 && name.rfind("_SIZE") == name.length()-5);
	}
	template<>
	bool isSize(const wstring& name)
	{
		return (name.length() > 5 && name.rfind(L"_SIZE") == name.length()-5);
	}
	
	template<typename T>
	static T baseNameSize(const T& name)
	{
		return name.substr(0, name.length()-5);
	}
	
	void BotSpeakBridge::interpreteCommand(const string& command)
	{
		const StringVector cmd(split<string>(command));
		if (cmd.size())
		{
			if (cmd[0] == "SCRIPT")
			{
				script.clear();
				recordScript = true;
				outputBotspeak("start script");
			}
			else if (cmd[0] == "RUN")
			{
				compileAndRunScript();
				outputBotspeak(1);
			}
			else if (cmd[0] == "RUN&WAIT")
			{
				compileAndRunScript();
				runAndWait = true;
			}
			else if (cmd[0] == "SET")
			{
				const StringVector tokens(split<string>(cmd.at(1), "[,] \t"));
				createBotspeakVarIfUndefined(UTF8ToWString(tokens.at(0)), atoi(tokens.back().c_str()));
				if (!isSize(tokens.at(0)))
				{
					const StringVector args(split<string>(cmd.at(1), ","));
					scheduleOperation("SET", args.at(0), args.at(1));
				}
			}
			else if (operationMap.find(cmd[0]) != operationMap.end())
			{
				const StringVector args(split<string>(cmd.at(1), ","));
				scheduleOperation(cmd[0], args.at(0), args.at(1));
			}
			else if (command == "GET VER")
			{
				if (verbose)
					cout << "Reporting version" << endl;
				outputBotspeak(1);
			}
			else if (cmd[0] == "GET")
			{
				scheduleGet(cmd.at(1));
			}
			else
			{
				cerr << "unknown: \"" + command + "\"" << endl;
				if (botSpeakStream)
				{
					const char* version("\r\n");
					botSpeakStream->write(version, 2);
					botSpeakStream->flush();
				}
			}
		}
	}
	
	void BotSpeakBridge::compileAndRunScript()
	{
		// extraction of basic blocks and variable definitions
		set<unsigned> splits;
		if (verbose) cout << "Compiling " << script.size() << " lines" << endl;
		for (size_t i(0); i<script.size(); ++i)
		{
			if (verbose) cout << "Compiling line " << script[i] << endl;
			if (script[i].size() >= 2 && script[i][0] == '/' && script[i][1] == '/')
			{
				if (verbose) cout << "comment: " << script[i] << endl;
				continue;
			}
			const StringVector cmd(split<string>(script[i]));
			if (cmd.size())
			{
				if ((cmd[0] == "WAIT") && (i+1<script.size()))
				{
					splits.insert(i+1);
				}
				if ((cmd[0] == "GOTO") && (i+1<script.size()))
				{
					splits.insert(i+1);
				}
				if (cmd[0] == "IF")
				{
					const unsigned gotoAddr(atoi(cmd.back().c_str()));
					if (gotoAddr && gotoAddr < script.size())
						splits.insert(gotoAddr);
					if (i+1<script.size())
						splits.insert(i+1);
				}
				if ((cmd[0] == "SET") && (cmd.size() > 1))
				{
					const StringVector tokens(split<string>(cmd[1], "[,] \t"));
					createBotspeakVarIfUndefined(UTF8ToWString(tokens.at(0)), atoi(tokens.back().c_str()));
				}
			}
		}
		if (verbose)
		{
			cout << "Found " << splits.size()+1 << " basic blocs starting at lines 0 ";
			for (set<unsigned>::const_iterator it(splits.begin()); it!=splits.end(); ++it)
				cout << *it << " ";
			cout << endl;
		}
		
		// build basic block lookup table
		vector<unsigned> blocToLineTable(script.size(), 0);
		unsigned line(0); unsigned bb(0);
		for (set<unsigned>::const_iterator it(splits.begin()); it!=splits.end(); ++it)
		{
			while (line < *it)
				blocToLineTable[line++] = bb;
			++bb;
		}
		while (line < script.size())
			blocToLineTable[line++] = bb;
		
		// generate aseba source
		wstring asebaSource;
		
		// generate variable definition
		for (vector<NamedVariable>::const_iterator it(botspeakVariables.begin()); it!=botspeakVariables.end(); ++it)
		{
			asebaSource += L"var " + it->name;
			if (it->size != 1)
				asebaSource += WFormatableString(L"[%0]").arg(it->size);
			asebaSource += L"\n";
		}
		// and initialization header
		asebaSource += asebaCodeHeader();
		
		// macro acting like a local function
		#define args1 (isSize(args.at(1)) ? FormatableString("%0").arg(variablesMap.at(UTF8ToWString(baseNameSize(args.at(1)))).second) : args.at(1))
		
		// subroutines for basic blocks
		set<unsigned>::const_iterator bbIt(splits.begin());
		asebaSource += L"sub bb0\n";
		bb = 0;
		bool nextBBDefined(false);
		// transform the source and put the result in the right basic block
		for (size_t i(0); i<script.size(); ++i)
		{
			if ((bbIt != splits.end()) && (*bbIt == i))
			{
				++bb;
				if (!nextBBDefined)
					asebaSource += WFormatableString(L"\t_currentBasicBlock = %0\n").arg(bb);
				asebaSource += WFormatableString(L"\nsub bb%0\n").arg(bb);
				++bbIt;
				nextBBDefined = false;
			}
			const StringVector cmd(split<string>(script[i]));
			if (cmd.empty())
				continue;
			if (cmd[0] == "SET")
			{
				const StringVector args(split<string>(cmd.at(1), ","));
				if (!isSize(args.at(0)))
					asebaSource += L"\t" + UTF8ToWString(args.at(0)) + L" = " + UTF8ToWString(args1) + L"\n";
			}
			else if (operationMap.find(cmd[0]) != operationMap.end())
			{
				const StringVector args(split<string>(cmd.at(1), ","));
				asebaSource += L"\t" + UTF8ToWString(args.at(0)) + L" " + UTF8ToWString(operationMap.at(cmd[0])) + L" " + UTF8ToWString(args1) + L"\n";
			}
			else if (cmd[0] == "WAIT")
			{
				if (cmd.at(1).find('.') != string::npos)
					asebaSource += WFormatableString(L"\ttimer.period[0] = %0\n").arg(unsigned(atof(cmd.at(1).c_str())*1000.));
				else
					asebaSource += WFormatableString(L"\ttimer.period[0] = %0\n").arg(atoi(cmd.at(1).c_str()));
			}
			else if (cmd[0] == "IF")
			{
				const unsigned destLine(atoi(cmd.back().c_str()));
				if (cmd.size() == 4)
				{
					// direct goto 
					asebaSource += WFormatableString(L"\t_currentBasicBlock = %0\n").arg(blocToLineTable[destLine]);
				}
				else if (cmd.size() == 6)
				{
					// test
					asebaSource += L"\tif " + UTF8ToWString(cmd[1]+cmd[2]+cmd[3]) + L" then\n";
					asebaSource += WFormatableString(L"\t\t_currentBasicBlock = %0\n").arg(blocToLineTable[destLine]);
					asebaSource += L"\telse\n";
					asebaSource += WFormatableString(L"\t\t_currentBasicBlock = %0\n").arg(bb+1);
					asebaSource += L"\tend\n";
				}
				nextBBDefined = true;
			}
			else if (cmd[0] == "GOTO")
			{
				const unsigned destLine(atoi(cmd.back().c_str()));
				asebaSource += WFormatableString(L"\t_currentBasicBlock = %0\n").arg(blocToLineTable[destLine]);
				nextBBDefined = true;
			}
			else
			{
				cerr << "Ignoring unknown BotSpeak instruction " << cmd[0] << endl;
			}
		}
		if (!nextBBDefined)
		{
			asebaSource += WFormatableString(L"\t_currentBasicBlock = %0\n").arg(bb+1);
			asebaSource += L"\temit stopped\n";
		}
		
		// basic block dispatch
		asebaSource += L"\nsub dispatchBB\n";
		asebaSource += L"\ttimer.period[0] = 0\n";
		asebaSource += WFormatableString(L"\twhile timer.period[0] == 0 and _currentBasicBlock < %0 do\n").arg(splits.size()+1);
		for (unsigned i(0); i<splits.size()+1; ++i)
		{
			if (i!=0)
				asebaSource += L"\t\telseif";
			else
				asebaSource += L"\t\tif";
			asebaSource += WFormatableString(L" _currentBasicBlock == %0 then\n\t\t\tcallsub bb%1\n").arg(i).arg(i);
			if (i == splits.size())
				asebaSource += L"\t\tend\n";
		}
		asebaSource += L"\tend\n";
		asebaSource += L"\tcallsub updateOutputs\n";
		
		// rest of code
		asebaSource += asebaCodeFooter();
		
		// print source
		if (verbose) wcerr << "Aseba source:\n" << asebaSource;
		
		// compile
		Error error;
		BytecodeVector bytecode;
		unsigned allocatedVariablesCount;
		wistringstream is(asebaSource);
		Compiler compiler;
		compiler.setTargetDescription(getDescription(nodeId));
		compiler.setCommonDefinitions(&commonDefinitions);
		const bool result(compiler.compile(is, bytecode, allocatedVariablesCount, error));
		
		if (result)
		{
			sendBytecode(asebaStream, nodeId, std::vector<uint16>(bytecode.begin(), bytecode.end()));
			Run(nodeId).serialize(asebaStream);
			UserMessage(eventId(L"start")).serialize(asebaStream);
			asebaStream->flush();
		}
		else
		{
			wcerr << L"Compilation failed: " << error.toWString() << endl;
		}
	}
	
	void BotSpeakBridge::scheduleOperation(const string& op, const string& arg0, const string& arg1)
	{
		if (verbose) cout << "Scheduling operation " << op << " " << arg0 << "," << arg1 << "" <<endl;
		nextOperationsOp.push(op);
		nextOperationsArg0.push(arg0);
		nextOperationsArg1.push(arg1);
		startNextOperationIfPossible();
	}
	
	void BotSpeakBridge::scheduleGet(const string& arg)
	{
		if (verbose) cout << "Scheduling GET " << arg << endl;
		nextOperationsOp.push("GET");
		nextOperationsArg0.push(arg);
		nextOperationsArg1.push("");
		startNextOperationIfPossible();
	}
	
	void BotSpeakBridge::startNextOperationIfPossible()
	{
		if (!currentOperation.get() && !getValue.get() && !nextOperationsOp.empty())
		{
			// pending one, update inputs and schedule next
			UserMessage(eventId(L"update_inputs")).serialize(asebaStream);
			asebaStream->flush();
			// depending whether it is a get or a binary operation
			if (nextOperationsOp.front() == "GET")
			{
				getValue.reset(new Value(this, nextOperationsArg0.front()));
				if (verbose) cout << "Next get started" << endl;
			}
			else
			{
				currentOperation.reset(new Operation(this, nextOperationsOp.front(), nextOperationsArg0.front(), nextOperationsArg1.front()));
				if (verbose) cout << "Next operation started" << endl;
			}
			nextOperationsOp.pop();
			nextOperationsArg0.pop();
			nextOperationsArg1.pop();
		}
	}
	
	std::wstring BotSpeakBridge::asebaCodeHeader() const
	{
		std::wstring code;
		code += L"_currentBasicBlock = 0\n";
		code += L"TMR = 0\n";
		code += L"\n";
		code += L"timer.period[1] = 10\n";
		code += L"\n";
		code += L"sub updateInputs\n";
		code += L"\tDIO[8] = button.backward\n";
		code += L"\tDIO[9] = button.left\n";
		code += L"\tDIO[10] = button.center\n";
		code += L"\tDIO[11] = button.forward\n";
		code += L"\tDIO[12] = button.right\n";
		code += L"\tcall math.copy(AI[0:6], prox.horizontal)\n";
		code += L"\tcall math.copy(AI[7:8], prox.ground.delta)\n";
		code += L"\tAI[9] = temperature\n";
		code += L"\tcall math.copy(AI[10:12], acc)\n";
		code += L"\tAI[13] = motor.left.speed\n";
		code += L"\tAI[14] = motor.right.speed\n";
		code += L"\tAI[15] = mic.intensity\n";
		code += L"\n";
		code += L"sub updateOutputs\n";
		code += L"\tcall leds.circle(DIO[0]*32, DIO[1]*32, DIO[2]*32, DIO[3]*32, DIO[4]*32, DIO[5]*32, DIO[6]*32, DIO[7]*32)\n";
		code += L"\tcall leds.top(PWM[0], PWM[1], PWM[2])\n";
		code += L"\tcall leds.bottom.left(PWM[3], PWM[4], PWM[5])\n";
		code += L"\tcall leds.bottom.right(PWM[6], PWM[7], PWM[8])\n";
		code += L"\tmotor.left.target = AO[0]\n";
		code += L"\tmotor.right.target = AO[1]\n";
		code += L"\n";
		return code;
	}
	
	std::wstring BotSpeakBridge::asebaCodeFooter() const
	{
		std::wstring code;
		// execution flow
		code += L"\nonevent timer0\n";
		code += L"\tcallsub updateInputs\n";
		code += L"\tcallsub dispatchBB\n";
		code += L"\nonevent start\n";
		code += L"\tcallsub updateInputs\n";
		code += L"\tcallsub dispatchBB\n";
		// user timer
		code += L"\nonevent timer1\n";
		code += L"\tTMR += 10\n";
		code += L"\tif timer.period[0] != 0 and (TMR % 1000) == 0 then\n";
		code += L"\t\temit running_ping\n";
		code += L"\tend\n";
		// to force I/o refresh
		code += L"\nonevent update_inputs\n";
		code += L"\tcallsub updateInputs\n";
		code += L"\nonevent update_outputs\n";
		code += L"\tcallsub updateOutputs\n";
		return code;
	}
	
	//! define a new Aseba variable
	void BotSpeakBridge::defineVar(const wstring& varName, unsigned varSize)
	{
		assert(variablesMap.find(varName) == variablesMap.end());
		// check for freeVariableIndex overflow
		const unsigned requiredSize(freeVariableIndex+varSize);
		const unsigned variablesSize(nodes.at(nodeId).variablesSize);
		if (requiredSize > variablesSize)
			throw runtime_error(WStringToUTF8(WFormatableString(L"Error, not enough space in target for variable %0 of size %1: requiring %2 words, only %3 available").arg(varName).arg(varSize).arg(requiredSize).arg(variablesSize)));
		// ok, allocate variable
		botspeakVariables.push_back(NamedVariable(varName, varSize));
		variablesMap[varName] = std::make_pair(freeVariableIndex, varSize);
		freeVariableIndex += varSize;
		if (verbose) wcout << L"Allocating variable " << varName << " of size " << varSize << endl;
	}
	
	//! create an Aseba variable for the corresponding BotSpeak variable, manage _SIZE, optionally give a size
	void BotSpeakBridge::createBotspeakVarIfUndefined(const wstring& varName, unsigned varSize)
	{
		// look in the allocated variables
		VariablesMap::const_iterator varIt(variablesMap.find(varName));
		if (varIt == variablesMap.end())
		{
			// create
			if (isSize(varName) && varSize)
				defineVar(baseNameSize(varName), varSize);
			else
				defineVar(varName, 1);
		}
	}
	
	//! return the address of a variable, make an exception if not found
	unsigned BotSpeakBridge::getVarAddress(const string& varName)
	{
		VariablesMap::const_iterator varIt(variablesMap.find(UTF8ToWString(varName)));
		if (varIt == variablesMap.end())
			throw runtime_error("Variable " + varName + " is not defined");
		return varIt->second.first;
	}
	
	//! return the size of a variable, make an exception if not found
	unsigned BotSpeakBridge::getVarSize(const std::string& varName)
	{
		VariablesMap::const_iterator varIt(variablesMap.find(UTF8ToWString(varName)));
		if (varIt == variablesMap.end())
			throw runtime_error("Variable " + varName + " is not defined");
		return varIt->second.second;
	}
	
	//! return the identifier of a global event given by name, the event must exist
	unsigned BotSpeakBridge::eventId(const std::wstring& eventName) const
	{
		size_t id;
		const bool ok(commonDefinitions.events.contains(eventName, &id));
		assert(ok);
		UNUSED(ok);
		return id;
	}
	
	//! output a value to the Botspeak client, if any
	void BotSpeakBridge::outputBotspeak(int value)
	{
		if (botSpeakStream)
		{
			if (verbose) cout << "Writing value " << value << " to Botspeak" << endl;
			string data(FormatableString("%0\r\n").arg(value));
			botSpeakStream->write(data.c_str(), data.length());
			botSpeakStream->flush();
		}
		else
		{
			if (verbose) cout << "Cannot write to BotSpeak, connection closed" << endl;
		}
	}
	
	//! output a value to the Botspeak client, if any
	void BotSpeakBridge::outputBotspeak(const std::string& message, bool noEol)
	{
		if (botSpeakStream)
		{
			if (verbose) cout << "Writing message " << message << " to Botspeak" << endl;
			string data(message + (noEol ? "" : "\r\n"));
			botSpeakStream->write(data.c_str(), data.length());
			botSpeakStream->flush();
		}
		else
		{
			if (verbose) cout << "Cannot write to BotSpeak, connection closed" << endl;
		}
	}
	
} // namespace Aseba

int showHelp(const char* programName)
{
	std::cerr << "Usage: " << programName << " ASEBA_TARGET [BOTSPEAK_PORT]" << std::endl;
	return 1;
}

int main(int argc, char *argv[])
{
	unsigned botSpeakPort(9999);
	
	// process command line
	if (argc < 2)
		return showHelp(argv[0]);
	if (argc > 2)
		botSpeakPort = atoi(argv[2]);
	
	// initialize Dashel plugins
	Dashel::initPlugins();
	
	// create and run bridge, catch Dashel exceptions
	try
	{
		Aseba::BotSpeakBridge bridge(botSpeakPort, argv[1]);
		while (true)
		{
			// ping the network
			bridge.pingNetwork();
			// wait 1s
			if (!bridge.run1s())
				break;
		}
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << "Unhandled Dashel exception: " << e.what() << std::endl;
	}
	
	return 0;
}
