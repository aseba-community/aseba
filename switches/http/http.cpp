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
    - POST /nodes/:NODENAME/:VARIABLE (fixme: sloppily allows GET and values in the request)
    - POST /nodes/:NODENAME/:EVENT (fixme: sloppily allows GET and values in the request)
    - handle asynchronous variable reporting (GET /nodes/:NODENAME/:VARIABLE)
    - JSON format for variable reporting (GET /nodes/:NODENAME/:VARIABLE)
 
 TODO:
    - allow only POST requests for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
    - form processing for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
    - handle more than just one Thymio-II node!
    - program flashing (PUT  /nodes/:NODENAME)
    - implement SSE streams and event filtering (GET /events) and (GET /nodes/:NODENAME/events)
 
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

// hack because I don't understand locales yet
char* wstringtocstr(std::wstring w)
{
    char* name = (char*)malloc(128);
    std::wcstombs(name, w.c_str(), 127);
    return name;
}

// split strings on delimiter
void split_string(std::string s, char delim, std::vector<std::string>& tokens)
{
    std::stringstream ss( s );
    std::string item;
    std::getline(ss, item, delim);
    while (std::getline(ss, item, '/'))
        tokens.push_back(item);
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
    verbose(true)
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
            
            // if variables, check for pending requests
            const Variables *variables(dynamic_cast<Variables *>(message));
            if (variables)
            {
                char* result = (char *)malloc(10 * variables->variables.size()), *pos = result;
                for (size_t i = 0; i < variables->variables.size(); ++i)
                    pos += sprintf(pos, "%s%d", (i>0 ? "," : ""), variables->variables[i]);
                if (verbose)
                    cout << "incomingData var ("<<variables->source<<","<<variables->start<<") = "<<result << endl;
                
                pending_variable pending = pending_vars_map[std::make_pair(variables->source,variables->start)];
                if (pending.connection) {
                    //mg_printf(pending.connection, "{ \"name\": \"%s\", \"result\": [ %s ] }\n", pending.name.c_str(), result);
                    //pending.connection->connection_param = this; // just a flag for next MG_POLL
                    pending.result = result;
                    pending_vars_map[std::make_pair(variables->source,variables->start)] = pending;
                }
                
//                mg_wakeup_server_ex(http_server, check_receive_variable, "%u %u %s",
//                                    variables->source, variables->start, (const char*)result);
                free(result);
            }
            
            // if event, retransmit it on an HTTP SSE channel if one exists
            const UserMessage *userMsg(dynamic_cast<UserMessage *>(message));
            if (userMsg)
            {
                // In the HTTP world we should set up a stream of Server-Sent Events for this.
                // Useful bits:
                //  userMessage->type
                //  userMessage->data
                //  commonDefinitions.events[userMessage->type].name
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

        mg_printf_data(conn, "%s{\n", indent);
        mg_printf_data(conn, "%s  \"name\": \"%s\",\n", indent, wstringtocstr(description.name));
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
                               wstringtocstr(description.namedVariables[i].name), description.namedVariables[i].size);
            mg_printf_data(conn, "    },\n");
            // local events variables
            mg_printf_data(conn, "    \"localEvents\": [\n" );
            for (size_t i = 0; i < description.localEvents.size(); ++i)
                mg_printf_data(conn, "      { \"name\": \"%s\", \"description\": \"%s\" }\n",
                               wstringtocstr(description.localEvents[i].name), wstringtocstr(description.localEvents[i].description));
            mg_printf_data(conn, "    ],\n");
            // constants from introspection
            mg_printf_data(conn, "    \"constants\": [\n" );
            for (size_t i = 0; i < commonDefinitions.constants.size(); ++i)
                mg_printf_data(conn, "      { \"name\": \"%s\", \"value\": %d }\n",
                               wstringtocstr(commonDefinitions.constants[i].name), commonDefinitions.constants[i].value);
            mg_printf_data(conn, "    ],\n");
            // events from introspection
            mg_printf_data(conn, "    \"events\": [\n" );
            for (size_t i = 0; i < commonDefinitions.events.size(); ++i)
                mg_printf_data(conn, "      { \"name\": \"%s\", \"size\": %d }\n",
                               wstringtocstr(commonDefinitions.events[i].name), commonDefinitions.events[i].value);
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
            if (strcmp(conn->request_method, "PUT") == 0 || conn->query_string != NULL || args.size() >= 3)
            {
                // set variable value
                strings values;
                if (args.size() >= 3)
                    values.assign(args.begin()+1, args.end());
                else
                {
                    // TODO parse POST form data
                }
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
                
                //pending_variable* pending = (pending_variable*)malloc(sizeof(pending_variable));
                //strncpy(pending->name, values[0].c_str(), 64);
                unsigned source, start;
                if (getNodeAndVarPos(nodeName, values[0], source, start))
                {
                    pending.source = source;
                    pending.start = start;
                    pending_vars_map[std::make_pair(source,start)] = pending;
                    conn->connection_param = &(pending_vars_map[std::make_pair(source,start)]); // doesn't work, connection forgets this
                    pending_cxn_map[conn] = &(pending_vars_map[std::make_pair(source,start)]);
                }
                if (verbose)
                    cout << "evVariableOrEevent schedule var " << pending.name << "(" << pending.source << ","
                    << pending.start << ") cxn=" << pending.connection << endl;
                return MG_TRUE; //MG_MORE;
            }
        }
        else
        {
            // this is an event
            // arguments are args 1..N
            UserMessage::DataVector data;
            if (args.size() >= 3)
                for (size_t i=2; i<args.size(); ++i)
                    data.push_back(atoi(args[i].c_str()));
            else
            {
                // TODO parse POST form data
            }
            UserMessage userMessage(eventPos, data);
            userMessage.serialize(asebaStream);
            asebaStream->flush();
            mg_send_data(conn, NULL, 0);
            return MG_TRUE;
        }
    }

    void HttpInterface::sendEvent(const std::string nodeName, const strings& args)
    {
        // check that there are enough arguments
        if (args.size() < 2)
        {
            wcerr << "missing argument, usage: emit EVENT_NAME EVENT_DATA*" << endl;
            return;
        }
        size_t pos;
        if (!commonDefinitions.events.contains(UTF8ToWString(args[1]), &pos))
        {
            wcerr << "event " << UTF8ToWString(args[1]) << " is unknown" << endl;
            return;
        }
        
        // build event and emit
        UserMessage::DataVector data;
        for (size_t i=2; i<args.size(); ++i)
            data.push_back(atoi(args[i].c_str()));
        UserMessage userMessage(pos, data);
        userMessage.serialize(asebaStream);
        asebaStream->flush();
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
            bool ok;
            const unsigned length(getVariableSize(nodePos, UTF8ToWString(*it), &ok));
            if (!ok)
                continue;
            
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
        {
            data.push_back(atoi(args[i].c_str()));
            cout << " " << args[i].c_str();
        }
        cout << "\n";
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
    
    // Flash a program into the node, and remember it for introspection
    mg_result HttpInterface::evLoad(struct mg_connection *conn, strings& args)
    {
        if (verbose)
            wcout << L"MG_PUT /nodes/" << args[0].c_str() << " trying to load aesl script\n";
        char buffer[8 * 1024]; // if more would need to use multipart chunking
        int size = mg_get_var(conn, "file", buffer, 8*1024);
        if (size > 0)
        {
            aeslLoadMemory(buffer, size);
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
            if (verbose)
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
            if (verbose)
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
    stream << "-p port         : listens to incoming connection HTTP on this port\n";
    stream << "-a aesl         : load program definitions from AESL file\n";
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
    switch (ev) {
        case MG_AUTH:
            return MG_TRUE;
        case MG_REQUEST:
            if (!strncmp(conn->uri, "/nodes", 6))
            {
                std::stringstream ss(conn->uri);
                std::vector<std::string> tokens;
                std::string item;
                std::getline(ss, item, '/');std::getline(ss, item, '/'); // skip first (empty) and second ("nodes")
                while (std::getline(ss, item, '/'))
                    tokens.push_back(item);
                
                if (tokens.size() <= 1)
                {
                    // one named node
                    if (strcmp(conn->request_method, "PUT") == 0)
                        return network->evLoad(conn, tokens);
                    else
                        return network->evNodes(conn, tokens);
                }
                else
                    return network->evVariableOrEvent(conn, tokens);
                return MG_TRUE;
            }
        case MG_POLL:
//            std::cout << "MG_POLL cxn=" << conn << " param=" << conn->server_param << "\n";
            if (Aseba::HttpInterface::pending_variable* pending = network->pending_cxn_map[conn])
                if (! pending->result.empty())
                {
                    mg_printf(conn, "{ \"name\": \"%s\", \"result\": [ %s ] }\n",
                              pending->name.c_str(), pending->result.c_str());
                    network->pending_cxn_map.erase(conn);
                    return MG_TRUE;
                }
            return MG_FALSE; // since pending variable request not yet satisfied...
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
        else if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--version") == 0))
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
        
        for (;;) {
            // single-threaded polling loop, limited to around 20 Hz. Expect more traffic on Dashel than on HTTP.
            mg_poll_server(network->http_server, 10);
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
