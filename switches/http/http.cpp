/*
 asebahttp - a switch to bridge HTTP to Aseba
 2014-12-01 David James Sherman <david dot sherman at inria dot fr>
 
 Provide a simple REST interface with introspection for Aseba devices.
 
    GET  /nodes                                 - JSON list of all known nodes
    GET  /nodes/:NODENAME                       - JSON attributes for :NODENAME
    PUT  /nodes/:NODENAME                       - write new Aesl program (file= in multipart/form-data)
    GET  /nodes/:NODENAME/:VARIABLE             - retrieve JSON value for :VARIABLE
    POST /nodes/:NODENAME/:VARIABLE             - send new values(s) for :VARIABLE
    POST /nodes/:NODENAME/:EVENT                - call an event :EVENT
    GET  /events[/:EVENT]*                      - create SSE stream for all known nodes
    GET  /nodes/:NODENAME/events[/:EVENT]*      - create SSE stream for :NODENAME
 
 Server-side event (SSE) streams are updated as events arrive.
 SSE stream for reserved event "poll" sends variables at approx 10 Hz.
 If a variable and an events have the same name, it is the EVENT the is called.
 
 DONE (mostly):
    - Dashel connection to one Thymio-II and round-robin scheduling between Dashel and Mongoose
    - read Aesl program at launch, upload to Thymio-II, and record interface for introspection
    - GET /nodes, GET /nodes/:NODENAME with introspection
    - POST /nodes/:NODENAME/:VARIABLE (sloppily allows GET /nodes/:NODENAME/:VARIABLE/:VALUE[/:VALUE]*)
    - POST /nodes/:NODENAME/:EVENT (sloppily allows GET /nodes/:NODENAME/:EVENT[/:VALUE]*)
    - form processing for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
    - handle asynchronous variable reporting (GET /nodes/:NODENAME/:VARIABLE)
    - JSON format for variable reporting (GET /nodes/:NODENAME/:VARIABLE)
    - implement SSE streams and event filtering (GET /events) and (GET /nodes/:NODENAME/events)
    - program flashing (PUT  /nodes/:NODENAME)
      use curl --data-urlencode "file=$(cat vmcode.aesl)" -X PUT http://127.0.0.1:3000/nodes/thymio-II
 
 TODO:
    - accept JSON payload rather than HTML form for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
    - handle more than just one Thymio-II node!
 
 This code includes source code from the Mongoose embedded web server from Cesanta
 Software Limited, Dublin, Ireland, and licensed under the GPL 2.
 
 This code borrows heavily from the rest of Aseba, especially switches/medulla and
 examples/clients/cpp-shell, which bear the copyright and LGPL 3 licensing notice below.
 
 */
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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <set>
#include <valarray>
#include <vector>
#include <iterator>
#include "http.h"
#include "mongoose.h"
#include "../../common/consts.h"
#include "../../common/types.h"
#include "../../common/utils/utils.h"
#include "../../transport/dashel_plugins/dashel-plugins.h"

#if defined(_WIN32) && defined(__MINGW32__)
/* This is a workaround for MinGW32, see libxml/xmlexports.h */
#define IN_LIBXML
#endif
#include <libxml/parser.h>
#include <libxml/tree.h>

#if DASHEL_VERSION_INT < 10003
#	error "You need at least Dashel version 1.0.3 to compile Http"
#endif // DAHSEL_VERSION_INT

// forward
//static int user_message(struct mg_connection *conn, enum mg_event ev);
static int http_event_handler(struct mg_connection *conn, enum mg_event ev);


// hack because I don't understand locales yet
char* wstringtocstr(std::wstring w, char* name)
{
    //char* name = (char*)malloc(128);
    std::wcstombs(name, w.c_str(), 127);
    return name;
}

// split strings on delimiter
void split_string(std::string s, char delim, std::vector<std::string>& tokens)
{
    std::stringstream ss( s );
    std::string item;
    //std::getline(ss, item, delim);
    while (std::getline(ss, item, delim))
        tokens.push_back(item);
    if (tokens[0].size() == 0)
        tokens.erase(tokens.begin(),tokens.begin()+1);
}

namespace Aseba
{
    using namespace std;
    using namespace Dashel;
    
    /** \addtogroup http */
    /*@{*/
    
