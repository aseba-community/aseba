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

#ifndef LOG_SIGNAL_MAPPER_H
#define LOG_SIGNAL_MAPPER_H

#include <QObject>
#include <QPair>
#include <QHash>

namespace Aseba { namespace ThymioVPL
{

typedef QPair<unsigned int, QObject *> SenderInfo;

class LogSignalMapper : public QObject{
	Q_OBJECT
	
signals:
	void mapped(unsigned int senderId, QObject *originalSender, QObject *logicalParent);
	
public slots:
	void map();
	void map(QObject *sender);
	
public:
	LogSignalMapper();
	~LogSignalMapper();
	void setMapping(const QObject *sender, unsigned int senderId, QObject * logicalParent);
	
	private:	
	QHash<const QObject *, SenderInfo > senderHash;
};

}}

#endif
