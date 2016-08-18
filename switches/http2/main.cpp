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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

#include "Switch.h"

#include "../../common/consts.h"
#include "../../common/types.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"

// modules

#include "modules/http/HttpModule.h"

using namespace std;
using namespace Aseba;

typedef vector<unique_ptr<Module>> Modules;

//! Show version
void dumpVersion(std::ostream &stream, const char *programName)
{
	stream << programName << "\n";
	stream << "Aseba version " << ASEBA_VERSION << "\n";
	stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << "\n";
	stream << "Aseba library licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
	stream << endl;
}

//! Show usage
void dumpHelp(ostream &stream, const char *programName, const Modules& modules)
{
	stream << "Usage: " << programName << " [options] [additional targets]*\n";
	stream << "Aseba Switch, connects aseba components together and with HTTP.\n\n";
	stream << "Options:\n";
// 	stream << "-v, --verbose   : makes the switch verbose\n";
// 	stream << "-p, --port port : listens to incoming connection HTTP on this port\n";
// 	stream << "-K, --Kiter n   : run I/O loop n thousand times (for profiling)\n";
 	stream << "-h, --help      : shows this help\n";
 	stream << "-V, --version   : shows the version number\n";
	for (auto const& module: modules)
		module->dumpDescription(stream);
	stream << "\nAdditional targets are any valid Dashel targets.\n\n";
	dumpVersion(stream, programName);
}

// Main
int main(int argc, char *argv[])
{
	// initialize Dashel plugins
	Dashel::initPlugins();
	
//	std::string httpPort = "3000";
//	bool verbose = false;
// //	int Kiterations = -1; // set to > 0 to limit run time e.g. for valgrind
// 
// 	// process command line
// 	int argCounter = 1;
// 	while (argCounter < argc)
// 	{
// 		const char *arg = argv[argCounter++];
// 
// 		if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
// 			dumpHelp(std::cout, argv[0]), exit(0);
// 		else if((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
// 			dumpVersion (std::cout, argv[0]), exit(0);
// // 		else if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0))
// // 			verbose = true;
// // 		else if ((strcmp(arg, "-p") == 0) || (strcmp(arg, "--port") == 0))
// // 			httpPort = argv[argCounter++];
// // 		else if ((strcmp(arg, "-K") == 0) || (strcmp(arg, "--Kiter") == 0))
// // 			Kiterations = atoi(argv[argCounter++]);
// 		else if (strncmp(arg, "-", 1) != 0)
// 			dashelTargetList.push_back(arg);
// 	}
	
	// create switch
	unique_ptr<Switch> asebaSwitch(new Switch());
	
	// create modules
	Modules modules;
	modules.push_back(unique_ptr<Module>(new HttpModule(asebaSwitch.get())));
	// TODO: add more modules
	
	// parse command line
	vector<string> dashelTargetList;
	Arguments arguments;
	int argCounter = 1;
	while (argCounter < argc)
	{
		const char *arg = argv[argCounter++];
		
		// show help/version and quit
		if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
			dumpHelp(cout, argv[0], modules), exit(0);
		if((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
			dumpVersion(cout, argv[0]), exit(0);
		// find whether a module knows this argument
		for (auto const& module: modules)
		{
			for (auto const& argDescr: module->describeArguments())
			{
				if (argDescr.name == arg)
				{
					Argument argument(argDescr);
					for (int i = 0; (i < argDescr.paramCount) && (argCounter < argc); ++i)
						argument.values.push_back(argv[argCounter++]);
					arguments.push_back(argument);
					goto doubleBreak;
				}
			}
		}
		// all given arguments must be handled, otherwise it is an error
		// because we do not know how many values to consume
		if (strncmp(arg, "-", 1) != 0)
			throw runtime_error(string("Unhandled argument ") + arg);
		// register additional dashel targets
		dashelTargetList.push_back(arg);
		
		doubleBreak: ;
	}
	
	// set module arguments
	for (auto const& module: modules)
		module->processArguments(arguments);
	
	// add command line targets
	for (auto const& target: dashelTargetList) 
	{
		Dashel::Stream* stream(asebaSwitch->connect(target));
		asebaSwitch->handleAutomaticReconnection(stream);
		// TODO: handle exception
	}
	
	// run switch, catch Dashel exceptions
	try
	{
		// run the switch
		asebaSwitch->run();
		
// 		while(Kiterations < 0 || Kiterations > 0) {
// 			interface->step();
// 
// 			if(Kiterations > 0) {
// 				Kiterations--;
// 			}
// 		}
	}
	catch(Dashel::DashelException e)
	{
		cerr << "Unhandled Dashel exception: " << e.what() << endl;
		return 1;
	}

	return 0;
}
