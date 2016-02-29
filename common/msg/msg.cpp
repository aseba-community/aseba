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

#include "msg.h"
#include "endian.h"
#include "../utils/utils.h"
#include <typeinfo>
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
			
			registerMessageType<ListNodes>(ASEBA_MESSAGE_LIST_NODES);
			
			registerMessageType<NodePresent>(ASEBA_MESSAGE_NODE_PRESENT);
			
			registerMessageType<GetDescription>(ASEBA_MESSAGE_GET_DESCRIPTION);
			registerMessageType<GetNodeDescription>(ASEBA_MESSAGE_GET_NODE_DESCRIPTION);
			
			registerMessageType<Description>(ASEBA_MESSAGE_DESCRIPTION);
			registerMessageType<NamedVariableDescription>(ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION);
			registerMessageType<LocalEventDescription>(ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION);
			registerMessageType<NativeFunctionDescription>(ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION);
			registerMessageType<Disconnected>(ASEBA_MESSAGE_DISCONNECTED);
			registerMessageType<Variables>(ASEBA_MESSAGE_VARIABLES);
			registerMessageType<ArrayAccessOutOfBounds>(ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS);
			registerMessageType<DivisionByZero>(ASEBA_MESSAGE_DIVISION_BY_ZERO);
			registerMessageType<EventExecutionKilled>(ASEBA_MESSAGE_EVENT_EXECUTION_KILLED);
			registerMessageType<NodeSpecificError>(ASEBA_MESSAGE_NODE_SPECIFIC_ERROR);
			registerMessageType<ExecutionStateChanged>(ASEBA_MESSAGE_EXECUTION_STATE_CHANGED);
			registerMessageType<BreakpointSetResult>(ASEBA_MESSAGE_BREAKPOINT_SET_RESULT);
			
			registerMessageType<BootloaderReset>(ASEBA_MESSAGE_BOOTLOADER_RESET);
			registerMessageType<BootloaderReadPage>(ASEBA_MESSAGE_BOOTLOADER_READ_PAGE);
			registerMessageType<BootloaderWritePage>(ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE);
			registerMessageType<BootloaderPageDataWrite>(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE);
			
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
			registerMessageType<Reboot>(ASEBA_MESSAGE_REBOOT);
			registerMessageType<Sleep>(ASEBA_MESSAGE_SUSPEND_TO_RAM);
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
		void dumpKnownMessagesTypes(wostream &stream) const
		{
			stream << hex << showbase;
			for (map<uint16, CreatorFunc>::const_iterator it = messagesTypes.begin(); it != messagesTypes.end(); ++it)
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
		type(type)
	{
	}
	
	Message::~Message()
	{
	
	}
	
	void Message::serialize(Stream* stream) const
	{
		SerializationBuffer buffer;
		serializeSpecific(buffer);
		uint16 len = static_cast<uint16>(buffer.rawData.size());
		
		if (len > ASEBA_MAX_EVENT_ARG_SIZE)
		{
			cerr << "Message::serialize() : fatal error: message size exceed maximum packet size.\n";
			cerr << "message payload size: " << len << ", maximum packet payload size (excluding type): " << ASEBA_MAX_EVENT_ARG_SIZE << ", message type: " << hex << showbase << type << dec << noshowbase;
			cerr << endl;
			abort();
		}
		uint16 t;
		swapEndian(len);
		stream->write(&len, 2);
		t = swapEndianCopy(source);
		stream->write(&t, 2);
		t = swapEndianCopy(type);
		stream->write(&t, 2);
		if (buffer.rawData.size())
			stream->write(&buffer.rawData[0], buffer.rawData.size());
	}
	
	Message *Message::receive(Stream* stream)
	{
		// read header
		uint16 len, source, type;
		stream->read(&len, 2);
		swapEndian(len);
		stream->read(&source, 2);
		swapEndian(source);
		stream->read(&type, 2);
		swapEndian(type);
		
		// create message
		Message *message = messageTypesInitializer.createMessage(type);
		
		// preapare message
		message->source = source;
		message->type = type;
		SerializationBuffer buffer;
		buffer.rawData.resize(len);
		if (len)
			stream->read(&buffer.rawData[0], len);
		
		// deserialize it
		message->deserializeSpecific(buffer);
		
		if (buffer.readPos != buffer.rawData.size())
		{
			cerr << "Message::receive() : fatal error: message not fully read.\n";
			cerr << "type: " << type << ", readPos: " << buffer.readPos << ", rawData size: " << buffer.rawData.size() << endl;
			buffer.dump(wcerr);
			abort();
		}
		
		return message;
	}
	
	void Message::dump(wostream &stream) const
	{
		stream << hex << showbase << setw(4) << type << " ";
		stream << dec << noshowbase << *this << " from ";
		stream << source << " : ";
		dumpSpecific(stream);
	}
	
	template<typename T>
	void Message::SerializationBuffer::add(const T& val)
	{
		size_t pos = rawData.size();
		rawData.reserve(pos + sizeof(T));
		
		const T swappedVal(swapEndianCopy(val));
		const uint8 *ptr = reinterpret_cast<const uint8 *>(&swappedVal);

		copy(ptr, ptr + sizeof(T), back_inserter(rawData));
	}
	
	template<>
	void Message::SerializationBuffer::add(const string& val)
	{
		if (val.length() > 255)
		{
			cerr << "Message::SerializationBuffer::add<string>() : fatal error, string length exceeds 255 characters.\n";
			cerr << "string size: " << val.length();
			cerr << endl;
			dump(wcerr);
			abort();
		}
		
		add(static_cast<uint8>(val.length()));
		for (size_t i = 0; i < val.length(); i++)
			add(val[i]);
	}
	
	template<typename T>
	T Message::SerializationBuffer::get()
	{
		if (readPos + sizeof(T) > rawData.size())
		{
			cerr << "Message::SerializationBuffer::get<" << typeid(T).name() << ">() : fatal error: attempt to overread.\n";
			cerr << "readPos: " << readPos << ", rawData size: " << rawData.size() << ", element size: " << sizeof(T);
			cerr << endl;
			dump(wcerr);
			abort();
		}
		
		size_t pos = readPos;
		readPos += sizeof(T);
		T val;
		uint8 *ptr = reinterpret_cast<uint8 *>(&val);
		copy(rawData.begin() + pos, rawData.begin() + pos + sizeof(T), ptr);
		swapEndian(val);
		return val;
	}
	
	template<>
	string Message::SerializationBuffer::get()
	{
		string s;
		size_t len = get<uint8>();
		s.resize(len);
		for (size_t i = 0; i < len; i++)
			s[i] = get<uint8>();
		return s;
	}
	
	void Message::SerializationBuffer::dump(std::wostream &stream) const
	{
		for (size_t i = 0; i < rawData.size(); ++i)
			stream << (unsigned)(rawData[i]) << " (" << rawData[i] << "), ";
		stream << endl;
	}
	
	//

	UserMessage::UserMessage(uint16 type, const sint16* data, const size_t length):
		Message(type)
	{
		this->data.resize(length);
		copy(data, data+length, this->data.begin());
	}
	
	void UserMessage::serializeSpecific(SerializationBuffer& buffer) const
	{
		for (size_t i = 0; i < data.size(); i++)
			buffer.add(data[i]);
	}
	
	void UserMessage::deserializeSpecific(SerializationBuffer& buffer)
	{
		if (buffer.rawData.size() % 2 != 0)
		{
			cerr << "UserMessage::deserializeSpecific(SerializationBuffer& buffer) : fatal error: odd size.\n";
			cerr << "message size: " << buffer.rawData.size() << ", message type: " << type;
			cerr << endl;
			abort();
		}
		data.resize(buffer.rawData.size() / 2);
		
		for (size_t i = 0; i < data.size(); i++)
			data[i] = buffer.get<sint16>();
	}
	
	void UserMessage::dumpSpecific(wostream &stream) const
	{
		stream << dec << "user message of size " << data.size() << " : ";
		for (size_t i = 0 ; i < data.size(); i++)
			stream << setw(4) << data[i] << " ";
		stream << dec << setfill(wchar_t(' '));
	}
	
	//
	
	void CmdMessage::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(dest);
	}
	
	void CmdMessage::deserializeSpecific(SerializationBuffer& buffer)
	{
		dest = buffer.get<uint16>();
	}
	
	void CmdMessage::dumpSpecific(wostream &stream) const
	{
		stream << "dest " << dest << " ";
	}
	
	//
	
	void BootloaderDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pageSize);
		buffer.add(pagesStart);
		buffer.add(pagesCount);
	}
	
	void BootloaderDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		pageSize = buffer.get<uint16>();
		pagesStart = buffer.get<uint16>();
		pagesCount = buffer.get<uint16>();
	}
	
	void BootloaderDescription::dumpSpecific(wostream &stream) const
	{
		stream << pagesCount << " pages of size " << pageSize << " starting at page " << pagesStart;
	}
	
	
	//
	
	void BootloaderDataRead::serializeSpecific(SerializationBuffer& buffer) const
	{
		for (size_t i = 0 ; i < sizeof(data); i++)
			buffer.add(data[i]);
	}
	
	void BootloaderDataRead::deserializeSpecific(SerializationBuffer& buffer)
	{
		for (size_t i = 0 ; i < sizeof(data); i++)
			data[i] = buffer.get<uint8>();
	}
	
	void BootloaderDataRead::dumpSpecific(wostream &stream) const
	{
		stream << hex << setfill(wchar_t('0'));
		for (size_t i = 0 ; i < sizeof(data); i++)
			stream << setw(2) << (unsigned)data[i] << " ";
		stream << dec << setfill(wchar_t(' '));
	}
	
	//
	
	void BootloaderAck::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(errorCode);
		if (errorCode == 2)
			buffer.add(errorAddress);
	}
	
	void BootloaderAck::deserializeSpecific(SerializationBuffer& buffer)
	{
		errorCode = buffer.get<uint16>();
		if (errorCode == 2)
			errorAddress = buffer.get<uint16>();
	}
	
	void BootloaderAck::dumpSpecific(wostream &stream) const
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
	
	void ListNodes::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(version);
	}
	
	void ListNodes::deserializeSpecific(SerializationBuffer& buffer)
	{
		version = buffer.get<uint16>();
	}
	
	void ListNodes::dumpSpecific(std::wostream  &stream) const
	{
		stream << "protocol version " << version;
	}
	
	//
	
	void NodePresent::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(version);
	}
	
	void NodePresent::deserializeSpecific(SerializationBuffer& buffer)
	{
		version = buffer.get<uint16>();
	}
	
	void NodePresent::dumpSpecific(std::wostream  &stream) const
	{
		stream << "protocol version " << version;
	}
	
	//
	
	void GetDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(version);
	}
	
	void GetDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		version = buffer.get<uint16>();
	}
	
	void GetDescription::dumpSpecific(std::wostream  &stream) const
	{
		stream << "protocol version " << version;
	}
	
	//
	
	void GetNodeDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(version);
	}
	
	void GetNodeDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		version = buffer.get<uint16>();
	}
	
	void GetNodeDescription::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "protocol version " << version;
	}
	
	//
	
	void Description::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(WStringToUTF8(name));
		buffer.add(static_cast<uint16>(protocolVersion));
		
		buffer.add(static_cast<uint16>(bytecodeSize));
		buffer.add(static_cast<uint16>(stackSize));
		buffer.add(static_cast<uint16>(variablesSize));
		
		buffer.add(static_cast<uint16>(namedVariables.size()));
		// named variables are sent separately
		
		buffer.add(static_cast<uint16>(localEvents.size()));
		// local events are sent separately
		
		buffer.add(static_cast<uint16>(nativeFunctions.size()));
		// native functions are sent separately
	}
	
	void Description::deserializeSpecific(SerializationBuffer& buffer)
	{
		name = UTF8ToWString(buffer.get<string>());
		protocolVersion = buffer.get<uint16>();
		
		bytecodeSize = buffer.get<uint16>();
		stackSize = buffer.get<uint16>();
		variablesSize = buffer.get<uint16>();
		
		namedVariables.resize(buffer.get<uint16>());
		// named variables are received separately
		
		localEvents.resize(buffer.get<uint16>());
		// local events are received separately
		
		nativeFunctions.resize(buffer.get<uint16>());
		// native functions are received separately
	}
	
	void Description::dumpSpecific(wostream &stream) const
	{
		stream << "Node " << name << " using protocol version " << protocolVersion << "\n";
		stream << "bytecode: " << bytecodeSize << ", stack: " << stackSize << ", variables: " << variablesSize;
		
		stream << "\nnamed variables: " << namedVariables.size() << "\n";
		// named variables are available separately
		
		stream << "\nlocal events: " << localEvents.size() << "\n";
		// local events are available separately
		
		stream << "native functions: " << nativeFunctions.size();
		// native functions are available separately
	}
	
	//
	
	void NamedVariableDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(static_cast<sint16>(size));
		buffer.add(WStringToUTF8(name));
	}
	
	void NamedVariableDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		size = buffer.get<uint16>();
		name = UTF8ToWString(buffer.get<string>());
	}
	
	void NamedVariableDescription::dumpSpecific(std::wostream  &stream) const
	{
		stream << name << " of size " << size;
	}
	
	//
	
	void LocalEventDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(WStringToUTF8(name));
		buffer.add(WStringToUTF8(description));
	}
	
	void LocalEventDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		name = UTF8ToWString(buffer.get<string>());
		description = UTF8ToWString(buffer.get<string>());
	}
	
	void LocalEventDescription::dumpSpecific(std::wostream  &stream) const
	{
		stream << name << " : " << description;
	}
	
	//
	
	void NativeFunctionDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(WStringToUTF8(name));
		buffer.add(WStringToUTF8(description));
		
		buffer.add(static_cast<uint16>(parameters.size()));
		for (size_t j = 0; j < parameters.size(); j++)
		{
			buffer.add(static_cast<sint16>(parameters[j].size));
			buffer.add(WStringToUTF8(parameters[j].name));
		}
	}
	
	void NativeFunctionDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		name = UTF8ToWString(buffer.get<string>());
		description = UTF8ToWString(buffer.get<string>());
		
		parameters.resize(buffer.get<uint16>());
		for (size_t j = 0; j < parameters.size(); j++)
		{
			parameters[j].size = buffer.get<sint16>();
			parameters[j].name = UTF8ToWString(buffer.get<string>());
		}
	}
	
	void NativeFunctionDescription::dumpSpecific(std::wostream  &stream) const
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
	
	void Variables::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(start);
		for (size_t i = 0; i < variables.size(); i++)
			buffer.add(variables[i]);
	}
	
	void Variables::deserializeSpecific(SerializationBuffer& buffer)
	{
		start = buffer.get<uint16>();
		variables.resize((buffer.rawData.size() - buffer.readPos) / 2);
		for (size_t i = 0; i < variables.size(); i++)
			variables[i] = buffer.get<sint16>();
	}
	
	void Variables::dumpSpecific(wostream &stream) const
	{
		stream << "start " << start << ", variables vector of size " << variables.size();
		/*for (size_t i = 0; i < variables.size(); i++)
			stream << "\n " << i << " : " << variables[i];
		stream << "\n";*/
	}
	
	//
	
	void ArrayAccessOutOfBounds::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
		buffer.add(size);
		buffer.add(index);
	}
	
	void ArrayAccessOutOfBounds::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16>();
		size = buffer.get<uint16>();
		index = buffer.get<uint16>();
	}
	
	void ArrayAccessOutOfBounds::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << ", size " << size << ", index " << index ;
	}
	
	//
	
	void DivisionByZero::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
	}
	
	void DivisionByZero::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16>();
	}
	
	void DivisionByZero::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc;
	}
	
	//
	
	void EventExecutionKilled::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
	}
	
	void EventExecutionKilled::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16>();
	}
	
	void EventExecutionKilled::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc;
	}
	
	//
	
	void NodeSpecificError::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
		buffer.add(WStringToUTF8(message));
	}
	
	void NodeSpecificError::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16>();
		message = UTF8ToWString(buffer.get<std::string>());
	}
	
	void NodeSpecificError::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << " " << message;
	}
	
	//
	
	void ExecutionStateChanged::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
		buffer.add(flags);
	}
	
	void ExecutionStateChanged::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16>();
		flags = buffer.get<uint16>();
	}
	
	void ExecutionStateChanged::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << ", flags ";
		stream << hex << showbase << setw(4) << flags;
		stream << dec << noshowbase;
	}
	
	//
	
	void BreakpointSetResult::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
		buffer.add(success);
	}
	
	void BreakpointSetResult::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16>();
		success = buffer.get<uint16>();
	}
	
	void BreakpointSetResult::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << ", success " << success;
	}
	
	//
	
	void BootloaderReadPage::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(pageNumber);
	}
	
	void BootloaderReadPage::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		pageNumber = buffer.get<uint16>();
	}
	
	void BootloaderReadPage::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "page " << pageNumber;
	}
	
	//
	
	void BootloaderWritePage::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(pageNumber);
	}
	
	void BootloaderWritePage::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		pageNumber = buffer.get<uint16>();
	}
	
	void BootloaderWritePage::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "page " << pageNumber;
	}
	
	//
	
	void BootloaderPageDataWrite::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		for (size_t i = 0 ; i < sizeof(data); i++)
			buffer.add(data[i]);
	}
	
	void BootloaderPageDataWrite::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		for (size_t i = 0 ; i < sizeof(data); i++)
			data[i] = buffer.get<uint8>();
	}
	
	void BootloaderPageDataWrite::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << hex << setfill(wchar_t('0'));
		for (size_t i = 0 ; i < sizeof(data); i++)
			stream << setw(2) << (unsigned)data[i] << " ";
		stream << dec << setfill(wchar_t(' '));
	}
	
	//
	
	void SetBytecode::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(start);
		for (size_t i = 0; i < bytecode.size(); i++)
			buffer.add(bytecode[i]);
	}
	
	void SetBytecode::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		start = buffer.get<uint16>();
		bytecode.resize((buffer.rawData.size() - buffer.readPos) / 2);
		for (size_t i = 0; i < bytecode.size(); i++)
			bytecode[i] = buffer.get<uint16>();
	}
	
	void SetBytecode::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << bytecode.size() << " words of bytecode of starting at " << start;
	}
	
	void sendBytecode(Dashel::Stream* stream, uint16 dest, const std::vector<uint16>& bytecode)
	{
		unsigned bytecodePayloadSize = ASEBA_MAX_EVENT_ARG_COUNT-2;
		unsigned bytecodeStart = 0;
		unsigned bytecodeCount = bytecode.size();
		
		while (bytecodeCount > bytecodePayloadSize)
		{
			SetBytecode setBytecodeMessage(dest, bytecodeStart);
			setBytecodeMessage.bytecode.resize(bytecodePayloadSize);
			copy(bytecode.begin()+bytecodeStart, bytecode.begin()+bytecodeStart+bytecodePayloadSize, setBytecodeMessage.bytecode.begin());
			setBytecodeMessage.serialize(stream);
			
			bytecodeStart += bytecodePayloadSize;
			bytecodeCount -= bytecodePayloadSize;
		}
		
		{
			SetBytecode setBytecodeMessage(dest, bytecodeStart);
			setBytecodeMessage.bytecode.resize(bytecodeCount);
			copy(bytecode.begin()+bytecodeStart, bytecode.end(), setBytecodeMessage.bytecode.begin());
			setBytecodeMessage.serialize(stream);
		}
	}
	
	void sendBytecode(std::vector<Message*>& messagesVector, uint16 dest, const std::vector<uint16>& bytecode)
	{
		unsigned bytecodePayloadSize = ASEBA_MAX_EVENT_ARG_COUNT-2;
		unsigned bytecodeStart = 0;
		unsigned bytecodeCount = bytecode.size();
		
		while (bytecodeCount > bytecodePayloadSize)
		{
			SetBytecode* setBytecodeMessage = new SetBytecode(dest, bytecodeStart);
			setBytecodeMessage->bytecode.resize(bytecodePayloadSize);
			copy(bytecode.begin()+bytecodeStart, bytecode.begin()+bytecodeStart+bytecodePayloadSize, setBytecodeMessage->bytecode.begin());
			messagesVector.push_back(setBytecodeMessage);
			
			bytecodeStart += bytecodePayloadSize;
			bytecodeCount -= bytecodePayloadSize;
		}
		
		{
			SetBytecode* setBytecodeMessage = new SetBytecode(dest, bytecodeStart);
			setBytecodeMessage->bytecode.resize(bytecodeCount);
			copy(bytecode.begin()+bytecodeStart, bytecode.end(), setBytecodeMessage->bytecode.begin());
			messagesVector.push_back(setBytecodeMessage);
		}
	}
	
	//
	
	void BreakpointSet::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(pc);
	}
	
	void BreakpointSet::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		pc = buffer.get<uint16>();
	}
	
	void BreakpointSet::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "pc " << pc;
	}
	
	//
	
	void BreakpointClear::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(pc);
	}
	
	void BreakpointClear::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		pc = buffer.get<uint16>();
	}
	
	void BreakpointClear::dumpSpecific(wostream &stream) const
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
	
	void GetVariables::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(start);
		buffer.add(length);
	}
	
	void GetVariables::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		start = buffer.get<uint16>();
		length = buffer.get<uint16>();
	}
	
	void GetVariables::dumpSpecific(wostream &stream) const
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
	
	void SetVariables::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(start);
		for (size_t i = 0; i < variables.size(); i++)
			buffer.add(variables[i]);
	}
	
	void SetVariables::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		start = buffer.get<uint16>();
		variables.resize((buffer.rawData.size() - buffer.readPos) / 2);
		for (size_t i = 0; i < variables.size(); i++)
			variables[i] = buffer.get<sint16>();
	}
	
	void SetVariables::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "start " << start << ", variables vector of size " << variables.size();
	}
} // namespace Aseba
