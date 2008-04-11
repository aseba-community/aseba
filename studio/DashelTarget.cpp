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

namespace Aseba
{
	using std::copy;
	using namespace Dashel;
	
	//! Asserts a dynamic cast.	Similar to the one in boost/cast.hpp
	template<typename Derived, typename Base>
	inline Derived polymorphic_downcast(Base base)
	{
		Derived derived = dynamic_cast<Derived>(base);
		assert(derived);
		return derived;
	}

	/** \addtogroup studio */
	/*@{*/
	
	DashelConnectionDialog::DashelConnectionDialog()
	{
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		
		netGroupBox = new QGroupBox("Network (TCP)");
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
		
		serialGroupBox = new QGroupBox("Serial");
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
		
		customGroupBox = new QGroupBox("Custom");
		customGroupBox->setCheckable(true);
		customGroupBox->setChecked(false);
		QHBoxLayout* customLayout = new QHBoxLayout();
		custom = new QLineEdit(ASEBA_DEFAULT_TARGET);
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
		
		setWindowTitle("Aseba Target Selection");
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
	
	enum InNextState
	{
		NOT_IN_NEXT,
		WAITING_INITAL_PC,
		WAITING_LINE_CHANGE
	};
	
	DashelTarget::Node::Node()
	{
		steppingInNext = NOT_IN_NEXT;
		executionMode = EXECUTION_UNKNOWN;
	}
	
	DashelTarget::DashelTarget() :
		stream(0)
	{
		messagesHandlersMap[ASEBA_MESSAGE_DESCRIPTION] = &Aseba::DashelTarget::receivedDescription;
		messagesHandlersMap[ASEBA_MESSAGE_DISCONNECTED] = &Aseba::DashelTarget::receivedDisconnected;
		messagesHandlersMap[ASEBA_MESSAGE_VARIABLES] = &Aseba::DashelTarget::receivedVariables;
		messagesHandlersMap[ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS] = &Aseba::DashelTarget::receivedArrayAccessOutOfBounds;
		messagesHandlersMap[ASEBA_MESSAGE_DIVISION_BY_ZERO] = &Aseba::DashelTarget::receivedDivisionByZero;
		messagesHandlersMap[ASEBA_MESSAGE_EXECUTION_STATE_CHANGED] = &Aseba::DashelTarget::receivedExecutionStateChanged;
		messagesHandlersMap[ASEBA_MESSAGE_BREAKPOINT_SET_RESULT] =
		&Aseba::DashelTarget::receivedBreakpointSetResult;
		
		DashelConnectionDialog targetSelector;
		while (true)
		{
			if (targetSelector.exec() == QDialog::Rejected)
				exit(0);
			
			try
			{
				qDebug() << "Connecting to " << targetSelector.getTarget().c_str();
				stream = Hub::connect(targetSelector.getTarget());
				break;
			}
			catch (DashelException e)
			{
				
				// exception, try again
			}
		}
		
		// Send presence query
		GetDescription().serialize(stream);
		stream->flush();
		netTimer = startTimer(20);
		quitting = false;
	}
	
	DashelTarget::~DashelTarget()
	{
		quitting = true;
		killTimer(netTimer);
		disconnect();
		#ifdef WIN32
		Sleep(200);
		#else
		usleep(200000);
		#endif
		step(0);
	}
		
	void DashelTarget::disconnect()
	{
		// detach all nodes
		for (NodesMap::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			//DetachDebugger(node->first).serialize(stream);
			BreakpointClearAll(node->first).serialize(stream);
			Run(node->first).serialize(stream);
			stream->flush();
		}
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
		
		SetBytecode setBytecodeMessage;
		setBytecodeMessage.dest = node;
		setBytecodeMessage.bytecode.resize(bytecode.size());
		copy(bytecode.begin(), bytecode.end(), setBytecodeMessage.bytecode.begin());
		
		setBytecodeMessage.serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::sendEvent(unsigned id, const VariablesDataVector &data)
	{
		UserMessage(id, data).serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::setVariables(unsigned node, unsigned start, const VariablesDataVector &data)
	{
		SetVariables(node, start, data).serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::getVariables(unsigned node, unsigned start, unsigned length)
	{
		GetVariables(node, start, length).serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::reset(unsigned node)
	{
		Reset(node).serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::run(unsigned node)
	{
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		if (nodeIt->second.executionMode == EXECUTION_STEP_BY_STEP)
			Step(node).serialize(stream);
		Run(node).serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::pause(unsigned node)
	{
		Pause(node).serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::next(unsigned node)
	{
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		nodeIt->second.steppingInNext = WAITING_INITAL_PC;
		
		GetExecutionState getExecutionStateMessage;
		getExecutionStateMessage.dest = node;
		
		getExecutionStateMessage.serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::stop(unsigned node)
	{
		Stop(node).serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::setBreakpoint(unsigned node, unsigned line)
	{
		int pc = getPCFromLine(node, line);
		if (pc >= 0)
		{
			BreakpointSet breakpointSetMessage;
			breakpointSetMessage.pc = pc;
			breakpointSetMessage.dest = node;
			
			breakpointSetMessage.serialize(stream);
			stream->flush();
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
			
			breakpointClearMessage.serialize(stream);
			stream->flush();
		}
	}
	
	void DashelTarget::clearBreakpoints(unsigned node)
	{
		BreakpointClearAll breakpointClearAllMessage;
		breakpointClearAllMessage.dest = node;
		
		breakpointClearAllMessage.serialize(stream);
		stream->flush();
	}
	
	void DashelTarget::timerEvent(QTimerEvent *event)
	{
		Q_UNUSED(event);
		step(0);
	}
	
	void DashelTarget::incomingData(Stream *stream)
	{
		Message *message = Message::receive(stream);
		message->dump(std::cout);
		std::cout << std::endl;
		
		if (!quitting)
		{
			MessagesHandlersMap::const_iterator messageHandler = messagesHandlersMap.find(message->type);
			if (messageHandler == messagesHandlersMap.end())
			{
				UserMessage *userMessage = dynamic_cast<UserMessage *>(message);
				if (userMessage)
					emit userEvent(userMessage->type, userMessage->data);
			}
			else
			{
				(this->*(messageHandler->second))(message);
			}
		}
		delete message;
	}
	
	void DashelTarget::connectionClosed(Stream* stream, bool abnormal)
	{
		Q_UNUSED(stream);
		emit networkDisconnected();
		nodes.clear();
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
		
		// attach debugger
		// AttachDebugger(id).serialize(stream);
		// flush();
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
			emit arrayAccessOutOfBounds(aa->source, line, aa->index);
	}
	
	void DashelTarget::receivedDivisionByZero(Message *message)
	{
		DivisionByZero *dz = polymorphic_downcast<DivisionByZero *>(message);
		
		int line = getLineFromPC(dz->source, dz->pc);
		if (line >= 0)
			emit divisionByZero(dz->source, line);
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
						
						Step(ess->source).serialize(stream);
						stream->flush();
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
							Step(ess->source).serialize(stream);
							stream->flush();
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
