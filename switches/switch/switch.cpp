/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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
#include <sstream>
#include <set>
#include <valarray>
#include <vector>
#include <iterator>
#include "switch.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"
#include "../../common/consts.h"
#include "../../common/utils/utils.h"
#include "../../common/utils/FormatableString.h"
#include "../../common/msg/msg.h"
#include "../../common/msg/endian.h"
#ifdef ZEROCONF_SUPPORT
#include "../../common/zeroconf/zeroconf-dashelhub.h"
#include "../../common/productids.h"
#endif // ZEROCONF_SUPPORT

#ifdef ZEROCONF_SUPPORT
// zeroconf requires gethostname
#ifdef _WIN32
#include <winsock2.h>
#else // _WIN32
#include <unistd.h>
#endif // _WIN32
#endif // ZEROCONF_SUPPORT

namespace Aseba 
{
	using namespace std;
	using namespace Dashel;

	/** \addtogroup switch */
	/*@{*/

	//! Broadcast messages form any data stream to all others data streams including itself.
	Switch::Switch(unsigned port, std::string name, bool verbose, bool dump, bool forward, bool rawTime) :
		#ifdef DASHEL_VERSION_INT
		Dashel::Hub(verbose || dump),
		#endif // DASHEL_VERSION_INT
#ifdef ZEROCONF_SUPPORT
		zeroconf(*this),
		zeroconfName(std::move(name)),
#endif // ZEROCONF_SUPPORT
		verbose(verbose),
		dump(dump),
		forward(forward),
		rawTime(rawTime)
	{
		ostringstream oss;
		oss << "tcpin:port=" << port;
		auto tcpin = connect(oss.str());
#ifdef ZEROCONF_SUPPORT
		// advertise target
		try
		{
			char hostname[256];
			if (gethostname(hostname, sizeof(hostname)) != 0)
				strcpy(hostname, "unknown host");
			Aseba::Zeroconf::TxtRecord txt{ ASEBA_PROTOCOL_VERSION, zeroconfName };
			zeroconf.advertise(Aseba::FormatableString("%0 on %1").arg(zeroconfName).arg(hostname), tcpin, txt);
		}
		catch (const std::runtime_error& e)
		{
			std::cerr << "Can't advertise switch " << zeroconfName << ": " << e.what() << std::endl;
		}
#endif // ZEROCONF_SUPPORT
	}

	void Switch::connectionCreated(Stream *stream)
	{
		if (verbose)
		{
			dumpTime(cout, rawTime);
			cout << "* Incoming connection from " << stream->getTargetName() << endl;
		}
	}

	void Switch::incomingData(Stream *stream)
	{
#ifdef ZEROCONF_SUPPORT
		if (zeroconf.isStreamHandled(stream))
		{
			zeroconf.dashelIncomingData(stream);
			return;
		}
#endif // ZEROCONF_SUPPORT

		Message* message(Message::receive(stream));

		// remap source
		{
			const IdRemapTable::const_iterator remapIt(idRemapTable.find(stream));
			if (remapIt != idRemapTable.end() &&
				(message->source == remapIt->second.second)
			)
				message->source = remapIt->second.first;
		}

		// if requested, dump
		if (dump)
		{
			dumpTime(cout, rawTime);
			std::cout << "  ";
			message->dump(std::wcout);
			std::wcout << std::endl;
		}

		// write on all connected streams
		CmdMessage* cmdMessage(dynamic_cast<CmdMessage*>(message));
		for (StreamsSet::iterator it = dataStreams.begin(); it != dataStreams.end();++it)
		{
			Stream* destStream = *it;

			if ((forward) && (destStream == stream))
				continue;

			try
			{
				const IdRemapTable::const_iterator remapIt(idRemapTable.find(destStream));
				if (cmdMessage && 
					remapIt != idRemapTable.end())
				{
					if (cmdMessage->dest == remapIt->second.first)
					{
						const uint16_t oldDest(cmdMessage->dest);
						cmdMessage->dest = remapIt->second.second;
						message->serialize(destStream);
						cmdMessage->dest = oldDest;
					}
				}
				else
				{
					message->serialize(destStream);
				}
				destStream->flush();
			}
			catch (DashelException e)
			{
				// if this stream has a problem, ignore it for now, and let Hub call connectionClosed later.
				std::cerr << "error while writing" << std::endl;
			}
		}

		delete message;
	}

	void Switch::connectionClosed(Stream *stream, bool abnormal)
	{
#ifdef ZEROCONF_SUPPORT
		if (zeroconf.isStreamHandled(stream))
		{
			zeroconf.dashelConnectionClosed(stream);
			return;
		}
#endif // ZEROCONF_SUPPORT

		if (verbose)
		{
			dumpTime(cout, rawTime);
			if (abnormal)
				cout << "* Abnormal connection closed to " << stream->getTargetName() << " : " << stream->getFailReason() << endl;
			else
				cout << "* Normal connection closed to " << stream->getTargetName() << endl;
		}
	}

