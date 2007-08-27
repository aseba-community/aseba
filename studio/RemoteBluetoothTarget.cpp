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

#include "RemoteBluetoothTarget.h"

#include <QtGui>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "../common/consts.h"
#include "../vm/vm.h"

namespace Aseba
{
	RemoteBluetoothTarget::RemoteBluetoothTarget()
	{
		rfcommSock = -1;
		descriptionDirty = true;
		runEvent = false;
		startTimer(100);
	}

	RemoteBluetoothTarget::~RemoteBluetoothTarget()
	{
		// close connection
		if (rfcommSock != -1)
			close(rfcommSock);
	}

	bool RemoteBluetoothTarget::connect()
	{
		// open device
		int devId = hci_get_route(NULL);
		if (devId < 0)
		{
			QMessageBox::critical(0, tr("Aseba Studio"), tr("Error, can't find bluetooth adapter"));
			return false;
		}
		
		// open socket
		int devSock = hci_open_dev(devId);
		if (devSock < 0)
		{
			QMessageBox::critical(0, tr("Aseba Studio"), tr("Error, can't access bluetooth adapter"));
			return false;
		}

		// query
		// TODO : asynchronous inquiry in thread
		//int length  = 8; /* ~10 seconds */
		int length  = 4; /* ~5 seconds */
		inquiry_info *inquiryInfo = NULL;
		// device id, query length (last 1.28 * length seconds), max devices, lap ??, returned array, flag
		int devicesCount = hci_inquiry(devId, length, 255, NULL, &inquiryInfo, 0);
		if (devicesCount < 0)
		{
			QMessageBox::critical(0, tr("Aseba Studio"), tr("Error, can't query bluetooth"));
			close(devSock);
			return false;
		}
		
		// search e-puck devices and build map
		QMap<QString, bdaddr_t> epucksList;
		for (int i = 0; i < devicesCount; i++)
		{
			char addrFriendlyName[256];
			// if we can read its name and it is an e-puck
			if ((hci_read_remote_name(devSock, &(inquiryInfo+i)->bdaddr, 256, addrFriendlyName, 0) >= 0) && (strncmp("e-puck_", addrFriendlyName, 7) == 0))
			{
				// if we can scan its id add it to the list
				epucksList[addrFriendlyName] = (inquiryInfo+i)->bdaddr;
			}
		}
		free(inquiryInfo);
		close(devSock);

		if (epucksList.isEmpty())
		{
			QMessageBox::critical(0, tr("Aseba Studio"), tr("Error, can't find any bluetooth target"));
			return false;
		}

		// dialog
		/*
		std::cout << "Contacting e-puck " << id << std::endl;
		char addrString[19];
		ba2str(&(info+i)->bdaddr, addrString);
		found = openRfcommConnection();*/
		QStringList targetsList;
		foreach (QString str, epucksList.keys())
			targetsList << str;
		bool ok;
		QString target = QInputDialog::getItem(0, tr("Aseba Studio"), tr("Please select a target"), targetsList, 0, false, &ok);
		if (!ok)
			return false;
			

		// opening connection
		return openRfcommConnection(&(epucksList.value(target)));
	}
	
	bool RemoteBluetoothTarget::read(void *buf, size_t count)
	{
		size_t origCount = count;
		
		char *ptr = (char *)buf;
		while (count)
		{
			ssize_t len = ::read(rfcommSock, ptr, count);
			if (len < 0)
			{
				// TODO : managed read error
				return false;
			}
			else if (len == 0)
			{
				#ifdef Q_OS_WIN32
				sleep(10);
				#else
				usleep(10000);
				#endif
			}
			else
			{
				ptr += len;
				count -= len;
			}
		}
		
		QString s;
		s = QString("bt read %1 : ").arg(origCount);
		for (size_t i = 0; i < origCount; i++)
			s += QString("%1 ").arg((int)((unsigned char *)buf)[i], 0, 16);
		//qDebug() << s;
		
		return true;
	}
	