    HttpInterface::HttpInterface(const std::string& asebaTarget, const std::string& http_port) :
    asebaStream(0),
    nodeId(0),
    verbose(false)
    {
        // connect to the Aseba target
        std::cerr << "HttpInterface connect asebaTarget " << asebaTarget << "\n";
        connect(asebaTarget);
        
        // request a description for aseba target
        GetDescription getDescription;
        getDescription.serialize(asebaStream);
        asebaStream->flush();
    }
    
    void HttpInterface::connectionCreated(Dashel::Stream *stream)
    {
        if (!asebaStream)
        {
            if (verbose)
                cout << "Incoming Aseba connection from " << stream->getTargetName() << endl;
            asebaStream = stream;
        }
    }
    
    void HttpInterface::connectionClosed(Stream * stream, bool abnormal)
    {
        if (stream == asebaStream)
        {
            asebaStream = 0;
            cout << "Connection closed to Aseba target" << endl;
            stop();
        }
        else
            abort();
    }
    
    void HttpInterface::nodeDescriptionReceived(unsigned nodeId)
    {
        if (verbose)
            wcout << L"Received description for " << getNodeName(nodeId) << endl;
        this->nodeId = nodeId;
    }
    
    void HttpInterface::incomingData(Stream *stream)
    {
        if (stream == asebaStream) {
            // receive message
            Message *message(Message::receive(stream));
            
            // pass message to description manager, which builds
            // the node descriptions in background
            DescriptionsManager::processMessage(message);
            
            if (! http_server)
            {
                if (verbose)
                    cout << "incomingData NO http_server" << endl;
                return;
            }
            
            // if variables, check for pending requests
            const Variables *variables(dynamic_cast<Variables *>(message));
            if (variables)
            {
                char* result = (char *)malloc(10 * variables->variables.size()), *pos = result;
                for (size_t i = 0; i < variables->variables.size(); ++i)
                    pos += sprintf(pos, "%s%d", (i>0 ? "," : ""), variables->variables[i]);
                if (verbose)
                    cout << "incomingData var ("<<variables->source<<","<<variables->start<<") = "<<result << endl;
                
                variable_cache[std::make_pair(variables->source,variables->start)] = std::vector<short>(variables->variables);
                
                pending_variable pending = pending_vars_map[std::make_pair(variables->source,variables->start)];
                if (pending.connection) {
                    pending.result = result;
                    pending_vars_map[std::make_pair(variables->source,variables->start)] = pending;
                }
                
                free(result);
            }
            
            // if event, retransmit it on an HTTP SSE channel if one exists
            const UserMessage *userMsg(dynamic_cast<UserMessage *>(message));
            if (userMsg)
            {
                if (verbose)
                    cout << "incomingData msg ("<< userMsg->type <<","<< &userMsg->data <<")" << endl;
                // In the HTTP world we set up a stream of Server-Sent Events for this.
                // Note that event name is in commonDefinitions.events[userMsg->type].name
                for (struct mg_connection * c = mg_next(http_server, NULL); c != NULL; c = mg_next(http_server, c)) {
                    if (! c->connection_param)
                        continue;
                    strings& tokens = *((strings *)c->connection_param);
                    if (tokens.size() >= 1 && strcmp(tokens[0].c_str(),"events")==0)
                    {
                        const char* filter_event = tokens[1].c_str();
                        char this_event[128];
                        wstringtocstr(commonDefinitions.events[userMsg->type].name, this_event);
                        if (tokens.size() >= 2 && strcmp(filter_event,this_event)!=0)
                            return;
                        mg_printf_data(c, "data: %s", this_event);
                        free(this_event);
                        for (size_t i = 0; i < userMsg->data.size(); ++i)
                            mg_printf_data(c, " %d", userMsg->data[i]);
                        mg_printf_data(c, "\n\n");
                    }
                }
            }
            
            delete message;
            
            return; //incomingAsebaData(stream);
        }
        else
            abort();
        return;
    }
    
    // Handlers for HTTP requests
    
