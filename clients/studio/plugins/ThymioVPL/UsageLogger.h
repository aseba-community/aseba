#ifndef VPL_USAGE_LOGGER_H
#define VPL_USAGE_LOGGER_H

#include <iostream>
#include <fstream>


namespace Aseba { namespace ThymioVPL
{

class UsageLogger
{
public:
	static UsageLogger & getLogger();

public:
	void logSave();
	void logInsertSet(int row);
	
private:
	UsageLogger();
	~UsageLogger();
	
protected:
	void storeAction(Action action);
	
	std::ofstream * fileOut;
};

}}

#endif // VPL_USAGE_LOGGER_H
