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
        time_t state_variable_update_time;
        std::set<int> busy_threads;
        std::map<unsigned, unsigned> blink_state;
        std::map<unsigned, unsigned> scratch_dial;
        std::map< std::pair<unsigned,unsigned>, unsigned > leds; // top, bottom-left, bottom-right
        
    public:
        ScratchInterface(const strings& targets = std::vector<std::string>(), const std::string& http_port="3000", const std::string& dashel_port="33332", const int iterations=-1);
        virtual std::string evPoll(const unsigned nodeId);
        
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
        virtual void receiveStateVariables(const UserMessage *userMsg);
    };
};