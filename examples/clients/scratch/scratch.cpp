//
//  main.cpp
//  
//
//  Created by David Sherman on 2014-12-30.
//
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <time.h>

#include "../../../switches/http/http.h"
#include "../../../common/consts.h"
#include "../../../common/types.h"
#include "../../../common/utils/utils.h"
#include "../../../transport/dashel_plugins/dashel-plugins.h"

#include "scratch.h"

std::vector<std::string>&  append_strings_unsigned_vector(std::vector<unsigned>& vec, std::vector<std::string>& result)
{
    for (std::vector<unsigned>::iterator i = vec.begin(); i != vec.end(); i++)
        result.push_back(std::to_string(*i));
    return result;
}

short clamp(short& var, short min, short max)
{
    return var = (var < min ? min : (var > max ? max : var));
}


//== Main event ============================================================================

namespace Aseba
{
    using namespace std;
    using namespace Dashel;
    
    //-- Subclassing Aseba::HttpInterface ------------------------------------------------------
    
    ScratchInterface::ScratchInterface(const strings& targets, const std::string& http_port,
                     const std::string& aseba_port, const int iterations, bool dump) :
    Aseba::HttpInterface(targets, http_port, aseba_port, iterations, dump), // use base class constructor
    state_variable_update_time(0)
    // blink_state(0), // default empty
    // scratch_dial(0), // default empty
    // leds(3,0) // default empty
    {
        verbose=false;
    };
    
    void ScratchInterface::incomingVariables(const Variables *variables)
    {
        // first record incoming value to variable cache
        variable_cache[VariableAddress(variables->source,variables->start)] = std::vector<short>(variables->variables);
        
        // if this is R_state, update the variables it encodes
        VariablesMap vm = allVariables[variables->source];
        VariablesMap::iterator v = vm.find(UTF8ToWString("R_state"));
        if (v != vm.end())
        {
            // parse R_state and store values in variable cache
            if (v->second.first == variables->start)
                receiveStateVariables(variables->source, variables->variables);
        }
        
        // finally, use base class method
        HttpInterface::incomingVariables(variables);
    }
    
    void ScratchInterface::incomingUserMsg(const UserMessage *userMsg)
    {
        // skip if event not known (yet, aesl probably not loaded)
        try { commonDefinitions[userMsg->source].events.at(userMsg->type); }
        catch (const std::out_of_range& oor) { return; }

        if (commonDefinitions[userMsg->source].events.size() >= userMsg->type &&
            commonDefinitions[userMsg->source].events[userMsg->type].name.find(L"Q_motion_ended")==0 &&
            userMsg->data.size() >= 1)
        {
            // Qid, Qtime, QspL, QspR, Qpc
            // remove userMsg->data[0] from busy_threads
            busy_threads.erase(userMsg->data[0]);
            if (verbose)
            {
                cerr << "Q_motion_ended id " << userMsg->data[0] << " time " << userMsg->data[1] << " spL "
                     << userMsg->data[2] << " spR " << userMsg->data[3] << " pc " << userMsg->data[4] << endl;
            }
            // remove from outgoingMessage queue
            unscheduleMessage(userMsg->source, userMsg->data[0]);
        }
        if (commonDefinitions[userMsg->source].events.size() >= userMsg->type &&
            commonDefinitions[userMsg->source].events[userMsg->type].name.find(L"Q_motion_added")==0 &&
            userMsg->data.size() >= 1)
        {
            // Qid, Qtime, QspL, QspR, Qpc
            busy_threads.insert(userMsg->data[0]);
            busy_threads_timeout[userMsg->data[0]] = UnifiedTime(1000);
            if (verbose)
            {
                cerr << "Q_motion_added id " << userMsg->data[0] << " time " << userMsg->data[1] << " spL "
                << userMsg->data[2] << " spR " << userMsg->data[3] << " pc " << userMsg->data[4] << endl;
            }
            // remove from outgoingMessage queue
            unscheduleMessage(userMsg->source, userMsg->data[0]);
        }
        if (commonDefinitions[userMsg->source].events.size() >= userMsg->type &&
            commonDefinitions[userMsg->source].events[userMsg->type].name.find(L"R_state_update")==0 &&
            userMsg->data.size() >= 1)
        {
            // parse R_state and store values in variable cache
            receiveStateVariables(userMsg->source, userMsg->data);

            // resend unacknowledged messages
            resendScheduledMessage(userMsg->source);
        }
        
        HttpInterface::incomingUserMsg(userMsg);
    }

