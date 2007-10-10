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

#ifndef TARGET_H
#define TARGET_H

#include <QObject>
#include <valarray>
#include "../compiler/compiler.h"

namespace Aseba
{
	class TargetDescription;
	
	//! The interface to an aseba network. Used to interact with the nodes
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
		void userEvent(unsigned id, const VariablesDataVector &data);
		
		//! A node did an access out of array bounds exception.
		void arrayAccessOutOfBounds(unsigned node, unsigned line, unsigned index);
		//! A node did a division by zero exception.
		void divisionByZero(unsigned node, unsigned line);
		
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
		
	public:
		//! Virtual destructor.
		virtual ~Target() { }
	
	public slots:
		//! Try to connect to the aseba network.
		virtual void connect() = 0;
	
	public:
		//! Return the name of a node. Returned value is always valid if node exists
		const QString getName(unsigned node) const;
		
		//! Return a constant description of a node. Returned value is always valid if node exists
		virtual const TargetDescription * const getConstDescription(unsigned node) const = 0;
		
		//! Upload bytecode to target.
		virtual void uploadBytecode(unsigned node, const BytecodeVector &bytecode) = 0;
		
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
	};
}; // Aseba

#endif
