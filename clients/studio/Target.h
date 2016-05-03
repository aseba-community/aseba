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

#ifndef TARGET_H
#define TARGET_H

#include <QObject>
#include <valarray>
#include "../../compiler/compiler.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	struct TargetDescription;
	
	//! The interface to an aseba network. Used to interact with the nodes
	/*! The target is responsible for maintaining the state of the network, querying new nodes, etc.
	*/
	class Target: public QObject
	{
		Q_OBJECT
	
	public:
		//! Mode of execution of target
		enum ExecutionMode
		{
			//! Stop execution
			EXECUTION_STOP,
			//! Run the event until a breakpoint or a stop
			EXECUTION_RUN,
			//! Wait step command
			EXECUTION_STEP_BY_STEP,
			//! Execution mode of target is unknown
			EXECUTION_UNKNOWN,
		};
		
	signals:
		//! A new node has connected to the network.
		void nodeConnected(unsigned node);
		//! A node has disconnected from the network.
		void nodeDisconnected(unsigned node);
		
		//! A user event has arrived from the network.
		void userEvent(unsigned id, const VariablesDataVector data);
		//! Some user events have been dropped, i.e. not sent to the gui
		void userEventsDropped(unsigned amount);
		
		//! A node did an access out of array bounds exception.
		void arrayAccessOutOfBounds(unsigned node, unsigned line, unsigned size, unsigned index);
		//! A node did a division by zero exception.
		void divisionByZero(unsigned node, unsigned line);
		//! A new event was run and the current killed on a node
		void eventExecutionKilled(unsigned node, unsigned line);
		//! A node has produced an error specific to it
		void nodeSpecificError(unsigned node, unsigned line, const QString& message);
		
		//! The program counter of a node has changed, causing a change of position in source code.
		void executionPosChanged(unsigned node, unsigned line);
		//! The mode of execution of a node (stop, run, step by step) has changed.
		void executionModeChanged(unsigned node, Target::ExecutionMode mode);
		//! The execution state logic thinks variables might need a refresh
		void variablesMemoryEstimatedDirty(unsigned node);
		
		//! The content of the variables memory of a node has changed.
		void variablesMemoryChanged(unsigned node, unsigned start, const VariablesDataVector &variables);
		
		//! The result of a set breakpoint call
		void breakpointSetResult(unsigned node, unsigned line, bool success);
		
		//! We received an ack from the bootloader
		void bootloaderAck(unsigned errorCode, unsigned errorAddress);
		
	public:
		//! Virtual destructor.
		virtual ~Target() { }
	
	public:
		//! Return the language that we choosen for this connection
		virtual QString getLanguage() const = 0;
		
		//! Return all the nodes
		virtual QList<unsigned> getNodesList() const = 0;
		
		//! Return the name of a node. Returned value is always valid if node exists
		const QString getName(unsigned node) const;
		
		//! Return a constant description of a node. Returned value is always valid if node exists
		virtual const TargetDescription * const getDescription(unsigned node) const = 0;
		
		//! Upload bytecode to target.
		virtual void uploadBytecode(unsigned node, const BytecodeVector &bytecode) = 0;
		
		//! Save the bytecode currently on target
		virtual void writeBytecode(unsigned node) = 0;
		
		//! Reboot (restart the whole microcontroller, not just reset the aseba VM)
		virtual void reboot(unsigned node) = 0;
		
		//! Send an event to the aseba network.
		virtual void sendEvent(unsigned id, const VariablesDataVector &data) = 0;
		
		// variables
		
		//! Set part of variables memory
		virtual void setVariables(unsigned node, unsigned start, const VariablesDataVector &data) = 0;
		
		//! Get part of variables memory
		virtual void getVariables(unsigned node, unsigned start, unsigned length) = 0;
		
		// execution
		
		//! Reset the execution of a node, do not clear bytecode nor breakpoints
		virtual void reset(unsigned node) = 0;
		
		//! Run the execution on a node.
		virtual void run(unsigned node) = 0;
		
		//! Interrupt (pause, go to step by step) the execution on a node.
		virtual void pause(unsigned node) = 0;
		
		//! Run bytecode on a node until next source line.
		virtual void next(unsigned node) = 0;
		
		//! Stop the execution on a node.
		virtual void stop(unsigned node) = 0;
		
		// breakpoints
		
		//! Set a breakpoint in a node at a specific line.
		virtual void setBreakpoint(unsigned node, unsigned line) = 0;
		
		//! Remove the breakpoint in a node at a specific line.
		virtual void clearBreakpoint(unsigned node, unsigned line) = 0;
		
		//! Remove all breakpoints in a node
		virtual void clearBreakpoints(unsigned node) = 0;
	
	protected:
		friend class ThymioBootloaderDialog;
		friend class ThymioVisualProgramming;
				
		//! Block all write operations, used by invasive plugins when they do something hacky with the underlying data stream
		virtual void blockWrite() = 0;
		
		//! Unblock write operatinos, used by invasive plugins when they do something hacky with the underlying data stream
		virtual void unblockWrite() = 0;
	};
	
	/*@}*/
} // namespace Aseba

#endif
