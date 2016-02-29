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

#include <dashel/dashel.h>
#include "../../common/consts.h"
#include "../../common/msg/msg.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"
#include <time.h>
#include <iostream>
#include <cstring>
#include <string>
#include <deque>

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	/**
	\defgroup play Message replayer
	*/
	/*@{*/
	
	//! A message player
	//! This class replay saved user messages
	class Player : public Hub
	{
	private:
		typedef deque<string> StringList;
		
		bool respectTimings;
		int speedFactor;
		Stream* in;
		string line;
		UnifiedTime lastTimeStamp;
		UnifiedTime lastEventTime;
	
	public:
		Player(const char* inputFile, bool respectTimings, int speedFactor) :
			respectTimings(respectTimings),
			speedFactor(speedFactor),
			lastTimeStamp(0)
		{
			if (inputFile)
				in = connect("file:" + string(inputFile) + ";mode=read");
			else
				in = connect("stdin:");
		}
		
		StringList tokenize(const string& input)
		{
			StringList list;
			const size_t inputSize(input.size());
			
			size_t pos(0);
			while ((pos != string::npos) && (pos < inputSize))
			{
				const size_t next = input.find(' ', pos);
				string word;
				if (next != string::npos)
					word = input.substr(pos, next-pos);
				else
					word = input.substr(pos);
				if (!word.empty())
					list.push_back(word);
				pos = next + 1;
			}
			
			return list;
		}
		
		void sendLine()
		{
			// parse line and build user message
			UserMessage userMessage;
			
			StringList tokenizedLine(tokenize(line));
			
			UnifiedTime timeStamp(UnifiedTime::fromRawTimeString(tokenizedLine.front()));
			tokenizedLine.pop_front();
			
			userMessage.source = atoi(tokenizedLine.front().c_str());
			tokenizedLine.pop_front();
			
			userMessage.type = atoi(tokenizedLine.front().c_str());
			tokenizedLine.pop_front();
			
			userMessage.data.reserve(size_t(atoi(tokenizedLine.front().c_str())));
			tokenizedLine.pop_front();
			
			while (!tokenizedLine.empty())
			{
				userMessage.data.push_back(atoi(tokenizedLine.front().c_str()));
				tokenizedLine.pop_front();
			}
			
			// if required, sleep
			if ((respectTimings) && (lastTimeStamp.value != 0))
			{
				const UnifiedTime lostTime(UnifiedTime() - lastEventTime);
				const UnifiedTime deltaTimeStamp(timeStamp - lastTimeStamp);
				if (lostTime < deltaTimeStamp)
				{
					UnifiedTime waitTime(deltaTimeStamp - lostTime);
					waitTime /= speedFactor;
					waitTime.sleep();
				}
			}
			
			// write message on all connected streams
			for (StreamsSet::iterator it = dataStreams.begin(); it != dataStreams.end();++it)
			{
				Stream* destStream(*it);
				if (destStream != in)
				{
					userMessage.serialize(destStream);
					destStream->flush();
				}
			}
			
			lastEventTime = UnifiedTime();
			lastTimeStamp = timeStamp;
			
			line.clear();
		}
		
	protected:
		
		void connectionCreated(Stream *stream)
		{
			//cerr << "got connection " << stream->getTargetName()  << endl;
		}
		
		void incomingData(Stream *stream)
		{
			char c(stream->read<char>());
			if (stream == in)
			{
				if (c == '\n')
					sendLine();
				else
					line += c;
			}
		}
		
		void connectionClosed(Stream *stream, bool abnormal)
		{
			if (stream == in)
				stop();
		}
	};
	
	/*@}*/
}


//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba play, play recorded user messages from a file or stdin, usage:\n";
	stream << programName << " [options] [targets]*\n";
	stream << "Options:\n";
	stream << "--fast          : replay messages twice the speed of real time\n";
	stream << "--faster        : replay messages four times the speed of real time\n";
	stream << "--fastest       : replay messages as fast as possible\n";
	stream << "-f INPUT_FILE   : open INPUT_FILE instead of stdin\n";
	stream << "-h, --help      : shows this help\n";
	stream << "-V, --version   : shows the version number\n";
	stream << "Targets are any valid Dashel targets." << std::endl;
	stream << "Report bugs to: aseba-dev@gna.org" << std::endl;
}

//! Show version
void dumpVersion(std::ostream &stream)
{
	stream << "Aseba play " << ASEBA_VERSION << std::endl;
	stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
	stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}

int main(int argc, char *argv[])
{
	Dashel::initPlugins();
	bool respectTimings = true;
	int speedFactor = 1;
	std::vector<std::string> targets;
	const char* inputFile = 0;
	
	int argCounter = 1;
	
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		
		if (strcmp(arg, "--fastest") == 0)
		{
			respectTimings = false;
		}
		else if (strcmp(arg, "--fast") == 0)
		{
			speedFactor = 2;
		}
		else if (strcmp(arg, "--faster") == 0)
		{
			speedFactor = 4;
		}
		else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
		{
			dumpHelp(std::cout, argv[0]);
			return 0;
		}
		else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
		{
			dumpVersion(std::cout);
			return 0;
		}
		else if (strcmp(arg, "-f") == 0)
		{
			argCounter++;
			if (argCounter >= argc)
			{
				dumpHelp(std::cout, argv[0]);
				return 1;
			}
			else
				inputFile = argv[argCounter];
		}
		else
		{
			targets.push_back(argv[argCounter]);
		}
		argCounter++;
	}
	
	if (targets.empty())
		targets.push_back(ASEBA_DEFAULT_TARGET);
	
	try
	{
		Aseba::Player player(inputFile, respectTimings, speedFactor);
		for (size_t i = 0; i < targets.size(); i++)
			player.connect(targets[i]);
		player.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
