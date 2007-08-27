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

#ifndef REMOTE_BLUETOOTH_TARGET_H
#define REMOTE_BLUETOOTH_TARGET_H

#include "Target.h"
#include "../compiler/compiler.h"
#include <QString>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

class QSocketNotifier;

namespace Aseba
{
	//! The virtual machine running on a distant target through the bluetooth link
	class RemoteBluetoothTarget : public Target
	{
		Q_OBJECT
		
	public:
		//! Constructor, build an initial description
		RemoteBluetoothTarget();
		~RemoteBluetoothTarget();

		virtual bool connect();
		
		virtual const TargetDescription * const getConstDescription();
		virtual TargetDescription * getDescription() { return 0; }
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

	protected slots:
		void dataAvailableOnRfcommSock();
		
	protected:
		bool read(void *buf, size_t count);
		bool write(const void *buf, size_t count);
		bool openRfcommConnection(const bdaddr_t *ba);
		bool readString(std::string &s);
		void readVariableMemory();
		void getVariablesMemory();
		void timerEvent(QTimerEvent *event);
	
	protected:
		int rfcommSock; //!< socket for bluetooth rfcomm connection
		QSocketNotifier *rfcommSockNotifier; //! notifier to get data when they become available
		BytecodeVector debugBytecode; //!< bytecode with debug information
		VariablesDataVector variables; //!< variables at last read/push
		TargetDescription description; //!< description of the target
		bool descriptionDirty; //!< is description of the target up to date ?
		bool runEvent; //!< when true, mode is run, otherwise step by step
	};
}; // Aseba

#endif
