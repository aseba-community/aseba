/*
 asebahttp - a switch to bridge HTTP to Aseba
 2014-12-01 David James Sherman <david dot sherman at inria dot fr>
 
 Provide a simple REST interface with introspection for Aseba devices.

 Unit tests:
 1. Aseba::HttpRequest object
 2. Aseba::HttpInterface hub -- "asebadummynode 0" must be running
 3. JSON parsing for integer arrays
*/

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"       // Catch is header-only
#if defined(_WIN32) && defined(__MINGW32__)
// Avoid conflict from /mingw32/.../include/winerror.h */
#undef ERROR_STACK_OVERFLOW
#endif
#include "../switches/http/http.h"

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

/*
  Use Catch to write tests in Behavior-driven design (BDD) style
*/

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
        req.initialize("GET", "/uri/a/b/c", "HTTP/1.0", cxn);
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
    REQUIRE( this != NULL );
    for (int i = 50; --i; )
        this->step(20);
    REQUIRE( asebaStream != NULL );
    REQUIRE( ! nodesDescriptions.empty() );
    REQUIRE( nodesDescriptions[1].name.size() != 0 );
};

TEST_CASE_METHOD(Aseba::HttpInterface, "StreamResponseQueueMap should manage pending responses" ) {
    Dashel::Stream* stream1 = (Dashel::Stream*)0x1111001;
    Aseba::HttpRequest* req11 = new Aseba::HttpRequest;
    Aseba::HttpRequest* req12 = new Aseba::HttpRequest;
    Dashel::Stream* stream2 = (Dashel::Stream*)0x1111002;
    Aseba::HttpRequest* req21 = new Aseba::HttpRequest;
    Aseba::HttpRequest* req22 = new Aseba::HttpRequest;
    Aseba::HttpRequest* req23 = new Aseba::HttpRequest;
    GIVEN( "queue was initialized" ) {
        REQUIRE( pendingResponses.size() == 0 );
        WHEN( "insert pendingResponses responses in queues" ) {
            scheduleResponse(stream1, req11);
            scheduleResponse(stream1, req12);
            scheduleResponse(stream2, req21);
            scheduleResponse(stream2, req22);
            scheduleResponse(stream2, req23);
            THEN( "queues have correct values" ) {
                REQUIRE( pendingResponses[stream1].size() == 2 );
                Aseba::HttpInterface::ResponseQueue::iterator i1 = pendingResponses[stream1].begin();
                REQUIRE( *i1++ == req11 );
                REQUIRE( *i1   == req12 );
                REQUIRE( pendingResponses[stream2].size() == 3 );
                Aseba::HttpInterface::ResponseQueue::iterator i2 = pendingResponses[stream2].begin();
                REQUIRE( *i2++ == req21 );
                REQUIRE( *i2++ == req22 );
                REQUIRE( *i2   == req23 );
                WHEN( "update results" ) {
                    finishResponse(req11, 200, "result 11");
                    finishResponse(req12, 200, "result 12");
                    finishResponse(req21, 201, "result 21");
                    finishResponse(req22, 202, "result 22");
                    finishResponse(req23, 203, "result 23");
                    i1 = pendingResponses[stream1].begin();
                    i2 = pendingResponses[stream2].begin();
                    REQUIRE( (*i1++)->result == "result 11" );
                    REQUIRE( (*i1  )->result == "result 12" );

                    REQUIRE( (*i2  )->status == 201 );
                    REQUIRE( (*i2++)->result == "result 21" );

                    REQUIRE( (*i2  )->status == 202 );
                    REQUIRE( (*i2++)->result == "result 22" );

                    REQUIRE( (*i2  )->status == 203 );
                    REQUIRE( (*i2  )->result == "result 23" );
                    WHEN( "unschedule responses" ) {
                        unscheduleResponse(stream2, req22);
                        i2 = pendingResponses[stream2].begin();
                        REQUIRE( (*i2++)->result == "result 21" );
                        REQUIRE( (*i2  )->result == "result 23" );
                        unscheduleResponse(stream1, req11);
                        unscheduleResponse(stream1, req11);
                        unscheduleResponse(stream1, req11);
                        i1 = pendingResponses[stream1].begin();
                        REQUIRE( (*i1)->result == "result 12" );
                        unscheduleResponse(stream1, req12);
                        REQUIRE( pendingResponses[stream1].size() == 0 );
                        WHEN( "unschedule all responses") {
                            REQUIRE( pendingResponses[stream2].size() == 2 );
                            unscheduleAllResponses(stream2);
                            REQUIRE( pendingResponses[stream2].size() == 0 );
                        }
                    }
                }
            }
        }
    }
}