	bool RemoteBluetoothTarget::write(const void *buf, size_t count)
	{
		QString s;
		s = QString("bt write %1 : ").arg(count);
		for (size_t i = 0; i < count; i++)
			s += QString("%1 ").arg((int)((const unsigned char *)buf)[i], 0, 16);
		//qDebug() << s;
		
		const char *ptr = (const char *)buf;
		while (count)
		{
			ssize_t len = ::write(rfcommSock, ptr, count);
			if (len < 0)
			{
				// TODO : managed write error
				return false;
			}
			else if (len == 0)
			{
				#ifdef Q_OS_WIN32
				sleep(10);
				#else
				usleep(10000);
				#endif
			}
			else
			{
				ptr += len;
				count -= len;
			}
		}
		return true;
	}

	bool RemoteBluetoothTarget::openRfcommConnection(const bdaddr_t *ba)
	{
		// set the connection parameters (who to connect to)
		struct sockaddr_rc addr;
		addr.rc_family = AF_BLUETOOTH;
		addr.rc_channel = (uint8_t) 1;
		addr.rc_bdaddr = *ba;
		
		// allocate a socket
		rfcommSock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
		if (rfcommSock == -1)
		{
			QMessageBox::critical(0, tr("Aseba Studio"), tr("Error, can't create rfcomm socket"));
			return false;
		}
		
		// connect to server
		int status = ::connect(rfcommSock, (struct sockaddr *)&addr, sizeof(addr));
		if (status != 0)
		{
			QMessageBox::critical(0, tr("Aseba Studio"), tr("Error, can't connect to target rfcomm"));
			close(rfcommSock);
			rfcommSock = -1;
			return false;
		}
		
		// create notifier
		rfcommSockNotifier = new QSocketNotifier(rfcommSock, QSocketNotifier::Read, this);
		QObject::connect(rfcommSockNotifier, SIGNAL(activated(int)), SLOT(dataAvailableOnRfcommSock()));
		
		return true;
	}
	
	bool RemoteBluetoothTarget::readString(std::string &s)
	{
		unsigned char len;
		read(&len, 1);
		if (len == 0)
			return false;
		s.resize(len);
		read(&s[0], len);
		qDebug() << "read bt string " << QString::fromUtf8(s.c_str());
		// TODO : managed read / write error
		return true;
	}
	
	void RemoteBluetoothTarget::readVariableMemory()
	{
		for (unsigned i = 0; i < description.variablesSize; i++)
			read(&variables[i], 2);
		emit variablesMemoryChanged(variables);
		// TODO : managed read / write error
	}
	
	void RemoteBluetoothTarget::getVariablesMemory()
	{
		rfcommSockNotifier->setEnabled(false);
		
		qDebug() << "ASEBA_DEBUG_GET_VARIABLES";
		unsigned char c = ASEBA_DEBUG_GET_VARIABLES;
		write(&c, 1);
		readVariableMemory();
		
		rfcommSockNotifier->setEnabled(true);
		// TODO : managed read / write error
	}
	
	const TargetDescription * const RemoteBluetoothTarget::getConstDescription()
	{
		if (descriptionDirty)
		{
			rfcommSockNotifier->setEnabled(false);
			
			qDebug() << "ASEBA_DEBUG_GET_DESCRIPTION";
			unsigned char c = ASEBA_DEBUG_GET_DESCRIPTION;
			write(&c, 1);
			read(&description.bytecodeSize, 2);
			read(&description.variablesSize, 2);
			read(&description.stackSize, 2);
			
			description.namedVariables.clear();
			std::string s;
			while (readString(s))
			{
				unsigned short sSize;
				read(&sSize, 2);
				description.namedVariables.push_back(TargetDescription::NamedVariable(s, sSize));
			}
			
			// TODO : add native functions std::vector<NativeFunction> nativeFunctions
			// TODO : managed read / write error
			variables.resize(description.variablesSize);
			
			rfcommSockNotifier->setEnabled(true);
			descriptionDirty = false;
		}
		
		return &description;
	}

	void RemoteBluetoothTarget::uploadBytecode(const BytecodeVector &bytecode)
	{
		Q_ASSERT(bytecode.size() <= description.bytecodeSize);
		
		debugBytecode = bytecode;
		
		rfcommSockNotifier->setEnabled(false);
		
		qDebug() << "ASEBA_DEBUG_SET_BYTECODE";
		unsigned char c = ASEBA_DEBUG_SET_BYTECODE;
		write(&c, 1);
		unsigned short len = bytecode.size();
		write(&len, 2);
		for (unsigned i = 0; i < len; i++)
			write(&(bytecode[i].bytecode), 2);
		// TODO : managed read / write error
		
		rfcommSockNotifier->setEnabled(true);
	}

