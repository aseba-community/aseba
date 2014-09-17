#ifndef LOG_SIGNAL_MAPPER_H
#define LOG_SIGNAL_MAPPER_H

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