typedef std::vector<std::string> strings;

TEST_CASE_METHOD(Aseba::HttpInterface, "JSON input is empty", "[empty]" ) {
    std::string content = "";
    strings values;
    REQUIRE( values.size() == 0 );

    GIVEN( "input is empty" ) {
        parse_json_form(content, values); // no CHECK_THROWS because catch
	REQUIRE( values.size() == 0 );
    }
}

TEST_CASE_METHOD(Aseba::HttpInterface, "JSON input is integer array", "[array]" ) {
    std::string content;
    strings values;
    REQUIRE( values.size() == 0 );

    GIVEN( "a JSON array of one value " ) {
        content = "[42]";
        WHEN( "parse JSON string " + content ) {
	    Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "array of single value 42 is returned" ) {
	        REQUIRE( values.size() == 1 );
	        REQUIRE( atoi(values[0].c_str()) == 42 );
	    }
        }
    }
    GIVEN( "a JSON array of zero values " ) {
        content = "[]";
        WHEN( "parse JSON string " + content ) {
	    Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "an empty array is returned" ) {
	        REQUIRE( values.size() == 0 );
	    }
        }
    }
    GIVEN( "a JSON array of four mixed values " ) {
        content = "[42,63,27,\"hike\"]";
        WHEN( "parse JSON string " + content ) {
	    Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "array of four values is returned" ) {
	        REQUIRE( values.size() == 0 );
	    }
        }
    }
    GIVEN( "a JSON array of two values with white space" ) {
        content = " [\t42,\r\n\t 63\n\r]\t\n";
        WHEN( "parse JSON string with white space " + content ) {
	    Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "array of two values is returned" ) {
	        REQUIRE( values.size() == 2 );
	        REQUIRE( atoi(values[0].c_str()) == 42 );
	        REQUIRE( atoi(values[1].c_str()) == 63 );
	    }
        }
    }
    GIVEN( "a malformed JSON array with a bad separator" ) {
        content = "[42;63]";
        WHEN( "parse JSON string with white space " + content ) {
	    Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "an empty array is returned" ) {
	        REQUIRE( values.size() == 0 );
	    }
        }
    }
    GIVEN( "a malformed JSON array with no opening" ) {
        content = "42,63]";
        WHEN( "parse JSON string with white space " + content ) {
	    Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "an empty array is returned" ) {
	        REQUIRE( values.size() == 0 );
	    }
        }
    }
    GIVEN( "a malformed JSON array with no close" ) {
        content = "[42,63";
        WHEN( "parse JSON string with white space " + content ) {
	    Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "an empty array is returned" ) {
	        REQUIRE( values.size() == 0 );
	    }
        }
    }
    GIVEN( "a JSON object" ) {
        content = "{ \"abc\": 12, \"def\": [1,2,3] }";
        WHEN( "parse JSON string " + content ) {
            Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "an empty array is returned" ) {
                REQUIRE( values.size() == 0 );
            }
        }
    }
    GIVEN( "a JSON array containing an object" ) {
        content = "[42, { \"abc\": 12, \"def\": [1,2,3] }, 63]";
        WHEN( "parse JSON string " + content ) {
            Aseba::HttpInterface::parse_json_form(content, values);
            THEN( "an empty array is returned" ) {
                REQUIRE( values.size() == 0 );
            }
        }
    }
}
