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
    - Dashel connection to one Thymio-II and round-robin scheduling between Aseba and HTTP connections
    - read Aesl program at launch, upload to Thymio-II, and record interface for introspection
    - GET /nodes, GET /nodes/:NODENAME with introspection
    - POST /nodes/:NODENAME/:VARIABLE (sloppily allows GET /nodes/:NODENAME/:VARIABLE/:VALUE[/:VALUE]*)
    - POST /nodes/:NODENAME/:EVENT (sloppily allows GET /nodes/:NODENAME/:EVENT[/:VALUE]*)
    - form processing for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
    - handle asynchronous variable reporting (GET /nodes/:NODENAME/:VARIABLE)
    - JSON format for variable reporting (GET /nodes/:NODENAME/:VARIABLE)
    - implement SSE streams and event filtering (GET /events) and (GET /nodes/:NODENAME/events)
    - program flashing (PUT  /nodes/:NODENAME)
      use curl --data-ascii "file=$(cat vmcode.aesl)" -X PUT http://127.0.0.1:3000/nodes/thymio-II
    - accept JSON payload rather than HTML form for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
 
 TODO:
    - handle more than just one Thymio-II node!
 
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
#include <regex>
#include <libjson/libjson.h>
#include "http.h"
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

//== Some utility functions and diverse hacks ==============================================

// because I don't understand locales and wide strings yet
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

// make JSON array from std::vector<short>
JSONNode json_vector(std::string name, std::vector<short> cachedval)
{
    JSONNode node(JSON_NODE);
    node.set_name(name);
    for (std::vector<short>::iterator i = cachedval.begin(); i != cachedval.end(); ++i)
        node.push_back(JSONNode(*i));
    return node;
}

// close the stream, this is a hack because Dashel lacks a proper shutdown mechanism
void shutdownStream(Dashel::Stream* stream)
{
    stream->fail(Dashel::DashelException::Unknown, 0, "Request handling complete");
}

std::string readLine(Dashel::Stream* stream)
{
    char c;
    std::string line;
    do
    {
        stream->read(&c, 1);
        line += c;
    }
    while (c != '\n');
    return line;
}


//== Main event ============================================================================

namespace Aseba
{
    using namespace std;
    using namespace Dashel;
    
    /** \addtogroup http */
    /*@{*/
    
    //-- Subclassing Dashel::Hub -----------------------------------------------------------
    
    // default constructor needed for unit testing
    HttpInterface::HttpInterface() :
    asebaStream(0),
    nodeId(0),
    verbose(true)
    {
        init("tcp:;port=33333", "3000");
    }

    HttpInterface::HttpInterface(const std::string& asebaTarget, const std::string& http_port) :
    asebaStream(0),
    nodeId(0),
    verbose(true)
    {
        init(asebaTarget, http_port);
    }

    void HttpInterface::init(const std::string& asebaTarget, const std::string& http_port)
    {
        asebaStream = NULL;
        nodeId = 0;
        verbose = true;

        // connect to the Aseba target
        std::cout << "HttpInterface connect asebaTarget " << asebaTarget << "\n";
        connect(asebaTarget); // triggers connectionCreated, which assigns asebaStream
        
        // request a description for aseba target
        GetDescription getDescription;
        getDescription.serialize(asebaStream);
        asebaStream->flush();
        
        // listen for incoming HTTP requests
        httpStream = connect("tcpin:port=" + http_port);
    }
    
