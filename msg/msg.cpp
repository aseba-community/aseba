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

#include "msg.h"
#include "../utils/utils.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <dashel/dashel.h>

using namespace std;

namespace Aseba
{
	using namespace Dashel;
	
	//! Static class that fills a table of known messages types in its constructor
	class MessageTypesInitializer
	{
	public:
		//! Constructor, register all known messages types
		MessageTypesInitializer()
		{
			registerMessageType<BootloaderDescription>(ASEBA_MESSAGE_BOOTLOADER_DESCRIPTION);
			registerMessageType<BootloaderDataRead>(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_READ);
			registerMessageType<BootloaderAck>(ASEBA_MESSAGE_BOOTLOADER_ACK);
			
			registerMessageType<Description>(ASEBA_MESSAGE_DESCRIPTION);
			registerMessageType<NativeFunctionDescription>(ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION);
			registerMessageType<Disconnected>(ASEBA_MESSAGE_DISCONNECTED);
			registerMessageType<Variables>(ASEBA_MESSAGE_VARIABLES);
			registerMessageType<ArrayAccessOutOfBounds>(ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS);
			registerMessageType<DivisionByZero>(ASEBA_MESSAGE_DIVISION_BY_ZERO);
			registerMessageType<NodeSpecificError>(ASEBA_MESSAGE_NODE_SPECIFIC_ERROR);
			registerMessageType<ExecutionStateChanged>(ASEBA_MESSAGE_EXECUTION_STATE_CHANGED);
			registerMessageType<BreakpointSetResult>(ASEBA_MESSAGE_BREAKPOINT_SET_RESULT);
			
			registerMessageType<GetDescription>(ASEBA_MESSAGE_GET_DESCRIPTION);
			
			registerMessageType<BootloaderReset>(ASEBA_MESSAGE_BOOTLOADER_RESET);
			registerMessageType<BootloaderReadPage>(ASEBA_MESSAGE_BOOTLOADER_READ_PAGE);
			registerMessageType<BootloaderWritePage>(ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE);
			registerMessageType<BootloaderPageDataWrite>(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE);
			
			//registerMessageType<AttachDebugger>(ASEBA_MESSAGE_ATTACH_DEBUGGER);
			//registerMessageType<DetachDebugger>(ASEBA_MESSAGE_DETACH_DEBUGGER);
			registerMessageType<SetBytecode>(ASEBA_MESSAGE_SET_BYTECODE);
			registerMessageType<Reset>(ASEBA_MESSAGE_RESET);
			registerMessageType<Run>(ASEBA_MESSAGE_RUN);
			registerMessageType<Pause>(ASEBA_MESSAGE_PAUSE);
			registerMessageType<Step>(ASEBA_MESSAGE_STEP);
			registerMessageType<Stop>(ASEBA_MESSAGE_STOP);
			registerMessageType<GetExecutionState>(ASEBA_MESSAGE_GET_EXECUTION_STATE);
			registerMessageType<BreakpointSet>(ASEBA_MESSAGE_BREAKPOINT_SET);
			registerMessageType<BreakpointClear>(ASEBA_MESSAGE_BREAKPOINT_CLEAR);
			registerMessageType<BreakpointClearAll>(ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL);
			registerMessageType<GetVariables>(ASEBA_MESSAGE_GET_VARIABLES);
			registerMessageType<SetVariables>(ASEBA_MESSAGE_SET_VARIABLES);
			registerMessageType<WriteBytecode>(ASEBA_MESSAGE_WRITE_BYTECODE);
			registerMessageType<ResetIntoBootloader>(ASEBA_MESSAGE_RESET_INTO_BOOTLOADER);
		}
		
		//! Register a message type by storing a pointer to its constructor
		template<typename Sub>
		void registerMessageType(uint16 type)
		{
			messagesTypes[type] = &Creator<Sub>;
		}
		
		//! Create an instance of a registered message type
		Message *createMessage(uint16 type)
		{
			if (messagesTypes.find(type) == messagesTypes.end())
				return new UserMessage;
			else
				return messagesTypes[type]();
		}
		
