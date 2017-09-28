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

#include <cstring>
#include <dashel/dashel.h>
#include "../../common/productids.h"
#include "../../common/consts.h"
#include "../../common/msg/msg.h"
#include "../../common/msg/NodesManager.h"
#include "../../common/utils/utils.h"
#include "../../common/zeroconf/zeroconf-dashelhub.h"

namespace Aseba
{
	using namespace Dashel;
	using namespace std;

	//! StreamNodesManager encapsulates a Dashel::Stream with its own NodesManager, to avoid node id conflicts.
	class StreamNodesManager : NodesManager
	{
	private:
		Stream* stream; //!< encapsulated stream
		map<unsigned, pair<unsigned,unsigned>> pidMap; //!< remember each node's _productId pos and value

	public:
		StreamNodesManager(Stream* stream) :
			stream(stream)
		{}

		void advertiseNodes(Aseba::Zeroconf & zeroconf)
		{
			vector<unsigned int> ids;
			vector<unsigned int> pids;
			vector<string> names;
			unsigned protocolVersion{99};
			for (auto const & node: nodes)
			{
				bool ok;
				auto description = getDescription(node.first, &ok);
				if (ok)
				{
					ids.push_back(node.first);
					names.push_back(WStringToUTF8(description->name));
					protocolVersion = min(protocolVersion, description->protocolVersion);
					if (pidMap.find(node.first) != pidMap.end())
						pids.push_back(pidMap.at(node.first).second);
				}
			}

			Aseba::Zeroconf::TxtRecord txt{protocolVersion, join(names," "), false, ids, pids};
			try
			{
				zeroconf.advertise("Aseba Local " + stream->getTargetParameter("port"), stream, txt);
			}
			catch (const runtime_error& e)
			{
				cerr << "Can't advertise stream " << stream->getTargetName() << " (" << join(names," ") << "): " << e.what() << endl;
			}
		}

		void requestProductIds()
		{
			for (auto const & node: nodes)
			{
				bool ok;
				auto pos = getVariablePos(node.first, L"_productId", &ok);
				if (ok)
				{
					pidMap[node.first] = make_pair(pos,0);
					GetVariables getVariables(node.first,pos,1);
					getVariables.serialize(stream);
					stream->flush();
				}
			}
		}

		void incomingVariable(const Variables * message)
		{
			auto nodeId = message->source;
			if (pidMap.find(nodeId) != pidMap.end() && pidMap.at(nodeId).first == message->start)
				pidMap.at(nodeId).second = message->variables[0];
		}

		void sendMessage(const Message& message)
		{
			message.serialize(stream);
			stream->flush();
		}

		void processMessage(const Message* message)
		{
			NodesManager::processMessage(message);
		}
	};

	//! Dashel::Hub::Advertiser collects node descriptions from a set of tcp Dashel streams
	//! and advertises them on the local network using Zeroconf.
	class Advertiser: public Hub
	{
	protected:
		map<Stream*,StreamNodesManager> streamMap;
		Aseba::DashelhubZeroconf zeroconf;

	protected:
		virtual void incomingData(Stream *stream) override
		{
			Message *message(Message::receive(stream));
			streamMap.at(stream).processMessage(message);
			const Variables *variables(dynamic_cast<Variables *>(message));
			if (variables)
				streamMap.at(stream).incomingVariable(variables);
		}

		virtual void connectionClosed(Stream *stream, bool abnormal) override
		{
			zeroconf.forget(stream);
		}

	public:
		Advertiser(const vector<string> & dashel_targets):
			zeroconf(*this)
		{
			for (auto const & dashel: dashel_targets)
			{
				try
				{
					if (Dashel::Stream* cxn = connect(dashel))
						streamMap.emplace(cxn,StreamNodesManager(cxn));
				}
				catch(Dashel::DashelException e)
				{
					cerr << "Can't connect target " << dashel << ": " << e.what() << endl;
				}
			}

			ListNodes listNodes;
			for (auto const & stream: streamMap)
			{
				listNodes.serialize(stream.first);
				stream.first->flush();
			}
			run5s(); // wait for descriptions
		}

		void advertiseNodes()
		{
			for (auto & stream: streamMap)
				stream.second.advertiseNodes(zeroconf);
		}

		void requestProductIds()
		{
			for (auto & stream: streamMap)
				stream.second.requestProductIds();
			run5s(); // wait for product id values
		}

		void run5s()
		{
			int timeout{5000};
			UnifiedTime startTime;
			while (timeout > 0)
			{
				if (!step(100))
					break;
				const UnifiedTime now;
				timeout -= (now - startTime).value;
				startTime = now;
			}
		}
	};
}

int main(int argc, char* argv[])
{
	std::vector<std::string> dashel_targets;

	int argCounter = 1;
	while (argCounter < argc)
	{
		auto arg = argv[argCounter++];
		if (strncmp(arg, "-", 1) != 0)
			dashel_targets.push_back(arg);
	}

	Aseba::Advertiser hub(dashel_targets);
	hub.requestProductIds();
	hub.advertiseNodes();

	hub.run();
}
