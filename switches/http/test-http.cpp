#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "http.h"
//#include "../../common/consts.h"
//#include "../../common/types.h"
//#include "../../common/utils/utils.h"
//#include "../../transport/dashel_plugins/dashel-plugins.h"

class Dummy: public Dashel::Hub
{
public:
    Dummy()
    {
        instream = connect("file:name=testdata-HttpRequest.txt;mode=read");
        outstream = connect("stdout:");
    }
    void connectionCreated(Dashel::Stream *stream) {};
    void connectionClosed(Dashel::Stream *stream, bool abnormal) {};
    void incomingData(Dashel::Stream *stream) {};
    Dashel::Stream *instream, *outstream;
};

Dummy* dummy;

TEST_CASE( "Dashel::Hub create" ) {
    dummy = new Dummy;
    REQUIRE( dummy->instream != NULL );
}

Aseba::HttpRequest req;

TEST_CASE( "HttpRequest using default constructor", "[init]" ) {
    REQUIRE( req.method.empty() );
    REQUIRE( req.uri.empty() );
    REQUIRE( req.stream == NULL );
    REQUIRE( req.tokens.empty() );
    REQUIRE( req.headers.empty() );
    REQUIRE( req.content.empty() );
}

SCENARIO( "HttpRequest should be initialized", "[init]" ) {
    Dashel::Stream* cxn = dummy->instream;
    
    GIVEN( "HttpRequest initialized by string" ) {
        req.initialize("GET /uri/a/b/c HTTP/1.1\r\n", cxn);
        REQUIRE( req.method.find("GET")==0 );
        REQUIRE( req.uri.find("/uri/a/b/c")==0 );
        REQUIRE( req.tokens[1].find("a")==0 );
        REQUIRE( req.stream == cxn );
    }
    GIVEN( "HttpRequest initialized by parts" ) {
        req.initialize("GET", "/uri/a/b/c", cxn);
        REQUIRE( req.method.find("GET")==0 );
        REQUIRE( req.uri.find("/uri/a/b/c")==0 );
        REQUIRE( req.tokens[1].find("a")==0 );
        REQUIRE( req.stream == cxn );
    }
}

SCENARIO( "HttpRequests should be read from file", "[read]" ) {
    GIVEN( "Stream was initialized" ) {
        REQUIRE( dummy->instream != NULL );
        WHEN( "read request 1 from file" ) {
            req.initialize(dummy->instream);
            req.incomingData(); // from dummy->instream
            THEN( "HttpRequest is correct" ) {
                REQUIRE( req.ready );
                REQUIRE( req.method.find("GET")==0 );
                REQUIRE( req.uri.find("/uri/a/b/c")==0 );
                REQUIRE( req.tokens[3].find("c")==0 );
                REQUIRE( req.headers["Content-Length"].find("19")==0 );
                REQUIRE( req.content.find("payload uri a b c\r\n")==0 );
            }
        }
        AND_WHEN( "read request 2 from file" ) {
            req.initialize(dummy->instream);
            req.incomingData(); // from dummy->instream
            THEN( "HttpRequest is correct" ) {
                REQUIRE( req.ready );
                REQUIRE( req.method.find("GET")==0 );
                REQUIRE( req.uri.find("/uri")==0 );
                REQUIRE( req.uri.size()==4 );
                REQUIRE( req.tokens[0].find("uri")==0 );
                REQUIRE( req.content.size()==0 );
            }
        }
    }
};

TEST_CASE_METHOD(Aseba::HttpInterface, "Aseba::HttpInterface should be initialized", "[create]") {
    Aseba::HttpInterface* network = this;
    REQUIRE( this != NULL );
    for (int i = 50; --i; )
        network->step(20);
    REQUIRE( asebaStream != NULL );
    REQUIRE( ! nodesDescriptions.empty() );
    REQUIRE( nodesDescriptions[1].name.size() != 0 );
};