	void RemoteBluetoothTarget::sizesChanged()
	{
		// This is a read-only target
		Q_ASSERT(false);
	}

	void RemoteBluetoothTarget::setupEvent(unsigned id)
	{
		rfcommSockNotifier->setEnabled(false);
		
		qDebug() << "ASEBA_DEBUG_SETUP_EVENT";
		
		unsigned char c = ASEBA_DEBUG_SETUP_EVENT;
		unsigned short event = id;
		unsigned short flags;
		unsigned short pc;
		
		write(&c, 1);
		write(&event, 2);
		read(&flags, 2);
		read(&pc, 2);
		// TODO : managed read / write error
		
		rfcommSockNotifier->setEnabled(true);
		
		if (AsebaMaskIsSet(flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			Q_ASSERT(pc < debugBytecode.size());
			if (AsebaMaskIsClear(flags, ASEBA_VM_RUN_BACKGROUND_MASK))
			{
				emit executionModeChanged(EXECUTION_STEP_BY_STEP);
				emit executionPosChanged(debugBytecode[pc].line);
			}
		}
	}

	void RemoteBluetoothTarget::next()
	{
		// run until we have switched line
		
		// get initial pc
		qDebug() << "ASEBA_DEBUG_GET_PC";
		unsigned char c = ASEBA_DEBUG_GET_PC;
		unsigned short previousPC;
		
		rfcommSockNotifier->setEnabled(false);
		write(&c, 1);
		read(&previousPC, 2);
		rfcommSockNotifier->setEnabled(true);
		
		unsigned previousLine = debugBytecode[previousPC].line;
		
		unsigned short flags;
		unsigned short pc;
		
		do
		{
			// execute
			qDebug() << "ASEBA_DEBUG_STEP";
			c = ASEBA_DEBUG_STEP;
			
			rfcommSockNotifier->setEnabled(false);
			write(&c, 1);
			read(&flags, 2);
			read(&pc, 2);
			rfcommSockNotifier->setEnabled(true);
		}
		while (AsebaMaskIsSet(flags, ASEBA_VM_EVENT_ACTIVE_MASK) && (pc != previousPC) && (debugBytecode[pc].line == previousLine));
		
		// emit states changes
		getVariablesMemory();
		if (AsebaMaskIsSet(flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			if (AsebaMaskIsSet(flags, ASEBA_VM_WAS_BREAKPOINT_MASK))
			{
				runEvent = false;
				emit executionModeChanged(EXECUTION_STEP_BY_STEP);
			}
			emit executionPosChanged(debugBytecode[pc].line);
		}
		else
		{
			runEvent = false;
			emit executionModeChanged(EXECUTION_INACTIVE);
		}
		// TODO : managed read / write error
	}
	
	void RemoteBluetoothTarget::runStepSwitch()
	{
		if (runEvent)
			emit executionModeChanged(EXECUTION_STEP_BY_STEP);
		else
			emit executionModeChanged(EXECUTION_RUN_EVENT);
		runEvent = !runEvent;
	}
	
	void RemoteBluetoothTarget::runBackground()
	{
		qDebug() << "ASEBA_DEBUG_BACKGROUND_RUN";
		unsigned char c = ASEBA_DEBUG_BACKGROUND_RUN;
		write(&c, 1);
		emit executionModeChanged(EXECUTION_RUN_BACKGROUND);
	}
	
	void RemoteBluetoothTarget::stop()
	{
		qDebug() << "ASEBA_DEBUG_STOP";
		unsigned char c = ASEBA_DEBUG_STOP;
		write(&c, 1);
		emit executionModeChanged(EXECUTION_INACTIVE);
	}

	bool RemoteBluetoothTarget::setBreakpoint(unsigned line)
	{
		for (size_t i = 0; i < debugBytecode.size(); i++)
			if (debugBytecode[i].line == line)
			{
				rfcommSockNotifier->setEnabled(false);
				
				qDebug() << "ASEBA_DEBUG_BREAKPOINT_SET";
				unsigned char c = ASEBA_DEBUG_BREAKPOINT_SET;
				write(&c, 1);
				unsigned short pc = i;
				write(&pc, 2);
				read(&c, 1);
				// TODO : managed read / write error
				
				rfcommSockNotifier->setEnabled(true);
				return c;
			}
		return false;
	}
	
	bool RemoteBluetoothTarget::clearBreakpoint(unsigned line)
	{
		for (size_t i = 0; i < debugBytecode.size(); i++)
			if (debugBytecode[i].line == line)
			{
				rfcommSockNotifier->setEnabled(false);
				
				qDebug() << "ASEBA_DEBUG_BREAKPOINT_CLEAR";
				unsigned char c = ASEBA_DEBUG_BREAKPOINT_CLEAR;
				write(&c, 1);
				unsigned short pc = i;
				write(&pc, 2);
				read(&c, 1);
				// TODO : managed read / write error
				
				rfcommSockNotifier->setEnabled(true);
				return c;
			}
		return false;
	}
	
	void RemoteBluetoothTarget::clearBreakpoints()
	{
		qDebug() << "ASEBA_DEBUG_BREAKPOINT_CLEAR_ALL";
		unsigned char c = ASEBA_DEBUG_BREAKPOINT_CLEAR_ALL;
		write(&c, 1);
	}
	
	void RemoteBluetoothTarget::timerEvent(QTimerEvent *event)
	{
		Q_UNUSED(event);
		if (runEvent)
			next();
	}
	
	void RemoteBluetoothTarget::dataAvailableOnRfcommSock()
	{
		unsigned char c;
		int count = ::read(rfcommSock, &c, 1);
		if (count < 0)	// TODO : managed read / write error
			return;
		if (count == 0)
			return;
		
		qDebug() << "bt data available :";
		rfcommSockNotifier->setEnabled(false);
		
		switch (c)
		{
			case ASEBA_DEBUG_PUSH_EVENT:
			{
				qDebug() << "ASEBA_DEBUG_PUSH_EVENT";
				
				readVariableMemory();
				
				unsigned short id, start, length;
				read(&id, 2);
				read(&start, 2);
				read(&length, 2);
				// TODO : managed read / write error
				
				emit event(id, start, length);
			}
			break;
			
			case ASEBA_DEBUG_PUSH_ARRAY_ACCESS_OUT_OF_BOUND:
			{
				qDebug() << "ASEBA_DEBUG_PUSH_ARRAY_ACCESS_OUT_OF_BOUND";
				
				readVariableMemory();
				
				unsigned short pc, index;
				read(&pc, 2);
				read(&index, 2);
				// TODO : managed read / write error
				
				Q_ASSERT(pc < debugBytecode.size());
				unsigned line = debugBytecode[pc].line;
				emit arrayAccessOutOfBounds(line, index);
			}
			break;
			
			case ASEBA_DEBUG_PUSH_DIVISION_BY_ZERO:
			{
				qDebug() << "ASEBA_DEBUG_PUSH_DIVISION_BY_ZERO";
				
				readVariableMemory();
				
				unsigned short pc;
				read(&pc, 2);
				// TODO : managed read / write error
				
				Q_ASSERT(pc < debugBytecode.size());
				unsigned line = debugBytecode[pc].line;
				emit divisionByZero(line);
			}
			break;
			
			case ASEBA_DEBUG_PUSH_STEP_BY_STEP:
			{
				qDebug() << "ASEBA_DEBUG_PUSH_STEP_BY_STEP";
				
				unsigned short pc;
				read(&pc, 2);
				// TODO : managed read / write error
				
				emit executionPosChanged(debugBytecode[pc].line);
				emit executionModeChanged(EXECUTION_STEP_BY_STEP);
			}
			break;
			
			case ASEBA_DEBUG_PUSH_VARIABLES:
			{
				qDebug() << "ASEBA_DEBUG_PUSH_VARIABLES";
				
				readVariableMemory();
			}
			break;
			
			default:
				qDebug() << "ERROR! unknown push command " << (int)c;
			break;
		}
		
		rfcommSockNotifier->setEnabled(true);
	}
}