		//! Print the list of registered messages types to stream
		void dumpKnownMessagesTypes(ostream &stream)
		{
			stream << hex << showbase;
			for (map<uint16, CreatorFunc>::iterator it = messagesTypes.begin(); it != messagesTypes.end(); ++it)
				stream << "\t" << setw(4) << it->first << "\n";
			stream << dec << noshowbase;
		}
	
	protected:
		//! Pointer to constructor of class Message
		typedef Message* (*CreatorFunc)();
		map<uint16, CreatorFunc> messagesTypes; //!< table of known messages types
		
		//! Create a new message of type Sub
		template<typename Sub>
		static Message* Creator()
		{
			return new Sub();
		}
	} messageTypesInitializer; //!< static instance, used only to have its constructor called on startup
	
	//
	
	Message::Message(uint16 type) :
		source(ASEBA_DEST_DEBUG),
		type(type),
		readPos(0)
	{
	}
	
	Message::~Message()
	{
	
	}
	
	void Message::serialize(Stream* stream)
	{
		rawData.resize(0);
		serializeSpecific();
		uint16 len = static_cast<uint16>(rawData.size());
		
		if (len + 6 > ASEBA_MAX_PACKET_SIZE)
		{
			cerr << "Message::serialize() : fatal error: message size exceed maximum packet size.\n";
			cerr << "message size: " << len + 6 << ", maximum packet size: " << ASEBA_MAX_PACKET_SIZE << ", message type: " << type;
			cerr << endl;
			abort();
		}
		
		stream->write(&len, 2);
		stream->write(&source, 2);
		stream->write(&type, 2);
		if(rawData.size())
			stream->write(&rawData[0], rawData.size());
	}
	
	Message *Message::receive(Stream* stream)
	{
		// read header
		uint16 len, source, type;
		stream->read(&len, 2);
		stream->read(&source, 2);
		stream->read(&type, 2);
		
		// create message
		Message *message = messageTypesInitializer.createMessage(type);
		
		// preapare message
		message->source = source;
		message->type = type;
		message->rawData.resize(len);
		if (len)
			stream->read(&message->rawData[0], len);
		message->readPos = 0;
		
		// deserialize it
		message->deserializeSpecific();
		
		if (message->readPos != message->rawData.size())
		{
			cerr << "Message::receive() : fatal error: message not fully read.\n";
			cerr << "type: " << type << ", readPos: " << message->readPos << ", rawData size: " << message->rawData.size();
			cerr << endl;
			abort();
		}
		
		return message;
	}
	
	void Message::dump(ostream &stream)
	{
		stream << hex << showbase << setw(4) << type << " ";
		stream << dec << noshowbase << *this << " from ";
		stream << source << " : ";
		dumpSpecific(stream);
	}
	
	template<typename T>
	void Message::add(const T& val)
	{
		size_t pos = rawData.size();
		rawData.reserve(pos + sizeof(T));
		const uint8 *ptr = reinterpret_cast<const uint8 *>(&val);
		copy(ptr, ptr + sizeof(T), back_inserter(rawData));
	}
	
	template<>
	void Message::add(const string& val)
	{
		if (val.length() > 255)
		{
			cerr << "Message::add<string>() : fatal error, string length exceeds 255 characters.\n";
			cerr << "string size: " << val.length();
			cerr << endl;
			abort();
		}
		
		add(static_cast<uint8>(val.length()));
		for (size_t i = 0; i < val.length(); i++)
			add(val[i]);
	}
	
	template<typename T>
	T Message::get()
	{
		if (readPos + sizeof(T) > rawData.size())
		{
			cerr << "Message<" << typeid(T).name() << ">::get() : fatal error: attempt to overread.\n";
			cerr << "type: " << type << ", readPos: " << readPos << ", rawData size: " << rawData.size() << ", element size: " << sizeof(T);
			cerr << endl;
			abort();
		}
		
		size_t pos = readPos;
		readPos += sizeof(T);
		T val;
		uint8 *ptr = reinterpret_cast<uint8 *>(&val);
		copy(rawData.begin() + pos, rawData.begin() + pos + sizeof(T), ptr);
		return val;
	}
	
	template<>
	string Message::get()
	{
		string s;
		size_t len = get<uint8>();
		s.resize(len);
		for (size_t i = 0; i < len; i++)
			s[i] = get<uint8>();
		return s;
	}
	
	//
	
