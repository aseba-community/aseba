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

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	/**
	\defgroup Exec Execute an external command upon a given message
	*/
	/*@{*/
	
	//! Execute an external command upon a given message
	class Exec : public Hub
	{
	private:
		const unsigned messageId; //!< message identifier to look for
		const char *programName; //!< program to execute
	
	public:
		Exec(const unsigned messageId, const char *programName) :
			messageId(messageId),
			programName(programName)
		{
		}
	
	protected:
		
		void incomingData(Stream *stream)
		{
			Message *message = Message::receive(stream);
			
			if (message->type == messageId)
			{
				const int ret(system(programName));
				if (ret == -1)
					cerr << "Executable " << programName << " cannot be run" << endl;
				else if (ret > 0)
					cerr << "Warning, execution of " << programName << " returned value " << ret << endl;
			}
		}
	};
	
	/*@}*/
}


//! Show usage
void DumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba Exec, execute an external command upon a given message, usage:\n";
	stream << programName << " MSG_ID PROG_NAME [options] [targets]*\n";
	stream << "Options:\n";
	stream << "-h, --help      : shows this help\n";
	stream << "-V, --version   : shows the version number\n";
	stream << "Targets are any valid Dashel targets." << std::endl << std::endl;
	stream << "Report bugs to: aseba-dev@gna.org" << std::endl;
}

//! Show version
void DumpVersion(std::ostream &stream)
{
	stream << "Aseba Exec " << ASEBA_VERSION << std::endl;
	stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
	stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}

int main(int argc, char *argv[])
{
	Dashel::initPlugins();
	std::vector<std::string> targets;
	
	if (argc < 3)
	{
		DumpHelp(std::cerr, argv[0]);
		return 1;
	}
	
	const unsigned msgId(atoi(argv[1]));
	const char *programName(argv[2]);
	int argCounter = 3;
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];
		
		if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
		{
			DumpHelp(std::cout, argv[0]);
			return 0;
		}
		else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
		{
			DumpVersion(std::cout);
			return 0;
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
		Aseba::Exec Exec(msgId, programName);
		for (size_t i = 0; i < targets.size(); i++)
			Exec.connect(targets[i]);
		Exec.run();
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
