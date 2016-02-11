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

#ifndef ASEBA_HTTP
#define ASEBA_HTTP

#include <stdint.h>
#include <list>
#include <queue>
#include <dashel/dashel.h>
#include "../../common/msg/msg.h"
#include "../../common/msg/NodesManager.h"

#if defined(_WIN32) && defined(__MINGW32__)
/* This is a workaround for MinGW32, see libxml/xmlexports.h */
#define IN_LIBXML
#endif
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

namespace Aseba
{
    /**
     \defgroup http Software router of messages on TCP and HTTP-over-TCP.
     */
    /*@{*/
    
    class HttpRequest;
    
    //! HTTP interface for aseba network
    class HttpInterface:  public Dashel::Hub, public Aseba::NodesManager
    {
    public: 
        typedef std::vector<std::string>      strings;
        typedef std::list<HttpRequest*>       ResponseQueue;
        typedef std::set<HttpRequest*>        ResponseSet;
        typedef std::pair<unsigned,unsigned>  VariableAddress;
        typedef std::map<std::string, Aseba::VariablesMap>      NodeNameVariablesMap;
        typedef std::map<VariableAddress, ResponseSet>          VariableResponseSetMap;
        typedef std::map<Dashel::Stream*, ResponseQueue>        StreamResponseQueueMap;
        typedef std::map<HttpRequest*, std::set<std::string> >  StreamEventSubscriptionMap;

    protected:
        // streams
        Dashel::Stream* asebaStream;
        Dashel::Stream* httpStream;
        StreamResponseQueueMap     pendingResponses;
        VariableResponseSetMap     pendingVariables;
        StreamEventSubscriptionMap eventSubscriptions;
        std::map<Dashel::Stream*, HttpRequest> httpRequests;
        std::set<Dashel::Stream*>  streamsToShutdown;
        unsigned nodeId;
        bool nodeDescriptionComplete;
        // debug variables
        bool verbose;
        int iterations;
        
        // Extract definitions from AESL file
        Aseba::CommonDefinitions commonDefinitions;
        NodeNameVariablesMap allVariables;

        //variable cache
        std::map<std::pair<unsigned,unsigned>, std::vector<short> > variable_cache;
        
    public:
        //default values needed for unit testing
        HttpInterface(const std::string& target="tcp:127.0.0.1;port=33333", const std::string& http_port="3000", const int iterations=-1);
        virtual void run();
        virtual bool descriptionReceived();
        virtual void broadcastGetDescription();
        virtual void evNodes(HttpRequest* req, strings& args);
        virtual void evVariableOrEvent(HttpRequest* req, strings& args);
        virtual void evSubscribe(HttpRequest* req, strings& args);
        virtual void evLoad(HttpRequest* req, strings& args);
        virtual void evReset(HttpRequest* req, strings& args);
        virtual void aeslLoadFile(const std::string& filename);
        virtual void aeslLoadMemory(const char* buffer, const int size);
        virtual void updateVariables(const std::string nodeName);
        
        virtual void scheduleResponse(Dashel::Stream* stream, HttpRequest* req);
        virtual void addHeaders(HttpRequest* req, strings& headers);
        virtual void finishResponse(HttpRequest* req, unsigned status, std::string result);
        virtual void appendResponse(HttpRequest* req, unsigned status, const bool& keep_open, std::string result);
        virtual void sendAvailableResponses();
        virtual void unscheduleResponse(Dashel::Stream* stream, HttpRequest* req);
        virtual void unscheduleAllResponses(Dashel::Stream* stream);
        
    protected:
        /* // reimplemented from parent classes */
        virtual void connectionCreated(Dashel::Stream* stream);
        virtual void connectionClosed(Dashel::Stream* stream, bool abnormal);
        virtual void incomingData(Dashel::Stream* stream);
		virtual void sendMessage(const Message& message);
        virtual void nodeDescriptionReceived(unsigned nodeId);
        // specific to http interface
        virtual void sendEvent(const std::string nodeName, const strings& args);
        virtual void sendSetVariable(const std::string nodeName, const strings& args);
        virtual std::pair<unsigned,unsigned> sendGetVariables(const std::string nodeName, const strings& args);
        virtual bool getNodeAndVarPos(const std::string& nodeName, const std::string& variableName, unsigned& nodeId, unsigned& pos);
        virtual void aeslLoad(xmlDoc* doc);
        virtual void incomingVariables(const Variables *variables);
        virtual void incomingUserMsg(const UserMessage *userMsg);
        virtual void routeRequest(HttpRequest* req);
        
        // helper functions
		bool run2s();
        bool getNodeAndVarPos(const std::string& nodeName, const std::string& variableName, unsigned& nodeId, unsigned& pos) const;
        bool compileAndSendCode(const std::wstring& source, unsigned nodeId, const std::string& nodeName);
        virtual void parse_json_form(std::string content, strings& values);

    };
    
    class HttpRequest
    {
    public:
        typedef std::vector<std::string> strings;
        std::string method;
        std::string uri;
        std::string protocol_version;
        Dashel::Stream* stream;
        strings tokens;  // parsed URI
        std::map<std::string,std::string> headers; // incoming headers
        std::string content; // incoming payload
        bool ready; // incoming request is ready
        unsigned status; // outging status
        std::string result; // outgoing payload
        strings outheaders;
        bool more; // keep connection open for SSE
    protected:
        bool headers_done; // flag for header parsing
        bool status_sent;  // flag for SSE
        bool verbose;
        
    public:
        HttpRequest();
        virtual ~HttpRequest() {};
        virtual bool initialize( Dashel::Stream *stream); //
        virtual bool initialize( std::string const& start_line, Dashel::Stream *stream); //
        virtual bool initialize( std::string const& method,  std::string const& uri, std::string const& _protocol_version, Dashel::Stream *stream);
        virtual void incomingData();
        virtual void sendResponse();
        virtual void sendStatus();
        virtual void sendPayload();
    };

    class InterruptException : public std::exception
    {
    public:
        InterruptException(int s) : S(s) {}
        int S;
    };
    
    /*@}*/
};

#endif