	void UserMessage::serializeSpecific()
	{
		for (size_t i = 0; i < data.size(); i++)
			add(data[i]);
	}
	
	void UserMessage::deserializeSpecific()
	{
		if (rawData.size() % 2 != 0)
		{
			cerr << "UserMessage::deserializeSpecific() : fatal error: odd size.\n";
			cerr << "message size: " << rawData.size() << ", message type: " << type;
			cerr << endl;
			abort();
		}
		data.resize(rawData.size() / 2);
		
		for (size_t i = 0; i < data.size(); i++)
			data[i] = get<sint16>();
	}
	
	void UserMessage::dumpSpecific(ostream &stream)
	{
		stream << "user message of size " << data.size() << " : ";
		stream << hex << setfill('0');
		for (size_t i = 0 ; i < data.size(); i++)
			stream << setw(4) << (unsigned)data[i] << " ";
		stream << dec << setfill(' ');
	}
	
	//
	
	void BootloaderDescription::serializeSpecific()
	{
		add(pageSize);
		add(pagesStart);
		add(pagesCount);
	}
	
	void BootloaderDescription::deserializeSpecific()
	{
		pageSize = get<uint16>();
		pagesStart = get<uint16>();
		pagesCount = get<uint16>();
	}
	
	void BootloaderDescription::dumpSpecific(ostream &stream)
	{
		stream << pagesCount << " pages of size " << pageSize << " starting at page " << pagesStart;
	}
	
	
	//
	
	void BootloaderDataRead::serializeSpecific()
	{
		for (size_t i = 0 ; i < sizeof(data); i++)
			add(data[i]);
	}
	
	void BootloaderDataRead::deserializeSpecific()
	{
		for (size_t i = 0 ; i < sizeof(data); i++)
			data[i] = get<uint8>();
	}
	
	void BootloaderDataRead::dumpSpecific(ostream &stream)
	{
		stream << hex << setfill('0');
		for (size_t i = 0 ; i < sizeof(data); i++)
			stream << setw(2) << (unsigned)data[i] << " ";
		stream << dec << setfill(' ');
	}
	
	//
	
	void BootloaderAck::serializeSpecific()
	{
		add(errorCode);
		if (errorCode == 2)
			add(errorAddress);
	}
	
	void BootloaderAck::deserializeSpecific()
	{
		errorCode = get<uint16>();
		if (errorCode == 2)
			errorAddress = get<uint16>();
	}
	
	void BootloaderAck::dumpSpecific(ostream &stream)
	{
		switch (errorCode)
		{
			case SUCCESS: stream << "success"; break;
			case ERROR_INVALID_FRAME_SIZE: stream << "error, invalid frame size"; break;
			case ERROR_PROGRAMMING_FAILED: stream << "error, programming failed at low address " << hex << showbase << errorAddress; break;
			case ERROR_NOT_PROGRAMMING: stream << "error, not programming"; break;
			default: stream << "unknown error"; break;
		}
		stream << dec << noshowbase;
	}
	
	//
	
	void GetDescription::serializeSpecific()
	{
		add(version);
	}
	
	void GetDescription::deserializeSpecific()
	{
		version = get<uint16>();
	}
	
	void GetDescription::dumpSpecific(std::ostream &stream)
	{
		stream << "protocol version " << version;
	}
	
	//
	
	void Description::serializeSpecific()
	{
		add(name);
		add(protocolVersion);
		
		add(bytecodeSize);
		add(stackSize);
		add(variablesSize);
		
		add(static_cast<uint16>(namedVariables.size()));
		for (size_t i = 0; i < namedVariables.size(); i++)
		{
			add(namedVariables[i].size);
			add(namedVariables[i].name);
		}
		
		add(static_cast<uint16>(localEvents.size()));
		for (size_t i = 0; i < localEvents.size(); i++)
		{
			add(localEvents[i]);
		}
		
		add(static_cast<uint16>(nativeFunctions.size()));
		// native functions are sent separately
	}
	
