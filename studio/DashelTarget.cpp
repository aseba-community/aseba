/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DashelTarget.h"
#include "../msg/msg.h"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <sstream>
#include <cassert>
#include <QInputDialog>
#include <QtGui>

#include <DashelTarget.moc>

#ifdef WIN32 // for Sleep
#include <windows.h>
#endif

// Asserts a dynamic cast.	Similar to the one in boost/cast.hpp
template<typename Derived, typename Base>
static inline Derived polymorphic_downcast(Base base)
{
	Derived derived = dynamic_cast<Derived>(base);
	if (!derived)
		abort();
	return derived;
}

namespace Aseba
{
	using std::copy;
	using namespace Dashel;

	/** \addtogroup studio */
	/*@{*/
	
	DashelConnectionDialog::DashelConnectionDialog()
	{
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		
		netGroupBox = new QGroupBox(tr("Network (TCP)"));
		netGroupBox->setCheckable(true);
		QGridLayout* netLayout = new QGridLayout;
		netLayout->addWidget(new QLabel(tr("Host")), 0, 0);
		netLayout->addWidget(new QLabel(tr("Port")), 0, 1);
		host = new QLineEdit(ASEBA_DEFAULT_HOST);
		netLayout->addWidget(host, 1, 0);
		port = new QSpinBox();
		port->setMinimum(0);
		port->setMaximum(65535);
		port->setValue(ASEBA_DEFAULT_PORT);
		netLayout->addWidget(port, 1, 1);
		netGroupBox->setLayout(netLayout);
		connect(netGroupBox, SIGNAL(clicked()), SLOT(netGroupChecked()));
		mainLayout->addWidget(netGroupBox);
		
		serialGroupBox = new QGroupBox(tr("Serial"));
		serialGroupBox->setCheckable(true);
		serialGroupBox->setChecked(false);
		QHBoxLayout* serialLayout = new QHBoxLayout();
		serial = new QListWidget();
		typedef std::map<int, std::pair<std::string, std::string> > PortsMap;
		PortsMap ports = SerialPortEnumerator::getPorts();
		for (PortsMap::const_iterator it = ports.begin(); it != ports.end(); ++it)
		{
			QListWidgetItem* item = new QListWidgetItem(QString(it->second.second.c_str()));
			item->setData(Qt::UserRole, QVariant(QString::fromUtf8(it->second.first.c_str())));
			serial->addItem(item);
		//
		}
		serial->setSelectionMode(QAbstractItemView::SingleSelection);
		serialLayout->addWidget(serial);
		connect(serial, SIGNAL(itemSelectionChanged()), SLOT(setupOkStateFromListSelection()));
		serialGroupBox->setLayout(serialLayout);
		connect(serialGroupBox, SIGNAL(clicked()), SLOT(serialGroupChecked()));
		mainLayout->addWidget(serialGroupBox);
		
		customGroupBox = new QGroupBox(tr("Custom"));
		customGroupBox->setCheckable(true);
		customGroupBox->setChecked(false);
		QHBoxLayout* customLayout = new QHBoxLayout();
		custom = new QLineEdit(QSettings("EPFL-LSRO-Mobots", "Aseba Studio").value("custom target", ASEBA_DEFAULT_TARGET).toString());
		customLayout->addWidget(custom);
		customGroupBox->setLayout(customLayout);
		connect(customGroupBox, SIGNAL(clicked()), SLOT(customGroupChecked()));
		mainLayout->addWidget(customGroupBox);
		
		QHBoxLayout* buttonLayout = new QHBoxLayout();
		connectButton = new QPushButton(QIcon(":/images/ok.png"), tr("Connect"));
		connect(connectButton, SIGNAL(clicked(bool)), SLOT(accept()));
		buttonLayout->addWidget(connectButton);
		QPushButton* cancelButton = new QPushButton(QIcon(":/images/no.png"), tr("Cancel"));
		connect(cancelButton, SIGNAL(clicked(bool)), SLOT(reject()));
		buttonLayout->addWidget(cancelButton);
		mainLayout->addLayout(buttonLayout);
		
		setWindowTitle(tr("Aseba Target Selection"));
	}
	