    void HttpInterface::connectionCreated(Dashel::Stream *stream)
    {
        if (!asebaStream)
        {
            // this is the connection to the Thymio-II
            std::cout << "Incoming Aseba connection from " << stream->getTargetName() << endl;
            asebaStream = stream;
        }
        else
        {
            // this is an incoming HTTP connection
            if (verbose)
                cerr << stream << " Connection created to " << stream->getTargetName() << endl;
            if (httpRequests[stream].ready)
            {
                // Houston, we have a problem.
                abort();
            }
            
            if (httpRequests[stream].initialize(stream))
            {   //success
                if (verbose)
                {
                    cerr << stream << " Request " << httpRequests[stream].method.c_str()
                    << " " << httpRequests[stream].uri.c_str() << " [ ";
                    for (int i = 0; i < httpRequests[stream].tokens.size(); ++i)
                        cerr << httpRequests[stream].tokens[i] << " ";
                    cerr << "]" << endl;
                }
                // continue with incomingData
            }
            else
            {   //failure
                stream->write("HTTP/1.1 400 Bad request\r\n");
                stream->fail(DashelException::Unknown, 0, "400 Bad request");
                httpRequests[stream].ready = false;
            }
        }
    }
    
    void HttpInterface::connectionClosed(Stream * stream, bool abnormal)
    {
        if (stream == asebaStream)
        {
            asebaStream = 0;
            if (verbose)
                cerr << "Connection closed to Aseba target" << endl;
            stop();
        }
        else
        {
            httpRequests.erase(stream);
            if (verbose)
                cerr << stream << " Connection closed to " << stream->getTargetName() << endl;
        }
    }
    
    void HttpInterface::nodeDescriptionReceived(unsigned nodeId)
    {
        if (verbose)
            wcerr << L"Received description for " << getNodeName(nodeId) << endl;
        this->nodeId = nodeId;
    }
    
    void HttpInterface::incomingData(Stream *stream)
    {
        if (stream == asebaStream) {
            // incoming Aseba message
            if (verbose)
                cerr << "incoming for asebaStream " << stream << endl;

            Message *message(Message::receive(stream));
            
            // pass message to description manager, which builds
            // the node descriptions in background
            DescriptionsManager::processMessage(message);
            
            // if variables, check for pending requests
            const Variables *variables(dynamic_cast<Variables *>(message));
            if (variables)
                incomingVariables(variables);
            
            // if event, retransmit it on an HTTP SSE channel if one exists
            const UserMessage *userMsg(dynamic_cast<UserMessage *>(message));
            if (userMsg)
                incomingUserMsg(userMsg);

            delete message;
        }
        else
        {
            // incoming HTTP request
            if (verbose)
                cerr << "incoming for HTTP stream " << stream << endl;
            httpRequests[stream].incomingData();
            if (httpRequests[stream].ready)
                routeRequest(httpRequests[stream]);
        }
    }
    
    // Incoming Variables
    void HttpInterface::incomingVariables(const Variables *variables)
    {
        JSONNode result(JSON_ARRAY);
        result.set_name("result");
        for (size_t i = 0; i < variables->variables.size(); ++i)
            result.push_back(JSONNode("", variables->variables[i]));
        string result_str = libjson::strip_white_space(result.write_formatted());
        
        // variable_cache[std::make_pair(variables->source,variables->start)] = std::vector<short>(variables->variables);
        
        std::set<Dashel::Stream*> *pending = &pending_vars_map[std::make_pair(variables->source,variables->start)];

        if (verbose)
        {
            cerr << "incomingVariables var (" << variables->source << "," << variables->start << ") = "
                 << result_str << endl << "\tupdates " << pending->size() << " pending";
            for (std::set<Dashel::Stream*>::iterator i = pending->begin(); i != pending->end(); i++)
                cerr << " " << *i;
            cerr << endl;
        }
        
        for (std::set<Dashel::Stream*>::iterator i = pending->begin(); i != pending->end(); )
        {
            // i points to a stream that is waiting for this variable value
            if (verbose)
                cerr << *i << " incoming var (" << variables->source << "," << variables->start << "), expect close cxn" << endl;
            httpRequests[*i].sendStatus(200,result_str);
            httpRequests.erase(*i);  // then remove the map entry (that now contains an invalid HttpRequest pointer)
            pending->erase(i++);      // finally remove the stream from the pending list
        }
        if (verbose)
        {
            cerr << "\tcheck " << pending_vars_map[std::make_pair(variables->source,variables->start)].size() << " pending";
            for (std::set<Dashel::Stream*>::iterator i =
                 pending_vars_map[std::make_pair(variables->source,variables->start)].begin();
                 i != pending_vars_map[std::make_pair(variables->source,variables->start)].end();
                 i++)
                cerr << " " << *i;
            cerr << endl;
        }
            
//        if (pending.connection) {
//            pending.result = result;
//            pending_vars_map[std::make_pair(variables->source,variables->start)] = pending;
//            // here is where we transmit the result on the stream, which we then close
//            Stream* conn = pending.connection;
//            string jc = libjson::strip_white_space(result.write_formatted());
//            
//            httpRequests[conn]->sendStatus(200,jc);
//            delete httpRequests[conn];
//            if (verbose)
//                cerr << conn << " incomingVariables 200 returned var (" << variables->source << "," << variables->start << ")\n";
//        }

    }
    
