/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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
#include "../common/consts.h"
#include "../msg/descriptions-manager.h"
#include <QString>
#include <QDialog>
#include <QQueue>
#include <QTimer>
#include <QThread>
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
		
	public slots:
		void netGroupChecked();
		void serialGroupChecked();
		void customGroupChecked();
		void setupOkStateFromListSelection();
	};
	
	class Message;
	class UserMessage;
	
	class DashelInterface: public QThread, public Dashel::Hub
	{
		Q_OBJECT
		
	public:
		Dashel::Stream* stream;
		QString language;
		
	public:
		DashelInterface(QVector<QTranslator*> translators, const QString& commandLineTarget);
		
	signals:
		void messageAvailable(Message *message);
		void dashelDisconnection();
	
	protected:
		// from QThread
		virtual void run();
		
		// from Dashel::Hub
		virtual void incomingData(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
	};
	
	//! Provides a signal/slot interface for the description manager
	class SignalingDescriptionsManager: public QObject, public DescriptionsManager
	{
		Q_OBJECT
	
	signals:
		void nodeDescriptionReceivedSignal(unsigned nodeId);
	
	protected:
		virtual void nodeProtocolVersionMismatch(const std::string &nodeName, uint16 protocolVersion);
		virtual void nodeDescriptionReceived(unsigned nodeId);
	};
	
	class DashelTarget: public Target
	{
		Q_OBJECT
		
	protected:
		struct Node
		{
			Node();
			
			BytecodeVector debugBytecode; //!< bytecode with debug information
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
		SignalingDescriptionsManager descriptionManager;
		NodesMap nodes;
		QTimer userEventsTimer;
		
	public:
		friend class InvasivePlugin;
		DashelTarget(QVector<QTranslator*> translators, const QString& commandLineTarget);
		~DashelTarget();
		
		virtual QString getLanguage() const { return dashelInterface.language; }
		
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
	
	protected slots:
		void updateUserEvents();
		void messageFromDashel(Message *message);
		void disconnectionFromDashel();
		void nodeDescriptionReceived(unsigned node);
	
	protected:
		void receivedDescription(Message *message);
		void receivedLocalEventDescription(Message *message);
		void receivedNativeFunctionDescription(Message *message);
		void receivedDisconnected(Message *message);
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
}; // Aseba

#endif