	std::string DashelConnectionDialog::getTarget()
	{
		if (netGroupBox->isChecked())
		{
			std::ostringstream oss;
			oss << "tcp:host=" << host->text().toStdString() << ";port=" << port->value();
			return oss.str();
		}
		else if (serialGroupBox->isChecked())
		{
			QString target("ser:device=%0");
			return target.arg(serial->selectionModel()->selectedRows().first().data(Qt::UserRole).toString()).toStdString();
		}
		else if (customGroupBox->isChecked())
		{
			QSettings("EPFL-LSRO-Mobots", "Aseba Studio").setValue("custom target", custom->text());
			return custom->text().toStdString();
		}
		else
		{
			assert(false);
			return "";
		}
	}
	
	

	void DashelConnectionDialog::netGroupChecked()
	{
		netGroupBox->setChecked(true);
		serialGroupBox->setChecked(false);
		customGroupBox->setChecked(false);
		setupOkStateFromListSelection();
	}
	
	void DashelConnectionDialog::serialGroupChecked()
	{
		netGroupBox->setChecked(false);
		serialGroupBox->setChecked(true);
		customGroupBox->setChecked(false);
		setupOkStateFromListSelection();
	}
	
	void DashelConnectionDialog::customGroupChecked()
	{
		netGroupBox->setChecked(false);
		serialGroupBox->setChecked(false);
		customGroupBox->setChecked(true);
		setupOkStateFromListSelection();
	}
	
	void DashelConnectionDialog::setupOkStateFromListSelection()
	{
		connectButton->setEnabled(serial->selectionModel()->hasSelection() || (!serialGroupBox->isChecked()));
	}
	
	DashelInterface::DashelInterface() :
		stream(0)
	{
		DashelConnectionDialog targetSelector;
		while (true)
		{
			if (targetSelector.exec() == QDialog::Rejected)
				exit(0);
			
			try
			{
				//qDebug() << "Connecting to " << targetSelector.getTarget().c_str();
				stream = Hub::connect(targetSelector.getTarget());
				break;
			}
			catch (DashelException e)
			{
				// exception, try again
			}
		}
	}
	
	// In QThread main function, we just make our Dashel hub switch listen for incoming data
	void DashelInterface::run()
	{
		Dashel::Hub::run();
	}
	
	enum InNextState
	{
		NOT_IN_NEXT,
		WAITING_INITAL_PC,
		WAITING_LINE_CHANGE
	};
	
	DashelTarget::Node::Node()
	{
		executionMode = EXECUTION_UNKNOWN;
	}
	
