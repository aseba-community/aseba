/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

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
