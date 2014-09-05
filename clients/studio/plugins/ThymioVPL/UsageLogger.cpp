#include "UsageLogger.h"
#include "UsageProfile.pb.h"

using namespace std;

namespace Aseba{ namespace ThymioVPL
{

#if defined(PROTOBUF_FOUND)


UsageLogger::UsageLogger()
{
	fileOut = new ofstream("test.log", ios::app | ios::binary);
}

UsageLogger::~UsageLogger()
{
	if(fileOut != 0){
		fileOut->close();
		delete fileOut;
	}
}

UsageLogger& UsageLogger::getLogger()
{
	static UsageLogger instance;
	return instance;
}

void UsageLogger::logInsertSet(int row)
{
	
}

void UsageLogger::storeAction(Action & action){
	int size = action.ByteSize();
	(*fileOut) << size;
	action.SerializeToOstream(fileOut);
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


