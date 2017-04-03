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
#include "../../common/zeroconf/zeroconf.h"

int main(int argc, char* argv[])
{
#ifdef DNSSD_AVAHI
	std::cerr << "Avahi's Bonjour compatibility is broken when used synchronously (#233)" << std::endl;
	exit(1);
#endif // DNSSD_AVAHI

	// Browse for _aseba._tcp services on all interfaces
	Aseba::Zeroconf zs;
	zs.browse();

	// Aseba::Zeroconf is a smart container for Aseba::Zeroconf::Target
	for (auto & target: zs)
	{
		// Resolve the host name and port of this target, retrieve TXT record
		target.resolve();

		// output could be JSON but for now is Dashel target [Target name (DNS domain)]
		std::cout << target.host << ";port=" << target.port;
		std::cout << " [" << target.name << " (" << target.regtype+"."+target.domain << ")]" << std::endl;
		// also output properties, typically the DNS-encoded full host name and fields from TXT record
		for (auto const& field: target.properties)
		{
			std::cout << "\t" << field.first << ":";
			// ids and pids are a special case because they contain vectors of 16-bit integers
			if (field.first.rfind("ids") == field.first.size()-3)
				for (size_t i = 0; i < field.second.length(); i += 2)
					std::cout << " " << (int(field.second[i])<<8) + int(field.second[i+1]);
			else
				std::cout << " " << field.second;
			std::cout << std::endl;
		}
	}
}
