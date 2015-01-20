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
    
    ScratchInterface::ScratchInterface(const std::string& target, const std::string& http_port,
                     const int iterations) :
    Aseba::HttpInterface(target, http_port, iterations), // use base class constructor
    blink_state(0),
    scratch_dial(0),
    leds(3,0)
    {
        verbose=false;
    };
    
    void ScratchInterface::incomingVariables(const Variables *variables)
    {
        variable_cache[VariableAddress(variables->source,variables->start)] = std::vector<short>(variables->variables);
        HttpInterface::incomingVariables(variables); // then use base class method
    }
    
    void ScratchInterface::incomingUserMsg(const UserMessage *userMsg)
    {
        if (commonDefinitions.events[userMsg->type].name.find(L"Q_motion_ended")==0 &&
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
        }
        HttpInterface::incomingUserMsg(userMsg);
    }

    void ScratchInterface::routeRequest(HttpRequest* req)
    {
        if (req->tokens[0].find("poll")==0)
        {   // poll for variables
            strings args;
            //args.push_back("thymio-II");
            args.push_back("V_leds_temperature");
            args.push_back(std::to_string(abs( (int)(blink_state = (blink_state+1)%240)/12 - 10 )));
            args.push_back(std::to_string(abs( (int)(blink_state = (blink_state+121)%240)/12 - 10 )));
            sendSetVariable(req->tokens[1], args);
            string result = evPoll("thymio-II");
            finishResponse(req, 200, result);
            return;
        }
        if (req->tokens[0].find("nodes")==0 && req->tokens[1].find("thymio-II")==0)
        {
            if (req->tokens[2].find("scratch_start")==0)
            { //
                strings args;
                args.push_back("motor.left.target");
                args.push_back(std::to_string(stoi(req->tokens[3])*32/10));
                sendSetVariable(req->tokens[1], args);
                args.at(0) = "motor.right.target";
                args.at(1) = std::to_string(stoi(req->tokens[4])*32/10);
                sendSetVariable(req->tokens[1], args);
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_change_speed")==0)
            { //
                std::vector<short> left_target, right_target;
                if (getCachedVal("thymio-II", "motor.left.target", left_target) &&
                    getCachedVal("thymio-II", "motor.right.target", right_target))
                {
                    strings args;
                    args.push_back("motor.left.target");
                    args.push_back(std::to_string(stoi(req->tokens[3])*32/10 + left_target.front()));
                    sendSetVariable(req->tokens[1], args);
                    args.at(0) = "motor.right.target";
                    args.at(1) = std::to_string(stoi(req->tokens[3])*32/10 + right_target.front());
                    sendSetVariable(req->tokens[1], args);
                    finishResponse(req, 200, "");
                }
                return;
            }
            else if (req->tokens[2].find("scratch_stop")==0)
            { //
                strings args;
                args.push_back("motor.left.target");
                args.push_back("0");
                sendSetVariable(req->tokens[1], args);
                args.at(0) = "motor.right.target";
                sendSetVariable(req->tokens[1], args);
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_set_dial")==0)
            { //
                scratch_dial = stoi(req->tokens[3]);
                sendEvent(req->tokens[1], makeLedsCircleVector(scratch_dial));
                finishResponse(req, 200, "");
            }
            else if (req->tokens[2].find("scratch_next_dial_limit")==0)
            { //
                unsigned limit = stoi(req->tokens[3]) % 648;
                scratch_dial = (scratch_dial + 1) % limit;
                sendEvent(req->tokens[1], makeLedsCircleVector(scratch_dial));
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_next_dial")==0)
            { //
                unsigned limit = 8;
                scratch_dial = (scratch_dial + 1) % limit;
                sendEvent(req->tokens[1], makeLedsCircleVector(scratch_dial));
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_clear_leds")==0)
            { //
                strings args;
                std::vector<unsigned> v8(8,0);
                args.push_back("V_leds_circle");
                append_strings_unsigned_vector(v8, args);
                sendEvent(req->tokens[1], args);

                std::vector<unsigned> v3(3,0);
                args.clear();
                args.push_back("V_leds_top");
                append_strings_unsigned_vector(v3, args);
                sendEvent(req->tokens[1], args);
                leds[0] = 200;

                std::vector<unsigned> v4(4,0);
                args.clear();
                args.push_back("V_leds_buttons");
                append_strings_unsigned_vector(v4, args);
                sendEvent(req->tokens[1], args);
                args.at(0) = "V_leds_bottom";
                sendEvent(req->tokens[1], args);
                args.at(1) = "1";
                sendEvent(req->tokens[1], args);
                leds[1] = 200;
                leds[2] = 200;

                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_set_leds")==0)
            { //
                unsigned color = stoi(req->tokens[3]) % 198;
                unsigned mask = stoi(req->tokens[4]) & 7;
                strings args = makeLedsRGBVector(color); // by default, "V_leds_top"
                if ((mask & 1) == 1)
                    leds[0]=color, sendEvent(req->tokens[1], args);
                args.insert(args.begin()+1,"0");
                if ((mask & 2) == 2)
                    args.at(0) = "V_leds_bottom", leds[1]=color, sendEvent(req->tokens[1], args);
                args.at(1) = "1";
                if ((mask & 4) == 4)
                    args.at(0) = "V_leds_bottom", leds[2]=color, sendEvent(req->tokens[1], args);
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_change_leds")==0)
            { //
                unsigned delta = stoi(req->tokens[3]) % 198;
                unsigned mask = stoi(req->tokens[4]) & 7;
                if ((mask & 1) == 1)
                {
                    strings args = makeLedsRGBVector(leds[0] = (leds[0] + delta) % 198);
                    sendEvent(req->tokens[1], args);
                }
                if ((mask & 2) == 2)
                {
                    strings args = makeLedsRGBVector(leds[1] = (leds[1] + delta) % 198);
                    args.at(0) = "V_leds_bottom";
                    args.insert(args.begin()+1,"0");
                    sendEvent(req->tokens[1], args);
                }
                if ((mask & 4) == 4)
                {
                    strings args = makeLedsRGBVector(leds[2] = (leds[2] + delta) % 198);
                    args.at(0) = "V_leds_bottom";
                    args.insert(args.begin()+1,"1");
                    sendEvent(req->tokens[1], args);
                }
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_avoid")==0)
            { //
                std::vector<short> dist_front, angle_front, left_target, right_target;
                for (int i = 0; i < 3; i++)
                {
                    if (getCachedVal("thymio-II", "distance.front", dist_front) &&
                        getCachedVal("thymio-II", "angle.front", angle_front) &&
                        getCachedVal("thymio-II", "motor.left.target", left_target) &&
                        getCachedVal("thymio-II", "motor.right.target", right_target))
                    {
                        int vL = ((-1520 + 12*dist_front.front() + 400*angle_front.front()) * left_target.front()) / 760;
                        int vR = ((-1520 + 12*dist_front.front() - 400*angle_front.front()) * right_target.front()) / 760;
                        if (abs(vL-vR) < 20)
                            vL += (rand() % 33) - 16;
                        strings args;
                        args.push_back("motor.left.target");
                        args.push_back(std::to_string(vL));
                        sendSetVariable(req->tokens[1], args);
                        args.at(0) = "motor.right.target";
                        args.at(1) = std::to_string(vR);
                        sendSetVariable(req->tokens[1], args);
                    }
                    sendPollVariables("thymio-II");
                    sendPollVariables("thymio-II");
                }
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_move")==0)
            { //
                int busyid = stoi(req->tokens[3]);
                int mm = stoi(req->tokens[4]);
                int speed = abs(mm) < 20 ? 20 : (abs(mm) > 150 ? 150 : abs(mm));
                int time = abs(mm) * 100 / speed; // time measured in 16 Hz ticks
                speed = speed * 32 / 10;
                strings args;
                args.push_back("Q_add_motion");
                args.push_back(std::to_string(busyid));
                args.push_back(std::to_string(time));
                args.push_back(std::to_string((mm > 0) ? speed : -speed ));
                args.push_back(std::to_string((mm > 0) ? speed : -speed ));
                sendEvent(req->tokens[1], args);
                busy_threads.insert(busyid);
                finishResponse(req, 200, "");
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
                }
                strings args;
                args.push_back("Q_add_motion");
                args.push_back(std::to_string(busyid));
                args.push_back(std::to_string(time));
                args.push_back(std::to_string((degrees > 0) ?  speed : -speed ));
                args.push_back(std::to_string((degrees > 0) ? -speed :  speed ));
                sendEvent(req->tokens[1], args);
                busy_threads.insert(busyid);
                finishResponse(req, 200, "");
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
                sendEvent(req->tokens[1], args);
                busy_threads.insert(busyid);
                finishResponse(req, 200, "");
                return;
            }
            else if (req->tokens[2].find("scratch_sound_freq")==0)
            { // TODO: need to set timer!
//                int busyid = stoi(req->tokens[3]);
//                int freq = stoi(req->tokens[4]);
//                int sixtieths = stoi(req->tokens[5]);
                finishResponse(req, 200, "");
                return;
            }
        }
        HttpInterface::routeRequest(req); // else use base class method
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
    bool ScratchInterface::getCachedVal(const std::string& nodeName, const std::string& varName,
                      std::vector<short>& cachedval)
    {
        unsigned source, pos, len;
        if (getVarAddrLen(nodeName, varName, source, pos, len))
        {
            cachedval = variable_cache[std::make_pair(source, pos)];
            return ! cachedval.empty();
        }
        return false;
    }
    bool ScratchInterface::getVarAddrLen(const std::string& nodeName, const std::string& varName,
                                         unsigned& source, unsigned& pos, unsigned& length)
    {
        if ( ! getNodeAndVarPos(nodeName, varName, source, pos) )
            return false;

        VariablesMap vm = allVariables[nodeName];
        VariablesMap::iterator it = vm.find(UTF8ToWString(varName));
        if (it == vm.end())
            return false;
        
        length = it->second.second; // *it is < varName, < pos, len > >
        return true;
    }
    string ScratchInterface::evPoll(const std::string nodeName)
    {
        sendPollVariables(nodeName);
        bool ok;
        nodeId = getNodeId(UTF8ToWString(nodeName), 0, &ok);
        std::stringstream result;
        char wbuf[128];
        
        result << "_busy";
        for (std::set<int>::iterator i = busy_threads.begin(); i != busy_threads.end(); i++)
            result << " " << *i;
        result << endl;
        
        for(VariablesMap::iterator it = allVariables[nodeName].begin(); it != allVariables[nodeName].end(); ++it)
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
        
        std::vector<short> cv;
        result << "b_button/center " << (getCachedVal("thymio-II", "button.center", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/forward " << (getCachedVal("thymio-II", "button.forward", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/backward " << (getCachedVal("thymio-II", "button.backward", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/left " << (getCachedVal("thymio-II", "button.left", cv) && cv.front() ? "true" : "false") << endl;
        result << "b_button/right " << (getCachedVal("thymio-II", "button.right", cv) && cv.front() ? "true" : "false") << endl;

        if (getCachedVal("thymio-II", "prox.horizontal", cv))
        {
            result << "b_touching/front " << ((cv[0]+cv[1]+cv[2]+cv[3]+cv[4])/1000 > 0 ? "true" : "false") << endl;
            result << "b_touching/back " << ((cv[5]+cv[6])/1000 > 0 ? "true" : "false") << endl;
        }
        if (getCachedVal("thymio-II", "prox.ground.delta", cv))
        {
            result << "b_touching/ground " << ((cv[0]+cv[1])/500 > 0 ? "true" : "false") << endl;
        }
        
        if (getCachedVal("thymio-II", "distance.front", cv))
            result << "distance/front " << clamp(cv.front(),(short)0,(short)190) << endl;
        if (getCachedVal("thymio-II", "distance.back", cv))
            result << "distance/back " << clamp(cv.front(),(short)0,(short)120) << endl;
        if (getCachedVal("thymio-II", "prox.ground.delta", cv))
            result << "distance/ground " << (cv[0]+cv[1] > 1000 ? 0 : 500) << endl;
        
        if (getCachedVal("thymio-II", "angle.front", cv))
            result << "angle/front " << cv.front() << endl;

        if (getCachedVal("thymio-II", "odo.theta", cv))
            result << "odo/direction " << 90 - cv.front()/182 << endl;
        if (getCachedVal("thymio-II", "odo.x", cv))
            result << "odo/x " << cv.front()/28 << endl;
        if (getCachedVal("thymio-II", "odo.y", cv))
            result << "odo/y " << cv.front()/28 << endl;

        if (getCachedVal("thymio-II", "motor.left.speed", cv))
            result << "motor.speed/left " << cv.front()  *10/32 << endl;
        if (getCachedVal("thymio-II", "motor.right.speed", cv))
            result << "motor.speed/right " << cv.front() *10/32 << endl;
        if (getCachedVal("thymio-II", "motor.left.target", cv))
            result << "motor.target/left " << cv.front() *10/32 << endl;
        if (getCachedVal("thymio-II", "motor.right.target", cv))
            result << "motor.target/right " << cv.front()*10/32 << endl;
        
        if (getCachedVal("thymio-II", "acc", cv))
        {
            result << "tilt/right_left " << cv[0] << endl;
            result << "tilt/front_back " << cv[1] << endl;
            result << "tilt/top_bottom " << cv[2] << endl;
        }

        result << "leds/top " << leds[0] << endl;
        result << "leds/bottom/left " << leds[1] << endl;
        result << "leds/bottom/right " << leds[2] << endl;
        
        std::vector<short> mic_threshold, mic_intensity;
        if (getCachedVal("thymio-II", "mic.threshold", mic_threshold) &&
            getCachedVal("thymio-II", "mic.intensity", mic_intensity))
            result << "b_clap " << ((mic_intensity.front() >= mic_threshold.front()) ? "true" : "false") << endl;

        //cerr << "poll result " << result.str() << endl;
        return result.str();
    }
    
    void ScratchInterface::sendPollVariables(const std::string nodeName)
    {
        if (polled_variables.size() == 0)
        {
            unsigned source, pos, len;
            if (getVarAddrLen(nodeName, "button.center", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "button.forward", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "button.backward", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "button.left", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "button.right", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "distance.front", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "distance.back", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "angle.front", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "odo.delta", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "odo.theta", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "odo.x", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "odo.y", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "motor.left.speed", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "motor.right.speed", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "motor.left.target", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "motor.right.target", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "prox.horizontal", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "prox.ground.delta", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "mic.threshold", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "mic.intensity", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "prox.comm.rx", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "prox.comm.tx", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "acc", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
            if (getVarAddrLen(nodeName, "temperature", source, pos, len))
                polled_variables.push_back(GetVariables(source, pos, len));
        }
        std::list<GetVariables>::iterator pvi = polled_variables.begin();
        if (verbose)
            cerr << "poll ";
        int window = polled_variables.size() <= 50 ? polled_variables.size() : polled_variables.size()/3;
        for (int i = 0; i < window; i++, pvi++)
        {
            GetVariables getVariables(*pvi);
            getVariables.serialize(asebaStream);
            if (verbose)
                cerr << "1," << pvi->start << "(" << pvi->length << ") ";
        }
        if (verbose)
            cerr << endl;
        asebaStream->flush();
        std::rotate(polled_variables.begin(), pvi, polled_variables.end());
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
    std::string aesl_filename;
    std::string dashel_target;
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
        else if ((strcmp(arg, "-p") == 0) || (strcmp(arg, "--port") == 0))
            http_port = argv[argCounter++];
        else if ((strcmp(arg, "-a") == 0) || (strcmp(arg, "--aesl") == 0))
            aesl_filename = argv[argCounter++];
        else if ((strcmp(arg, "-K") == 0) || (strcmp(arg, "--Kiter") == 0))
            Kiterations = atoi(argv[argCounter++]);
        else if (strncmp(arg, "-", 1) != 0)
            dashel_target = arg;
    }
    
    // initialize Dashel plugins
    Dashel::initPlugins();
    
    // create and run bridge, catch Dashel exceptions
    do {
        try
        {
            Aseba::ScratchInterface* network(new Aseba::ScratchInterface(dashel_target, http_port, 1000*Kiterations));
            
            if (aesl_filename.size() > 0)
            {
                while (! network->descriptionReceived())
                {
                    network->broadcastGetDescription();
                    network->step(20); // wait for description, variables, etc
                }
                network->aeslLoadFile(aesl_filename);
            }
            
            network->run();
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
    } while (true);

    return 0;
}
