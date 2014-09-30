#include "LogSignalMapper.h"

namespace Aseba { namespace ThymioVPL
{

LogSignalMapper::LogSignalMapper(){
}

LogSignalMapper::~LogSignalMapper(){
	
}

void LogSignalMapper::setMapping(const QObject *sender, unsigned int senderId, QObject * logicalParent){
	senderHash.insert(sender,SenderInfo(senderId,logicalParent));
}


void LogSignalMapper::map() { map(sender()); }

void LogSignalMapper::map(QObject *sender)
{
    if (senderHash.contains(sender))
	{
		SenderInfo p = senderHash.value(sender);
		emit mapped(p.first, sender, p.second);
	}
        
    
}

}}