    mg_result HttpInterface::evNodes(struct mg_connection *conn, strings& args)
    {
        bool do_one_node(args.size() > 0 && !strcmp(args[0].c_str(),"thymio-II"));
        char indent[] = "  ";

        const NodesDescriptionsMap::const_iterator descIt(nodesDescriptions.find(nodeId));
        const NodeDescription& description(descIt->second);
        
        strings desc_vars;
        desc_vars.push_back("_id");
        desc_vars.push_back("_fwversion");
        desc_vars.push_back("_productId");
        getVariables("thymio-II", desc_vars);

        if (!do_one_node)
            mg_printf_data(conn, "[\n");
        else
            indent[0] = 0;
        
        char wbuf[128];

        mg_printf_data(conn, "%s{\n", indent);
        mg_printf_data(conn, "%s  \"name\": \"%s\",\n", indent, wstringtocstr(description.name,wbuf));
        mg_printf_data(conn, "%s  \"protocolVersion\": \"%d\",\n", indent, description.protocolVersion);
        //mg_printf_data(conn, "    \"id\": \"%d\",\n", XXX);
        //mg_printf_data(conn, "    \"productId\": \"%d\",\n", XXX);
        //mg_printf_data(conn, "    \"firmwareVersion\": \"%d\",\n", XXX);
        if (do_one_node)
        {
            // For one node, list details
            mg_printf_data(conn, "    \"bytecodeSize\": \"%d\",\n",  description.bytecodeSize);
            mg_printf_data(conn, "    \"variablesSize\": \"%d\",\n", description.variablesSize);
            mg_printf_data(conn, "    \"stackSize\": \"%d\",\n",     description.stackSize);
            // named variables
            mg_printf_data(conn, "    \"namedVariables\": {\n" );
            for (size_t i = 0; i < description.namedVariables.size(); ++i)
                mg_printf_data(conn, "      \"%s\": %d,\n",
                               wstringtocstr(description.namedVariables[i].name,wbuf), description.namedVariables[i].size);
            mg_printf_data(conn, "    },\n");
            // local events variables
            mg_printf_data(conn, "    \"localEvents\": [\n" );
            for (size_t i = 0; i < description.localEvents.size(); ++i)
                mg_printf_data(conn, "      { \"name\": \"%s\", \"description\": \"%s\" }\n",
                               wstringtocstr(description.localEvents[i].name,wbuf), wstringtocstr(description.localEvents[i].description,wbuf));
            mg_printf_data(conn, "    ],\n");
            // constants from introspection
            mg_printf_data(conn, "    \"constants\": [\n" );
            for (size_t i = 0; i < commonDefinitions.constants.size(); ++i)
                mg_printf_data(conn, "      { \"name\": \"%s\", \"value\": %d }\n",
                               wstringtocstr(commonDefinitions.constants[i].name,wbuf), commonDefinitions.constants[i].value);
            mg_printf_data(conn, "    ],\n");
            // events from introspection
            mg_printf_data(conn, "    \"events\": [\n" );
            for (size_t i = 0; i < commonDefinitions.events.size(); ++i)
                mg_printf_data(conn, "      { \"name\": \"%s\", \"size\": %d }\n",
                               wstringtocstr(commonDefinitions.events[i].name,wbuf), commonDefinitions.events[i].value);
            mg_printf_data(conn, "    ],\n");

        }
        mg_printf_data(conn, "%s},\n", indent);
        
        if (!do_one_node)
            mg_printf_data(conn, "]");
        return MG_TRUE;
    }
    
    mg_result HttpInterface::evVariableOrEvent(struct mg_connection *conn, strings& args)
    {
        //const NodesDescriptionsMap::const_iterator descIt(nodesDescriptions.find(nodeId));
        //const NodeDescription& description(descIt->second);
        
        string nodeName(args[0]);
        size_t eventPos;
        
        if (!commonDefinitions.events.contains(UTF8ToWString(args[1]), &eventPos))
        {
            // this is a variable
            if (strcmp(conn->request_method, "POST") == 0 || conn->query_string != NULL || args.size() >= 3)
            {
                // set variable value
                strings values;
                if (args.size() >= 3)
                    values.assign(args.begin()+1, args.end());
                else
                {
                    // Parse POST form data
                    char buffer[8 * 1024]; // if more would need to use multipart chunking
                    if (mg_get_var(conn, args[1].c_str(), buffer, 8*1024) > 0)
                    {
                        split_string(std::string(buffer), ' ', values);
                        values.insert(values.begin(), args[1].c_str());
                    }
                }
                if (values.size() == 0)
                    return MG_FALSE;
                setVariable(nodeName, values);
                mg_send_data(conn, NULL, 0);
                return MG_TRUE;
            }
            else
            {
                // get variable value
                strings values;
                values.assign(args.begin()+1, args.begin()+2);
                getVariables(nodeName, values);
                
                pending_variable pending = { 0,0, values[0], conn };
                
                unsigned source, start;
                if (getNodeAndVarPos(nodeName, values[0], source, start))
                {
                    pending.source = source;
                    pending.start = start;
                    pending_vars_map[std::make_pair(source,start)] = pending;
                    conn->connection_param = &(pending_vars_map[std::make_pair(source,start)]); // doesn't work, connection forgets this
                    pending_cxn_map[conn] = &(pending_vars_map[std::make_pair(source,start)]);
                }
                //if (verbose)
                    cout << "evVariableOrEevent schedule var " << pending.name << "(" << pending.source << ","
                    << pending.start << ") cxn=" << pending.connection << endl;
                return MG_TRUE; //MG_MORE;
            }
        }
        else
        {
            // this is an event
            // arguments are args 1..N
            strings data;
            data.push_back(args[1]);
            if (args.size() >= 3)
                for (size_t i=2; i<args.size(); ++i)
                    data.push_back((args[i].c_str()));
            else if (strcmp(conn->request_method, "POST") == 0 || conn->query_string != NULL)
            {
                // Parse POST form data
                char buffer[8 * 1024]; // if more would need to use multipart chunking
                if (mg_get_var(conn, args[1].c_str(), buffer, 8*1024) > 0)
                {
                    strings values;
                    split_string(std::string(buffer), ' ', values);
                    for (size_t i=0; i<values.size(); ++i)
                        data.push_back((values[i].c_str()));
                }
            }
            sendEvent(nodeName, data);
            mg_send_data(conn, NULL, 0);
            return MG_TRUE;
        }
    }

