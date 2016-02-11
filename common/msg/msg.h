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

#ifndef ASEBA_MSG
#define ASEBA_MSG

#include "../types.h"
#include "../consts.h"
#include "../../compiler/compiler.h"
#include <vector>
#include <string>

namespace Dashel
{
	class Stream;
}

namespace Aseba
{
	/**
	\defgroup msg Messages exchanged over the network
		
		Aseba messages payload data must be 512 bytes or smaller, which means that the total
		packets size (len + source + type + payload) must be 518 bytes or smaller.
	*/
	/*@{*/

	//! Parent class of any message exchanged over the network
	class Message
	{
	public:
		uint16 source;
		uint16 type;
		
	public:
		Message(uint16 type);
		virtual ~Message();
		
		void serialize(Dashel::Stream* stream) const;
		static Message *receive(Dashel::Stream* stream);
		void dump(std::wostream &stream) const;

	protected:
		//! Helper class that contains the raw data being (de-)serialialized
		struct SerializationBuffer
		{
			SerializationBuffer():readPos(0) {}
			
			template<typename T> void add(const T& val);
			template<typename T> T get();
			void dump(std::wostream &stream) const;
			
			std::vector<uint8> rawData;
			size_t readPos;
		};
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const = 0;
		virtual void deserializeSpecific(SerializationBuffer& buffer) = 0;
		virtual void dumpSpecific(std::wostream &stream) const = 0;
		virtual operator const char * () const { return "message super class"; }
	};
	
	//! Any message sent by a script on a node
	class UserMessage : public Message
	{
	public:
		typedef std::vector<sint16> DataVector;
		DataVector data;
	
