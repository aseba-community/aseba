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

#ifndef ASEBA_MSG
#define ASEBA_MSG

#include "../common/types.h"
#include "../common/consts.h"
#include <vector>
#include <string>

namespace Aseba
{
	
	class Message
	{
	public:
		uint16 source;
		uint16 type;
		
	public:
		Message(uint16 type);
		virtual ~Message();
		
		void serialize(int fd);
		static Message *receive(int fd);
		void dump(std::ostream &stream);
		
	protected:
		virtual void serializeSpecific() = 0;
		virtual void deserializeSpecific() = 0;
		virtual void dumpSpecific(std::ostream &stream) = 0;
		virtual operator const char * () const { return "message super class"; }
	
	protected:
		template<typename T> void add(const T& val);
		template<typename T> T get();
		
	protected:
		std::vector<uint8> rawData;
		size_t readPos;
	};
	
	
	class UserMessage : public Message
	{
	public:
		typedef std::vector<sint16> DataVector;
		DataVector data;
	
	public:
		UserMessage() : Message(ASEBA_MESSAGE_INVALID) { }
		UserMessage(uint16 type, const DataVector& data) : Message(type), data(data) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "user message"; }
	};
	
	class BootloaderDescription : public Message
	{
	public:
		uint16 pageSize;
		uint16 pagesStart;
		uint16 pagesCount;
		
	public:
		BootloaderDescription() : Message(ASEBA_MESSAGE_BOOTLOADER_DESCRIPTION) { }
	
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "bootloader description"; }
	};
	
	class BootloaderDataRead : public Message
	{
	public:
		uint8 data[4];
		
	public:
		BootloaderDataRead() : Message(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_READ) { }
	
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "bootloader page data read"; }
	};
	
	class BootloaderAck : public Message
	{
	public:
		enum ErrorTypes
		{
			SUCCESS = 0,
			ERROR_INVALID_FRAME_SIZE,
			ERROR_PROGRAMMING_FAILED,
			ERROR_NOT_PROGRAMMING
		};
		
	public:
		uint16 errorCode;
		uint16 errorAddress;
		
	public:
		BootloaderAck() : Message(ASEBA_MESSAGE_BOOTLOADER_ACK) { }
	
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "bootloader ack"; }
	};
	
	class Presence : public Message
	{
	public:
		Presence() : Message(ASEBA_MESSAGE_PRESENCE) { }
		
	protected:
		virtual void serializeSpecific() {}
		virtual void deserializeSpecific() {}
		virtual void dumpSpecific(std::ostream &) {}
		virtual operator const char * () const { return "presence"; }
	};
	
	class Description : public Message
	{
	public:
		std::string nodeName;
		
		uint16 bytecodeSize;
		
		uint16 stackSize;
		
		uint16 variablesSize;
		std::vector<std::string> variablesNames;
		std::vector<uint16> variablesSizes;
		
	public:
		Description() : Message(ASEBA_MESSAGE_DESCRIPTION) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "description"; }
	};
	
	class Disconnected : public Message
	{
	public:
		Disconnected() : Message(ASEBA_MESSAGE_DISCONNECTED) { }
		
	protected:
		virtual void serializeSpecific() {}
		virtual void deserializeSpecific() {}
		virtual void dumpSpecific(std::ostream &) {}
		virtual operator const char * () const { return "disconnected"; }
	};
	
	class Variables : public Message
	{
	public:
		uint16 start;
		std::vector<sint16> variables;
		
	public:
		Variables() : Message(ASEBA_MESSAGE_VARIABLES) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "variables"; }
	};
	
	class ArrayAccessOutOfBounds : public Message
	{
	public:
		uint16 pc;
		uint16 index;
		
	public:
		ArrayAccessOutOfBounds() : Message(ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "array access out of bounds"; }
	};
	
	class DivisionByZero : public Message
	{
	public:
		uint16 pc;
		
	public:
		DivisionByZero() : Message(ASEBA_MESSAGE_DIVISION_BY_ZERO) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "division by zero"; }
	};
	
	class ExecutionStateChanged : public Message
	{
	public:
		uint16 pc;
		uint16 flags;
		
	public:
		ExecutionStateChanged() : Message(ASEBA_MESSAGE_EXECUTION_STATE_CHANGED) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "execution state"; }
	};
	
	class BreakpointSetResult : public Message
	{
	public:
		uint16 pc;
		uint16 success;
		
	public:
		BreakpointSetResult() : Message(ASEBA_MESSAGE_BREAKPOINT_SET_RESULT) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "breakpoint set result"; }
	};
	
	
	//! Cammands messages talk to a specific node
	class CmdMessage : public Message
	{
	public:
		uint16 dest;
	
	public:
		CmdMessage(uint16 type, uint16 dest) : Message(type), dest(dest) { }
		
	protected:	
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "command message super class"; }
	};
	
	class BootloaderReset : public CmdMessage
	{
	public:
		BootloaderReset() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_RESET, ASEBA_DEST_INVALID) { }
		BootloaderReset(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_RESET, dest) { }
	
	protected:
		virtual operator const char * () const { return "bootloader reset"; }
	};
	
	class BootloaderReadPage : public CmdMessage
	{
	public:
		uint16 pageNumber;
		
	public:
		BootloaderReadPage() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_READ_PAGE, ASEBA_DEST_INVALID) { }
		BootloaderReadPage(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_READ_PAGE, dest) { }
	
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "bootloader read page"; }
	};
	
	class BootloaderWritePage : public CmdMessage
	{
	public:
		uint16 pageNumber;
		
	public:
		BootloaderWritePage() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE, ASEBA_DEST_INVALID) { }
		BootloaderWritePage(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE, dest) { }
	
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "bootloader write page"; }
	};
	
	class BootloaderPageDataWrite : public CmdMessage
	{
	public:
		uint8 data[4];
		
	public:
		BootloaderPageDataWrite() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE, ASEBA_DEST_INVALID) { }
		BootloaderPageDataWrite(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE, dest) { }
	
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "bootloader page data write"; }
	};
	
	
	class SetBytecode : public CmdMessage
	{
	public:
		std::vector<uint16> bytecode;
		
	public:
		SetBytecode() : CmdMessage(ASEBA_MESSAGE_SET_BYTECODE, ASEBA_DEST_INVALID) { }
		SetBytecode(uint16 dest) : CmdMessage(ASEBA_MESSAGE_SET_BYTECODE, dest) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "set bytecode"; }
	};
	
	class Reset : public CmdMessage
	{
	public:
		Reset() : CmdMessage(ASEBA_MESSAGE_RESET, ASEBA_DEST_INVALID) { }
		Reset(uint16 dest) : CmdMessage(ASEBA_MESSAGE_RESET, dest) { }
		
	protected:
		virtual operator const char * () const { return "reset"; }
	};
	
	class Run : public CmdMessage
	{
	public:
		Run() : CmdMessage(ASEBA_MESSAGE_RUN, ASEBA_DEST_INVALID) { }
		Run(uint16 dest) : CmdMessage(ASEBA_MESSAGE_RUN, dest) { }
		
	protected:
		virtual operator const char * () const { return "run"; }
	};
	
	class Pause : public CmdMessage
	{
	public:
		Pause() : CmdMessage(ASEBA_MESSAGE_PAUSE, ASEBA_DEST_INVALID) { }
		Pause(uint16 dest) : CmdMessage(ASEBA_MESSAGE_PAUSE, dest) { }
		
	protected:
		virtual operator const char * () const { return "pause"; }
	};
	
	class Step : public CmdMessage
	{
	public:
		Step() : CmdMessage(ASEBA_MESSAGE_STEP, ASEBA_DEST_INVALID) { }
		Step(uint16 dest) : CmdMessage(ASEBA_MESSAGE_STEP, dest) { }
		
	protected:
		virtual operator const char * () const { return "step"; }
	};
	
	class Stop : public CmdMessage
	{
	public:
		Stop() : CmdMessage(ASEBA_MESSAGE_STOP, ASEBA_DEST_INVALID) { }
		Stop(uint16 dest) : CmdMessage(ASEBA_MESSAGE_STOP, dest) { }
		
	protected:
		virtual operator const char * () const { return "stop"; }
	};
	
	class GetExecutionState : public CmdMessage
	{
	public:
		GetExecutionState() : CmdMessage(ASEBA_MESSAGE_GET_EXECUTION_STATE, ASEBA_DEST_INVALID) { }
		GetExecutionState(uint16 dest) : CmdMessage(ASEBA_MESSAGE_GET_EXECUTION_STATE, dest) { }
		
	protected:
		virtual operator const char * () const { return "get execution state"; }
	};
	
	class BreakpointSet : public CmdMessage
	{
	public:
		uint16 pc;
		
	public:
		BreakpointSet() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_SET, ASEBA_DEST_INVALID) { }
		BreakpointSet(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_SET, dest) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "breakpoint set"; }
	};
	
	class BreakpointClear : public CmdMessage
	{
	public:
		uint16 pc;
		
	public:
		BreakpointClear() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR, ASEBA_DEST_INVALID) { }
		BreakpointClear(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR, dest) { }
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "breakpoint clear"; }
	};
	
	class BreakpointClearAll : public CmdMessage
	{
	public:
		BreakpointClearAll() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL, ASEBA_DEST_INVALID) { }
		BreakpointClearAll(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL, dest) { }
		
	protected:
		virtual operator const char * () const { return "breakpoint clear all"; }
	};
	
	class GetVariables : public CmdMessage
	{
	public:
		uint16 start;
		uint16 length;
		
	public:
		GetVariables() : CmdMessage(ASEBA_MESSAGE_GET_VARIABLES, ASEBA_DEST_INVALID) { }
		GetVariables(uint16 dest, uint16 start, uint16 length);
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "get variables"; }
	};
	
	class SetVariables : public CmdMessage
	{
	public:
		uint16 start;
		typedef std::vector<sint16> VariablesVector;
		VariablesVector variables;
		
	public:
		SetVariables() : CmdMessage(ASEBA_MESSAGE_SET_VARIABLES, ASEBA_DEST_INVALID) { }
		SetVariables(uint16 dest, uint16 start, const VariablesVector& variables);
		
	protected:
		virtual void serializeSpecific();
		virtual void deserializeSpecific();
		virtual void dumpSpecific(std::ostream &stream);
		virtual operator const char * () const { return "set variables"; }
	};
}

#endif