    void HttpInterface::sendEvent(const std::string nodeName, const strings& args)
    {
        size_t eventPos;
        
        if (commonDefinitions.events.contains(UTF8ToWString(args[0]), &eventPos))
        {
            // build event and emit
            UserMessage::DataVector data;
            for (size_t i=1; i<args.size(); ++i)
                data.push_back(atoi(args[i].c_str()));
            UserMessage userMessage(eventPos, data);
            userMessage.serialize(asebaStream);
            asebaStream->flush();
        }
        else if (verbose)
            cerr << "sendEvent " << nodeName << ": no event " << args[0] << endl;
    }
    
    void HttpInterface::getVariables(const std::string nodeName, const strings& args)
    {
        for (strings::const_iterator it(args.begin()); it != args.end(); ++it)
        {
            // get node id, variable position and length
            if (verbose)
                cout << "getVariables " << nodeName << " " << *it;
            unsigned nodePos, varPos;
            const bool exists(getNodeAndVarPos(nodeName, *it, nodePos, varPos));
            if (!exists)
                continue;
//            bool ok;
//            const unsigned length(getVariableSize(nodePos, UTF8ToWString(*it), &ok));
//            if (!ok)
//                continue;
            VariablesMap vm = allVariables[nodeName];
            const unsigned length(vm[UTF8ToWString(*it)].second);
            
            if (verbose)
                cout << " (" << nodePos << "," << varPos << "):" << length << "\n";
            // send the message
            GetVariables getVariables(nodePos, varPos, length);
            getVariables.serialize(asebaStream);
        }
        asebaStream->flush();
    }
    
    void HttpInterface::setVariable(const std::string nodeName, const strings& args)
    {
        // get node id, variable position and length
        if (verbose)
            cout << "setVariables " << nodeName << " " << args[0];
        unsigned nodePos, varPos;
        const bool exists(getNodeAndVarPos(nodeName, args[0], nodePos, varPos));
        if (!exists)
            return;
        
        if (verbose)
            cout << " (" << nodePos << "," << varPos << "):" << args.size() << " =";
        // send the message
        SetVariables::VariablesVector data;
        for (size_t i=1; i<args.size(); ++i)
            data.push_back(atoi(args[i].c_str()));
        SetVariables setVariables(nodePos, varPos, data);
        setVariables.serialize(asebaStream);
        asebaStream->flush();
    }
    
