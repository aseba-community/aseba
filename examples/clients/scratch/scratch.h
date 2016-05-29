#include <cstdlib>
#include <cstring>
#include <iostream>

#include "../../../switches/http/http.h"
#include "../../../common/consts.h"
#include "../../../common/types.h"
#include "../../../common/utils/utils.h"
#include "../../../transport/dashel_plugins/dashel-plugins.h"

namespace Aseba
{
    //! Scratch HTTP interface for aseba network
    class ScratchInterface:  public Aseba::HttpInterface
    {
    public:
        typedef std::vector<std::string>      strings;
        
    protected:
        //variable cache
        //std::map<std::pair<unsigned,unsigned>, std::vector<short> > variable_cache;
        std::map<VariableAddress, std::vector<short> > variable_cache;
        std::map<std::string, VariableAddress> state_variable_addresses;
        std::list<GetVariables> polled_variables;
        UnifiedTime state_variable_update_time;
        std::set<int> busy_threads;
        std::map<int, UnifiedTime> busy_threads_timeout;
        std::map<unsigned, unsigned> blink_state;
        std::map<unsigned, unsigned> scratch_dial;
        std::map< std::pair<unsigned,unsigned>, unsigned > leds; // top, bottom-left, bottom-right
        
    public:
        ScratchInterface(const strings& targets = std::vector<std::string>(), const std::string& http_port="3000", const std::string& aseba_port="33332", const int iterations=-1, bool dump=false);
        virtual std::string evPoll(const unsigned nodeId);
        virtual void evNodes(HttpRequest* req, strings& args);
        
    protected:
        virtual void routeRequest(HttpRequest* req);
        virtual void incomingVariables(const Variables *variables);
        virtual void incomingUserMsg(const UserMessage *userMsg);
        virtual void sendPollVariables(const unsigned nodeId);
        virtual void sendSetVariable(const unsigned nodeId, const strings& args);
        virtual bool getCachedVal(const unsigned nodeId, const std::string& varName,
                                  std::vector<short>& cachedval);
        virtual bool setCachedVal(const unsigned nodeId, const std::string& varName,
                                  std::vector<short>& val);
        virtual bool getVarAddrLen(const unsigned nodeId, const std::string& varName,
                                   unsigned& source, unsigned& pos, unsigned& length);
        virtual strings makeLedsCircleVector(unsigned dial);
        virtual strings makeLedsRGBVector(unsigned color);
        virtual void receiveStateVariables(const unsigned nodeId, const std::vector<short> data);
        
    protected:
        const std::map<std::string, std::pair<unsigned, unsigned> > interesting_state_variables =
        std::map<std::string, std::pair<unsigned, unsigned> >({
            {"acc",{0,1}}, {"button.backward",{1,2}}, {"button.center",{1,2}},
            {"button.forward",{1,2}}, {"button.left",{1,2}}, {"button.right",{1,2}},
            {"mic.intensity",{1,2}},
            {"angle.back",{2,3}}, {"angle.front",{3,4}},
            {"distance.back",{4,5}}, {"distance.front",{4,5}}, {"motor.left.target",{5,6}},
            {"motor.right.target",{6,7}}, {"motor.left.speed",{7,8}}, {"motor.right.speed",{8,9}},
            {"odo.degree",{9,10}}, {"odo.x",{10,11}}, {"odo.y",{11,12}}, {"prox.comm.rx",{12,13}},
            {"prox.comm.tx",{13,14}}, {"prox.ground.delta",{14,16}}, {"prox.horizontal",{16,23}},
            {"Qid",{23,27}} });
        std::map<unsigned, GetVariables> node_r_state;
        const std::map<std::wstring, unsigned> standard_scratch_events =
        std::map<std::wstring, unsigned>({
            {L"scratch_start",2}, {L"scratch_change_speed",2}, {L"scratch_stop",0},
            {L"scratch_set_dial",1}, {L"scratch_next_dial_limit",1}, {L"scratch_next_dial",0},
            {L"scratch_clear_leds",0}, {L"scratch_set_leds",2}, {L"scratch_change_leds",2},
            {L"scratch_move",2}, {L"scratch_turn",2}, {L"scratch_arc",3} });
    };
};