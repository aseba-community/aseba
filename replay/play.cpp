/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
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

#include "../msg/msg.h"
#include "../utils/utils.h"
#include <dashel/dashel.h>
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
		Stream* in;
		string line;
		UnifiedTime lastTimeStamp;
		UnifiedTime lastEventTime;
	
	public:
		Player(const char* inputFile, bool respectTimings) :
			respectTimings(respectTimings),
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
					(deltaTimeStamp - lostTime).sleep();
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
			if (stream == in)
			{
				char c(in->read<char>());
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
	stream << "--fastest       : replay messages as fast as possible\n";
	stream << "-f INPUT_FILE   : open INPUT_FILE instead of stdin\n";
	stream << "-h, --help      : shows this help\n";
	stream << "Targets are any valid Dashel targets." << std::endl;
}

int main(int argc, char *argv[])
{
	bool respectTimings = true;
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
		else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
		{
			dumpHelp(std::cout, argv[0]);
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
		Aseba::Player player(inputFile, respectTimings);
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