	DashelTarget::DashelTarget()
	{
		userEventsTimer.setSingleShot(true);
		connect(&userEventsTimer, SIGNAL(timeout()), SLOT(updateUserEvents()));
		// we connect the events from the stream listening thread to slots living in our gui thread
		connect(&dashelInterface, SIGNAL(messageAvailable(Message *)), SLOT(messageFromDashel(Message *)), Qt::QueuedConnection);
		connect(&dashelInterface, SIGNAL(dashelDisconnection()), SLOT(disconnectionFromDashel()), Qt::QueuedConnection);
		
		messagesHandlersMap[ASEBA_MESSAGE_DESCRIPTION] = &Aseba::DashelTarget::receivedDescription;
		messagesHandlersMap[ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION] = &Aseba::DashelTarget::receivedLocalEventDescription;
		messagesHandlersMap[ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION] = &Aseba::DashelTarget::receivedNativeFunctionDescription;
		messagesHandlersMap[ASEBA_MESSAGE_DISCONNECTED] = &Aseba::DashelTarget::receivedDisconnected;
		messagesHandlersMap[ASEBA_MESSAGE_VARIABLES] = &Aseba::DashelTarget::receivedVariables;
		messagesHandlersMap[ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS] = &Aseba::DashelTarget::receivedArrayAccessOutOfBounds;
		messagesHandlersMap[ASEBA_MESSAGE_DIVISION_BY_ZERO] = &Aseba::DashelTarget::receivedDivisionByZero;
		messagesHandlersMap[ASEBA_MESSAGE_EVENT_EXECUTION_KILLED] = &Aseba::DashelTarget::receivedEventExecutionKilled;
		messagesHandlersMap[ASEBA_MESSAGE_NODE_SPECIFIC_ERROR] = &Aseba::DashelTarget::receivedNodeSpecificError;
		messagesHandlersMap[ASEBA_MESSAGE_EXECUTION_STATE_CHANGED] = &Aseba::DashelTarget::receivedExecutionStateChanged;
		messagesHandlersMap[ASEBA_MESSAGE_BREAKPOINT_SET_RESULT] = &Aseba::DashelTarget::receivedBreakpointSetResult;
		
		// Send presence query
		GetDescription().serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
		dashelInterface.start();
	}
	
	DashelTarget::~DashelTarget()
	{
		dashelInterface.stop();
		dashelInterface.wait();
		DashelTarget::disconnect();
	}
	
	void DashelTarget::disconnect()
	{
		// detach all nodes
		for (NodesMap::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			//DetachDebugger(node->first).serialize(dashelInterface.stream);
			BreakpointClearAll(node->first).serialize(dashelInterface.stream);
			Run(node->first).serialize(dashelInterface.stream);
		}
		dashelInterface.stream->flush();
	}
	
	const TargetDescription * const DashelTarget::getConstDescription(unsigned node) const
	{
		NodesMap::const_iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		return &(nodeIt->second.description);
	}
	