	void Description::deserializeSpecific()
	{
		name = get<string>();
		protocolVersion = get<uint16>();
		
		bytecodeSize = get<uint16>();
		stackSize = get<uint16>();
		variablesSize = get<uint16>();
		
		namedVariables.resize(get<uint16>());
		for (size_t i = 0; i < namedVariables.size(); i++)
		{
			namedVariables[i].size = get<uint16>();
			namedVariables[i].name = get<string>();
		}
		
		localEvents.resize(get<uint16>());
		for (size_t i = 0; i < localEvents.size(); i++)
		{
			localEvents[i] = get<string>();
		}
		
		nativeFunctions.resize(get<uint16>());
		// native functions are received separately
	}
	
	void Description::dumpSpecific(ostream &stream)
	{
		stream << "Node " << name << " using protocol version " << protocolVersion << "\n";
		stream << "bytecode: " << bytecodeSize << ", stack: " << stackSize << ", variables: " << variablesSize;
		for (size_t i = 0; i < namedVariables.size(); i++)
			stream << "\n\t" << namedVariables[i].name << " : " << namedVariables[i].size;
		
		stream << "local events: " << localEvents.size();
		for (size_t i = 0; i < localEvents.size(); i++)
			stream << "\n\t" << localEvents[i];
		
		stream << "native functions: " << nativeFunctions.size();
		// native functions are available separately
	}
	
	//
	
	void NativeFunctionDescription::serializeSpecific()
	{
		add(name);
		add(description);
		
		add(static_cast<uint16>(parameters.size()));
		for (size_t j = 0; j < parameters.size(); j++)
		{
			add(parameters[j].size);
			add(parameters[j].name);
		}
	}
	
	void NativeFunctionDescription::deserializeSpecific()
	{
		name = get<string>();
		description = get<string>();
		
		parameters.resize(get<uint16>());
		for (size_t j = 0; j < parameters.size(); j++)
		{
			parameters[j].size = get<sint16>();
			parameters[j].name = get<string>();
		}
	}
	
	void NativeFunctionDescription::dumpSpecific(std::ostream &stream)
	{
		stream << name << " (";
		for (size_t j = 0; j < parameters.size(); j++)
		{
			stream << parameters[j].name;
			stream << "<" << parameters[j].size << ">";
			if (j + 1 < parameters.size())
				stream << ", ";
		}
		stream << ") : " << description;
	}
	
	//
	
	void Variables::serializeSpecific()
	{
		add(start);
		for (size_t i = 0; i < variables.size(); i++)
			add(variables[i]);
	}
	
	void Variables::deserializeSpecific()
	{
		start = get<uint16>();
		variables.resize((rawData.size() - readPos) / 2);
		for (size_t i = 0; i < variables.size(); i++)
			variables[i] = get<sint16>();
	}
	
	void Variables::dumpSpecific(ostream &stream)
	{
		stream << "start " << start << ", variables vector of size " << variables.size();
		/*for (size_t i = 0; i < variables.size(); i++)
			stream << "\n " << i << " : " << variables[i];
		stream << "\n";*/
	}
	
	//
	
	void ArrayAccessOutOfBounds::serializeSpecific()
	{
		add(pc);
		add(index);
	}
	
	void ArrayAccessOutOfBounds::deserializeSpecific()
	{
		pc = get<uint16>();
		index = get<uint16>();
	}
	
	void ArrayAccessOutOfBounds::dumpSpecific(ostream &stream)
	{
		stream << "pc " << pc << ", index " << index;
	}
	
	//
	
	void DivisionByZero::serializeSpecific()
	{
		add(pc);
	}
	
	void DivisionByZero::deserializeSpecific()
	{
		pc = get<uint16>();
	}
	
	void DivisionByZero::dumpSpecific(ostream &stream)
	{
		stream << "pc " << pc;
	}
	
	//
	
	void NodeSpecificError::serializeSpecific()
	{
		add(pc);
		add(message);
	}
	
	void NodeSpecificError::deserializeSpecific()
	{
		pc = get<uint16>();
		message = get<std::string>();
	}
	
	void NodeSpecificError::dumpSpecific(ostream &stream)
	{
		stream << "pc " << pc << " " << message;
	}
	
	//
	
	void ExecutionStateChanged::serializeSpecific()
	{
		add(pc);
		add(flags);
	}
	
	void ExecutionStateChanged::deserializeSpecific()
	{
		pc = get<uint16>();
		flags = get<uint16>();
	}
	