    // Incoming User Messages
    void HttpInterface::incomingUserMsg(const UserMessage *userMsg)
    {
        if (verbose)
            cerr << "incomingUserMsg msg ("<< userMsg->type <<","<< &userMsg->data <<")" << endl;
        
        // set up SSE message
        std::stringstream reply;
        char this_event[128];
        wstringtocstr(commonDefinitions.events[userMsg->type].name, this_event);
        string event_name = std::string(this_event);
        reply << "data: " << event_name;
        for (size_t i = 0; i < userMsg->data.size(); ++i)
            reply << " " << userMsg->data[i];
        reply << "\r\n" << "\r\n";
        
        // In the HTTP world we set up a stream of Server-Sent Events for this.
        // Note that event name is in commonDefinitions.events[userMsg->type].name
        
        for (std::map<Dashel::Stream*, std::set<std::string> >::iterator subscriber = eventSubscriptions.begin();
             subscriber != eventSubscriptions.end(); ++subscriber)
        {
            if (subscriber->second.count("*") >= 1 || subscriber->second.count(event_name) >= 1)
            {
                Stream* conn = subscriber->first;
                const char* reply_str = reply.str().c_str();
                int reply_len = reply.str().size();
                conn->write(reply_str, reply_len);
                conn->flush();
            }
        }
    }

    //-- Routing for HTTP requests ---------------------------------------------------------

    void HttpInterface::routeRequest(HttpRequest& req)
    {
        // route based on uri prefix
        if (req.tokens[0].find("nodes")==0)
        {
            req.tokens.erase(req.tokens.begin(),req.tokens.begin()+1);
            
            if (req.tokens.size() <= 1)
            {   // one named node
                if (req.method.find("PUT")==0)
                    evLoad(req.stream, req.tokens);  // load bytecode for one node
                else
                    evNodes(req.stream, req.tokens); // get info for one node
            }
            else if (req.tokens.size() >= 2 && req.tokens[1].find("events")==0)
            {   // subscribe to event stream for this node
                req.tokens.erase(req.tokens.begin(),req.tokens.begin()+1);
                evSubscribe(req.stream, req.tokens);
            }
            else
            {   // request for a varibale or an event
                evVariableOrEvent(req.stream, req.tokens);
            }
            return;
        }
        if (req.tokens[0].find("events")==0)
        {   // subscribe to event stream for all nodes
            return evSubscribe(req.stream, req.tokens);
        }
        if (req.tokens[0].find("reset")==0 || req.tokens[0].find("reset_all")==0)
        {   // reset nodes
            return evReset(req.stream, req.tokens);
        }
    }
    
    // Handler: Node descriptions
    
