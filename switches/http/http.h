/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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
#include <queue>
#include <dashel/dashel.h>
#include <libjson/libjson.h>
#include "../../common/msg/msg.h"
#include "../../common/msg/descriptions-manager.h"

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
    class HttpInterface:  public Dashel::Hub, public Aseba::DescriptionsManager
    {
    public: 
        typedef std::vector<std::string> strings;
        typedef std::map<std::string, Aseba::VariablesMap> NodeNameToVariablesMap;
        // embedded http server
        struct pending_variable  {  unsigned source;  unsigned start;  std::string name; Dashel::Stream* connection; JSONNode result;};
        typedef std::map<std::pair<unsigned,unsigned>, pending_variable> pending_variable_map;
        struct http_request {
            std::string method;
            std::string uri;
            strings tokens;
            std::map<std::string,std::string> headers;
            bool headers_done;
            std::string content;
        };
        JSONNode nodeInfoJson;

    protected:
        // streams
        Dashel::Stream* asebaStream;
        Dashel::Stream* httpStream;
        std::vector<char*> knownNodes;
        std::map<Dashel::Stream*, HttpRequest*> httpRequests;
        std::map<Dashel::Stream*, std::set<std::string> > eventSubscriptions;
        unsigned nodeId;
        // debug variables
        bool verbose;
        
        // Extract definitions from AESL file
        Aseba::CommonDefinitions commonDefinitions;
        NodeNameToVariablesMap allVariables;
        
        // variables
        pending_variable_map pending_vars_map;
        
        //variable cache
        std::map<std::pair<unsigned,unsigned>, std::vector<short> > variable_cache;
        
    public:
        HttpInterface(const std::string& target, const std::string& http_port);
        virtual void evNodes(Dashel::Stream* conn, strings& args);
        virtual void evVariableOrEvent(Dashel::Stream* conn, strings& args);
        virtual void evSubscribe(Dashel::Stream* conn, strings& args);
        virtual void evLoad(Dashel::Stream* conn, strings& args);
        virtual void evReset(Dashel::Stream* conn, strings& args);
        virtual void aeslLoadFile(const std::string& filename);
        virtual void aeslLoadMemory(const char* buffer, const int size);
        virtual void updateVariables(const std::string nodeName);
        
    protected:
        /* // reimplemented from parent classes */
        virtual void connectionCreated(Dashel::Stream* stream);
        virtual void connectionClosed(Dashel::Stream* stream, bool abnormal);
        virtual void incomingData(Dashel::Stream* stream);
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
        Dashel::Stream* stream;
        strings tokens;
        std::map<std::string,std::string> headers;
        std::string content;
        bool complete;
    protected:
        bool headers_done;
        bool verbose;
        
    public:
        HttpRequest();
        virtual ~HttpRequest();
        HttpRequest( std::string const& method,  std::string const& uri, Dashel::Stream *stream);
        virtual void incomingData();
        virtual void sendStatus(unsigned status, std::string& payload, strings& headers);
        virtual void sendStatus(unsigned status, std::string& payload);
        virtual void sendStatus(unsigned status, strings& headers);
        virtual void sendStatus(unsigned status);
    };


    /*@}*/
};

#endif