	void ExecutionStateChanged::dumpSpecific(ostream &stream)
	{
		stream << "pc " << pc << ", flags ";
		stream << hex << showbase << setw(4) << flags;
		stream << dec << noshowbase;
	}
	
	//
	
	void BreakpointSetResult::serializeSpecific()
	{
		add(pc);
		add(success);
	}
	
	void BreakpointSetResult::deserializeSpecific()
	{
		pc = get<uint16>();
		success = get<uint16>();
	}
	
	void BreakpointSetResult::dumpSpecific(ostream &stream)
	{
		stream << "pc " << pc << ", success " << success;
	}
	
	//
	
	void CmdMessage::serializeSpecific()
	{
		add(dest);
	}
	
	void CmdMessage::deserializeSpecific()
	{
		dest = get<uint16>();
	}
	
	void CmdMessage::dumpSpecific(ostream &stream)
	{
		stream << "dest " << dest << " ";
	}
	
	//
	
	void BootloaderReadPage::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		add(pageNumber);
	}
	
	void BootloaderReadPage::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		pageNumber = get<uint16>();
	}
	
	void BootloaderReadPage::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "page " << pageNumber;
	}
	
	//
	
	void BootloaderWritePage::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		add(pageNumber);
	}
	
	void BootloaderWritePage::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		pageNumber = get<uint16>();
	}
	
	void BootloaderWritePage::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "page " << pageNumber;
	}
	
	//
	
	void BootloaderPageDataWrite::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		for (size_t i = 0 ; i < sizeof(data); i++)
			add(data[i]);
	}
	
	void BootloaderPageDataWrite::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		for (size_t i = 0 ; i < sizeof(data); i++)
			data[i] = get<uint8>();
	}
	
	void BootloaderPageDataWrite::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << hex << setfill('0');
		for (size_t i = 0 ; i < sizeof(data); i++)
			stream << setw(2) << (unsigned)data[i] << " ";
		stream << dec << setfill(' ');
	}
	
	//
	
	void SetBytecode::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		add(start);
		for (size_t i = 0; i < bytecode.size(); i++)
			add(bytecode[i]);
	}
	
	void SetBytecode::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		start = get<uint16>();
		bytecode.resize((rawData.size() - readPos) / 2);
		for (size_t i = 0; i < bytecode.size(); i++)
			bytecode[i] = get<uint16>();
	}
	
	void SetBytecode::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << bytecode.size() << " words of bytecode of starting at " << start;
	}
	
	//
	
	void BreakpointSet::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		add(pc);
	}
	
	void BreakpointSet::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		pc = get<uint16>();
	}
	
	void BreakpointSet::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "pc " << pc;
	}
	
	//
	
	void BreakpointClear::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		add(pc);
	}
	
	void BreakpointClear::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		pc = get<uint16>();
	}
	
	void BreakpointClear::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "pc " << pc;
	}
	
	//
	
	GetVariables::GetVariables(uint16 dest, uint16 start, uint16 length) :
		CmdMessage(ASEBA_MESSAGE_GET_VARIABLES, dest),
		start(start),
		length(length)
	{
	}
	
	void GetVariables::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		add(start);
		add(length);
	}
	
	void GetVariables::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		start = get<uint16>();
		length = get<uint16>();
	}
	
	void GetVariables::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "start " << start << ", length " << length;
	}
	
	//
	
	SetVariables::SetVariables(uint16 dest, uint16 start, const VariablesVector& variables) :
		CmdMessage(ASEBA_MESSAGE_SET_VARIABLES, dest),
		start(start),
		variables(variables)
	{
	}
	
	void SetVariables::serializeSpecific()
	{
		CmdMessage::serializeSpecific();
		
		add(start);
		for (size_t i = 0; i < variables.size(); i++)
			add(variables[i]);
	}
	
	void SetVariables::deserializeSpecific()
	{
		CmdMessage::deserializeSpecific();
		
		start = get<uint16>();
		variables.resize((rawData.size() - readPos) / 2);
		for (size_t i = 0; i < variables.size(); i++)
			variables[i] = get<sint16>();
	}
	
	void SetVariables::dumpSpecific(ostream &stream)
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "start " << start << ", variables vector of size " << variables.size();
	}
}