    void ScratchInterface::routeRequest(HttpRequest* req)
    {
        if (req->tokens[0].find("poll")==0)
        {   // poll for variables
            strings args;
            
            unsigned nodeId;
            for (NodesMap::iterator descIt = nodes.begin();
                 descIt != nodes.end(); ++descIt)
                nodeId = descIt->first;
            
//            // breathe temperature led to show polling
//            args.push_back("thymio-II");
//            args.push_back("V_leds_temperature");
//            args.push_back(std::to_string(abs( (int)(blink_state = (blink_state+1)%240)/12 - 10 )));
//            args.push_back(std::to_string(abs( (int)(blink_state = (blink_state+121)%240)/12 - 10 )));
//            sendSetVariable(req->tokens[1], args);
            string result = evPoll(nodeId);
            req->outheaders.push_back("Content-Length: " + std::to_string(result.size()));
            req->outheaders.push_back("Content-Type: text/plain");
            req->outheaders.push_back("Access-Control-Allow-Origin: *");
            finishResponse(req, 200, result);
            return;
        }
        if (req->tokens[0].find("reset")==0 || req->tokens[0].find("reset_all")==0)
        {
            // first, empty busy queue
            busy_threads.clear();
            // then, call base class method HttpInterface::evReset
            return HttpInterface::evReset(req, req->tokens );
        }
        if (req->tokens[0].find("nodes")==0)
        {
            strings args(&(req->tokens[1]),&(req->tokens[req->tokens.size()]));
            std::vector<unsigned> todo;
            if (args.size() > 0)
                todo = HttpInterface::getIdsFromURI(args);

            // Try to find a node for this request. Only the first one can succeed, it will end break out of the loop with return
            for (std::vector<unsigned>::const_iterator it = todo.begin(); it != todo.end(); ++it)
            {
                unsigned nodeId = *it;
                size_t eventPos;

                // If this node isn't running thymio_motion.aesl, try next node or else fall through to HttpInterface::routeRequest
                if (! commonDefinitions[nodeId].events.contains(L"Q_add_motion", &eventPos))
                    continue;

                // If requesting a scratch event, check that enough args are supplied
                for (auto scratch_event: standard_scratch_events)
                    if (req->tokens[2].find(WStringToUTF8(scratch_event.first))==0 &&
                        req->tokens.size() < scratch_event.second + 3)
                        {
                            finishResponse(req, 400, WStringToUTF8(scratch_event.first)+" requires "+to_string(scratch_event.second)+
                                           " args but request provided "+to_string(req->tokens.size()-3)+"\n"); // fails with BAD REQUEST
                            return;
                        }

                // Try to route a scratch event; if not, fall through to HttpInterface::routeRequest
                if (req->tokens[2].find("scratch_start")==0)
                { //
                    strings args;
                    args.push_back("motor.left.target");
                    args.push_back(std::to_string(stoi(req->tokens[3])*32/10));
                    sendSetVariable(nodeId, args);
                    args.at(0) = "motor.right.target";
                    args.at(1) = std::to_string(stoi(req->tokens[4])*32/10);
                    sendSetVariable(nodeId, args);
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_change_speed")==0)
                { //
                    std::vector<short> left_target, right_target;
                    if (getCachedVal(nodeId, "motor.left.target", left_target) &&
                        getCachedVal(nodeId, "motor.right.target", right_target))
                    {
                        strings args;
                        args.push_back("motor.left.target");
                        args.push_back(std::to_string(stoi(req->tokens[3])*32/10 + left_target.front()));
                        sendSetVariable(nodeId, args);
                        args.at(0) = "motor.right.target";
                        args.at(1) = std::to_string(stoi(req->tokens[4])*32/10 + right_target.front());
                        sendSetVariable(nodeId, args);
                        finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    }
                    return;
                }
                else if (req->tokens[2].find("scratch_stop")==0)
                { //
                    strings args;
                    args.push_back("motor.left.target");
                    args.push_back("0");
                    sendSetVariable(nodeId, args);
                    args.at(0) = "motor.right.target";
                    sendSetVariable(nodeId, args);
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_set_dial")==0)
                { //
                    scratch_dial[nodeId] = stoi(req->tokens[3]);
                    sendEvent(nodeId, makeLedsCircleVector(scratch_dial[nodeId]));
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                }
                else if (req->tokens[2].find("scratch_next_dial_limit")==0)
                { //
                    unsigned limit = stoi(req->tokens[3]) % 648;
                    scratch_dial[nodeId] = (scratch_dial[nodeId] + 1) % limit;
                    sendEvent(nodeId, makeLedsCircleVector(scratch_dial[nodeId]));
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_next_dial")==0)
                { //
                    unsigned limit = 8;
                    scratch_dial[nodeId] = (scratch_dial[nodeId] + 1) % limit;
                    sendEvent(nodeId, makeLedsCircleVector(scratch_dial[nodeId]));
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_clear_leds")==0)
                { //
                    strings args;
                    std::vector<unsigned> v8(8,0);
                    args.push_back("V_leds_circle");
                    append_strings_unsigned_vector(v8, args);
                    sendEvent(nodeId, args);
                    
                    std::vector<unsigned> v3(3,0);
                    args.clear();
                    args.push_back("V_leds_top");
                    append_strings_unsigned_vector(v3, args);
                    sendEvent(nodeId, args);
                    leds[std::make_pair(nodeId,0)] = 200;
                    
                    std::vector<unsigned> v4(4,0);
                    args.clear();
                    args.push_back("V_leds_buttons");
                    append_strings_unsigned_vector(v4, args);
                    sendEvent(nodeId, args);
                    args.at(0) = "V_leds_bottom";
                    sendEvent(nodeId, args);
                    args.at(1) = "1";
                    sendEvent(nodeId, args);
                    leds[std::make_pair(nodeId,1)] = 200;
                    leds[std::make_pair(nodeId,2)] = 200;
                    
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_set_leds")==0)
                { //
                    unsigned color = stoi(req->tokens[3]) % 198;
                    unsigned mask;
                    if (req->tokens[4].compare("all")==0)
                        mask = 7;
                    else if (req->tokens[4].compare("top")==0)
                        mask = 1;
                    else if (req->tokens[4].compare("bottom")==0)
                        mask = 6;
                    else if (req->tokens[4].compare("bottom%20left")==0)
                        mask = 2;
                    else if (req->tokens[4].compare("bottom%20right")==0)
                        mask = 4;
                    else
                        mask = stoi(req->tokens[4]) & 7;
                    strings args = makeLedsRGBVector(color); // by default, "V_leds_top"
                    if ((mask & 1) == 1)
                        leds[std::make_pair(nodeId,0)]=color, sendEvent(nodeId, args);
                    args.insert(args.begin()+1,"0");
                    if ((mask & 2) == 2)
                        args.at(0) = "V_leds_bottom", leds[std::make_pair(nodeId,1)]=color, sendEvent(nodeId, args);
                    args.at(1) = "1";
                    if ((mask & 4) == 4)
                        args.at(0) = "V_leds_bottom", leds[std::make_pair(nodeId,2)]=color, sendEvent(nodeId, args);
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_change_leds")==0)
                { //
                    unsigned delta = stoi(req->tokens[3]) % 198;
                    unsigned mask;
                    if (req->tokens[4].compare("all")==0)
                        mask = 7;
                    else if (req->tokens[4].compare("top")==0)
                        mask = 1;
                    else if (req->tokens[4].compare("bottom")==0)
                        mask = 6;
                    else if (req->tokens[4].compare("bottom%20left")==0)
                        mask = 2;
                    else if (req->tokens[4].compare("bottom%20right")==0)
                        mask = 4;
                    else
                        mask = stoi(req->tokens[4]) & 7;
                    if ((mask & 1) == 1)
                    {
                        strings args = makeLedsRGBVector(leds[std::make_pair(nodeId,0)] = (leds[std::make_pair(nodeId,0)] + delta) % 198);
                        sendEvent(nodeId, args);
                    }
                    if ((mask & 2) == 2)
                    {
                        strings args = makeLedsRGBVector(leds[std::make_pair(nodeId,1)] = (leds[std::make_pair(nodeId,1)] + delta) % 198);
                        args.at(0) = "V_leds_bottom";
                        args.insert(args.begin()+1,"0");
                        sendEvent(nodeId, args);
                    }
                    if ((mask & 4) == 4)
                    {
                        strings args = makeLedsRGBVector(leds[std::make_pair(nodeId,2)] = (leds[std::make_pair(nodeId,2)] + delta) % 198);
                        args.at(0) = "V_leds_bottom";
                        args.insert(args.begin()+1,"1");
                        sendEvent(nodeId, args);
                    }
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_avoid")==0)
                { //
                    std::vector<short> dist_front, angle_front, left_target, right_target;
                    for (int i = 0; i < 3; i++)
                    {
                        if (getCachedVal(nodeId, "distance.front", dist_front) &&
                            getCachedVal(nodeId, "angle.front", angle_front) &&
                            getCachedVal(nodeId, "motor.left.target", left_target) &&
                            getCachedVal(nodeId, "motor.right.target", right_target))
                        {
                            int vL = ((-1520 + 12*dist_front.front() + 400*angle_front.front()) * left_target.front()) / 760;
                            int vR = ((-1520 + 12*dist_front.front() - 400*angle_front.front()) * right_target.front()) / 760;
                            if (abs(vL-vR) < 20)
                                vL += (rand() % 33) - 16;
                            strings args;
                            args.push_back("motor.left.target");
                            args.push_back(std::to_string(vL));
                            sendSetVariable(nodeId, args);
                            args.at(0) = "motor.right.target";
                            args.at(1) = std::to_string(vR);
                            sendSetVariable(nodeId, args);
                        }
                        sendPollVariables(nodeId); // urgently get updated sensor values
                    }
                    finishResponse(req, 204, ""); // succeeds with 204 NO CONTENT
                    return;
                }
                else if (req->tokens[2].find("scratch_move")==0)
                { //
                    int busyid = stoi(req->tokens[3]);
                    int mm = stoi(req->tokens[4]);
                    int speed = abs(mm) < 20 ? 20 : (abs(mm) > 150 ? 150 : abs(mm));
                    int time = abs(mm) * 100 / speed; // time measured in 100 Hz ticks
                    speed = speed * 32 / 10;
                    strings args;
                    args.push_back("Q_add_motion");
                    args.push_back(std::to_string(busyid));
                    args.push_back(std::to_string(time));
                    args.push_back(std::to_string((mm > 0) ? speed : -speed ));
                    args.push_back(std::to_string((mm > 0) ? speed : -speed ));
                    sendScheduledEvent(nodeId, args, busyid);
                    finishResponse(req, 200, std::to_string(busyid)); // succeeds with 200 OK, reports busy id
                    return;
                }
                else if (req->tokens[2].find("scratch_turn")==0)
                { // dist = 39.76 * degrees + 1.225 * speed - 96.57
                    // int dist = (24850 * abs(degrees) + 763 * speed) / 100;
                    int busyid = stoi(req->tokens[3]);
                    int degrees = stoi(req->tokens[4]);
                    int speed, time;
                    if (abs(degrees) > 90)
                    {
                        speed = 65 * 32/10;
                        time = abs(degrees) * 1.3;
                    }
                    else
                    {
                        speed = 43 * 32/10;
                        time = abs(degrees) * 2.0;
                        time = degrees*degrees * 2.0 / (abs(degrees)*1.016 - 0.52); // nonlinear correction
                    }
                    strings args;
                    args.push_back("Q_add_motion");
                    args.push_back(std::to_string(busyid));
                    args.push_back(std::to_string(time));
                    args.push_back(std::to_string((degrees > 0) ?  speed : -speed ));
                    args.push_back(std::to_string((degrees > 0) ? -speed :  speed ));
                    sendScheduledEvent(nodeId, args, busyid);
                    finishResponse(req, 200, std::to_string(busyid)); // succeeds with 200 OK, reports busy id
                    return;
                }
                else if (req->tokens[2].find("scratch_arc")==0)
                { //
                    int busyid = stoi(req->tokens[3]);
                    int radius = stoi(req->tokens[4]);
                    int degrees = stoi(req->tokens[5]);
                    if (abs(radius) < 100)
                        radius = (radius < 0) ? -100 : 100; // although actually, we should just call scratch_turn
                    int ratio = (abs(radius)-95) * 10000 / abs(radius);
                    //                int time = degrees * 35 * radius / 8 / 2000;
                    int time = (degrees * (50.36 * radius + 25)) / 3600;
                    int v_out = 400;
                    int v_in = v_out * ratio / 10000;
                    if (radius < 0)
                        v_in = -v_in, v_out = -v_out;
                    strings args;
                    args.push_back("Q_add_motion");
                    args.push_back(std::to_string(busyid));
                    args.push_back(std::to_string(time));
                    args.push_back(std::to_string((degrees > 0) ?  v_out : v_in  ));
                    args.push_back(std::to_string((degrees > 0) ?  v_in  : v_out ));
                    sendScheduledEvent(nodeId, args, busyid);
                    finishResponse(req, 200, std::to_string(busyid)); // succeeds with 200 OK, reports busy id
                    return;
                }
                else if (req->tokens[2].find("scratch_sound_freq")==0)
                { // TODO: need to set timer!
                    //                int busyid = stoi(req->tokens[3]);
                    //                int freq = stoi(req->tokens[4]);
                    //                int sixtieths = stoi(req->tokens[5]);
                    finishResponse(req, 501, ""); // fails with NOT IMPLEMENTED
                    return;
                }
            }
        }
        HttpInterface::routeRequest(req); // else use base class method
    }

    void ScratchInterface::evNodes(HttpRequest* req, strings& args)
    {
        // Patch the events map of every node running thymio_motion.aesl to add scratch-specific events if needed.
        // They are implemented by hard-coded routes in ScratchInterface::routeRequest and will never be called by HttpInterface::sendEvent
        for (NodesMap::iterator descIt = nodes.begin(); descIt != nodes.end(); ++descIt)
        {
            unsigned nodeId = descIt->first;
            size_t eventPos;
            if (commonDefinitions[nodeId].events.contains(L"Q_add_motion", &eventPos))
                for (auto ev: standard_scratch_events)
                    if ( ! commonDefinitions[nodeId].events.contains(ev.first, &eventPos))
                        commonDefinitions[nodeId].events.push_back(NamedValue(ev.first, ev.second));
        }
        HttpInterface::evNodes(req, args); // else use base class method
    }
    
    void ScratchInterface::sendSetVariable(const unsigned nodeId, const strings& args)
    {
        // update variable cache
        SetVariables::VariablesVector data;
        for (size_t i=1; i<args.size(); ++i)
            data.push_back(atoi(args[i].c_str()));
        setCachedVal(nodeId, args[0], data);
        // then continue with base class method
        HttpInterface::sendSetVariable(nodeId, args);
    }
    
    // helpers
    
    ScratchInterface::strings ScratchInterface::makeLedsCircleVector(unsigned dial)
    {
        std::vector<unsigned> leds(8,0);
        leds.at(dial % 8) = 8;
        if ((dial /  8) % 9) leds.at(((dial / 8) % 9) - 1) = 24;
        if ((dial / 72) % 9) leds.at(((dial / 72) % 9) - 1) = 32;
        strings args;
        args.push_back("V_leds_circle");
        append_strings_unsigned_vector(leds, args);
        return args;
    }
    
    ScratchInterface::strings ScratchInterface::makeLedsRGBVector(unsigned color)
    {
        std::vector<unsigned> rgb(3,0);
        switch (color / 33)
        {
            case 0: rgb.at(0)=33; rgb.at(1)=color%33; rgb.at(2)=0; break;
            case 1: rgb.at(0)=33-color%33; rgb.at(1)=33; rgb.at(2)=0; break;
            case 2: rgb.at(0)=0; rgb.at(1)=33; rgb.at(2)=color%33; break;
            case 3: rgb.at(0)=0; rgb.at(1)=33-color%33; rgb.at(2)=33; break;
            case 4: rgb.at(0)=color%33; rgb.at(1)=0; rgb.at(2)=33; break;
            case 5: rgb.at(0)=33; rgb.at(1)=0; rgb.at(2)=33-color%33; break;
        }
        strings args;
        args.push_back("V_leds_top");
        append_strings_unsigned_vector(rgb, args);
        return args;
    }
    
    // New methods for ScratchInterface

    char* wstringtocstr(std::wstring w, char* name)
    {
        //char* name = (char*)malloc(128);
        std::wcstombs(name, w.c_str(), 127);
        return name;
    }
    
    void ScratchInterface::sendScheduledEvent(const unsigned nodeId, const strings& args, int busyid)
    {
        assert(busyid == atoi(args[1].c_str()));
        sendEvent(nodeId, args);
        
        UnifiedTime now;
        ScheduledUserMessage msg(now,args);
        outgoingMessages[nodeId].push(msg);

        busy_threads.insert(busyid); // don't rely on Q_motion_added
        busy_threads_timeout[busyid] = UnifiedTime(5000);
        UnifiedTime delayForPoll(200);
        delayForPoll.sleep();
    }
    
    void ScratchInterface::unscheduleMessage(const unsigned nodeId, const unsigned busyid)
    {
        if ( ! outgoingMessages[nodeId].empty())
        {
            strings args(outgoingMessages[nodeId].front().second);
            if (args.size() > 1)
                if (busyid == atoi(args[1].c_str()))
                    outgoingMessages[nodeId].pop();
        }
        
    }

    void ScratchInterface::resendScheduledMessage(const unsigned nodeId)
    {
        if ( ! outgoingMessages[nodeId].empty())
        {
            UnifiedTime scheduled = outgoingMessages[nodeId].front().first;
            UnifiedTime now;
            if (now - scheduled <= 800)
                sendEvent(nodeId, outgoingMessages[nodeId].front().second);
            else
                outgoingMessages[nodeId].pop();
        }
    }
    
    bool ScratchInterface::getCachedVal(const unsigned nodeId, const std::string& varName,
                                        std::vector<short>& cachedval)
    {
        unsigned source, pos, len;
        if (getVarAddrLen(nodeId, varName, source, pos, len))
        {
            cachedval = variable_cache[std::make_pair(source, pos)];
            return ! cachedval.empty();
        }
        return false;
    }
    bool ScratchInterface::setCachedVal(const unsigned nodeId, const std::string& varName,
                                        std::vector<short>& val)
    {
        unsigned source, pos, len;
        if (getVarAddrLen(nodeId, varName, source, pos, len))
        {
            variable_cache[std::make_pair(source, pos)] = val;
            return ! val.empty();
        }
        return false;
    }
    bool ScratchInterface::getVarAddrLen(const unsigned nodeId, const std::string& varName,
                                         unsigned& source, unsigned& pos, unsigned& length)
    {
        if ( ! HttpInterface::getVarPos(nodeId, varName, pos) )
            return false;
        
        source = nodeId; // is this right?

        VariablesMap vm = allVariables[nodeId];
        VariablesMap::iterator it = vm.find(UTF8ToWString(varName));
        if (it == vm.end())
            return false;
        
        length = it->second.second; // *it is < varName, < pos, len > >
        return true;
    }
    string ScratchInterface::evPoll(const unsigned nodeId)
    {
        // If it is time to poll for an update, send a request for variables
        UnifiedTime now;
        if (now - state_variable_update_time > 2000)
            sendPollVariables(nodeId); // ask for value of R_state

        // each line of the poll result given
        std::stringstream result;
        char wbuf[128];
        
        // Tell Scratch which threads are still busy
        result << "_busy";

        // - look in Qid for threads not in busy_threads
        std::set<int> busy_qid;
        unsigned qid_source, qid_pos, qid_len;
        if (getVarAddrLen(nodeId, "Qid", qid_source, qid_pos, qid_len))
        {
            for (auto i: variable_cache[std::make_pair(qid_source,qid_pos)])
                if (i != 0)
                    busy_qid.insert(i);
        }
        for (auto i: busy_qid)
            busy_threads_timeout[i] = UnifiedTime(1000);

        // - temp for testing, check whether Q_motion_ended event had arrived
        std::vector<int> vec_qid(busy_qid.begin(),busy_qid.end());
        std::sort(vec_qid.begin(),vec_qid.end());
        std::vector<int> vec_thr(busy_threads.begin(),busy_threads.end());
        std::sort(vec_thr.begin(),vec_thr.end());

//        std::vector<int> diff_qid_thr;
//        if (vec_qid.size() > 0 && vec_thr.size() > 0)
//            std::set_difference(vec_qid.begin(), vec_qid.end(), vec_thr.begin(), vec_thr.end(),
//                                std::inserter(diff_qid_thr, diff_qid_thr.end()));
//        for (auto i: diff_qid_thr)
//            cerr << "Warning " << i << " in Qid but not in busy_threads" << endl;
//
//        std::vector<int> diff_thr_qid;
//        if (vec_qid.size() > 0 && vec_thr.size() > 0)
//            std::set_difference(vec_thr.begin(), vec_thr.end(), vec_qid.begin(), vec_qid.end(),
//                                std::inserter(diff_thr_qid, diff_thr_qid.end()));
//        for (auto i: diff_thr_qid)
//            cerr << "Warning " << i << " in busy_threads but not in Qid" << endl;

        // - report all busy threads conservatively
        std::set<int> busy_all(busy_threads.begin(), busy_threads.end());
        for (auto i: busy_qid)
            busy_all.insert(i);
        for (auto i: busy_all)
        {
            if (busy_threads_timeout[i] < UnifiedTime(33))
            {
                busy_threads_timeout.erase(i);
                busy_threads.erase(i);
                unscheduleMessage(nodeId, i);
                cerr << "Warning killing unresponsive thread " << i << endl;
                continue;
            }
            busy_threads_timeout[i] -= UnifiedTime(33);
            result << " " << i;
            // // temp for testing, check whether Q_motion_ended event had arrived
            // if (std::count(busy_threads.begin(), busy_threads.end(), i) == 0 && i != 0)
            //     cerr << "Warning " << i << " in Qid but not in busy_threads" << endl;
        }
        result << endl;
        
        // Tell Scratch the values of all variables named in the AESL program
        // If the variable name starts with "b_" report its boolean value as a string "true" or "false"
        for(VariablesMap::iterator it = allVariables[nodeId].begin(); it != allVariables[nodeId].end(); ++it)
        {
            bool is_boolean_variable = (it->first.find(L"b_",0) == 0);
            std::vector<short> cachedval = variable_cache[std::make_pair(nodeId,it->second.first)];
            
            if (cachedval.size() == 0)
                continue;
            
            result << wstringtocstr(it->first,wbuf);
            for (std::vector<short>::iterator i = cachedval.begin(); i != cachedval.end(); ++i)
                if (is_boolean_variable)
                    result << " " << (*i ? "true" : "false");
                else
                    result << " " << *i;
            result << endl;
        }
        
        // Tell Scratch the values of derived variables that are created on the fly
        // derived 1: button boolean flags; formatted for menus
        std::vector<short> cv;
        result << "b_button/center " << (getCachedVal(nodeId, "button.center", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/forward " << (getCachedVal(nodeId, "button.forward", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/backward " << (getCachedVal(nodeId, "button.backward", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/left " << (getCachedVal(nodeId, "button.left", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/right " << (getCachedVal(nodeId, "button.right", cv) && cv.front() ? "true" : "false") << endl;

        // derived 2: proximity warnings; formatted for menus
        if (getCachedVal(nodeId, "prox.horizontal", cv))
        {
            result << "b_touching/front " << ((cv[0]+cv[1]+cv[2]+cv[3]+cv[4])/1000 > 0 ? "true" : "false") << endl;
            result << "b_touching/back " << ((cv[5]+cv[6])/1000 > 0 ? "true" : "false") << endl;
            for (unsigned i=0; i <= 6; i++)
                result << "proximity/" << i << " " << cv[i] << endl;
        }
        if (getCachedVal(nodeId, "prox.ground.delta", cv))
        {
            result << "b_touching/ground " << ((cv[0]+cv[1])/500 > 0 ? "true" : "false") << endl;
            for (unsigned i=0; i <= 1; i++)
                result << "ground/" << i << " " << cv[i] << endl;
        }
        
        // derived 3: proximity distances; formatted for menus
        if (getCachedVal(nodeId, "distance.front", cv))
            result << "distance/front " << clamp(cv.front(),(short)0,(short)190) << endl;
        if (getCachedVal(nodeId, "distance.back", cv))
            result << "distance/back " << clamp(cv.front(),(short)0,(short)120) << endl;
        if (getCachedVal(nodeId, "prox.ground.delta", cv))
            result << "distance/ground " << (cv[0]+cv[1] > 1000 ? 0 : 500) << endl;
        
        if (getCachedVal(nodeId, "angle.front", cv))
            result << "angle/front " << cv.front() << endl;
        if (getCachedVal(nodeId, "angle.back", cv))
            result << "angle/back " << cv.front() << endl;
        if (getCachedVal(nodeId, "angle.ground", cv))
            result << "angle/ground " << cv.front() << endl;

        // derived 4: odometry; formatted for menus
        if (getCachedVal(nodeId, "odo.degree", cv))
            result << "odo/direction " << cv.front() << endl; // same as 90 - odo.theta/182
        if (getCachedVal(nodeId, "odo.x", cv))
            result << "odo/x " << cv.front()/28 << endl;
        if (getCachedVal(nodeId, "odo.y", cv))
            result << "odo/y " << cv.front()/28 << endl;

        // derived 5: motor speeds; formatted for menus
        if (getCachedVal(nodeId, "motor.left.speed", cv))
            result << "motor.speed/left " << cv.front()  *10/32 << endl;
        if (getCachedVal(nodeId, "motor.right.speed", cv))
            result << "motor.speed/right " << cv.front() *10/32 << endl;
        if (getCachedVal(nodeId, "motor.left.target", cv))
            result << "motor.target/left " << cv.front() *10/32 << endl;
        if (getCachedVal(nodeId, "motor.right.target", cv))
            result << "motor.target/right " << cv.front()*10/32 << endl;
        
        // derived 6: accelerometer values; formatted for menus
        if (getCachedVal(nodeId, "acc", cv))
        {
            result << "tilt/right-left " << cv[0] << endl;
            result << "tilt/front-back " << cv[1] << endl;
            result << "tilt/top-bottom " << cv[2] << endl;
        }
        
        // derived 7: LED values; formatted for menus
        result << "leds/top " << leds[std::make_pair(nodeId,0)] << endl;
        result << "leds/bottom%20left " << leds[std::make_pair(nodeId,1)] << endl;
        result << "leds/bottom%20right " << leds[std::make_pair(nodeId,2)] << endl;
        
        // derived 8: clap warning
        std::vector<short> mic_threshold, mic_intensity;
        if (getCachedVal(nodeId, "mic.threshold", mic_threshold) &&
            getCachedVal(nodeId, "mic.intensity", mic_intensity))
            result << "b_clap " << ((mic_intensity.front() >= mic_threshold.front()) ? "true" : "false") << endl;

        //cerr << "poll result " << result.str() << endl;
        return result.str();
    }
    
    void ScratchInterface::sendPollVariables(const unsigned nodeId)
    {
        if (polled_variables.size() == 0)
        {
            unsigned source, pos, len;
            
            // poll R_state if we can to learn values of state variables
            std::map<unsigned, GetVariables>::iterator i = node_r_state.find(source);
            if (i != node_r_state.end())
            {
                // already know address of R_state
                polled_variables.push_back(i->second);
            }
            else if (getVarAddrLen(nodeId, "R_state", source, pos, len))
            {
                // didn't know address but can find it
                node_r_state[source] = GetVariables(source, pos, len);
                polled_variables.push_back(node_r_state[source]);
            }
            // if we can't use R_state, then poll all variables individually
            else
                for (auto variable: interesting_state_variables) {
                    if (getVarAddrLen(nodeId, variable.first, source, pos, len))
                        polled_variables.push_back(GetVariables(source, pos, len));
            }

            // poll Qid to learn which threads are busy
            if (getVarAddrLen(nodeId, "Qid", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));

        }
        std::list<GetVariables>::iterator pvi = polled_variables.begin();
        if (verbose)
            cerr << "poll ";
        int window = polled_variables.size(); //!// <= 50 ? polled_variables.size() : polled_variables.size()/3;

        Dashel::Stream* stream;
        try {
            stream = getStreamFromNodeId(nodeId); // may fail
        }
        catch(runtime_error(e))
        {
            cerr << "sendEvent node id " << nodeId << ": bad node id" << endl;
            // HTTP response should be 400 BAD REQUEST, response body should be bad node id
            return; // hack, should be using exceptions for HTTP errors
        }

        for (int i = 0; i < window; i++, pvi++)
        {
            GetVariables getVariables(*pvi);
            getVariables.serialize(stream);
            if (verbose)
                cerr << "1," << pvi->start << "(" << pvi->length << ") ";
        }
        if (verbose)
            cerr << endl;
        stream->flush();
        //!// std::rotate(polled_variables.begin(), pvi, polled_variables.end());
    }
    
    void ScratchInterface::receiveStateVariables(const unsigned nodeId, const std::vector<short> data)
    {
        unsigned source, pos, len;
        UnifiedTime now;
        state_variable_update_time = now; // set to current time
        
        for (auto variable: interesting_state_variables) {
            string varName = variable.first;
            unsigned dataBegin = variable.second.first;
            unsigned dataEnd = variable.second.second;
            // fill in state variable addresses if needed
            if (state_variable_addresses.count(variable.first) == 0)
                if (getVarAddrLen(nodeId, variable.first, source, pos, len))
                    state_variable_addresses[variable.first] = make_pair(source, pos);

            // continue if we know this variable
            if (state_variable_addresses.count(variable.first) > 0)
            {
                variable_cache[state_variable_addresses[varName]].assign(data.begin()+dataBegin, data.begin()+dataEnd);
            }
        }
        // extract encoded values 1: buttons + thresholded microphone
        std::vector<short> payload = variable_cache[state_variable_addresses["button.backward"]];
        if (payload.size() == 1)
        {
            variable_cache[state_variable_addresses["button.backward"]].assign(1, (payload[0] >> 4) % 1);
            variable_cache[state_variable_addresses["button.center"]].assign(1, (payload[0] >> 3) % 1);
            variable_cache[state_variable_addresses["button.forward"]].assign(1, (payload[0] >> 2) % 1);
            variable_cache[state_variable_addresses["button.left"]].assign(1, (payload[0] >> 1) % 1);
            variable_cache[state_variable_addresses["button.right"]].assign(1, payload[0] % 1);
            variable_cache[state_variable_addresses["mic.intensity"]].assign(1, (payload[0] >> 8) % 8);
        }
        // extract encoded values 2: acc
        payload = variable_cache[state_variable_addresses["acc"]];
        if (payload.size() == 1)
        {
            variable_cache[state_variable_addresses["acc"]].assign(3, (((payload[0] >> 10) % 32) - 16) * 2);
            variable_cache[state_variable_addresses["acc"]][1] = (((payload[0] >> 5) % 32) - 16) * 2;
            variable_cache[state_variable_addresses["acc"]][2] = ((payload[0] % 32) - 16) * 2;
        }
        // extract encoded values 3: distance
        payload = variable_cache[state_variable_addresses["distance.back"]];
        if (payload.size() == 1)
        {
            variable_cache[state_variable_addresses["distance.back"]].assign(1, (payload[0] >> 8) % 256);
            variable_cache[state_variable_addresses["distance.front"]].assign(1, payload[0] % 256);
        }
        // extract encoded values 4: angle.back and angle.ground
        payload = variable_cache[state_variable_addresses["angle.back"]];
        if (payload.size() == 1)
        {
            variable_cache[state_variable_addresses["angle.ground"]].assign(1, ((payload[0] >> 8) % 256) - 90);
            variable_cache[state_variable_addresses["angle.back"]].assign(1, (payload[0] % 256) - 90);
        }
    }

    //== end of class ScratchInterface ============================================================
    
};  //== end of namespace Aseba ================================================================



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
    stream << "-K, --Kiter n   : run I/O loop n thousand times (for profiling)\n";
    stream << "-h, --help      : shows this help\n";
    stream << "-V, --version   : shows the version number\n";
    stream << "Additional targets are any valid Dashel targets." << std::endl;
    stream << "Report bugs to: david.sherman@inria.fr" << std::endl;
}

//! Show version
void dumpVersion(std::ostream &stream)
{
    stream << "asebahttp 2015-01-01 David James Sherman <david dot sherman at inria dot fr>" << std::endl;
    stream << "Aseba version " << ASEBA_VERSION << std::endl;
    stream << "Aseba protocol " << ASEBA_PROTOCOL_VERSION << std::endl;
    stream << "Aseba library licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n";
}


// Main
int main(int argc, char *argv[])
{
    Dashel::initPlugins();
    
    std::string http_port = "3000";
    std::string aseba_port;
    std::string aesl_filename;
    std::vector<std::string> dashel_target_list;
    bool verbose = false;
    bool dump = false;
    int Kiterations = -1; // set to > 0 to limit run time e.g. for valgrind
        
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
        else if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
            dumpVersion(std::cout), exit(1);
        else if ((strcmp(arg, "-p") == 0) || (strcmp(arg, "--http") == 0))
            http_port = argv[argCounter++];
        else if ((strcmp(arg, "-s") == 0) || (strcmp(arg, "--port") == 0))
            aseba_port = argv[argCounter++];
        else if ((strcmp(arg, "-a") == 0) || (strcmp(arg, "--aesl") == 0))
            aesl_filename = argv[argCounter++];
        else if ((strcmp(arg, "-K") == 0) || (strcmp(arg, "--Kiter") == 0))
            Kiterations = atoi(argv[argCounter++]);
        else if (strncmp(arg, "-", 1) != 0)
            dashel_target_list.push_back(arg);
    }

    if (dashel_target_list.size() == 0)
        dashel_target_list.push_back("ser:name=Thymio");

    // initialize Dashel plugins
    Dashel::initPlugins();
    
    // create and run bridge, catch Dashel exceptions
    try
    {
        Aseba::ScratchInterface* network(new Aseba::ScratchInterface(dashel_target_list, http_port, aseba_port,
                                                                     Kiterations > 0 ? 1000*Kiterations : 10, dump));
        
        for (auto nodeId: network->allNodeIds())
            try {
                if (aesl_filename.size() > 0)
                    network->aeslLoadFile(nodeId, aesl_filename);
                else
                {
                    network->aeslLoadFile(nodeId, "thymio_motion.aesl"); // in Windows will be in same directory as executable
                }
            } catch ( std::runtime_error(e) ) {
                const char* failsafe = "<!DOCTYPE aesl-source><network><keywords flag=\"true\"/><node nodeId=\"1\" name=\"thymio-II\"></node></network>\n";
                network->aeslLoadMemory(nodeId, failsafe,strlen(failsafe));
            }
        
        do {
            network->pingNetwork(); // possibly not, if do_ping == false
        } while (network->run1s());
        delete network;
    }
    catch(Dashel::DashelException e)
    {
        if (e.source == Dashel::DashelException::ConnectionFailed)
            std::this_thread::sleep_for (std::chrono::seconds(2)); // cable disconnected, keep looking
        else
        {
            std::cerr << "Unhandled Dashel exception: " << e.what() << std::endl;
            return 1;
        }
    }
    catch (Aseba::InterruptException& e) {
        std::cerr << " attempting graceful network shutdown" << std::endl;
    }
    //std::this_thread::sleep_for (std::chrono::seconds(10));

    return 0;
}