    // Utility: find variable address
    bool HttpInterface::getNodeAndVarPos(const string& nodeName, const string& variableName, unsigned& nodeId, unsigned& pos)
    {
        // make sure the node exists
        bool ok;
        nodeId = getNodeId(UTF8ToWString(nodeName), 0, &ok);
        if (!ok)
        {
            wcerr << "invalid node name " << UTF8ToWString(nodeName) << endl;
            return false;
        }
        pos = unsigned(-1);
        
        // check whether variable is known from a compilation, if so, get position
        const NodeNameToVariablesMap::const_iterator allVarMapIt(allVariables.find(nodeName));
        if (allVarMapIt != allVariables.end())
        {
            const VariablesMap& varMap(allVarMapIt->second);
            const VariablesMap::const_iterator varIt(varMap.find(UTF8ToWString(variableName)));
            if (varIt != varMap.end())
                pos = varIt->second.first;
        }
        
        // if variable is not user-defined, check whether it is provided by this node
        if (pos == unsigned(-1))
        {
            bool ok;
            pos = getVariablePos(nodeId, UTF8ToWString(variableName), &ok);
            if (!ok)
            {
                wcerr << "variable " << UTF8ToWString(variableName) << " does not exist in node " << UTF8ToWString(nodeName);
                return false;
            }
        }
        return true;
    }
    
    // Subscribe to an event stream
    mg_result HttpInterface::evSubscribe(struct mg_connection *conn, strings& args)
    {
        conn->connection_param = new strings(args);
        return MG_MORE;
    }
    
    // Poll cached variable values
    mg_result HttpInterface::evPoll(struct mg_connection *conn, strings& args)
    {
        string nodeName("thymio-II");
        bool ok;
        nodeId = getNodeId(UTF8ToWString(nodeName), 0, &ok);

        for(VariablesMap::iterator it = allVariables[nodeName].begin(); it != allVariables[nodeName].end(); ++it)
        {
            bool is_boolean_variable = (it->first.find(L"b_",0) == 0);
            
            std::vector<short> cachedval = variable_cache[std::make_pair(nodeId,it->second.first)];

            std::stringstream result;
            for (std::vector<short>::iterator i = cachedval.begin(); i != cachedval.end(); ++i)
                if (is_boolean_variable)
                    result << " " << (*i ? "true" : "false");
                else
                    result << " " << *i;
            
            char wbuf[128];
            mg_printf_data(conn, "%s%s\n", wstringtocstr(it->first,wbuf), result.str().c_str());
            
            std::string patched_name(wbuf);
            std::string::size_type n = 0;
            while ( (n=patched_name.find(".",n)) != std::string::npos)
                patched_name.replace(n,1,"/"), n += 1;

            mg_printf_data(conn, "%s%s\n", patched_name.c_str(), result.str().c_str());
        }
        mg_printf_data(conn, "\n");
        return MG_TRUE;
    }

    void HttpInterface::updateVariables(const std::string nodeName)
    {
        strings all_variables;
        char wbuf[128];
        for(VariablesMap::iterator it = allVariables[nodeName].begin(); it != allVariables[nodeName].end(); ++it)
            all_variables.push_back(wstringtocstr(it->first,wbuf));
        getVariables(nodeName, all_variables);
    }

    // Reset nodes and rerun
    mg_result HttpInterface::evReset(struct mg_connection *conn, strings& args)
    {
        for (NodesDescriptionsMap::iterator descIt = nodesDescriptions.begin(); descIt != nodesDescriptions.end(); ++descIt)
        {
            bool ok;
            nodeId = getNodeId(descIt->second.name, 0, &ok);
            if (!ok)
                continue;
            char nodeName[128];
            wstringtocstr(descIt->second.name, nodeName);
            
            this->lock(); // inherited from Dashel::Hub

            Reset(nodeId).serialize(asebaStream); // reset node
            asebaStream->flush();
            Run(nodeId).serialize(asebaStream);   // re-run node
            asebaStream->flush();
            if (strcmp(nodeName, "thymio-II") == 0)
            {
                strings args;
                //args.push_back(nodeName);
                args.push_back("motor.left.target");
                args.push_back("0");
                setVariable(nodeName, args);
                args[0] = "motor.right.target";
                setVariable(nodeName, args);
            }
            size_t eventPos;
            if (commonDefinitions.events.contains(UTF8ToWString("reset"), &eventPos))
            {
                strings data;
                data.push_back("reset");
                sendEvent(nodeName,data);
            }

            this->unlock();
            
            mg_send_data(conn, NULL, 0);
            
        }
        return MG_TRUE;
    }
    
    
    // Compile and store a program into the node, and remember it for introspection
    mg_result HttpInterface::evLoad(struct mg_connection *conn, strings& args)
    {
        if (verbose)
            wcout << L"MG_PUT /nodes/" << args[0].c_str() << " trying to load aesl script\n";
        char buffer[8 * 1024]; // if more would need to use multipart chunking
        int size = mg_get_var(conn, "file", buffer, 8*1024);
        if (size > 0)
        {
            aeslLoadMemory(buffer, size);
            mg_printf_data(conn,"\n");
            return MG_TRUE; // assume success (!)
        }
        return MG_FALSE; // send error result
    }
    