	public:
		UserMessage() : Message(ASEBA_MESSAGE_INVALID) { }
		UserMessage(uint16 type, const DataVector& data = DataVector()) : Message(type), data(data) { }
		UserMessage(uint16 type, const sint16* data, const size_t length);
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "user message"; }
	};
	
	//! Commands messages talk to a specific node
	class CmdMessage : public Message
	{
	public:
		uint16 dest;
	
	public:
		CmdMessage(uint16 type, uint16 dest) : Message(type), dest(dest) { }
		
	protected:	
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "command message super class"; }
	};
	
	//! Message for bootloader: description of the flash memory layout
	class BootloaderDescription : public Message
	{
	public:
		uint16 pageSize;
		uint16 pagesStart;
		uint16 pagesCount;
		
	public:
		BootloaderDescription() : Message(ASEBA_MESSAGE_BOOTLOADER_DESCRIPTION) { }
	
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "bootloader description"; }
	};
	
	//! Message for bootloader: data from the flash
	class BootloaderDataRead : public Message
	{
	public:
		uint8 data[4];
		
	public:
		BootloaderDataRead() : Message(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_READ) { }
	
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "bootloader page data read"; }
	};
	
	//! Message for bootloader: acknowledge, with optional error code
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
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "bootloader ack"; }
	};
	
	//! Request nodes to notify their presence
	class ListNodes : public Message
	{
	public:
		uint16 version;
		
	public:
		ListNodes() : Message(ASEBA_MESSAGE_LIST_NODES), version(ASEBA_PROTOCOL_VERSION) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &) const;
		virtual operator const char * () const { return "list nodes"; }
	};
	
	//! Answer of a node notifying its presence
	class NodePresent : public Message
	{
	public:
		uint16 version;
		
	public:
		NodePresent() : Message(ASEBA_MESSAGE_NODE_PRESENT), version(ASEBA_PROTOCOL_VERSION) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &) const;
		virtual operator const char * () const { return "node present"; }
	};
	
	//! Request all nodes to send their description
	class GetDescription : public Message
	{
	public:
		uint16 version;
		
	public:
		GetDescription() : Message(ASEBA_MESSAGE_GET_DESCRIPTION), version(ASEBA_PROTOCOL_VERSION) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &) const;
		virtual operator const char * () const { return "presence"; }
	};
	
	//! Request a specific node to send its description
	class GetNodeDescription : public CmdMessage
	{
	public:
		uint16 version;
		
	public:
		GetNodeDescription() : CmdMessage(ASEBA_MESSAGE_GET_NODE_DESCRIPTION, ASEBA_DEST_INVALID), version(ASEBA_PROTOCOL_VERSION) { }
		GetNodeDescription(uint16 dest) : CmdMessage(ASEBA_MESSAGE_GET_NODE_DESCRIPTION, dest), version(ASEBA_PROTOCOL_VERSION) { }
	
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &) const;
		virtual operator const char * () const { return "get node description"; }
	};
	
	//! Description of a node, local events and native functions are omitted and further received by other messages
	class Description : public Message, public TargetDescription
	{
	public:
		Description() : Message(ASEBA_MESSAGE_DESCRIPTION) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "description"; }
	};
	
	//! Description of a named variable available on a node, the description of the node itself must have been sent first
	class NamedVariableDescription : public Message, public TargetDescription::NamedVariable
	{
	public:
		NamedVariableDescription() : Message(ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "named variable"; }
	};
	
	//! Description of a local event available on a node, the description of the node itself must have been sent first
	class LocalEventDescription : public Message, public TargetDescription::LocalEvent
	{
	public:
		LocalEventDescription() : Message(ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "local event"; }
	};
	
	//! Description of a native function available on a node, the description of the node itself must have been sent first
	class NativeFunctionDescription : public Message, public TargetDescription::NativeFunction
	{
	public:
		NativeFunctionDescription() : Message(ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "native function description"; }
	};
	
	//! A node has disconnected from the network
	class Disconnected : public Message
	{
	public:
		Disconnected() : Message(ASEBA_MESSAGE_DISCONNECTED) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const {}
		virtual void deserializeSpecific(SerializationBuffer& buffer) {}
		virtual void dumpSpecific(std::wostream &) const {}
		virtual operator const char * () const { return "disconnected"; }
	};
	
	//! Content of some variables
	class Variables : public Message
	{
	public:
		uint16 start;
		std::vector<sint16> variables;
		
	public:
		Variables() : Message(ASEBA_MESSAGE_VARIABLES) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "variables"; }
	};
	
	//! Exception: an array acces attempted to read past memory
	class ArrayAccessOutOfBounds : public Message
	{
	public:
		uint16 pc;
		uint16 size;
		uint16 index;
		
	public:
		ArrayAccessOutOfBounds() : Message(ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "array access out of bounds"; }
	};
	
	//! Exception: division by zero
	class DivisionByZero : public Message
	{
	public:
		uint16 pc;
		
	public:
		DivisionByZero() : Message(ASEBA_MESSAGE_DIVISION_BY_ZERO) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "division by zero"; }
	};
	
	//! Exception: an event execution was killed by a new event
	class EventExecutionKilled : public Message
	{
	public:
		uint16 pc;
		
	public:
		EventExecutionKilled() : Message(ASEBA_MESSAGE_EVENT_EXECUTION_KILLED) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "event execution killed"; }
	};
	
	//! A node as produced an error specific to it
	class NodeSpecificError : public Message
	{
	public:
		uint16 pc;
		std::wstring message;
		
	public:
		NodeSpecificError() : Message(ASEBA_MESSAGE_NODE_SPECIFIC_ERROR) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &) const;
		virtual operator const char * () const { return "node specific error"; }
	};
	
	//! The Program Counter or the flags have changed
	class ExecutionStateChanged : public Message
	{
	public:
		uint16 pc;
		uint16 flags;
		
	public:
		ExecutionStateChanged() : Message(ASEBA_MESSAGE_EXECUTION_STATE_CHANGED) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "execution state"; }
	};
	
	//! A breakpoint has been set
	class BreakpointSetResult : public Message
	{
	public:
		uint16 pc;
		uint16 success;
		
	public:
		BreakpointSetResult() : Message(ASEBA_MESSAGE_BREAKPOINT_SET_RESULT) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "breakpoint set result"; }
	};
	
	//! Message for bootloader: reset node
	class BootloaderReset : public CmdMessage
	{
	public:
		BootloaderReset() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_RESET, ASEBA_DEST_INVALID) { }
		BootloaderReset(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_RESET, dest) { }
	
	protected:
		virtual operator const char * () const { return "bootloader reset"; }
	};
	
	//! Message for bootloader: read a page of flash
	class BootloaderReadPage : public CmdMessage
	{
	public:
		uint16 pageNumber;
		
	public:
		BootloaderReadPage() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_READ_PAGE, ASEBA_DEST_INVALID) { }
		BootloaderReadPage(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_READ_PAGE, dest) { }
	
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "bootloader read page"; }
	};
	
	//! Message for bootloader: write a page of flash
	class BootloaderWritePage : public CmdMessage
	{
	public:
		uint16 pageNumber;
		
	public:
		BootloaderWritePage() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE, ASEBA_DEST_INVALID) { }
		BootloaderWritePage(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE, dest) { }
	
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "bootloader write page"; }
	};
	
	//! Message for bootloader: data for flash
	class BootloaderPageDataWrite : public CmdMessage
	{
	public:
		uint8 data[4];
		
	public:
		BootloaderPageDataWrite() : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE, ASEBA_DEST_INVALID) { }
		BootloaderPageDataWrite(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE, dest) { }
	
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "bootloader page data write"; }
	};
	
	//! Upload bytecode to a node
	class SetBytecode : public CmdMessage
	{
	public:
		uint16 start;
		std::vector<uint16> bytecode;
		
	public:
		SetBytecode() : CmdMessage(ASEBA_MESSAGE_SET_BYTECODE, ASEBA_DEST_INVALID), start(0) { }
		SetBytecode(uint16 dest, uint16 start) : CmdMessage(ASEBA_MESSAGE_SET_BYTECODE, dest), start(start) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "set bytecode"; }
	};
	
	//! Call the SetBytecode multiple time in order to send all the bytecode 
	void sendBytecode(Dashel::Stream* stream, uint16 dest, const std::vector<uint16>& bytecode);
	//! Call the SetBytecode multiple time in order to send all the bytecode 
	void sendBytecode(std::vector<Message*>& messagesVector, uint16 dest, const std::vector<uint16>& bytecode);
	
	//! Reset a node
	class Reset : public CmdMessage
	{
	public:
		Reset() : CmdMessage(ASEBA_MESSAGE_RESET, ASEBA_DEST_INVALID) { }
		Reset(uint16 dest) : CmdMessage(ASEBA_MESSAGE_RESET, dest) { }
		
	protected:
		virtual operator const char * () const { return "reset"; }
	};
	
	//! Run a node
	class Run : public CmdMessage
	{
	public:
		Run() : CmdMessage(ASEBA_MESSAGE_RUN, ASEBA_DEST_INVALID) { }
		Run(uint16 dest) : CmdMessage(ASEBA_MESSAGE_RUN, dest) { }
		
	protected:
		virtual operator const char * () const { return "run"; }
	};
	
	//! Pause a node
	class Pause : public CmdMessage
	{
	public:
		Pause() : CmdMessage(ASEBA_MESSAGE_PAUSE, ASEBA_DEST_INVALID) { }
		Pause(uint16 dest) : CmdMessage(ASEBA_MESSAGE_PAUSE, dest) { }
		
	protected:
		virtual operator const char * () const { return "pause"; }
	};
	
	//! Step a node
	class Step : public CmdMessage
	{
	public:
		Step() : CmdMessage(ASEBA_MESSAGE_STEP, ASEBA_DEST_INVALID) { }
		Step(uint16 dest) : CmdMessage(ASEBA_MESSAGE_STEP, dest) { }
		
	protected:
		virtual operator const char * () const { return "step"; }
	};
	
	//! Stop a node
	class Stop : public CmdMessage
	{
	public:
		Stop() : CmdMessage(ASEBA_MESSAGE_STOP, ASEBA_DEST_INVALID) { }
		Stop(uint16 dest) : CmdMessage(ASEBA_MESSAGE_STOP, dest) { }
		
	protected:
		virtual operator const char * () const { return "stop"; }
	};
	
	//! Request the current execution state of a node
	class GetExecutionState : public CmdMessage
	{
	public:
		GetExecutionState() : CmdMessage(ASEBA_MESSAGE_GET_EXECUTION_STATE, ASEBA_DEST_INVALID) { }
		GetExecutionState(uint16 dest) : CmdMessage(ASEBA_MESSAGE_GET_EXECUTION_STATE, dest) { }
		
	protected:
		virtual operator const char * () const { return "get execution state"; }
	};
	
	//! Set a breakpoint on a node
	class BreakpointSet : public CmdMessage
	{
	public:
		uint16 pc;
		
	public:
		BreakpointSet() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_SET, ASEBA_DEST_INVALID) { }
		BreakpointSet(uint16 dest, uint16 pc) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_SET, dest), pc(pc) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "breakpoint set"; }
	};
	
	//! Clear a breakpoint on a node
	class BreakpointClear : public CmdMessage
	{
	public:
		uint16 pc;
		
	public:
		BreakpointClear() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR, ASEBA_DEST_INVALID) { }
		BreakpointClear(uint16 dest, uint16 pc) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR, dest), pc(pc) { }
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "breakpoint clear"; }
	};
	
	//! Clear all breakpoints on a node
	class BreakpointClearAll : public CmdMessage
	{
	public:
		BreakpointClearAll() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL, ASEBA_DEST_INVALID) { }
		BreakpointClearAll(uint16 dest) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL, dest) { }
		
	protected:
		virtual operator const char * () const { return "breakpoint clear all"; }
	};
	
	//! Read some variables from a node
	class GetVariables : public CmdMessage
	{
	public:
		uint16 start;
		uint16 length;
		
	public:
		GetVariables() : CmdMessage(ASEBA_MESSAGE_GET_VARIABLES, ASEBA_DEST_INVALID) { }
		GetVariables(uint16 dest, uint16 start, uint16 length);
		
	protected:
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "get variables"; }
	};
	
	//! Set some variables on a node
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
		virtual void serializeSpecific(SerializationBuffer& buffer) const;
		virtual void deserializeSpecific(SerializationBuffer& buffer);
		virtual void dumpSpecific(std::wostream &stream) const;
		virtual operator const char * () const { return "set variables"; }
	};
	
	//! Save the current bytecode of a node
	class WriteBytecode : public CmdMessage
	{
	public:
		WriteBytecode() : CmdMessage(ASEBA_MESSAGE_WRITE_BYTECODE, ASEBA_DEST_INVALID) { }
		WriteBytecode(uint16 dest) : CmdMessage(ASEBA_MESSAGE_WRITE_BYTECODE, dest) { }
		
	protected:
		virtual operator const char * () const { return "save bytecode"; }
	};
	
	//! Reboot a node, useful to go into its bootloader, if present
	class Reboot : public CmdMessage
	{
	public:
		Reboot() : CmdMessage(ASEBA_MESSAGE_REBOOT, ASEBA_DEST_INVALID) { }
		Reboot(uint16 dest) : CmdMessage(ASEBA_MESSAGE_REBOOT, dest) { }
		
	protected:
		virtual operator const char * () const { return "reboot"; }
	};
	
	//! Put the node in sleep mode. After receiving this event, the node might go into sleep depending on its 
	//! internal state. If it goes to sleep, the node will not receive events any 
	//! more, until it is waken up by an implementation-specific action.
	class Sleep : public CmdMessage
	{
	public:
		Sleep() : CmdMessage(ASEBA_MESSAGE_SUSPEND_TO_RAM, ASEBA_DEST_INVALID) { }
		Sleep(uint16 dest) : CmdMessage(ASEBA_MESSAGE_SUSPEND_TO_RAM, dest) { }
	
	protected:
		virtual operator const char * () const { return "sleep"; }
	};
	
	/*@}*/
} // namespace Aseba

#endif

