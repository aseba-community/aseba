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
#include "mongoose.h"
#include "../../common/msg/msg.h"
#include "../../common/msg/descriptions-manager.h"

#if defined(_WIN32) && defined(__MINGW32__)
/* This is a workaround for MinGW32, see libxml/xmlexports.h */
#define IN_LIBXML
#endif
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace Aseba
{
    /**
     \defgroup http Software router of messages on TCP and HTTP-over-TCP.
     */
    /*@{*/
    
    //! HTTP interface for aseba network
    class HttpInterface:  public Dashel::Hub, public Aseba::DescriptionsManager
    {
    public: 
        typedef std::vector<std::string> strings;
        typedef std::map<std::string, Aseba::VariablesMap> NodeNameToVariablesMap;
        // embedded http server
        struct mg_server *http_server;
        struct pending_variable  {  unsigned source;  unsigned start;  std::string name; struct mg_connection* connection; std::string result;};
        typedef std::map<std::pair<unsigned,unsigned>, pending_variable> pending_variable_map;
        // remember pending variables
        std::map<struct mg_connection *, pending_variable *> pending_cxn_map;

    protected:
        // streams
        Dashel::Stream* asebaStream;
        std::vector<char*> knownNodes;
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
        virtual void aeslLoadFile(const std::string& filename);
        virtual void aeslLoadMemory(const char* buffer, const int size);
        virtual void updateVariables(const std::string nodeName);
        virtual mg_result evNodes(struct mg_connection* conn, strings& args);
        virtual mg_result evVariableOrEvent(struct mg_connection* conn, strings& args);
        virtual mg_result evSubscribe(struct mg_connection *conn, strings& args);
        virtual mg_result evLoad(struct mg_connection *conn, strings& args);
        virtual mg_result evPoll(struct mg_connection *conn, strings& args);
        virtual mg_result evReset(struct mg_connection *conn, strings& args);
        
    protected:
        /* // reimplemented from parent classes */
        virtual void connectionCreated(Dashel::Stream* stream);
        virtual void connectionClosed(Dashel::Stream* stream, bool abnormal);
        virtual void incomingData(Dashel::Stream* stream);
        virtual void nodeDescriptionReceived(unsigned nodeId);
        // specific to http interface
        virtual void getVariables(const std::string nodeName, const strings& args);
        virtual void setVariable(const std::string nodeName, const strings& args);
        virtual void sendEvent(const std::string nodeName, const strings& args);
        virtual bool getNodeAndVarPos(const std::string& nodeName, const std::string& variableName, unsigned& nodeId, unsigned& pos);
        virtual void aeslLoad(xmlDoc* doc);
        
        // helper functions
        bool getNodeAndVarPos(const std::string& nodeName, const std::string& variableName, unsigned& nodeId, unsigned& pos) const;
        bool compileAndSendCode(const std::wstring& source, unsigned nodeId, const std::string& nodeName);

    };
    /*@}*/
};

#endif