	void Switch::broadcastDummyUserMessage()
	{
		Aseba::UserMessage uMsg;
		uMsg.type = 0;
		//for (int i = 0; i < 80; i++)
		for (StreamsSet::iterator it = dataStreams.begin(); it != dataStreams.end();++it)
		{
			uMsg.serialize(*it);
			(*it)->flush();
		}
	}

	void Switch::remapId(Dashel::Stream* stream, const uint16_t localId, const uint16_t targetId)
	{
		idRemapTable[stream] = IdPair(localId, targetId);
	}

	/*@}*/
};

//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
	stream << "Aseba switch, connects aseba components together, usage:\n";
	stream << programName << " [options] [additional targets]*\n";
	stream << "Options:\n";
	stream << "-v, --verbose   : makes the switch verbose\n";
	stream << "-d, --dump      : makes the switch dump all data\n";
	stream << "-l, --loop      : makes the switch transmit messages back to the send, not only forward them.\n";
	stream << "-p port         : listens to incoming connection on this port\n";
	stream << "-n, --name name : use this name if advertising\n";
	stream << "--rawtime       : shows time in the form of sec:usec since 1970\n";
	stream << "-h, --help      : shows this help\n";
	stream << "-V, --version   : shows the version number\n";
	stream << "Additional targets are any valid Dashel targets." << std::endl;
	stream << "Report bugs to: aseba-dev@gna.org" << std::endl;
}

//! Show version
void dumpVersion(std::ostream &stream)
{
	stream << "Aseba switch " << ASEBA_VERSION << std::endl;
	stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
	stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}

int main(int argc, char *argv[])
{
	Dashel::initPlugins();
	unsigned port = ASEBA_DEFAULT_PORT;
	std::string name = "Aseba Switch";
	bool verbose = false;
	bool dump = false;
	bool forward = true;
	bool rawTime = false;
	std::vector<std::string> additionalTargets;

	int argCounter = 1;

	while (argCounter < argc)
	{
		const char *arg = argv[argCounter];

		if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0))
		{
			verbose = true;
		}
		else if ((strcmp(arg, "-d") == 0) || (strcmp(arg, "--dump") == 0))
		{
			dump = true;
			verbose = true;
		}
		else if ((strcmp(arg, "-l") == 0) || (strcmp(arg, "--loop") == 0))
		{
			forward = false;
		}
		else if (strcmp(arg, "-p") == 0)
		{
			if (argCounter + 1 >= argc)
			{
				std::cerr << "port value needed" << std::endl;
				return 1;
			}
			arg = argv[++argCounter];
			port = atoi(arg);
		}
		else if ((strcmp(arg, "-n") == 0) || (strcmp(arg, "--name") == 0))
		{
			if (argCounter + 1 >= argc)
			{
				std::cerr << "name needed" << std::endl;
				return 1;
			}
			arg = argv[++argCounter];
			name = arg;
		}
		else if (strcmp(arg, "--rawtime") == 0)
		{
			rawTime = true;
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
		else
		{
			additionalTargets.push_back(argv[argCounter]);
		}
		argCounter++;
	}

	try
	{
		Aseba::Switch aswitch(port, name, verbose, dump, forward, rawTime);
		for (size_t i = 0; i < additionalTargets.size(); i++)
		{
			const std::string& target(additionalTargets[i]);
			Dashel::Stream* stream = aswitch.connect(target);

			// see whether we have to remap the id of this stream
			Dashel::ParameterSet remapIdDecoder;
			remapIdDecoder.add("dummy:remapLocal=-1;remapTarget=1");
			remapIdDecoder.add(target.c_str());
			const int remappedLocalId(remapIdDecoder.get<int>("remapLocal"));
			const int remappedTargetId(remapIdDecoder.get<int>("remapTarget"));
			if (target.find("remapLocal=") != std::string::npos)
			{
				aswitch.remapId(stream, uint16_t(remappedLocalId), uint16_t(remappedTargetId));
				if (verbose)
					std::cout << "* Remapping local " << remappedLocalId << " with remote " << remappedTargetId << std::endl;
			}
		}
		/*
		Uncomment this and comment aswitch.run() to flood all peers with dummy user messages
		while (1)
		{
			aswitch.step(10);
			aswitch.broadcastDummyUserMessage();
		}*/
#ifdef ZEROCONF_SUPPORT
		while(aswitch.zeroconf.dashelStep(-1));
#else // ZEROCONF_SUPPORT
		while(aswitch.step(-1));
#endif // ZEROCONF_SUPPORT
	}
	catch(Dashel::DashelException e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}


