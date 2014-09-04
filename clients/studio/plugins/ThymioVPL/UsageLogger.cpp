#include "UsageLogger.h"

namespace Aseba{ namespace ThymioVPL
{

#if defined(PROTOBUF_FOUND)
UsageLogger::UsageLogger()
{
}

UsageLogger::~UsageLogger()
{
}

UsageLogger& UsageLogger::getLogger()
{
	static UsageLogger instance;
	return instance;
}



#else
// Provide dummy implementation in case no usage logging is desired.
UsageLogger::UsageLogger()
{
}

UsageLogger::~UsageLogger()
{
}

UsageLogger& UsageLogger::getLogger()
{
	static UsageLogger instance;
	return instance;
}

	
#endif //#ifdef PROTOBUF_FOUND


}
}