    void HttpInterface::aeslLoadFile(const std::string& filename)
    {
        // local file or URL
        xmlDoc *doc(xmlReadFile(filename.c_str(), NULL, 0));
        if (!doc)
            wcerr << "cannot read aesl script XML from file " << UTF8ToWString(filename) << endl;
        else
        {
            aeslLoad(doc);
            //if (verbose)
                wcout << L"Loaded aesl script from " << filename.c_str() << "\n";
        }
    }

    void HttpInterface::aeslLoadMemory(const char * buffer, const int size)
    {
        // open document
        xmlDoc *doc(xmlReadMemory(buffer, size, "vmcode.xml", NULL, 0));
        if (!doc)
            wcerr << "cannot read XML from memory " << buffer << endl;
        else
        {
            aeslLoad(doc);
            //if (verbose)
                wcout << L"Loaded aesl script in-memory buffer " << buffer << "\n";
        }
    }
    
    // Stolen from Shell::load in examples/clients/cpp-shell by Stephane Magnenat et alii
    void HttpInterface::aeslLoad(xmlDoc* doc)
    {
        xmlNode *domRoot(xmlDocGetRootElement(doc));
        
        // clear existing data
        commonDefinitions.events.clear();
        commonDefinitions.constants.clear();
        allVariables.clear();
        
        // load new data
        int noNodeCount(0);
        bool wasError(false);
        if (!xmlStrEqual(domRoot->name, BAD_CAST("network")))
        {
            wcerr << "root node is not \"network\", XML considered invalid" << endl;
            wasError = true;
        }
        else for (xmlNode *domNode(xmlFirstElementChild(domRoot)); domNode; domNode = domNode->next)
        {
            if (domNode->type == XML_ELEMENT_NODE)
            {
                // an Aseba node, which contains a virtual machine
                if (xmlStrEqual(domNode->name, BAD_CAST("node")))
                {
                    // get attributes, child and content
                    xmlChar *name(xmlGetProp(domNode, BAD_CAST("name")));
                    if (!name)
                    {
                        wcerr << "missing \"name\" attribute in \"node\" entry" << endl;
                    }
                    else
                    {
                        const string _name((const char *)name);
                        xmlChar * text(xmlNodeGetContent(domNode));
                        if (!text)
                        {
                            wcerr << "missing text in \"node\" entry" << endl;
                        }
                        else
                        {
                            // got the identifier of the node and compile the code
                            unsigned preferedId(0);
                            xmlChar *storedId = xmlGetProp(domNode, BAD_CAST("nodeId"));
                            if (storedId)
                                preferedId = unsigned(atoi((char*)storedId));
                            bool ok;
                            unsigned nodeId(getNodeId(UTF8ToWString(_name), preferedId, &ok));
                            if (ok)
                            {
                                if (!compileAndSendCode(UTF8ToWString((const char *)text), nodeId, _name))
                                    wasError = true;
                            }
                            else
                                noNodeCount++;
                            
                            // free attribute and content
                            xmlFree(text);
                        }
                        xmlFree(name);
                    }
                }
                // a global event
                else if (xmlStrEqual(domNode->name, BAD_CAST("event")))
                {
                    // get attributes
                    xmlChar *name(xmlGetProp(domNode, BAD_CAST("name")));
                    if (!name)
                        wcerr << "missing \"name\" attribute in \"event\" entry" << endl;
                    xmlChar *size(xmlGetProp(domNode, BAD_CAST("size")));
                    if (!size)
                        wcerr << "missing \"size\" attribute in \"event\" entry" << endl;
                    // add event
                    if (name && size)
                    {
                        int eventSize(atoi((const char *)size));
                        if (eventSize > ASEBA_MAX_EVENT_ARG_SIZE)
                        {
                            wcerr << "Event " << name << " has a length " << eventSize << "larger than maximum" <<  ASEBA_MAX_EVENT_ARG_SIZE << endl;
                            wasError = true;
                            break;
                        }
                        else
                        {
                            commonDefinitions.events.push_back(NamedValue(UTF8ToWString((const char *)name), eventSize));
                        }
                    }
                    // free attributes
                    if (name)
                        xmlFree(name);
                    if (size)
                        xmlFree(size);
                }
                // a keyword
                else if (xmlStrEqual(domNode->name, BAD_CAST("keywords")))
                {
                    // get attributes
                    xmlChar *name(xmlGetProp(domNode, BAD_CAST("flag")));
                    if (!name)
                        wcerr << "missing \"flag\" attribute in \"keywords\" entry" << endl;
                    // anyway, do nothing because compiler doesn't pay attention to keywords
                }
                // a global constant
                else if (xmlStrEqual(domNode->name, BAD_CAST("constant")))
                {
                    // get attributes
                    xmlChar *name(xmlGetProp(domNode, BAD_CAST("name")));
                    if (!name)
                        wcerr << "missing \"name\" attribute in \"constant\" entry" << endl;
                    xmlChar *value(xmlGetProp(domNode, BAD_CAST("value"))); 
                    if (!value)
                        wcerr << "missing \"value\" attribute in \"constant\" entry" << endl;
                    // add constant if attributes are valid
                    if (name && value)
                    {
                        commonDefinitions.constants.push_back(NamedValue(UTF8ToWString((const char *)name), atoi((const char *)value)));
                    }
                    // free attributes
                    if (name)
                        xmlFree(name);
                    if (value)
                        xmlFree(value);
                }
                else
                    wcerr << "Unknown XML node seen in .aesl file: " << domNode->name << endl;
            }
        }
        
        // release memory
        xmlFreeDoc(doc);
        
        // check if there was an error
        if (wasError)
        {
            wcerr << "There was an error while loading script " << endl;
            commonDefinitions.events.clear();
            commonDefinitions.constants.clear();
            allVariables.clear();
        }
        
        // check if there was some matching problem
        if (noNodeCount)
        {
            wcerr << noNodeCount << " scripts have no corresponding nodes in the current network and have not been loaded." << endl;
        }
    }
    
