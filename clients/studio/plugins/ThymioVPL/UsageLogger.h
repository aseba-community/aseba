#ifndef VPL_USAGE_LOGGER_H
#define VPL_USAGE_LOGGER_H

namespace Aseba { namespace ThymioVPL
{

class UsageLogger
{
public:
	static UsageLogger & getLogger();
	
private:
	UsageLogger();
	~UsageLogger();
	
protected:
static UsageLogger * instance;
	

};

}}

#endif // VPL_USAGE_LOGGER_H