    void HttpInterface::evNodes(Stream *conn, strings& args)
    {
        bool do_one_node(args.size() > 0);
        
        JSONNode list(JSON_ARRAY);
        
        for (NodesDescriptionsMap::iterator descIt = nodesDescriptions.begin();
             descIt != nodesDescriptions.end(); ++descIt)
        {
            const NodeDescription& description(descIt->second);
            char wbuf[128];
            
            JSONNode node(JSON_NODE);
            node.set_name("nodeInfo");
            node.push_back(JSONNode("name", wstringtocstr(description.name,wbuf)));
            node.push_back(JSONNode("protocolVersion", description.protocolVersion));
            
            if (do_one_node)
            {
                node.push_back(JSONNode("bytecodeSize", description.bytecodeSize));
                node.push_back(JSONNode("variablesSize", description.variablesSize));
                node.push_back(JSONNode("stackSize", description.stackSize));
                
                // named variables
                JSONNode named_variables(JSON_NODE);
                named_variables.set_name("namedVariables");
                for (size_t i = 0; i < description.namedVariables.size(); ++i)
                    named_variables.push_back(JSONNode(wstringtocstr(description.namedVariables[i].name,wbuf),
                                                       description.namedVariables[i].size));
                node.push_back(named_variables);
                
                // local events variables
                JSONNode local_events(JSON_NODE);
                local_events.set_name("localEvents");
                for (size_t i = 0; i < description.localEvents.size(); ++i)
                {
                    string ev(wstringtocstr(description.localEvents[i].name,wbuf));
                    local_events.push_back(JSONNode(ev,wstringtocstr(description.localEvents[i].description,wbuf)));
                }
                node.push_back(local_events);
                
                // constants from introspection
                JSONNode constants(JSON_NODE);
                constants.set_name("constants");
                for (size_t i = 0; i < commonDefinitions.constants.size(); ++i)
                    constants.push_back(JSONNode(wstringtocstr(commonDefinitions.constants[i].name,wbuf),
                                                 commonDefinitions.constants[i].value));
                node.push_back(constants);
                
                // events from introspection
                JSONNode events(JSON_NODE);
                events.set_name("events");
                for (size_t i = 0; i < commonDefinitions.events.size(); ++i)
                    events.push_back(JSONNode(wstringtocstr(commonDefinitions.events[i].name,wbuf),
                                              commonDefinitions.events[i].value));
                node.push_back(events);
            }
            list.push_back(node);
        }
        
        std::string jc = (do_one_node && !list.empty()) ? list[0].write_formatted() : list.write_formatted();
        httpRequests[conn].sendStatus(200,jc);
        httpRequests.erase(conn);
    }
    
    // Handler: Variable get/set or Event call
    
