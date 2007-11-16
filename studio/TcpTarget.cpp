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

#include "TcpTarget.h"
#include "../msg/msg.h"
#include <algorithm>
#include <boost/cast.hpp>
#include<iostream>

using namespace boost;
using std::copy;


namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	enum InNextState
	{
		NOT_IN_NEXT,
		WAITING_INITAL_PC,
		WAITING_LINE_CHANGE
	};
	
	TcpTarget::Node::Node()
	{
		steppingInNext = NOT_IN_NEXT;
		executionMode = EXECUTION_UNKNOWN;
	}
	
	TcpTarget::TcpTarget()
	{
		messagesHandlersMap[ASEBA_MESSAGE_DESCRIPTION] = &Aseba::TcpTarget::receivedDescription;
		messagesHandlersMap[ASEBA_MESSAGE_DISCONNECTED] = &Aseba::TcpTarget::receivedDisconnected;
		messagesHandlersMap[ASEBA_MESSAGE_VARIABLES] = &Aseba::TcpTarget::receivedVariables;
		messagesHandlersMap[ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS] = &Aseba::TcpTarget::receivedArrayAccessOutOfBounds;
		messagesHandlersMap[ASEBA_MESSAGE_DIVISION_BY_ZERO] = &Aseba::TcpTarget::receivedDivisionByZero;
		messagesHandlersMap[ASEBA_MESSAGE_EXECUTION_STATE_CHANGED] = &Aseba::TcpTarget::receivedExecutionStateChanged;
		messagesHandlersMap[ASEBA_MESSAGE_BREAKPOINT_SET_RESULT] =
		&Aseba::TcpTarget::receivedBreakpointSetResult;
	}
	
	TcpTarget::~TcpTarget()
	{
		disconnect();
	}
	
	void TcpTarget::connect()
	{
		// TODO: do a connection box
		
		NetworkClient::connect("localhost", ASEBA_DEFAULT_PORT);
	}
	
	void TcpTarget::disconnect()
	{
		// detach all nodes
		for (NodesMap::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			//DetachDebugger(node->first).serialize(socketDescriptor());
			BreakpointClearAll(node->first).serialize(socketDescriptor());
			Run(node->first).serialize(socketDescriptor());
			flush();
		}
	}
	
	const TargetDescription * const TcpTarget::getConstDescription(unsigned node) const
	{
		NodesMap::const_iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		return &(nodeIt->second.description);
	}
	
	void TcpTarget::uploadBytecode(unsigned node, const BytecodeVector &bytecode)
	{
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		nodeIt->second.debugBytecode = bytecode;
		
		SetBytecode setBytecodeMessage;
		setBytecodeMessage.dest = node;
		setBytecodeMessage.bytecode.resize(bytecode.size());
		copy(bytecode.begin(), bytecode.end(), setBytecodeMessage.bytecode.begin());
		
		setBytecodeMessage.serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::sendEvent(unsigned id, const VariablesDataVector &data)
	{
		UserMessage(id, data).serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::setVariables(unsigned node, unsigned start, const VariablesDataVector &data)
	{
		SetVariables(node, start, data).serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::getVariables(unsigned node, unsigned start, unsigned length)
	{
		GetVariables(node, start, length).serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::reset(unsigned node)
	{
		Reset(node).serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::run(unsigned node)
	{
		Run(node).serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::pause(unsigned node)
	{
		Pause(node).serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::next(unsigned node)
	{
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		nodeIt->second.steppingInNext = WAITING_INITAL_PC;
		
		GetExecutionState getExecutionStateMessage;
		getExecutionStateMessage.dest = node;
		
		getExecutionStateMessage.serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::stop(unsigned node)
	{
		Stop(node).serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::setBreakpoint(unsigned node, unsigned line)
	{
		int pc = getPCFromLine(node, line);
		if (pc >= 0)
		{
			BreakpointSet breakpointSetMessage;
			breakpointSetMessage.pc = pc;
			breakpointSetMessage.dest = node;
			
			breakpointSetMessage.serialize(socketDescriptor());
			flush();
		}
	}
	
	void TcpTarget::clearBreakpoint(unsigned node, unsigned line)
	{
		int pc = getPCFromLine(node, line);
		if (pc >= 0)
		{
			BreakpointClear breakpointClearMessage;
			breakpointClearMessage.pc = pc;
			breakpointClearMessage.dest = node;
			
			breakpointClearMessage.serialize(socketDescriptor());
			flush();
		}
	}
	
	void TcpTarget::clearBreakpoints(unsigned node)
	{
		BreakpointClearAll breakpointClearAllMessage;
		breakpointClearAllMessage.dest = node;
		
		breakpointClearAllMessage.serialize(socketDescriptor());
		flush();
	}
	
	void TcpTarget::timerEvent(QTimerEvent *event)
	{
		Q_UNUSED(event);
		step();
	}
	
	void TcpTarget::connectionEstablished()
	{
		// Send presence query
		Presence().serialize(socketDescriptor());
		flush();
		netTimer = startTimer(20);
	}
	
	void TcpTarget::incomingData()
	{
		Message *message = Message::receive(socketDescriptor());
		
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
		
		delete message;
	}
	
	void TcpTarget::connectionClosed(void)
	{
		emit networkDisconnected();
		nodes.clear();
		killTimer(netTimer);
	}
	
	void TcpTarget::receivedDescription(Message *message)
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
		// AttachDebugger(id).serialize(socketDescriptor());
		// flush();
		emit nodeConnected(id);
	}
	
	void TcpTarget::receivedVariables(Message *message)
	{
		Variables *variables = polymorphic_downcast<Variables *>(message);
		
		emit variablesMemoryChanged(variables->source, variables->start, variables->variables);
	}
	
	void TcpTarget::receivedArrayAccessOutOfBounds(Message *message)
	{
		ArrayAccessOutOfBounds *aa = polymorphic_downcast<ArrayAccessOutOfBounds *>(message);
		
		int line = getLineFromPC(aa->source, aa->pc);
		if (line >= 0)
			emit arrayAccessOutOfBounds(aa->source, line, aa->index);
	}
	
	void TcpTarget::receivedDivisionByZero(Message *message)
	{
		DivisionByZero *dz = polymorphic_downcast<DivisionByZero *>(message);
		
		int line = getLineFromPC(dz->source, dz->pc);
		if (line >= 0)
			emit divisionByZero(dz->source, line);
	}
	
	void TcpTarget::receivedExecutionStateChanged(Message *message)
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
						
						Step(ess->source).serialize(socketDescriptor());
						flush();
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
							Step(ess->source).serialize(socketDescriptor());
							flush();
						}
					}
					else
						assert(false);
					// we can safely return here, all case that require
					// emitting signals have been handeled
					return;
				}
			}
			else
				mode = EXECUTION_STOP;
		}
		else
			mode = EXECUTION_RUN;
		
		emit executionModeChanged(ess->source, mode);
		if (node.executionMode != mode)
		{
			emit variablesMemoryEstimatedDirty(ess->source);
			node.executionMode = mode;
		}
	}
	
	void TcpTarget::receivedDisconnected(Message *message)
	{
		Disconnected *disconnected = polymorphic_downcast<Disconnected *>(message);
		
		emit nodeDisconnected(disconnected->source);
	}
	
	void TcpTarget::receivedBreakpointSetResult(Message *message)
	{
		BreakpointSetResult *bsr = polymorphic_downcast<BreakpointSetResult *>(message);
		unsigned node = bsr->source;
		emit breakpointSetResult(node, getLineFromPC(node, bsr->pc), bsr->success);
	}
	
	int TcpTarget::getPCFromLine(unsigned node, unsigned line)
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
	
	int TcpTarget::getLineFromPC(unsigned node, unsigned pc)
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
