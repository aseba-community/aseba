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

#include "HttpInterface.h"
#include "../../common/consts.h"
#include "../../common/types.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"

//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba http, connects aseba components together and with HTTP, usage:\n";
	stream << programName << " [options] [additional targets]*\n";
	stream << "Options:\n";
	stream << "-v, --verbose   : makes the switch verbose\n";
	stream << "-p, --port port : listens to incoming connection HTTP on this port\n";
	stream << "-K, --Kiter n   : run I/O loop n thousand times (for profiling)\n";
	stream << "-h, --help      : shows this help\n";
	stream << "-V, --version   : shows the version number\n";
	stream << "Additional targets are any valid Dashel targets." << std::endl;
}

//! Show version
void dumpVersion(std::ostream &stream)
{
	stream << "asebahttp2" << std::endl;
	stream << "Aseba version " << ASEBA_VERSION << std::endl;
	stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
	stream << "Aseba library licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}

// Main
int main(int argc, char *argv[])
{
	Dashel::initPlugins();

	std::string httpPort = "3000";
	std::vector < std::string > dashelTargetList;
	bool verbose = false;
	int Kiterations = -1; // set to > 0 to limit run time e.g. for valgrind

	// process command line
	int argCounter = 1;
	while(argCounter < argc) {
		const char *arg = argv[argCounter++];

		if((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0)) verbose = true;
		else if((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0)) dumpHelp(std::cout, argv[0]), exit(1);
		else if((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0)) dumpVersion (std::cout), exit(1);
		else if((strcmp(arg, "-p") == 0) || (strcmp(arg, "--port") == 0)) httpPort = argv[argCounter++];
		else if((strcmp(arg, "-K") == 0) || (strcmp(arg, "--Kiter") == 0)) Kiterations = atoi(argv[argCounter++]);
		else if(strncmp(arg, "-", 1) != 0) dashelTargetList.push_back(arg);
	}

	// initialize Dashel plugins
	Dashel::initPlugins();

	// create and run bridge, catch Dashel exceptions
	try {
		std::auto_ptr<Aseba::Http::HttpInterface> interface(new Aseba::Http::HttpInterface(httpPort));
		interface->setVerbose(verbose);

		int numTargets = (int) dashelTargetList.size();
		for(int i = 0; i < numTargets; i++) {
			interface->addTarget(dashelTargetList[i]);
		}

		while(Kiterations < 0 || Kiterations > 0) {
			interface->step();

			if(Kiterations > 0) {
				Kiterations--;
			}
		}
	} catch(Dashel::DashelException e) {
		std::cerr << "Unhandled Dashel exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
