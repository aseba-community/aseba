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

#ifndef VIRTUALMACHINE_H
#define VIRTUALMACHINE_H

#include "Target.h"
#include "../compiler/compiler.h"
#include "../vm/vm.h"

namespace Aseba
{
	//! The virtual machine running in local
	class LocalVirtualMachine : public Target
	{
	public:
		//! Constructor, build an initial description
		LocalVirtualMachine();

		virtual bool connect() { return true; }
		
		virtual const TargetDescription * const getConstDescription() { return &description; }
		virtual TargetDescription * getDescription() { return &description; }
		virtual void uploadBytecode(const BytecodeVector &bytecode);
		
		virtual void sizesChanged();
		
		virtual void setupEvent(unsigned id);
		
		virtual void next();
		virtual void runStepSwitch();
		virtual void runBackground();
		virtual void stop();
		
		virtual bool setBreakpoint(unsigned line);
		virtual bool clearBreakpoint(unsigned line);
		virtual void clearBreakpoints();
	
	public:
		// The following functions should only be called by the VM C bindings. They call the corresponding Qt signals or popup a message box
		void emitEvent(unsigned short id, unsigned short start, unsigned short length);
		void emitArrayAccessOutOfBounds(unsigned short pc, unsigned short sp, unsigned short index);
		void emitDivisionByZero(unsigned short pc, unsigned short sp);
		
	protected:
		void timerEvent(QTimerEvent *event);
		
	protected:
		TargetDescription description; //!< description of the virtual machine, can be modified by GUI
		bool runEvent; //!< when true, mode is run, otherwise step by step
		AsebaVMState vmState; //!< state of virtual machine
		std::valarray<uint16> vmBytecode; //!< bytecodes content
		std::valarray<sint16> vmVariables; //!< variables content
		std::valarray<sint16> vmStack; //!< stack content
		BytecodeVector debugBytecode; //!< bytecode with debug information
	};
}; // Aseba

#endif