    void HttpInterface::evVariableOrEvent(Stream *conn, strings& args)
    {
        string nodeName(args[0]);
        size_t eventPos;
        
        if ( ! commonDefinitions.events.contains(UTF8ToWString(args[1]), &eventPos))
        {
            // this is a variable
            if (httpRequests[conn].method.find("POST") == 0 || args.size() >= 3)
            {
                // set variable value
                strings values;
                if (args.size() >= 3)
                    values.assign(args.begin()+1, args.end());
                else
                {
                    // Parse POST form data
                    values.push_back(args[1]);
                    parse_json_form(httpRequests[conn].content, values);
                }
                if (values.size() == 0)
                {
                    httpRequests[conn].sendStatus(404);
                        httpRequests.erase(conn);
                    if (verbose)
                        cerr << conn << " evVariableOrEevent 404 can't set variable " << args[0] << ", no values" <<  endl;
                    return;
                }
                sendSetVariable(nodeName, values);
                httpRequests[conn].sendStatus(200);
                httpRequests.erase(conn);
                if (verbose)
                    cerr << conn << " evVariableOrEevent 200 set variable " << values[0] <<  endl;
            }
            else
            {
                // get variable value
                strings values;
                values.assign(args.begin()+1, args.begin()+2);
                sendGetVariables(nodeName, values);
                
                //pending_variable pending = { 0,0, values[0], conn };
                
                unsigned source, start;
                if ( ! getNodeAndVarPos(nodeName, values[0], source, start))
                {
                    httpRequests[conn].sendStatus(404);
                        httpRequests.erase(conn);
                    if (verbose)
                        cerr << conn << " evVariableOrEevent 404 no such variable " << values[0] <<  endl;
                    return;
                }

//                pending.source = source;
//                pending.start = start;
                // pending_vars_map[ s,s ] is a std::set of Dashel::Stream*
                pending_vars_map[std::make_pair(source,start)].insert(conn);

                if (verbose)
                    cerr << conn << " evVariableOrEevent (open) schedule var " << values[0]
                    << "(" << source << "," << start << ") add " << conn << " to subscribers" <<  endl;
                return;
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
            else if (httpRequests[conn].method.find("POST") == 0)
            {
                // Parse POST form data
                parse_json_form(std::string(httpRequests[conn].content,httpRequests[conn].content.size()), data);
            }
            sendEvent(nodeName, data);
//            JSONNode n(JSON_NODE);
//            n.push_back(JSONNode("return_value", JSON_NULL));
//            n.push_back(JSONNode("cmd", "sendEvent"));
//            n.push_back(JSONNode("name", nodeName));
            httpRequests[conn].sendStatus(200);
            httpRequests.erase(conn);
            return;
        }
    }
    
    // Handler: Subscribe to an event stream
    
    void HttpInterface::evSubscribe(Stream *conn, strings& args)
    {
        // eventSubscriptions[conn] is an unordered set of strings
        if (args.size() == 1)
            eventSubscriptions[conn].insert("*");
        else
            for (strings::iterator i = args.begin()+1; i != args.end(); ++i)
                eventSubscriptions[conn].insert(*i);
        
        strings headers;
        headers.push_back("Content-Type: text/event-stream");
        headers.push_back("Cache-Control: no-cache");
        headers.push_back("Connection: keep-alive");
        httpRequests[conn].sendStatus(200,headers);
        // connection must stay open!
    }
    
    // Handler: Compile and store a program into the node, and remember it for introspection
    
    void HttpInterface::evLoad(Stream *conn, strings& args)
    {
        if (verbose)
            cerr << "PUT /nodes/" << args[0].c_str() << " trying to load aesl script\n";
        const char* buffer = httpRequests[conn].content.c_str();
        int pos = httpRequests[conn].content.find("file=");
        if (pos != std::string::npos)
        {
            aeslLoadMemory(buffer+pos+5, httpRequests[conn].content.size()-pos-5);
            httpRequests[conn].sendStatus(200);
        }
        else
            httpRequests[conn].sendStatus(400);
        httpRequests.erase(conn);
    }
    
    // Handler: Reset nodes and rerun
    
    void HttpInterface::evReset(Stream *conn, strings& args)
    {
        for (NodesDescriptionsMap::iterator descIt = nodesDescriptions.begin();
             descIt != nodesDescriptions.end(); ++descIt)
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
                args.push_back("motor.left.target");
                args.push_back("0");
                sendSetVariable(nodeName, args);
                args[0] = "motor.right.target";
                sendSetVariable(nodeName, args);
            }
            size_t eventPos;
            if (commonDefinitions.events.contains(UTF8ToWString("reset"), &eventPos))
            {
                strings data;
                data.push_back("reset");
                sendEvent(nodeName,data);
            }
            
            this->unlock();
            
            httpRequests[conn].sendStatus(200);
            httpRequests.erase(conn);
        }
    }
    

    
    //-- Sending messages on the Aseba bus -------------------------------------------------

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
    
    std::pair<unsigned,unsigned> HttpInterface::sendGetVariables(const std::string nodeName, const strings& args)
    {
        unsigned nodePos, varPos;
        for (strings::const_iterator it(args.begin()); it != args.end(); ++it)
        {
            // get node id, variable position and length
            if (verbose)
                cerr << "getVariables " << nodeName << " " << *it;
            const bool exists(getNodeAndVarPos(nodeName, *it, nodePos, varPos));
            if (!exists)
                continue;

            VariablesMap vm = allVariables[nodeName];
            const unsigned length(vm[UTF8ToWString(*it)].second);
            
            // variable_cache[std::pair<unsigned,unsigned>(nodePos,varPos)] = std::vector<short>();
            
            if (verbose)
                cerr << " (" << nodePos << "," << varPos << "):" << length << "\n";
            // send the message
            GetVariables getVariables(nodePos, varPos, length);
            getVariables.serialize(asebaStream);
        }
        asebaStream->flush();
        return std::pair<unsigned,unsigned>(nodePos,varPos); // just last one
    }
    
    void HttpInterface::sendSetVariable(const std::string nodeName, const strings& args)
    {
        // get node id, variable position and length
        if (verbose)
            cerr << "setVariables " << nodeName << " " << args[0];
        unsigned nodePos, varPos;
        const bool exists(getNodeAndVarPos(nodeName, args[0], nodePos, varPos));
        if (!exists)
            return;
        
        if (verbose)
            cerr << " (" << nodePos << "," << varPos << "):" << args.size()-1 << endl;
        // send the message
        SetVariables::VariablesVector data;
        for (size_t i=1; i<args.size(); ++i)
            data.push_back(atoi(args[i].c_str()));
        SetVariables setVariables(nodePos, varPos, data);
        setVariables.serialize(asebaStream);
        asebaStream->flush();
    }
    
    // Utility: find variable address
    bool HttpInterface::getNodeAndVarPos(const string& nodeName, const string& variableName,
                                         unsigned& nodeId, unsigned& pos)
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
                wcerr << "variable " << UTF8ToWString(variableName) << " does not exist in node "
                      << UTF8ToWString(nodeName);
                return false;
            }
        }
        return true;
    }
    
    // Utility: request update of all variables, used for variable caching
    void HttpInterface::updateVariables(const std::string nodeName)
    {
        strings all_variables;
        char wbuf[128];
        for(VariablesMap::iterator it = allVariables[nodeName].begin(); it != allVariables[nodeName].end(); ++it)
            all_variables.push_back(wstringtocstr(it->first,wbuf));
        sendGetVariables(nodeName, all_variables);
    }

    // Utility: extract argument values from JSON request body
    void HttpInterface::parse_json_form(std::string content, strings& values)
    {
        try
        {
            JSONNode form = libjson::parse(content);
            
            if (form.type() == JSON_NODE)
            {
                JSONNode::const_iterator args = form.find("args");
                if (! args->empty())
                    form = *args;
            }
            if (form.type() != JSON_ARRAY)
                throw std::invalid_argument("not a JSON_ARRAY");
            
            for (JSONNode::const_iterator i = form.begin(); i != form.end(); ++i)
                values.push_back(i->as_string());
        }
        catch(std::invalid_argument e)
        {
            std::cerr << "Invalid JSON value \"" << content << "\"; libjson exception: " << e.what() << std::endl;
            // values is empty, request will fail
        }
        
        return;
    }
    

    // Load Aesl file from file
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
                cerr << "Loaded aesl script from " << filename.c_str() << "\n";
            this->nodeInfoJson = JSONNode(JSON_NODE);
            this->nodeInfoJson.set_name("nodeInfo");
            this->nodeInfoJson.push_back(JSONNode("id", this->nodeId));
            this->nodeInfoJson.push_back(JSONNode("connected", true));
        }
    }

    // Load Aesl file from memory
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
                cerr << "Loaded aesl script in-memory buffer " << buffer << "\n";
        }
    }
    
    // Parse Aesl program using XPath
    void HttpInterface::aeslLoad(xmlDoc* doc)
    {
        // clear existing data
        commonDefinitions.events.clear();
        commonDefinitions.constants.clear();
        allVariables.clear();
        
        // load new data
        int noNodeCount(0);
        bool wasError(false);

        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        xmlXPathObjectPtr obj;
        
        // 1. Path network/event
        if ((obj = xmlXPathEvalExpression(BAD_CAST"/network/event", context)))
        {
            xmlNodeSetPtr nodeset = obj->nodesetval;
            for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i)
            {
                xmlChar *name  (xmlGetProp(nodeset->nodeTab[i], BAD_CAST("name")));
                xmlChar *size  (xmlGetProp(nodeset->nodeTab[i], BAD_CAST("size")));
                if (name && size)
                {
                    int eventSize = atoi((const char *)size);
                    if (eventSize > ASEBA_MAX_EVENT_ARG_SIZE)
                    {
                        wcerr << "Event " << name << " has a length " << eventSize << " larger than maximum"
                              <<  ASEBA_MAX_EVENT_ARG_SIZE << endl;
                        wasError = true;
                        break;
                    }
                    else
                        commonDefinitions.events.push_back(NamedValue(UTF8ToWString((const char *)name), eventSize));
                }
                xmlFree(name);
                xmlFree(size);
            }
        }
        // 2. Path network/constant
        if ((obj = xmlXPathEvalExpression(BAD_CAST"/network/constant", context)))
        {
            xmlNodeSetPtr nodeset = obj->nodesetval;
            for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i)
            {
                xmlChar *name  (xmlGetProp(nodeset->nodeTab[i], BAD_CAST("name")));
                xmlChar *value (xmlGetProp(nodeset->nodeTab[i], BAD_CAST("value")));
                if (name && value)
                    commonDefinitions.constants.push_back(NamedValue(UTF8ToWString((const char *)name),
                                                                     atoi((const char *)value)));
                xmlFree(name);  // nop if name is NULL
                xmlFree(value); // nop if value is NULL
            }
        }
        // 3. Path network/keywords
        if ((obj = xmlXPathEvalExpression(BAD_CAST"/network/keywords", context)))
        {
            xmlNodeSetPtr nodeset = obj->nodesetval;
            for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i)
            {
                xmlChar *flag  (xmlGetProp(nodeset->nodeTab[i], BAD_CAST("flag")));
                // do nothing because compiler doesn't pay attention to keywords
                xmlFree(flag);
            }
        }
        // 4. Path network/node
        if ((obj = xmlXPathEvalExpression(BAD_CAST"/network/node", context)))
        {
            xmlNodeSetPtr nodeset = obj->nodesetval;
            for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i)
            {
                xmlChar *name     (xmlGetProp(nodeset->nodeTab[i], BAD_CAST("name")));
                xmlChar *storedId (xmlGetProp(nodeset->nodeTab[i], BAD_CAST("nodeId")));
                xmlChar *text     (xmlNodeGetContent(nodeset->nodeTab[i]));
                
                if (!name)
                    wcerr << "missing \"name\" attribute in \"node\" entry" << endl;
                else if (!text)
                    wcerr << "missing text in \"node\" entry" << endl;
                else
                {
                    const string _name((const char *)name);
                    // get the identifier of the node and compile the code
                    unsigned preferedId = storedId ? unsigned(atoi((char*)storedId)) : 0;
                    bool ok;
                    unsigned nodeId(getNodeId(UTF8ToWString(_name), preferedId, &ok));
                    if (ok)
                        wasError = !compileAndSendCode(UTF8ToWString((const char *)text), nodeId, _name);
                    else
                        noNodeCount++;
                }
                // free attribute and content
                xmlFree(text); // nop if text is NULL
                xmlFree(name); // nop if name is NULL
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
    
    // Upload bytecode to node
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
    //== end of class HttpInterface ============================================================
    
    HttpRequest::HttpRequest(){};

    HttpRequest::~HttpRequest()
    {
//        try
//        {
//            stream->flush();
//            shutdownStream(stream);
//        }
//        catch(Dashel::DashelException e)
//        {
//        }
    }

    bool HttpRequest::initialize( Dashel::Stream *_stream)
    {
        string start_line = readLine(_stream);
        return initialize(start_line, _stream);
    }
    
    bool HttpRequest::initialize( std::string const& start_line, Dashel::Stream *_stream)
    {
        strings parts;
        split_string(start_line, ' ',parts);
        if (parts.size() == 3
            && (parts[0].find("GET",0)==0 || parts[0].find("PUT",0)==0 || parts[0].find("POST",0)==0)
            && (parts[2].find("HTTP/1.1\r\n",0)==0 || parts[2].find("HTTP/1.0\r\n",0)==0) )
        {
            // valid start-line, parse uri
            return initialize(parts[0], parts[1], _stream);
        }
        else
            return false;
    }
    
    bool HttpRequest::initialize( std::string const& _method,  std::string const& _uri, Dashel::Stream *_stream)
    {
        tokens.clear();
        headers.clear();
        content.clear();
        headers_done = false;
        ready = false;
        verbose = false;

        method = std::string(_method);
        uri = std::string(_uri);
        stream = _stream;
        split_string(uri, '/', tokens);
        return true;
    }
    
    void HttpRequest::incomingData()
    {
        while (! headers_done)
        {
            const string header_field(readLine(stream));
            if (header_field.find("\r\n",0) != 0)
            {
                std::smatch field;
                std::regex e ("([-A-Za-z0-9_]+): (.*)\r\n");
                std::regex_match (header_field,field,e);
                if (field.size() == 3)
                    headers[field[1]] = field[2];
            }
            else
                headers_done = true;
        }
        if (verbose)
        {
            cerr << stream << " Headers complete; (" << headers.size() << " headers)\n";
            for (std::map<std::string,std::string>::iterator i = headers.begin(); i != headers.end(); ++i)
                cerr << "\t\t" << i->first.c_str() << ": " << i->second.c_str() << endl;
        }
        int content_length = atoi(headers["Content-Length"].c_str());
        content_length = (content_length > 40000) ? 40000 : content_length; // truncate at 40000 bytes
        char* buffer = new char[ content_length ];
        stream->read(buffer, content_length);
        content = std::string(buffer,content_length);
        delete buffer;
        ready = true;
    }
    
    void HttpRequest::sendStatus(unsigned status)
    {
        string empty;
        sendStatus(status,empty);
    }

    void HttpRequest::sendStatus(unsigned status, strings& headers)
    {
        string empty;
        sendStatus(status,empty,headers);
    }

    void HttpRequest::sendStatus(unsigned status, std::string& payload)
    {
        strings empty;
        sendStatus(status,payload,empty);
    }
    
    void HttpRequest::sendStatus(unsigned status, std::string& payload, strings& headers)
    {
        std::stringstream reply;
        reply << "HTTP/1.1 " << status << " ";
        switch (status)
        {
            case 200: reply << "OK";                    break;
            case 201: reply << "Created";               break;
            case 400: reply << "Bad Request";           break;
            case 403: reply << "Forbidden";             break;
            case 408: reply << "Request Timeout";       break;
            case 500: reply << "Internal Server Error"; break;
            case 501: reply << "Not Implemented";       break;
            case 503: reply << "Service Unavailable";   break;
            case 404:
            default:  reply << "Not Found";
        }
        string message = reply.str();
        if (headers.size() == 0)
            reply << "\r\nContent-Length: " << payload.size();
        for (strings::iterator i = headers.begin(); i != headers.end(); i++)
            reply << *i << "\r\n";
        reply << "\r\n\r\n" << payload.c_str();
        
        const char* reply_str = reply.str().c_str();
        int reply_len = reply.str().size();
        stream->write(reply_str, reply_len);
        stream->flush();
        
//        if (close_connection)
//            shutdownStream(stream);
//        if (status / 100 != 2)
//            stream->fail(DashelException::Unknown, 0, message.c_str());
    }
    //== end of class HttpInterface ============================================================

};  //== end of namespace Aseba ================================================================