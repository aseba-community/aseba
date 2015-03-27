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
        std::list<GetVariables> polled_variables;
        std::set<int> busy_threads;
        unsigned blink_state;
        unsigned scratch_dial;
        std::vector<unsigned> leds; // top, bottom-left, bottom-right
        
    public:
        ScratchInterface(const std::string& target, const std::string& http_port, const int iterations);
        virtual std::string evPoll(const std::string nodeName);
        
    protected:
        virtual void routeRequest(HttpRequest* req);
        virtual void incomingVariables(const Variables *variables);
        virtual void incomingUserMsg(const UserMessage *userMsg);
        virtual void sendPollVariables(const std::string nodeName);
        virtual bool getCachedVal(const std::string& nodeName, const std::string& varName,
                                  std::vector<short>& cachedval);
        virtual bool getVarAddrLen(const std::string& nodeName, const std::string& varName,
                                   unsigned& source, unsigned& pos, unsigned& length);
        virtual strings makeLedsCircleVector(unsigned dial);
        virtual strings makeLedsRGBVector(unsigned color);
        virtual void receiveStateVariables(const UserMessage *userMsg);
    };
};