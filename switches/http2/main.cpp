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
#include "Globals.h"

#include "../../common/consts.h"
#include "../../common/types.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"

// modules

#include "modules/http/HttpModule.h"

using namespace std;
using namespace Aseba;

//! List of instanciated modules
typedef vector<unique_ptr<Module>> Modules;

//! Show version
void dumpVersion(std::ostream &stream, const char *programName)
{
	// TODO: add copyright
	stream << "Aseba version " << ASEBA_VERSION << "\n";
	stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << "\n";
	stream << "Aseba library licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>";
	stream << endl;
}

//! Show usage
void dumpHelp(ostream &stream, const char *programName, const Switch* asebaSwitch, const Modules& modules)
{
	stream << "Usage: " << programName << " [options] [additional targets]*\n";
	stream << "Aseba Switch, connects Aseba networks together and with HTTP.\n\n";
	stream << "Valid command-line options:\n";
	stream << "  General\n";
 	stream << "    -h, --help      : shows this help, and exit\n";
 	stream << "    -V, --version   : shows the version number, and exit\n";
	asebaSwitch->dumpArgumentsDescription(stream);
	for (auto const& module: modules)
		module->dumpArgumentsDescription(stream);
	stream << "\nAdditional targets are any valid Dashel targets.\n\n";
	dumpVersion(stream, programName);
}

// Main
int main(int argc, const char *argv[])
{
	// initialize Dashel plugins
	Dashel::initPlugins();
	
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
		const char *arg(argv[argCounter++]);
		
		// show help/version and quit
		if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
			dumpHelp(cout, argv[0], asebaSwitch.get(), modules), exit(0);
		if((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
			dumpVersion(cout, argv[0]), exit(0);
		// find whether the switch knows this argument
		if (asebaSwitch->describeArguments().parse(arg, argc, argv, argCounter, arguments))
			goto argFound;
		// find whether a module knows this argument
		for (auto const& module: modules)
			if (module->describeArguments().parse(arg, argc, argv, argCounter, arguments))
				goto argFound;
		// all given arguments must be handled, otherwise it is an error
		// because we do not know how many values to consume
		if (strncmp(arg, "-", 1) == 0)
		{
			cerr << string("Unhandled command line argument: ") + arg << endl;
			exit(2);
		}
		// register additional dashel targets
		dashelTargetList.push_back(arg);
		
		argFound: ;
	}
	
	// set Switch and module arguments
	asebaSwitch->processArguments(arguments);
	for (auto const& module: modules)
		module->processArguments(arguments);
	
	// add command line targets
	for (auto const& target: dashelTargetList) 
	{
		try
		{
			Dashel::Stream* stream(asebaSwitch->connect(target));
		}
		catch (const Dashel::DashelException& e)
		{
			LOG_ERROR << "Core | Error connecting target " << target << " : " << e.what() << endl;
		}
	}
	
	// run switch, catch Dashel exceptions
	try
	{
		asebaSwitch->run();
	}
	catch (const Dashel::DashelException& e)
	{
		LOG_ERROR << "Core | Unhandled Dashel exception: " << e.what() << endl;
		return 1; 
	}

	return 0;
}
