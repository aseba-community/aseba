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

#ifndef TCPTARGET_H
#define TCPTARGET_H

#include "Target.h"
#include "../../common/consts.h"
#include "../../common/msg/NodesManager.h"
#include <QString>
#include <QDialog>
#include <QQueue>
#include <QTimer>
#include <QThread>
#include <QTime>
#include <QMap>
#include <QSet>
#include <map>
#include <dashel/dashel.h>

class QPushButton;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QListWidget;
class QComboBox;
class QTranslator;


namespace Dashel
{
	class Stream;
}

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class DashelConnectionDialog : public QDialog
	{
		Q_OBJECT
		
	protected:
		QPushButton* connectButton;
		QGroupBox* netGroupBox;
		QLineEdit* host;
		QSpinBox* port;
		QGroupBox* serialGroupBox;
		QListWidget* serial;
		QGroupBox* customGroupBox;
		QLineEdit* custom;
		QComboBox* languageSelectionBox;
		
	public:
		DashelConnectionDialog();
		std::string getTarget();
		QString getLocaleName();
		
	protected:
		bool updatePortList(const QString& toSelect);
		virtual void timerEvent ( QTimerEvent * event );
		
	protected slots:
		void netGroupChecked();
		void serialGroupChecked();
		void customGroupChecked();
		void setupOkStateFromListSelection();
	};
	
	class Message;
	class UserMessage;
	
	class DashelInterface: public QThread, public Dashel::Hub, public NodesManager
	{
		Q_OBJECT
		
	public:
		bool isRunning;
		Dashel::Stream* stream;
		std::string lastConnectedTarget;
		std::string lastConnectedTargetName;
		QString language;
		
	public:
		DashelInterface(QVector<QTranslator*> translators, const QString& commandLineTarget);
		bool attemptToReconnect();
		
		// from Dashel::Hub
		virtual void stop();
		
	signals:
		void messageAvailable(Message *message);
		void dashelDisconnection();
		void nodeDescriptionReceivedSignal(unsigned nodeId);
		void nodeConnectedSignal(unsigned nodeId);
		void nodeDisconnectedSignal(unsigned nodeId);
	
	protected:
		// from QThread
		virtual void run();
		
		// from Dashel::Hub
		virtual void incomingData(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
		
		// from NodesManager
		virtual void nodeProtocolVersionMismatch(unsigned nodeId, const std::wstring &nodeName, uint16 protocolVersion);
		virtual void nodeDescriptionReceived(unsigned nodeId);
		virtual void nodeConnected(unsigned nodeId);
		virtual void nodeDisconnected(unsigned nodeId);
	public:
		// from NodesManager, now as public as we want DashelTarget to call this method
		virtual void sendMessage(const Message& message);
	};
	
	class DashelTarget: public Target
	{
		Q_OBJECT
		
	protected:
		struct Node
		{
			Node();
			
			BytecodeVector debugBytecode; //!< bytecode with debug information
			BytecodeVector::EventAddressesToIdsMap eventAddressToId; //!< map event start addresses to event identifiers
			unsigned steppingInNext; //!< state of node when in next and stepping
			unsigned lineInNext; //!< line of node to execute when in next and stepping
			ExecutionMode executionMode; //!< last known execution mode if this node
		};
		
		typedef void (DashelTarget::*MessageHandler)(Message *message);
		typedef std::map<unsigned, MessageHandler> MessagesHandlersMap;
		typedef std::map<unsigned, Node> NodesMap;
		
		DashelInterface dashelInterface;
		
		MessagesHandlersMap messagesHandlersMap;
		
		QQueue<UserMessage *> userEventsQueue;
		NodesMap nodes;
		QTimer userEventsTimer;
		// Note: this timer is here rather than in DashelInterface because writeBlocked is here, if wiretBlocked is removed, this timer should be moved
		QTimer listNodesTimer;
		bool writeBlocked; //!< true if write is being blocked by invasive plugins, false if write is allowed
		
	public:
		friend class InvasivePlugin;
		DashelTarget(QVector<QTranslator*> translators, const QString& commandLineTarget);
		~DashelTarget();
		
		virtual QString getLanguage() const { return dashelInterface.language; }
		virtual QList<unsigned> getNodesList() const;
		
		virtual void disconnect();
		
		virtual const TargetDescription * const getDescription(unsigned node) const;
		
		virtual void uploadBytecode(unsigned node, const BytecodeVector &bytecode);
		virtual void writeBytecode(unsigned node);
		virtual void reboot(unsigned node);
		
		virtual void sendEvent(unsigned id, const VariablesDataVector &data);
		
		virtual void setVariables(unsigned node, unsigned start, const VariablesDataVector &data);
		virtual void getVariables(unsigned node, unsigned start, unsigned length);
		
		virtual void reset(unsigned node);
		virtual void run(unsigned node);
		virtual void pause(unsigned node);
		virtual void next(unsigned node);
		virtual void stop(unsigned node);
		
		virtual void setBreakpoint(unsigned node, unsigned line);
		virtual void clearBreakpoint(unsigned node, unsigned line);
		virtual void clearBreakpoints(unsigned node);
	
	protected:
		virtual void blockWrite();
		virtual void unblockWrite();
	
	protected slots:
		void updateUserEvents();
		void listNodes();
		void messageFromDashel(Message *message);
		void disconnectionFromDashel();
		void nodeDescriptionReceived(unsigned node);
	
	protected:
		void receivedDescription(Message *message);
		void receivedLocalEventDescription(Message *message);
		void receivedNativeFunctionDescription(Message *message);
		void receivedVariables(Message *message);
		void receivedArrayAccessOutOfBounds(Message *message);
		void receivedDivisionByZero(Message *message);
		void receivedEventExecutionKilled(Message *message);
		void receivedNodeSpecificError(Message *message);
		void receivedExecutionStateChanged(Message *message);
		void receivedBreakpointSetResult(Message *message);
		void receivedBootloaderAck(Message *message);
		
	protected:
		bool emitNodeConnectedIfDescriptionComplete(unsigned id, const Node& node);
		int getPCFromLine(unsigned node, unsigned line);
		int getLineFromPC(unsigned node, unsigned pc);
	};
	
	/*@}*/
} // namespace Aseba

#endif
