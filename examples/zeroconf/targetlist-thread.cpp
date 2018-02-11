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
#include <functional>
#include <unordered_set>
#include "common/utils/utils.h"
#include "common/zeroconf/zeroconf-thread.h"

namespace Aseba
{
	using namespace std;

	class TargetLister : public ThreadZeroconf
	{
		virtual void targetFound(const Aseba::Zeroconf::TargetInformation & target) override
		{
			// output could be JSON but for now is Dashel target [Target name (DNS domain)]
			cout << target.host << ";port=" << target.port;
			cout << " [" << target.name << " (" << target.regtype + "." + target.domain << ")]" << endl;
			// also output properties, typically the DNS-encoded full host name and fields from TXT record
			for (auto const& field: target.properties)
			{
				cout << "\t" << field.first << ":";
				// ids and pids are a special case because they contain vectors of 16-bit integers
				if (field.first.rfind("ids") == field.first.size()-3)
					for (size_t i = 0; i < field.second.length(); i += 2)
						cout << " " << (int(field.second[i])<<8) + int(field.second[i+1]);
				else
					cout << " " << field.second;
				cout << endl;
			}
		}

	public:
		void run()
		{
			// run 10 seconds
			UnifiedTime(10, 0).sleep();
		}
	};
}

int main(int argc, char* argv[])
{
	// Browse for _aseba._tcp services on all interfaces
	Aseba::TargetLister lister;
	lister.browse();
	lister.run();
}