    bool HttpInterface::compileAndSendCode(const wstring& source, unsigned nodeId, const string& nodeName)
    {
        // compile code
        std::wistringstream is(source);
        Error error;
        BytecodeVector bytecode;
        unsigned allocatedVariablesCount;
        
        Compiler compiler;
        compiler.setTargetDescription(getDescription(nodeId));
        compiler.setCommonDefinitions(&commonDefinitions);
        bool result = compiler.compile(is, bytecode, allocatedVariablesCount, error);
        
        if (result)
        {
            // send bytecode
            sendBytecode(asebaStream, nodeId, std::vector<uint16>(bytecode.begin(), bytecode.end()));
            // run node
            Run msg(nodeId);
            msg.serialize(asebaStream);
            asebaStream->flush();
            // retrieve user-defined variables for use in get/set
            allVariables[nodeName] = *compiler.getVariablesMap();
            return true;
        }
        else
        {
            wcerr << "compilation for node " << UTF8ToWString(nodeName) << " failed: " << error.toWString() << endl;
            return false;
        }
    }
    
}; // end of class HttpInterface


//! Show usage
void dumpHelp(std::ostream &stream, const char *programName)
{
    stream << "Aseba http, connects aseba components together and with HTTP, usage:\n";
    stream << programName << " [options] [additional targets]*\n";
    stream << "Options:\n";
    stream << "-v, --verbose   : makes the switch verbose\n";
    stream << "-d, --dump      : makes the switch dump the content of messages\n";
    stream << "-p, --port port : listens to incoming connection HTTP on this port\n";
    stream << "-a, --aesl file : load program definitions from AESL file\n";
    stream << "-u, --update    : update variables at 10 Hz for polling\n";
    stream << "-h, --help      : shows this help\n";
    stream << "-V, --version   : shows the version number\n";
    stream << "Additional targets are any valid Dashel targets." << std::endl;
    stream << "Report bugs to: david.sherman@inria.fr" << std::endl;
}

//! Show version
void dumpVersion(std::ostream &stream)
{
    stream << "Aseba http " << ASEBA_VERSION << std::endl;
    stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
    stream << "Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
    stream << "Mongoose embedded HTTP server: GNU GPL version 2 <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n";
}

