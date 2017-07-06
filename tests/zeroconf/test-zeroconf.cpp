/*
 test-zeroconf - unit testing for Aseba::Zeroconf
 2017-06-28 David James Sherman <david dot sherman at inria dot fr>
 
 Unit tests:
 1. Aseba::Zeroconf as container
 2. Aseba::Zeroconf advertise and find local targets
 3. Aseba::Zeroconf advertise and find remote targets
*/

#define CATCH_CONFIG_MAIN  // Ask Catch to provide a main()
#include "../catch.hpp"       // Catch is header-only
#if defined(_WIN32) && defined(__MINGW32__)
// Avoid conflict from /mingw32/.../include/winerror.h */
#undef ERROR_STACK_OVERFLOW
#endif

#include "../../common/zeroconf/zeroconf.h"

/*
 Setup: create Dashel Stream to imitate tcp: target
 */
#include <dashel/dashel.h>
class MockHub: public Dashel::Hub
{
	Dashel::Stream* stream;
public:
	Dashel::Stream* listen() { return stream = Dashel::Hub::connect("tcpin:port=0"); }
} mock;
Dashel::Stream* mock_stream = mock.listen();

/*
  Use Catch to write tests in Behavior-driven design (BDD) style
*/

SCENARIO( "Aseba::Zeroconf is a container", "[container]" ) {
	GIVEN( "Zeroconf collection initialized by name and by stream" ) {
		Aseba::Zeroconf zs;
		REQUIRE( zs.size() == 0 );

		zs.insert("Fizbin", 33333); // explicit name
		zs.insert(mock_stream); // generated name Aseba Local
		REQUIRE( zs.size() == 2 );

		WHEN( "targets are searched for" ) {
			auto fizbin_i = zs.find("Fizbin");
			auto foobar_i = zs.find("Aseba Local");
			THEN( "both targets are found" ) {
				REQUIRE( fizbin_i != zs.end() );
				REQUIRE( fizbin_i->name.find("Fizbin") == 0 );
				REQUIRE( foobar_i != zs.end() );
				REQUIRE( foobar_i->name.find("Aseba Local") == 0 );
			}
		}

		WHEN( "targets are traversed with a loop" ) {
			for (auto & target: zs)
			{
				REQUIRE( target.regtype.find("_aseba._tcp") == 0 );
				REQUIRE( target.domain.find("local.") == 0 );
				REQUIRE( target.local == true );
			}
		}

		WHEN( "targets are erased" ) {
			zs.erase(zs.begin()+1);
			auto fizbin_i = zs.find("Fizbin");
			THEN( "collection contains Fizbin but not Aseba Local" ) {
				REQUIRE( zs.size() == 1 );
				REQUIRE( fizbin_i != zs.end() );
				REQUIRE( fizbin_i->name.find("Fizbin") == 0 );
			}
			zs.clear();
			THEN( "clearing collection removes all elements" ) {
				REQUIRE( zs.size() == 0 );
			}
		}
	}
}

SCENARIO( "Aseba::Zeroconf local targets can be advertised and browsed", "[local][browse][container]" ) {
	GIVEN( "Zeroconf collection with 10 local targets" ) {
		Aseba::Zeroconf zs;

		// Add 10 local targets
		for (unsigned int i=0; i<10; ++i)
		{
			Aseba::Zeroconf::TxtRecord txt{5, "Test", {10+i,20+i}, {8,8}};
			zs.insert("Fizbin", 33333+i).advertise(txt); // NB original names not unique
		}
		REQUIRE( zs.size() == 10 );

		WHEN( "local targets are browsed" ) {
			zs.browse();
			THEN( "local resolved target has correct metadata" ) {
				REQUIRE( zs.size() >= 10 );
				for (auto & target: zs)
				{
					// Resolve the host name and port of this target, retrieve TXT record
					target.resolve();
					// Check fields in SRV record
					REQUIRE( target.port >= 33333);
					REQUIRE( target.name.find("Fizbin") == 0 );
					REQUIRE( target.host.find("localhost") == 0 );
					REQUIRE( target.regtype.find("_aseba._tcp") == 0 );
					// Check fields in TXT record
					for (auto const& field: target.properties)
						REQUIRE( field.second.length() > 0 );
					REQUIRE( target.properties["ids"].length() == 4 );
					REQUIRE( target.properties["pids"].length() == 4 );
					REQUIRE( target.properties["type"].find("Test") == 0 );
					REQUIRE( target.properties["protovers"].find("5") == 0 );
				}
			}
			THEN( "zeroconf has made unique names" ) {
				// TODO
			}
		}
	}
}

// This test is hidden by default, since basic Zeroconf will block on the browse call to
// the system zeroconf daemon, unless another process (or thread) has advertised a target

SCENARIO( "Aseba::Zeroconf can browse", "[!hide][browse]" ) {
	GIVEN( "A Zeroconf instance but no local targets" ) {
		Aseba::Zeroconf zs;

		WHEN( "targets are browsed" ) {
			zs.browse(); // will block until a target is advertised
			THEN( "no local targets are found" ) {
				for (auto & target: zs)
					REQUIRE( target.local != true );
			}
		}
	}
}
