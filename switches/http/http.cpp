
/*
 asebahttp - a switch to bridge Aseba to HTTP
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
 
 Typical use: asebahttp --port 3000 --aesl vmcode.aesl ser:name=Thymio-II & After vmcode.aesl is compiled 
 and uploaded, check with curl http://127.0.0.1:3000/nodes/thymio-II
 
 Substitute appropriate values for :NODENAME, :VARIABLE, :EVENT and their parameters. In most cases, 
 :NODENAME is thymio-II. For example.
 
   - curl http://localhost:3000/nodes/thymio-II/motor.left.target/100 sets the speed of the left motor
   - curl http://nodes/thymio-II/sound_system/4/10 might record sound number 4 for 10 seconds, if such an event were defined in AESL.
 
 Variables and events are learned from the node description and parsed from AESL source when provided. 
 Server-side event (SSE) streams are updated as events arrive. If a variable and an event have the same 
 name, it is the EVENT that is called. On a local machine the server can handle 600 requests/sec with 10 
 concurrent connections, more (up to 2.5 times more) if the requests are pipelined as is the HTTP/1.1 default.
 
 Start an 'asebadummynode 0' and run 'make test' to execute some basic unit tests. Example Node-RED flows 
 can be found in ../../examples/http/node-red.
 
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
    - Aesl program bytecode upload (PUT /nodes/:NODENAME)
      use curl --data-ascii "file=$(cat vmcode.aesl)" -X PUT http://127.0.0.1:3000/nodes/thymio-II
    - accept JSON payload rather than HTML form for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
 
 TODO:
    - handle more than just one Thymio-II node
    - gracefully shut down TCP/IP connections (half-close, wait, close)
 
 This code borrows from the rest of Aseba, especially switches/medulla and examples/clients/cpp-shell,
 which bear the copyright and LGPL 3 licensing notice below.
 
 */
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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <set>
#include <valarray>
#include <vector>
#include <iterator>
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
    
    
    HttpInterface::HttpInterface(const std::string& asebaTarget, const std::string& http_port, const int iterations) :
    Hub(false),  // don't resolve hostnames for incoming connections (there are a lot of them!)
    asebaStream(0),
    httpStream(0),
    nodeId(0),
    nodeDescriptionComplete(false),
    verbose(false),
    iterations(iterations)
    // created empty: pendingResponses, pendingVariables, eventSubscriptions, httpRequests, streamsToShutdown
    {
        // connect to the Aseba target
        std::cout << "HttpInterface connect asebaTarget " << asebaTarget << "\n";
        connect(asebaTarget); // triggers connectionCreated, which assigns asebaStream
        
        // request a description for aseba target
        broadcastGetDescription();
        
        // listen for incoming HTTP requests
        httpStream = connect("tcpin:port=" + http_port);
    }
    
    void HttpInterface::broadcastGetDescription()
    {
        GetDescription getDescription;
        getDescription.serialize(asebaStream);
        asebaStream->flush();
    }
    
    bool HttpInterface::run2s()
	{
		int timeout(2000);
		UnifiedTime startTime;
		while (timeout > 0)
		{
			if (!step(timeout))
				return false;
			timeout -= (Aseba::UnifiedTime() - startTime).value;
		}
		return true;
	}
    
    void HttpInterface::run()
    {
        do
        {
            sendAvailableResponses();
			pingNetwork();
            if (!run2s())
				break;
            if (verbose && streamsToShutdown.size() > 0)
            {
                cerr << "HttpInterface::run "<< streamsToShutdown.size() <<" streams to shut down";
                for (std::set<Dashel::Stream*>::iterator si = streamsToShutdown.begin(); si != streamsToShutdown.end(); si++)
                    cerr << " " << *si;
                cerr << endl;
            }
            if (!streamsToShutdown.empty())
            {
                std::set<Dashel::Stream*>::iterator i = streamsToShutdown.begin();
                Dashel::Stream* stream_to_shutdown = *i;
                streamsToShutdown.erase(*i); // invalidates iterator
                try
                {
                    if (verbose)
                        cerr << stream_to_shutdown << " shutting down stream" << endl;
                    shutdownStream(stream_to_shutdown);
                }
                catch(Dashel::DashelException& e)
                { }
            }
        } while (iterations-- != 0 and asebaStream != 0);
        for (StreamResponseQueueMap::iterator i = pendingResponses.begin(); i != pendingResponses.end(); i++)
            unscheduleAllResponses(i->first);
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
            
            assert( pendingResponses[stream].empty() );
        }
    }
    
    void HttpInterface::connectionClosed(Stream * stream, bool abnormal)
    {
        if (stream == asebaStream)
        {
            // first close all HTTP connections
            for (StreamResponseQueueMap::iterator m = pendingResponses.begin(); m != pendingResponses.end(); m++)
                closeStream(m->first);
            // then stop the hub
            asebaStream = 0;
            if (verbose)
                cerr << "Connection closed to Aseba target" << endl;
            stop();
        }
        else
        {
            if (verbose)
                cerr << stream << " Connection closed to " << stream->getTargetName() << endl;
            unscheduleAllResponses(stream);
            pendingResponses.erase(stream);
            unsigned num = streamsToShutdown.erase(stream);
            if (verbose)
                cerr << stream << " Connection closed, removed " << num << " pending shutdowns" << endl;
        }
    }
    
    void HttpInterface::sendMessage(const Message& message)
	{
		message.serialize(asebaStream);
		asebaStream->flush();
	}
    
    void HttpInterface::nodeDescriptionReceived(unsigned nodeId)
    {
        if (verbose)
            wcerr << this << L"Received description for " << getNodeName(nodeId) << endl;
        if (!nodeId) return;
        this->nodeId = nodeId;
        nodeDescriptionComplete = true;
    }
    
    bool HttpInterface::descriptionReceived()
    {
        if (verbose)
            wcerr << this << L"check descriptionReceived: nodeId = " << nodeId << L", flag = " << nodeDescriptionComplete << L", name=" << getNodeName(nodeId) << endl;
        //return (nodeId > 0);
        return nodeDescriptionComplete;
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
            NodesManager::processMessage(message);
            
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
            HttpRequest* req = new HttpRequest; // [promise] we will eventually delete req in sendAvailableResponses, unscheduleResponse, or stream shutdown
            if ( ! req->initialize(stream))
            {   // protocol failure, shut down connection
                stream->write("HTTP/1.1 400 Bad request\r\n");
                stream->fail(DashelException::Unknown, 0, "400 Bad request");
                unscheduleAllResponses(stream);
                delete req; // not yet in queue, so delete it here [promise]
                return;
            }
            
            if (verbose)
            {
                cerr << stream << " Request " << req->method.c_str() << " " << req->uri.c_str() << " [ ";
                for (unsigned int i = 0; i < req->tokens.size(); ++i)
                    cerr << req->tokens[i] << " ";
                cerr << "] " << req->protocol_version << " new req " << req << endl;
            }
            // continue with incomingData
            scheduleResponse(stream, req);
            req->incomingData(); // read request all at once
            if (req->ready)
                routeRequest(req);
            // run response queues immediately to save time
            sendAvailableResponses();
        }
    }
    
    // Incoming Variables
    void HttpInterface::incomingVariables(const Variables *variables)
    {
        // first, build result string from message
        std::stringstream result;
        result << "[";
        for (size_t i = 0; i < variables->variables.size(); ++i)
            result << (i ? "," : "") << variables->variables[i];
        result << "]";
        string result_str = result.str();
        
        VariableAddress address = std::make_pair(variables->source,variables->start);
        ResponseSet *pending = &pendingVariables[address];
        
        if (verbose)
        {
            cerr << "incomingVariables var (" << variables->source << "," << variables->start << ") = "
                 << result_str << endl << "\tupdates " << pending->size() << " pending";
            for (ResponseSet::iterator i = pending->begin(); i != pending->end(); i++)
                cerr << " " << *i;
            cerr << endl;
        }
        
        for (ResponseSet::iterator i = pending->begin(); i != pending->end(); )
        {
            // i points to an HttpRequest* that is waiting for this variable value
            if (verbose)
                cerr << *i << " updating response var (" << variables->source << "," << variables->start << ")" << endl;
            finishResponse(*i, 200, result_str);
            pending->erase(i++);
        }
        if (verbose)
        {
            cerr << "\tcheck " << pendingVariables[address].size() << " pending";
            for (ResponseSet::iterator i =
                 pendingVariables[address].begin(); i != pendingVariables[address].end(); i++)
                cerr << " " << *i;
            cerr << endl;
        }
        sendAvailableResponses();
    }
    
    // Incoming User Messages
    void HttpInterface::incomingUserMsg(const UserMessage *userMsg)
    {
        if (verbose)
            cerr << "incomingUserMsg msg ("<< userMsg->type <<","<< &userMsg->data <<")" << endl;
        
        // skip if event not known (yet, aesl probably not loaded)
        try { commonDefinitions.events.at(userMsg->type); }
        catch (const std::out_of_range& oor) { return; }
        
        if (commonDefinitions.events[userMsg->type].name.find(L"R_state")==0)
        {
            // update variables
        }
        if (eventSubscriptions.size() > 0)
        {
            // set up SSE message
            std::stringstream reply;
            string event_name = WStringToUTF8(commonDefinitions.events[userMsg->type].name);
            reply << "data: " << event_name;
            for (size_t i = 0; i < userMsg->data.size(); ++i)
                reply << " " << userMsg->data[i];
            reply << "\r\n\r\n";
            
            // In the HTTP world we set up a stream of Server-Sent Events for this.
            // Note that event name is in commonDefinitions.events[userMsg->type].name
            
            for (StreamEventSubscriptionMap::iterator subscriber = eventSubscriptions.begin();
                 subscriber != eventSubscriptions.end(); ++subscriber)
            {
                if (subscriber->second.count("*") >= 1 || subscriber->second.count(event_name) >= 1)
                {
                    appendResponse(subscriber->first, 200, true, reply.str().c_str());
                }
            }
        }
    }
    
    //-- Routing for HTTP requests ---------------------------------------------------------
    
    void HttpInterface::routeRequest(HttpRequest* req)
    {
        // route based on uri prefix
        if (req->tokens[0].find("nodes")==0)
        {
            req->tokens.erase(req->tokens.begin(),req->tokens.begin()+1);
            
            if (req->tokens.size() <= 1)
            {   // one named node
                if (req->method.find("PUT")==0)
                    evLoad(req, req->tokens);  // load bytecode for one node
                else
                    evNodes(req, req->tokens); // get info for one node
            }
            else if (req->tokens.size() >= 2 && req->tokens[1].find("events")==0)
            {   // subscribe to event stream for this node
                req->tokens.erase(req->tokens.begin(),req->tokens.begin()+1);
                evSubscribe(req, req->tokens);
            }
            else
            {   // request for a varibale or an event
                evVariableOrEvent(req, req->tokens);
            }
            return;
        }
        if (req->tokens[0].find("events")==0)
        {   // subscribe to event stream for all nodes
            return evSubscribe(req, req->tokens);
        }
        if (req->tokens[0].find("reset")==0 || req->tokens[0].find("reset_all")==0)
        {   // reset nodes
            return evReset(req, req->tokens);
        }
        else
            finishResponse(req, 404, "");
    }
    
    // Handler: Node descriptions
    
    void HttpInterface::evNodes(HttpRequest* req, strings& args)
    {
        bool do_one_node(args.size() > 0);
        
        std::stringstream json;
        json << (do_one_node ? "" : "[");
        
        for (NodesMap::iterator descIt = nodes.begin();
             descIt != nodes.end(); ++descIt)
        {
            const Node& description(descIt->second);
            string nodeName = WStringToUTF8(description.name);
            
            json << "{";
            json << "\"name\":\"" << nodeName << "\",\"protocolVersion\":" << description.protocolVersion;
            
            if (do_one_node)
            {
                json << ",\"bytecodeSize\":" << description.bytecodeSize;
                json << ",\"variablesSize\":" <<description.variablesSize;
                json << ",\"stackSize\":" << description.stackSize;
                
                // named variables
                json << ",\"namedVariables\":{";
                bool seen_named_variables = false;
                for (NodeNameVariablesMap::const_iterator n(allVariables.find(nodeName));
                     n != allVariables.end(); ++n)
                {
                    VariablesMap vm = n->second;
                    for (VariablesMap::iterator i = vm.begin();
                         i != vm.end(); ++i)
                    {
                        json << (i == vm.begin() ? "" : ",") << "\"" << WStringToUTF8(i->first) << "\":" << i->second.second;
                        seen_named_variables = true;
                    }
                }
                if ( ! seen_named_variables )
                {
                    // failsafe: if compiler hasn't found any variables, get them from the node description
                    for (vector<Aseba::TargetDescription::NamedVariable>::const_iterator i(description.namedVariables.begin());
                         i != description.namedVariables.end(); ++i)
                        json << (i == description.namedVariables.begin() ? "" : ",")
                        << "\"" << WStringToUTF8(i->name) << "\":" << i->size;
                }
                json << "}";
                
                // local events variables
                json << ",\"localEvents\":{";
                for (size_t i = 0; i < description.localEvents.size(); ++i)
                {
                    string ev(WStringToUTF8(description.localEvents[i].name));
                    json << (i == 0 ? "" : ",")
                    << "\"" << ev << "\":"
                    << "\"" << WStringToUTF8(description.localEvents[i].description) << "\"";
                }
                json << "}";
                
                // constants from introspection
                json << ",\"constants\":{";
                for (size_t i = 0; i < commonDefinitions.constants.size(); ++i)
                    json << (i == 0 ? "" : ",")
                    << "\"" << WStringToUTF8(commonDefinitions.constants[i].name) << "\":"
                    << commonDefinitions.constants[i].value;
                json << "}";
                
                // events from introspection
                json << ",\"events\":{";
                for (size_t i = 0; i < commonDefinitions.events.size(); ++i)
                    json << (i == 0 ? "" : ",")
                    << "\"" << WStringToUTF8(commonDefinitions.events[i].name) << "\":"
                    << commonDefinitions.events[i].value;
                json << "}";
            }
            json << "}";
        }
        
        json <<(do_one_node ? "" : "]");
        finishResponse(req,200,json.str());
    }
    
    // Handler: Variable get/set or Event call
    
    void HttpInterface::evVariableOrEvent(HttpRequest* req, strings& args)
    {
        string nodeName(args[0]);
        size_t eventPos;
        
        if ( ! commonDefinitions.events.contains(UTF8ToWString(args[1]), &eventPos))
        {
            // this is a variable
            if (req->method.find("POST") == 0 || args.size() >= 3)
            {
                // set variable value
                strings values;
                if (args.size() >= 3)
                    values.assign(args.begin()+1, args.end());
                else
                {
                    // Parse POST form data
                    values.push_back(args[1]);
                    parse_json_form(req->content, values);
                }
                if (values.size() == 0)
                {
                    finishResponse(req, 404, "");
                    if (verbose)
                        cerr << req << " evVariableOrEevent 404 can't set variable " << args[0] << ", no values" <<  endl;
                    return;
                }
                sendSetVariable(nodeName, values);
                finishResponse(req, 200, "");
                if (verbose)
                    cerr << req << " evVariableOrEevent 200 set variable " << values[0] <<  endl;
            }
            else
            {
                // get variable value
                strings values;
                values.assign(args.begin()+1, args.begin()+2);
                
                unsigned source, start;
                if ( ! getNodeAndVarPos(nodeName, values[0], source, start))
                {
                    finishResponse(req, 404, "");
                    if (verbose)
                        cerr << req << " evVariableOrEevent 404 no such variable " << values[0] <<  endl;
                    return;
                }
                
                sendGetVariables(nodeName, values);
                pendingVariables[std::make_pair(source,start)].insert(req);
                
                if (verbose)
                    cerr << req << " evVariableOrEevent schedule var " << values[0]
                    << "(" << source << "," << start << ") add " << req << " to subscribers" <<  endl;
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
            else if (req->method.find("POST") == 0)
            {
                // Parse POST form data
                parse_json_form(std::string(req->content, req->content.size()), data);
            }
            sendEvent(nodeName, data);
            finishResponse(req, 200, ""); // or perhaps {"return_value":null,"cmd":"sendEvent","name":nodeName}?
            return;
        }
    }
    
    // Handler: Subscribe to an event stream
    
    void HttpInterface::evSubscribe(HttpRequest* req, strings& args)
    {
        // eventSubscriptions[conn] is an unordered set of strings
        if (args.size() == 1)
            eventSubscriptions[req].insert("*");
        else
            for (strings::iterator i = args.begin()+1; i != args.end(); ++i)
                eventSubscriptions[req].insert(*i);
        
        strings headers;
        headers.push_back("Content-Type: text/event-stream");
        headers.push_back("Cache-Control: no-cache");
        headers.push_back("Connection: keep-alive");
        addHeaders(req, headers);
        appendResponse(req,200,true,"");
        // connection must stay open!
    }
    
    // Handler: Compile and store a program into the node, and remember it for introspection
    
    void HttpInterface::evLoad(HttpRequest* req, strings& args)
    {
        if (verbose)
            cerr << "PUT /nodes/" << args[0].c_str() << " trying to load aesl script\n";
        const char* buffer = req->content.c_str();
        size_t pos = req->content.find("file=");
        if (pos != std::string::npos)
        {
            aeslLoadMemory(buffer+pos+5, req->content.size()-pos-5);
            finishResponse(req, 200, "");
        }
        else
            finishResponse(req, 400, "");
    }
    
    // Handler: Reset nodes and rerun
    
    void HttpInterface::evReset(HttpRequest* req, strings& args)
    {
        for (NodesMap::iterator descIt = nodes.begin();
             descIt != nodes.end(); ++descIt)
        {
            bool ok;
            nodeId = getNodeId(descIt->second.name, 0, &ok);
            if (!ok)
                continue;
            string nodeName = WStringToUTF8(descIt->second.name);
            
            Reset(nodeId).serialize(asebaStream); // reset node
            asebaStream->flush();
            Run(nodeId).serialize(asebaStream);   // re-run node
            asebaStream->flush();
            if (nodeName.find("thymio-II") == 0)
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
            
            finishResponse(req, 200, "");
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
            if (verbose)
                wcerr << "invalid node name " << UTF8ToWString(nodeName) << endl;
            return false;
        }
        pos = unsigned(-1);
        
        // check whether variable is known from a compilation, if so, get position
        const NodeNameVariablesMap::const_iterator allVarMapIt(allVariables.find(nodeName));
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
                if (verbose)
                    wcerr << "no variable " << UTF8ToWString(variableName) << " in node " << UTF8ToWString(nodeName);
                return false;
            }
        }
        return true;
    }
    
    // Utility: request update of all variables, used for variable caching
    void HttpInterface::updateVariables(const std::string nodeName)
    {
        strings all_variables;
        for(VariablesMap::iterator it = allVariables[nodeName].begin(); it != allVariables[nodeName].end(); ++it)
            all_variables.push_back(WStringToUTF8(it->first));
        sendGetVariables(nodeName, all_variables);
    }
    
    // Utility: extract argument values from JSON request body
    void HttpInterface::parse_json_form(std::string content, strings& values)
    {
        std::string buffer = content;
        buffer.erase(std::remove_if(buffer.begin(), buffer.end(), ::isspace), buffer.end());
        std::stringstream ss(buffer);
        
        if (ss.get() == '[')
        {
            int i;
            while (ss >> i)
            {
                // values.push_back(std::to_string(short(i)));
                std::stringstream valss;
                valss << short(i);
                values.push_back(valss.str());
                if (ss.peek() == ']')
                    break;
                else if (ss.peek() == ',')
                    ss.ignore();
            }
            if (ss.get() != ']')
                values.erase(values.begin(),values.end());
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
        }
        xmlFreeDoc(doc);
        xmlCleanupParser();
    }
    
    // Load Aesl file from memory
    void HttpInterface::aeslLoadMemory(const char * buffer, const int size)
    {
        // open document
        xmlDoc *doc(xmlReadMemory(buffer, size, "vmcode.aesl", NULL, 0));
        if (!doc)
            wcerr << "cannot read XML from memory " << buffer << endl;
        else
        {
            aeslLoad(doc);
            //if (verbose)
            cerr << "Loaded aesl script in-memory buffer " << buffer << "\n";
        }
        xmlFreeDoc(doc);
        xmlCleanupParser();
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
            xmlXPathFreeObject(obj); // also frees nodeset
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
            xmlXPathFreeObject(obj); // also frees nodeset
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
            xmlXPathFreeObject(obj); // also frees nodeset
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
                xmlFree(name);     // nop if name is NULL
                xmlFree(storedId); // nop if name is NULL
                xmlFree(text);     // nop if text is NULL
            }
            xmlXPathFreeObject(obj); // also frees nodeset
        }
        
        // release memory
        xmlXPathFreeContext(context);
        
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
    
    // Manipulate response queues
    void HttpInterface::scheduleResponse(Dashel::Stream* stream, HttpRequest* req)
    {
        pendingResponses[stream].push_back(req);
    }
    
    void HttpInterface::unscheduleResponse(Dashel::Stream* stream, HttpRequest* req)
    {
        for (ResponseQueue::iterator i = pendingResponses[stream].begin();
             i != pendingResponses[stream].end(); ++i)
            if (*i == req)
            {
                delete req; // [promise]
                pendingResponses[stream].erase(i);
                break;
            }
    }
    
    void HttpInterface::unscheduleAllResponses(Dashel::Stream* stream)
    {
        while(!pendingResponses[stream].empty())
        {
            eventSubscriptions.erase(pendingResponses[stream].front());
            delete pendingResponses[stream].front(); // [promise]
            pendingResponses[stream].pop_front();
        }
    }
    
    void HttpInterface::addHeaders(HttpRequest* req, strings& outheaders)
    {
        req->outheaders = outheaders;
    }
    
    void HttpInterface::finishResponse(HttpRequest* req, unsigned status, std::string result)
    {
        req->result = result;
        req->status = status;
        req->more = false;
        if (verbose)
            cerr << req << " finishResponse " << status << " <" << result << ">" << endl;
    }
    
    void HttpInterface::appendResponse(HttpRequest* req, unsigned status, const bool& keep_open, std::string result)
    {
        req->status = status;
        req->result += result;
        req->more = keep_open;
        if (verbose)
            cerr << req << " appendResponse " << req->status << " <" << req->result.substr(0,7) << "...>" <<(keep_open?", keep open":"")<< endl;
    }
    
    void HttpInterface::sendAvailableResponses()
    {
        // scan through all streams; use while to post-increment when we remove a stream
        for (StreamResponseQueueMap::iterator m = pendingResponses.begin();
             m != pendingResponses.end(); m++)
        {
            if (verbose)
            {
                cerr << m->first << " sendAvailableResponses " << m->second.size() << " in queue";
                for (ResponseQueue::iterator qi = m->second.begin(); qi != m->second.end(); qi++)
                    cerr << " " << *qi;
                cerr << endl;
            }
            bool close_this_stream = false;
            ResponseQueue* q = &(m->second);
            
            // scan through queue for this stream
            q->begin();
            while (! q->empty() && q->front()->status != 0)
            {
                HttpRequest* req = q->front();
                req->sendResponse();
                if ( req->more )
                    break; // keep this request open
                
                if (req->headers["Connection"].find("close")==0 ||
                    (req->protocol_version == "HTTP/1.0" && !(req->headers["Connection"].find("keep-alive")==0)) )
                {
                    // delete all requests, including this one, and remove them from queue
                    unscheduleAllResponses(req->stream);
                    close_this_stream = true;
                }
                else
                {
                    // just this request is finished, delete and remove from queue
                    delete req; // [promise]
                    q->pop_front();
                }
            }
            if (verbose)
                cerr << m->first << " available responses sent, now " << m->second.size() << " in queue" << endl;
            
            if (close_this_stream)
                streamsToShutdown.insert(m->first);
        }
    }
    //== end of class HttpInterface ============================================================
    
    
    HttpRequest::HttpRequest():
    verbose(false)
    {};
    
    bool HttpRequest::initialize( Dashel::Stream *_stream)
    {
        string start_line = readLine(_stream);
        return initialize(start_line, _stream);
    }
    
    bool HttpRequest::initialize( std::string const& start_line, Dashel::Stream *_stream)
    {
        strings parts = split<string>(start_line, " ");
        if (parts.size() == 3
            && (parts[0].find("GET",0)==0 || parts[0].find("PUT",0)==0 || parts[0].find("POST",0)==0)
            && (parts[2].find("HTTP/1.1\r\n",0)==0 || parts[2].find("HTTP/1.0\r\n",0)==0) )
        {
            // valid start-line, parse uri
            return initialize(parts[0], parts[1], parts[2].substr(0,8), _stream);
        }
        else
            return false;
    }
    
    bool HttpRequest::initialize(std::string const& _method,
                                 std::string const& _uri,
                                 std::string const& _protocol_version,
                                 Dashel::Stream *_stream)
    {
        tokens.clear();  // parsed URI
        headers.clear(); // incoming headers
        content.clear(); // incoming payload
        ready = false;
        status = 0;      // outging status
        result.clear();  // outgoing payload
        outheaders.clear();  // outgoing payload
        more = false;
        headers_done = false;
        status_sent = false;
        
        method = std::string(_method);
        uri = std::string(_uri);
        protocol_version = std::string(_protocol_version);
        stream = _stream;
        
        // Also allow %2F as URL part delimiter (see Scratch v430)
        std::string::size_type n = 0;
        while ( (n=uri.find("%2F",n)) != std::string::npos)
            uri.replace(n,3,"/"), n += 1;
        
        tokens = split<string>(uri, "/");
        if (tokens[0].size() == 0)
            tokens.erase(tokens.begin(),tokens.begin()+1);
        return true;
    }
    
    void HttpRequest::incomingData()
    {
        // for now, snarf complete request from stream
        while (! headers_done)
        {
            const string header_field(readLine(stream));
            int term = header_field.find("\r\n",0);
            if (term != 0)
            {
                //                // LLVM regexp search is incredibly slow
                //                std::smatch field;
                //                std::regex e ("([-A-Za-z0-9_]+): (.*)\r\n");
                //                std::regex_match (header_field,field,e);
                //                if (field.size() == 3)
                //                    headers[field[1]] = field[2];
                if (header_field.find("Content-Length: ",0,16)==0)
                    headers["Content-Length"] = header_field.substr(16,term-16);
                else if (header_field.find("Connection: ",0,12)==0)
                    headers["Connection"] = header_field.substr(12,term-12);
            }
            else
                headers_done = true;
        }
        if (verbose)
        {
            cerr << stream << " Headers complete; (" << headers.size() << " headers)";
            for (std::map<std::string,std::string>::iterator i = headers.begin(); i != headers.end(); ++i)
                cerr << " " << i->first.c_str() << ":" << i->second.c_str();
            cerr << endl;
        }
        int content_length = atoi(headers["Content-Length"].c_str());
        content_length = (content_length > 40000) ? 40000 : content_length; // truncate at 40000 bytes
        char* buffer = new char[ content_length ];
        stream->read(buffer, content_length);
        content = std::string(buffer,content_length);
        delete []buffer;
        ready = true;
    }
    
    void HttpRequest::sendResponse()
    {
        if (verbose)
            cerr << this << " sendResponse, status " << (status_sent?"":"not ") << "already sent, have "
            << (result.empty()?"no ":"") << "result" << (more?", more":"") << endl;
        assert( status >= 100 and status <= 599 );
        if ( ! status_sent )
            sendStatus();
        if ( ! result.empty() )
            sendPayload();
        stream->flush();
    }
    
    void HttpRequest::sendStatus()
    {
        if (verbose)
            cerr << this << " sendStatus " << status << endl;
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
        if (outheaders.size() == 0)
        {
            reply << "\r\nContent-Length: " << result.size() << "\r\n";
            reply << "Content-Type: application/json\r\n"; // NO ";charset=UTF-8" cf. RFC 7159
            reply << "Access-Control-Allow-Origin: *\r\n";
            if (headers["Connection"].find("Keep-Alive")==0)
                reply << "Connection: Keep-Alive\r\n";
        }
        else
        {
            for (strings::iterator i = outheaders.begin(); i != outheaders.end(); i++)
                reply << *i << "\r\n";
        }
        reply << "\r\n";
        
        int reply_len = reply.str().size();
        char* reply_str = (char*)malloc(reply_len);
        memcpy(reply_str, reply.str().c_str(), reply_len);
        stream->write(reply_str, reply_len);
        free(reply_str);
        stream->flush();
        status_sent = true;
    }
    
    void HttpRequest::sendPayload()
    {
        if (verbose)
            cerr << this << " sendPayload " << result.size() << " bytes" << endl;
        stream->write(result.c_str(), result.size());
        result = "";
    }
    //== end of class HttpInterface ============================================================
    
};  //== end of namespace Aseba ================================================================