	void DashelTarget::uploadBytecode(unsigned node, const BytecodeVector &bytecode)
	{
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		nodeIt->second.debugBytecode = bytecode;
		
		unsigned bytecodePayloadSize = (ASEBA_MAX_PACKET_SIZE - 6) / 2;
		unsigned bytecodeStart = 0;
		unsigned bytecodeCount = bytecode.size();
		
		while (bytecodeCount > bytecodePayloadSize)
		{
			SetBytecode setBytecodeMessage(node, bytecodeStart);
			setBytecodeMessage.bytecode.resize(bytecodePayloadSize);
			copy(bytecode.begin()+bytecodeStart, bytecode.begin()+bytecodeStart+bytecodePayloadSize, setBytecodeMessage.bytecode.begin());
			setBytecodeMessage.serialize(dashelInterface.stream);
			
			bytecodeStart += bytecodePayloadSize;
			bytecodeCount -= bytecodePayloadSize;
		}
		
		{
			SetBytecode setBytecodeMessage(node, bytecodeStart);
			setBytecodeMessage.bytecode.resize(bytecodeCount);
			copy(bytecode.begin()+bytecodeStart, bytecode.end(), setBytecodeMessage.bytecode.begin());
			setBytecodeMessage.serialize(dashelInterface.stream);
		}
		
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::writeBytecode(unsigned node)
	{
		WriteBytecode(node).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::sendEvent(unsigned id, const VariablesDataVector &data)
	{
		UserMessage(id, data).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::setVariables(unsigned node, unsigned start, const VariablesDataVector &data)
	{
		SetVariables(node, start, data).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::getVariables(unsigned node, unsigned start, unsigned length)
	{
		unsigned variablesPayloadSize = (ASEBA_MAX_PACKET_SIZE - 4) / 2;
		while (length > variablesPayloadSize)
		{
			GetVariables(node, start, variablesPayloadSize).serialize(dashelInterface.stream);
			start += variablesPayloadSize;
			length -= variablesPayloadSize;
		}
		GetVariables(node, start, length).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::reset(unsigned node)
	{
		Reset(node).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::run(unsigned node)
	{
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		if (nodeIt->second.executionMode == EXECUTION_STEP_BY_STEP)
			Step(node).serialize(dashelInterface.stream);
		Run(node).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::pause(unsigned node)
	{
		Pause(node).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::next(unsigned node)
	{
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		nodeIt->second.steppingInNext = WAITING_INITAL_PC;
		
		GetExecutionState getExecutionStateMessage;
		getExecutionStateMessage.dest = node;
		
		getExecutionStateMessage.serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::stop(unsigned node)
	{
		Stop(node).serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::setBreakpoint(unsigned node, unsigned line)
	{
		int pc = getPCFromLine(node, line);
		if (pc >= 0)
		{
			BreakpointSet breakpointSetMessage;
			breakpointSetMessage.pc = pc;
			breakpointSetMessage.dest = node;
			
			breakpointSetMessage.serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
	}
	
	void DashelTarget::clearBreakpoint(unsigned node, unsigned line)
	{
		int pc = getPCFromLine(node, line);
		if (pc >= 0)
		{
			BreakpointClear breakpointClearMessage;
			breakpointClearMessage.pc = pc;
			breakpointClearMessage.dest = node;
			
			breakpointClearMessage.serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
	}
	
	void DashelTarget::clearBreakpoints(unsigned node)
	{
		BreakpointClearAll breakpointClearAllMessage;
		breakpointClearAllMessage.dest = node;
		
		breakpointClearAllMessage.serialize(dashelInterface.stream);
		dashelInterface.stream->flush();
	}
	
	void DashelTarget::updateUserEvents()
	{
		// send only 20 latest user events
		if (userEventsQueue.size() > 20)
			emit userEventsDropped(userEventsQueue.size() - 20);
		while (userEventsQueue.size() > 20)
		{
			delete userEventsQueue.head();
			userEventsQueue.dequeue();
		}
		
		while (!userEventsQueue.isEmpty())
		{
			emit userEvent(userEventsQueue.head()->type, userEventsQueue.head()->data);
			userEventsQueue.dequeue();
		}
	}
	
	void DashelInterface::incomingData(Stream *stream)
	{
		Message *message = Message::receive(stream);
		emit messageAvailable(message);
	}
	
	void DashelTarget::messageFromDashel(Message *message)
	{
		bool deleteMessage = true;
		//message->dump(std::cout);
		//std::cout << std::endl;
		
		MessagesHandlersMap::const_iterator messageHandler = messagesHandlersMap.find(message->type);
		if (messageHandler == messagesHandlersMap.end())
		{
			UserMessage *userMessage = dynamic_cast<UserMessage *>(message);
			if (userMessage)
			{
				userEventsQueue.enqueue(userMessage);
				if (!userEventsTimer.isActive())
					userEventsTimer.start(50);
				deleteMessage = false;
			}
			else
				qDebug() << QString("Unknown non user message of type 0x%0 received from %1").arg(message->type, 6).arg(message->source);
		}
		else
		{
			(this->*(messageHandler->second))(message);
		}
		
		if (deleteMessage)
			delete message;
	}
	
	void DashelInterface::connectionClosed(Stream* stream, bool abnormal)
	{
		Q_UNUSED(stream);
		Q_UNUSED(abnormal);
		emit dashelDisconnection();
	}
	
	void DashelTarget::disconnectionFromDashel()
	{
		QMessageBox::critical(0, tr("Connection closed"), tr("Warning, connection closed, save your work and quit Studio."));
		// temporary disabling this dynamism as aseba is still in development
		//emit networkDisconnected();
		//nodes.clear();
	}
	
	void DashelTarget::receivedDescription(Message *message)
	{
		Description *description = polymorphic_downcast<Description *>(message);
		unsigned id = description->source;
		
		// We can receive a description twice, for instance if there is another IDE connected
		if (nodes.find(id) != nodes.end())
			return;
		
		// create node
		Node& node = nodes[id];
		
		// copy description into it
		node.steppingInNext = NOT_IN_NEXT;
		node.lineInNext = 0;
		node.description = *description;
		node.localEventsReceptionCounter = 0;
		node.nativeFunctionReceptionCounter = 0;
		
		emitNodeConnectedIfDescriptionComplete(id, node);
	}
	
	void DashelTarget::receivedLocalEventDescription(Message *message)
	{
		LocalEventDescription *description = polymorphic_downcast<LocalEventDescription *>(message);
		unsigned id = description->source;
		
		// we must have received a description first
		Q_ASSERT(nodes.find(id) != nodes.end());
		Node& node = nodes[id];
		
		// copy description into array if array is empty
		if (node.localEventsReceptionCounter < node.description.localEvents.size())
		{
			node.description.localEvents[node.localEventsReceptionCounter++] = *description;
			
			emitNodeConnectedIfDescriptionComplete(id, node);
		}
	}
	
	void DashelTarget::receivedNativeFunctionDescription(Message *message)
	{
		NativeFunctionDescription *description = polymorphic_downcast<NativeFunctionDescription *>(message);
		unsigned id = description->source;
		
		// we must have received a description first
		Q_ASSERT(nodes.find(id) != nodes.end());
		Node& node = nodes[id];
		
		// copy description into array
		if (node.nativeFunctionReceptionCounter < node.description.nativeFunctions.size())
		{
			node.description.nativeFunctions[node.nativeFunctionReceptionCounter++] = *description;
			
			emitNodeConnectedIfDescriptionComplete(id, node);
		}
	}
	
	bool DashelTarget::emitNodeConnectedIfDescriptionComplete(unsigned id, const Node& node)
	{
		// we will emit the connected signal only when we have received all local events and native functions
		if ((node.localEventsReceptionCounter == node.description.localEvents.size()) &&
			(node.nativeFunctionReceptionCounter == node.description.nativeFunctions.size()))
			emit nodeConnected(id);
	}
	
	void DashelTarget::receivedVariables(Message *message)
	{
		Variables *variables = polymorphic_downcast<Variables *>(message);
		
		emit variablesMemoryChanged(variables->source, variables->start, variables->variables);
	}
	
	void DashelTarget::receivedArrayAccessOutOfBounds(Message *message)
	{
		ArrayAccessOutOfBounds *aa = polymorphic_downcast<ArrayAccessOutOfBounds *>(message);
		
		int line = getLineFromPC(aa->source, aa->pc);
		if (line >= 0)
		{
			emit arrayAccessOutOfBounds(aa->source, line, aa->index);
			emit executionModeChanged(aa->source, EXECUTION_STOP);
		}
	}
	
	void DashelTarget::receivedDivisionByZero(Message *message)
	{
		DivisionByZero *dz = polymorphic_downcast<DivisionByZero *>(message);
		
		int line = getLineFromPC(dz->source, dz->pc);
		if (line >= 0)
		{
			emit divisionByZero(dz->source, line);
			emit executionModeChanged(dz->source, EXECUTION_STOP);
		}
	}
	
	void DashelTarget::receivedEventExecutionKilled(Message *message)
	{
		EventExecutionKilled *eek = polymorphic_downcast<EventExecutionKilled *>(message);
		
		int line = getLineFromPC(eek->source, eek->pc);
		if (line >= 0)
		{
			emit eventExecutionKilled(eek->source, line);
		}
	}
	
	void DashelTarget::receivedNodeSpecificError(Message *message)
	{
		NodeSpecificError *nse = polymorphic_downcast<NodeSpecificError *>(message);
		
		int line = getLineFromPC(nse->source, nse->pc);
		// The NodeSpecificError can be triggered even if the pc is not valid
//		if (line >= 0)
//		{
			emit nodeSpecificError(nse->source, line, QString::fromUtf8(nse->message.c_str()));
			emit executionModeChanged(nse->source, EXECUTION_STOP);
//		}
	}
	
	void DashelTarget::receivedExecutionStateChanged(Message *message)
	{
		ExecutionStateChanged *ess = polymorphic_downcast<ExecutionStateChanged *>(message);
		
		Node &node = nodes[ess->source];
		int line = getLineFromPC(ess->source, ess->pc);
		
		Target::ExecutionMode mode;
		
		if (ess->flags & ASEBA_VM_STEP_BY_STEP_MASK)
		{
			if (ess->flags & ASEBA_VM_EVENT_ACTIVE_MASK)
			{
				mode = EXECUTION_STEP_BY_STEP;
				if (line >= 0)
				{
					// Step by step, manage next
					if (node.steppingInNext == NOT_IN_NEXT)
					{
						emit executionPosChanged(ess->source, line);
						emit executionModeChanged(ess->source, mode);
						emit variablesMemoryEstimatedDirty(ess->source);
					}
					else if (node.steppingInNext == WAITING_INITAL_PC)
					{
						// we have line, do steps now
						node.lineInNext = line;
						node.steppingInNext = WAITING_LINE_CHANGE;
						
						Step(ess->source).serialize(dashelInterface.stream);
						dashelInterface.stream->flush();
					}
					else if (node.steppingInNext == WAITING_LINE_CHANGE)
					{
						if (line != static_cast<int>(node.lineInNext))
						{
							node.steppingInNext = NOT_IN_NEXT;
							emit executionPosChanged(ess->source, line);
							emit executionModeChanged(ess->source, mode);
							emit variablesMemoryEstimatedDirty(ess->source);
						}
						else
						{
							Step(ess->source).serialize(dashelInterface.stream);
							dashelInterface.stream->flush();
						}
					}
					else
						assert(false);
					// we can safely return here, all case that require
					// emitting signals have been handeled
					node.executionMode = mode;
					return;
				}
			}
			else
			{
				mode = EXECUTION_STOP;
			}
		}
		else
		{
			mode = EXECUTION_RUN;
		}
		
		emit executionModeChanged(ess->source, mode);
		if (node.executionMode != mode)
		{
			emit variablesMemoryEstimatedDirty(ess->source);
			node.executionMode = mode;
		}
	}
	
	void DashelTarget::receivedDisconnected(Message *message)
	{
		Disconnected *disconnected = polymorphic_downcast<Disconnected *>(message);
		
		emit nodeDisconnected(disconnected->source);
	}
	
	void DashelTarget::receivedBreakpointSetResult(Message *message)
	{
		BreakpointSetResult *bsr = polymorphic_downcast<BreakpointSetResult *>(message);
		unsigned node = bsr->source;
		emit breakpointSetResult(node, getLineFromPC(node, bsr->pc), bsr->success);
	}
	
	int DashelTarget::getPCFromLine(unsigned node, unsigned line)
	{
		// first lookup node
		NodesMap::const_iterator nodeIt = nodes.find(node);
		
		if (nodeIt == nodes.end())
			return -1;
		
		// then find PC
		for (size_t i = 0; i < nodeIt->second.debugBytecode.size(); i++)
			if (nodeIt->second.debugBytecode[i].line == line)
				return i;
		
		return -1;
	}
	
	int DashelTarget::getLineFromPC(unsigned node, unsigned pc)
	{
		// first lookup node
		NodesMap::const_iterator nodeIt = nodes.find(node);
		
		if (nodeIt == nodes.end())
			return -1;
		
		// then get line
		if (pc < nodeIt->second.debugBytecode.size())
			return nodeIt->second.debugBytecode[pc].line;
		
		return -1;
	}
	
	/*@}*/
}
