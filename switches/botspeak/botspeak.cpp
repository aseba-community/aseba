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

#include <iostream>
#include <sstream>
#include <cassert>
#include <memory>
#include <functional>
#include <cstdlib>
#include "botspeak.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	static bool isNumber(const std::string& s)
	{
		std::string::const_iterator it(s.begin());
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
	} operationMap;
	
	BotSpeakBridge::BotSpeakBridge(unsigned botSpeakPort, const char* asebaTarget):
		botSpeakStream(0),
		asebaStream(0),
		nodeId(0),
		freeVariableIndex(0),
		recordScript(false),
		runAndWait(false),
		verbose(true)
	{
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
			stop();
		}
		else if (stream == botSpeakStream)
		{
			botSpeakStream = 0;
			cout << "Connection closed to botspeak client" << endl;
			stop();
		}
		else
			abort();
	}
	
	void BotSpeakBridge::nodeDescriptionReceived(unsigned nodeId)
	{
		if (verbose)
			wcout << L"Received description for " << getNodeName(nodeId) << endl;
		this->nodeId = nodeId;
		variablesMap = getDescription(nodeId)->getVariablesMap(freeVariableIndex);
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
		DescriptionsManager::processMessage(message);
		
		// if variables, check for pending requests
		const Variables *variables(dynamic_cast<Variables *>(message));
		if (variables)
		{
			// TODO
		}
		
		// if event
		const UserMessage *userMsg(dynamic_cast<UserMessage *>(message));
		if (userMsg)
		{
			if (userMsg->type == 1) // event stopped
			{
				cout << "Execution completed" << endl;
				if (runAndWait)
				{
					const char* version("1\r\n");
					botSpeakStream->write(version, 3);
					botSpeakStream->flush();
					runAndWait = 0;
				}
			}
			else if (userMsg->type == 2) // event running_ping
			{
				const char c('\n');
				botSpeakStream->write(&c, 1);
				botSpeakStream->flush();
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
			const char* version("1\r\n");
			botSpeakStream->write(version, 3);
			botSpeakStream->flush();
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
	
	void BotSpeakBridge::interpreteCommand(const string& command)
	{
		const StringVector cmd(split<string>(command));
		if (cmd.size())
		{
			if (cmd[0] == "SCRIPT")
			{
				script.clear();
				recordScript = true;
			}
			else if (cmd[0] == "RUN")
			{
				compileAndRunScript();
				const char* version("1\r\n");
				botSpeakStream->write(version, 3);
				botSpeakStream->flush();
			}
			else if (cmd[0] == "RUN&WAIT")
			{
				compileAndRunScript();
				runAndWait = true;
			}
			else if (command == "GET VER")
			{
				if (verbose)
					cout << "Reporting version" << endl;
				const char* version("1\r\n");
				botSpeakStream->write(version, 3);
				botSpeakStream->flush();
			}
			else
			{
				cerr << "unknown: \"" + command + "\"" << endl;
				// TODO: do interprete command
				const char* version("\r\n");
				botSpeakStream->write(version, 2);
				botSpeakStream->flush();
			}
		}
	}
	
	bool isSize(const string& name)
	{
		return (name.length() > 5 && name.rfind("_SIZE") == name.length()-5);
	}
	
	string baseNameSize(const string& name)
	{
		return name.substr(0, name.length()-5);
	}
	
	void BotSpeakBridge::compileAndRunScript()
	{
		// extraction of basic blocks and variable definitions
		set<unsigned> splits;
		map<string, unsigned> newVars;
		set<string> bridgeVars;
		bridgeVars.insert("DIO");
		bridgeVars.insert("PWM");
		bridgeVars.insert("AI");
		bridgeVars.insert("AO");
		bridgeVars.insert("TMR");
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
					if (i+1<script.size())
						splits.insert(i+1);
				}
				if (cmd[0] == "IF")
				{
					unsigned gotoAddr(atoi(cmd.back().c_str()));
					if (gotoAddr && gotoAddr < script.size())
						splits.insert(gotoAddr);
					if (i+1<script.size())
						splits.insert(i+1);
				}
				if ((cmd[0] == "SET") && (cmd.size() > 1))
				{
					const StringVector args(split<string>(cmd[1], "[,] \t"));
					for (StringVector::const_iterator jt(args.begin()); jt!=args.end(); ++jt)
					{
						const string arg(*jt);
						// if number
						if (isNumber(arg))
							continue;
						// if variable from target
						bool ok(true);
						getVariablePos(nodeId, UTF8ToWString(arg), &ok);
						if (ok)
							continue;
						// if variable from bridge
						if (bridgeVars.find(arg) != bridgeVars.end())
							continue;
						// if already declared
						if (newVars.find(arg) != newVars.end())
							continue;
						// otherwise, define
						if (isSize(arg))
						{
							if (jt == args.begin())
							{
								// seen on left
								const unsigned varSize(atoi(args.back().c_str()));
								newVars[baseNameSize(arg)] = varSize;
							}
							else
							{
								// seen or right, must be known, but ignore for now
							}
						}
						else
							newVars[arg] = 1;
					}
				}
			}
		}
		if (verbose)
		{
			cout << "Found " << splits.size()+1 << " basic blocs starting at lines 0 ";
			for (set<unsigned>::const_iterator it(splits.begin()); it!=splits.end(); ++it)
				cout << *it << " ";
			cout << endl;
			cout << "Allocating " << newVars.size() << " variables: ";
			for (map<string, unsigned>::const_iterator it(newVars.begin()); it!=newVars.end(); ++it)
				cout << it->first << " (" << it->second << ") ";
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
		for (map<string, unsigned>::const_iterator it(newVars.begin()); it!=newVars.end(); ++it)
		{
			asebaSource += L"var " + UTF8ToWString(it->first);
			if (it->second != 1)
				asebaSource += WFormatableString(L"[%0]").arg(it->second);
			asebaSource += L"\n";
		}
		asebaSource += asebaCodeHeader();
		
		// macro acting like a local function
		#define args1 (isSize(args.at(1)) ? FormatableString("%0").arg(newVars.at(baseNameSize(args.at(1)))) : args.at(1))
		
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
					asebaSource += WFormatableString(L"\tcurrentBasicBlock = %0\n").arg(bb);
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
				asebaSource += L"\t" + UTF8ToWString(args.at(0)) + L" " + UTF8ToWString(operationMap.at(cmd[0])) + L" " + UTF8ToWString(args.at(1)) + L"\n";
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
					asebaSource += WFormatableString(L"\tcurrentBasicBlock = %0\n").arg(blocToLineTable[destLine]);
				}
				else if (cmd.size() == 6)
				{
					// test
					asebaSource += L"\tif " + UTF8ToWString(cmd[1]+cmd[2]+cmd[3]) + L" then\n";
					asebaSource += WFormatableString(L"\t\tcurrentBasicBlock = %0\n").arg(blocToLineTable[destLine]);
					asebaSource += L"\telse\n";
					asebaSource += WFormatableString(L"\t\tcurrentBasicBlock = %0\n").arg(bb+1);
					asebaSource += L"\tend\n";
				}
				nextBBDefined = true;
			}
			else
			{
				cerr << "Ignoring unknown BotSpeak instruction " << cmd[0] << endl;
			}
		}
		if (!nextBBDefined)
		{
			asebaSource += WFormatableString(L"\tcurrentBasicBlock = %0\n").arg(bb+1);
			asebaSource += L"\temit stopped\n";
		}
		
		// basic block dispatch
		asebaSource += L"\nsub dispatchBB\n";
		asebaSource += L"\ttimer.period[0] = 0\n";
		asebaSource += WFormatableString(L"\twhile timer.period[0] == 0 and currentBasicBlock < %0 do\n").arg(splits.size()+1);
		for (unsigned i(0); i<splits.size()+1; ++i)
		{
			if (i!=0)
				asebaSource += L"\t\telseif";
			else
				asebaSource += L"\t\tif";
			asebaSource += WFormatableString(L" currentBasicBlock == %0 then\n\t\t\tcallsub bb%1\n").arg(i).arg(i);
			if (i == splits.size())
				asebaSource += L"\t\tend\n";
		}
		asebaSource += L"\tend\n";
		asebaSource += L"\tcallsub updateOutputs\n";
		
		// rest of code
		asebaSource += asebaCodeFooter();
		
		// print source
		wcerr << "Aseba source:\n" << asebaSource;
		
		// compile
		CommonDefinitions commonDefinitions;
		commonDefinitions.events.push_back(NamedValue(L"start", 0));
		commonDefinitions.events.push_back(NamedValue(L"stopped", 0));
		commonDefinitions.events.push_back(NamedValue(L"running_ping", 0));
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
			UserMessage(0).serialize(asebaStream);
			asebaStream->flush();
		}
		else
		{
			wcerr << L"Compilation failed: " << error.toWString() << endl;
		}
	}
	
	std::wstring BotSpeakBridge::asebaCodeHeader() const
	{
		std::wstring code;
		code += L"var currentBasicBlock = 0\n";
		code += L"var DIO[13]\n";
		code += L"var AI[16]\n";
		code += L"var AO[2]\n";
		code += L"var PWM[9]\n";
		code += L"var TMR = 0\n";
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
		code += L"\tif timer.period[0] != 0 and (TMR & 0x3ff) != 0 then\n";
		code += L"\t\temit running_ping\n";
		code += L"\tend\n";
// 		// get/set for HW
// 		onevent get_AI
// 		callsub 
// 		
// 		onevent set_AO
// 		
// 		onevent set_PWM
// 		
// 		onevent get_TMR
// 		
// 		onevent set_TMR
// 		
		
		return code;
	}
	
	unsigned BotSpeakBridge::getVarAddress(const wstring& varName)
	{
		// look in the allocated variables
		VariablesMap::const_iterator varIt(variablesMap.find(varName));
		if (varIt != variablesMap.end())
			return varIt->second.first;
		
		// allocate a new one
		unsigned varAddr = freeVariableIndex++;
		variablesMap[varName] = std::make_pair(varAddr, 1);
		if (verbose)
			wcout << L"Allocating variable " << varName << " at address " << varAddr << endl;
		return varAddr;
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
		bridge.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}