// Mongoose
static int http_event_handler(struct mg_connection *conn, enum mg_event ev) {
    Aseba::HttpInterface* network = (Aseba::HttpInterface*)conn->server_param; // else 505!
    std::vector<std::string> tokens;
    switch (ev) {
        case MG_AUTH:
            return MG_TRUE;
        case MG_REQUEST:
        {
            split_string(std::string(conn->uri), '/', tokens);
            // route based on uri prefix
            if (!strncmp(conn->uri, "/nodes", 6))
            {
                tokens.erase(tokens.begin(),tokens.begin()+1);
                
                if (tokens.size() <= 1)
                {
                    // one named node
                    if (strcmp(conn->request_method, "PUT") == 0)
                        return network->evLoad(conn, tokens);
                    else
                        return network->evNodes(conn, tokens);
                }
                else if (tokens.size() >= 2 && strcmp(tokens[1].c_str(),"events")==0)
                {
                    tokens.erase(tokens.begin(),tokens.begin()+1);
                    return network->evSubscribe(conn, tokens);
                }
                else
                {
                    conn->connection_param = new std::vector<std::string>(tokens);
                    return network->evVariableOrEvent(conn, tokens);
                }
                return MG_TRUE;
            }
            if (!strncmp(conn->uri, "/events", 7))
            {
                return network->evSubscribe(conn, tokens);
            }
            if (!strncmp(conn->uri, "/poll", 5))
            {
                return network->evPoll(conn, tokens);
            }
            if (!strncmp(conn->uri, "/reset", 6) || !strncmp(conn->uri, "/reset_all", 10))
            {
                return network->evReset(conn, tokens);
            }
        }
        case MG_POLL:
        {
//            std::cout << "MG_POLL cxn=" << conn << " param=" << conn->server_param << "\n";
            if (Aseba::HttpInterface::pending_variable* pending = network->pending_cxn_map[conn])
                if (! pending->result.empty())
                {
                    mg_printf_data(conn, "{ \"name\": \"%s\", \"result\": [ %s ] }\n",
                              pending->name.c_str(), pending->result.c_str());
                    network->pending_cxn_map.erase(conn);
                    return MG_TRUE;
                }
            return MG_FALSE; // since pending variable request not yet satisfied...
        }
        case MG_CLOSE:
            delete (std::vector<std::string> *)conn->connection_param;
        default:
            return MG_FALSE;
    }
}

// Main
int main(int argc, char *argv[])
{
    Dashel::initPlugins();
    
    std::string http_port = "3000";
    std::string aesl_filename;
    std::string dashel_target;
    bool verbose = false;
    bool update = false;
    bool dump = false;
    
    // process command line
    int argCounter = 1;
    while (argCounter < argc)
    {
        const char *arg = argv[argCounter++];
        
        if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--verbose") == 0))
            verbose = true;
        else if ((strcmp(arg, "-d") == 0) || (strcmp(arg, "--dump") == 0))
            dump = true;
        else if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
            dumpHelp(std::cout, argv[0]), exit(1);
        else if ((strcmp(arg, "-u") == 0) || (strcmp(arg, "--update") == 0))
            update = true;
        else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
            dumpVersion(std::cout), exit(1);
        else if ((strcmp(arg, "-p") == 0) || (strcmp(arg, "--port") == 0))
            http_port = argv[argCounter++];
        else if ((strcmp(arg, "-a") == 0) || (strcmp(arg, "--aesl") == 0))
            aesl_filename = argv[argCounter++];
        else if (strncmp(arg, "-", 1) != 0)
            dashel_target = arg;
    }
    
    // initialize Dashel plugins
    Dashel::initPlugins();
    
    // create and run bridge, catch Dashel exceptions
    try
    {
        Aseba::HttpInterface* network(new Aseba::HttpInterface(dashel_target, http_port));
        
        if (aesl_filename.size() > 0)
        {
            for (int i = 0; i < 500; i++)
                network->step(10); // wait for description, variables, etc
            network->aeslLoadFile(aesl_filename);
        }

        network->http_server = mg_create_server((void*)network, http_event_handler);
        mg_set_option(network->http_server, "document_root", ".");      // Serve current directory -- should disallow this!!
        mg_set_option(network->http_server, "listening_port", http_port.c_str());  // Open port 3000
        mg_set_option(network->http_server, "auth_domain", "inirobot.inria.fr");

        for (;;) {
            // single-threaded polling loop, limited to around 20 Hz. Expect more traffic on Dashel than on HTTP.
            mg_poll_server(network->http_server, 10);
            if (update)
                network->updateVariables("thymio-II");
            for (int i = 0; i < 5; i++)
                network->step(2); // inherited from Dashel::Hub; poll Dashel once, or timeout
        }
        mg_destroy_server(&(network->http_server));
    }
    catch(Dashel::DashelException e)
    {
        std::cerr << "Unhandled Dashel exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
