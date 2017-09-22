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
#include <iterator>
#include <utility>
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
		void registerMessageType(uint16_t type)
		{
			messagesTypes[type] = &Creator<Sub>;
		}
		
		//! Create an instance of a registered message type
		Message *createMessage(uint16_t type)
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
			for (const auto& messageTypeKV: messagesTypes)
				stream << "\t" << setw(4) << messageTypeKV.first << "\n";
			stream << dec << noshowbase;
		}
	
	protected:
		//! Pointer to constructor of class Message
		using CreatorFunc = Message *(*)();
		map<uint16_t, CreatorFunc> messagesTypes; //!< table of known messages types
		
		//! Create a new message of type Sub
		template<typename Sub>
		static Message* Creator()
		{
			return new Sub();
		}
	} messageTypesInitializer; //!< static instance, used only to have its constructor called on startup
	
	//
	
	//! Compare all data members
	bool operator ==(const TargetDescription::NamedVariable &lhs, const TargetDescription::NamedVariable &rhs)
	{
		return lhs.size == rhs.size && lhs.name == rhs.name;
	}
	
	//! Compare all data members
	bool operator ==(const TargetDescription::LocalEvent &lhs, const TargetDescription::LocalEvent &rhs)
	{
		return lhs.name == rhs.name && lhs.description == rhs.description;
	}
	
	//! Compare all data members
	bool operator ==(const TargetDescription::NativeFunctionParameter &lhs, const TargetDescription::NativeFunctionParameter &rhs)
	{
		return lhs.size == rhs.size && lhs.name == rhs.name;
	}
	
	//! Compare all data members
	bool operator ==(const TargetDescription::NativeFunction &lhs, const TargetDescription::NativeFunction &rhs)
	{
		return lhs.name == rhs.name && lhs.description == rhs.description && lhs.parameters == rhs.parameters;
	}
	
	//
	
	void Message::serialize(Stream* stream) const
	{
		SerializationBuffer buffer;
		serializeSpecific(buffer);
		auto len = static_cast<uint16_t>(buffer.rawData.size());
		
		if (len > ASEBA_MAX_EVENT_ARG_SIZE)
		{
			cerr << "Message::serialize() : fatal error: message size exceeds maximum packet size.\n";
			cerr << "message payload size: " << len << ", maximum packet payload size (excluding type): " << ASEBA_MAX_EVENT_ARG_SIZE << ", message type: " << hex << showbase << type << dec << noshowbase;
			cerr << endl;
			terminate();
		}
		uint16_t t;
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
		uint16_t len, source, type;
		stream->read(&len, 2);
		swapEndian(len);
		stream->read(&source, 2);
		swapEndian(source);
		stream->read(&type, 2);
		swapEndian(type);
		
		// read content
		SerializationBuffer buffer;
		buffer.rawData.resize(len);
		if (len)
			stream->read(&buffer.rawData[0], len);
		
		// deserialize message
		return create(source, type, buffer);
	}
	
	Message *Message::create(uint16_t source, uint16_t type, SerializationBuffer& buffer)
	{
		// create message
		Message *message = messageTypesInitializer.createMessage(type);
		
		// prepare message
		message->source = source;
		message->type = type;
		
		// deserialize it
		message->deserializeSpecific(buffer);
		
		if (buffer.readPos != buffer.rawData.size())
		{
			cerr << "Message::create() : fatal error: message not fully deserialized.\n";
			cerr << "type: " << type << ", readPos: " << buffer.readPos << ", rawData size: " << buffer.rawData.size() << endl;
			buffer.dump(wcerr);
			terminate();
		}
		
		return message;
	}

	Message* Message::clone() const
	{
		// create message
		Message *message = messageTypesInitializer.createMessage(type);

		// fill headers
		message->source = source;
		message->type = type;

		// copy content through the serialization buffer
		SerializationBuffer content;
		serializeSpecific(content);
		message->deserializeSpecific(content);

		return message;
	}
	
	void Message::dump(wostream &stream) const
	{
		stream << hex << setw(4) << setfill(wchar_t('0')) << type << " ";
		stream << dec << setfill(wchar_t(' ')) << *this << " from ";
		stream << source << " : ";
		dumpSpecific(stream);
	}
	
	bool operator ==(const Message &lhs, const Message &rhs)
	{
		return
			lhs.source == rhs.source &&
			lhs.type == rhs.type
		;
	}
	
	template<typename T>
	void Message::SerializationBuffer::add(const T& val)
	{
		const size_t pos = rawData.size();
		rawData.reserve(pos + sizeof(T));
		
		const T swappedVal(swapEndianCopy(val));
		const auto *ptr = reinterpret_cast<const uint8_t *>(&swappedVal);

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
			terminate();
		}
		
		add(static_cast<uint8_t>(val.length()));
		for (const uint8_t c: val)
			add(c);
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
			terminate();
		}
		
		size_t pos = readPos;
		readPos += sizeof(T);
		T val;
		auto *ptr = reinterpret_cast<uint8_t *>(&val);
		copy(rawData.cbegin() + pos, rawData.cbegin() + pos + sizeof(T), ptr);
		swapEndian(val);
		return val;
	}
	
	template<>
	string Message::SerializationBuffer::get()
	{
		string s;
		size_t len = get<uint8_t>();
		s.resize(len);
		for (auto& c: s)
			c = get<uint8_t>();
		return s;
	}
	
	void Message::SerializationBuffer::dump(std::wostream &stream) const
	{
		for (const auto word: rawData)
			stream << unsigned(word) << " (" << word << "), ";
		stream << endl;
	}
	
	//

	UserMessage::UserMessage(uint16_t type, const int16_t* data, const size_t length):
		Message(type)
	{
		this->data.resize(length);
		copy(data, data+length, this->data.begin());
	}
	
	void UserMessage::serializeSpecific(SerializationBuffer& buffer) const
	{
		for (const auto word: data)
			buffer.add(word);
	}
	
	void UserMessage::deserializeSpecific(SerializationBuffer& buffer)
	{
		if (buffer.rawData.size() % 2 != 0)
		{
			cerr << "UserMessage::deserializeSpecific(SerializationBuffer& buffer) : fatal error: odd size.\n";
			cerr << "message size: " << buffer.rawData.size() << ", message type: " << type;
			cerr << endl;
			terminate();
		}
		data.resize(buffer.rawData.size() / 2);
		
		for (auto& word: data)
			word = buffer.get<int16_t>();
	}
	
	void UserMessage::dumpSpecific(wostream &stream) const
	{
		stream << dec << "user message of size " << data.size() << " : ";
		for (const auto word: data)
			stream << setw(4) << word << " ";
		stream << dec << setfill(wchar_t(' '));
	}
	
	bool operator ==(const UserMessage &lhs, const UserMessage &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.data == rhs.data
		;
	}
	
	//
	
	void CmdMessage::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(dest);
	}
	
	void CmdMessage::deserializeSpecific(SerializationBuffer& buffer)
	{
		dest = buffer.get<uint16_t>();
	}
	
	void CmdMessage::dumpSpecific(wostream &stream) const
	{
		stream << "dest " << dest << " ";
	}
	
	bool operator ==(const CmdMessage &lhs, const CmdMessage &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.dest == rhs.dest
		;
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
		pageSize = buffer.get<uint16_t>();
		pagesStart = buffer.get<uint16_t>();
		pagesCount = buffer.get<uint16_t>();
	}
	
	void BootloaderDescription::dumpSpecific(wostream &stream) const
	{
		stream << pagesCount << " pages of size " << pageSize << " starting at page " << pagesStart;
	}
	
	bool operator ==(const BootloaderDescription &lhs, const BootloaderDescription &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.pageSize == rhs.pageSize &&
			lhs.pagesStart == rhs.pagesStart &&
			lhs.pagesCount == rhs.pagesCount
		;
	}
	
	//
	
	void BootloaderDataRead::serializeSpecific(SerializationBuffer& buffer) const
	{
		for (const auto word: data)
			buffer.add(word);
	}
	
	void BootloaderDataRead::deserializeSpecific(SerializationBuffer& buffer)
	{
		for (auto& word: data)
			word = buffer.get<uint8_t>();
	}
	
	void BootloaderDataRead::dumpSpecific(wostream &stream) const
	{
		stream << hex << setfill(wchar_t('0'));
		for (const auto word: data)
			stream << setw(2) << unsigned(word) << " ";
		stream << dec << setfill(wchar_t(' '));
	}
	
	bool operator ==(const BootloaderDataRead &lhs, const BootloaderDataRead &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.data == rhs.data
		;
	}
	
	//
	
	void BootloaderAck::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(static_cast<uint16_t>(errorCode));
		if (errorCode == ErrorCode::PROGRAMMING_FAILED)
			buffer.add(errorAddress);
	}
	
	void BootloaderAck::deserializeSpecific(SerializationBuffer& buffer)
	{
		errorCode = static_cast<ErrorCode>(buffer.get<uint16_t>());
		if (errorCode == ErrorCode::PROGRAMMING_FAILED)
			errorAddress = buffer.get<uint16_t>();
	}
	
	void BootloaderAck::dumpSpecific(wostream &stream) const
	{
		switch (errorCode)
		{
			case ErrorCode::SUCCESS: stream << "success"; break;
			case ErrorCode::INVALID_FRAME_SIZE: stream << "error, invalid frame size"; break;
			case ErrorCode::PROGRAMMING_FAILED: stream << "error, programming failed at low address " << hex << showbase << errorAddress; break;
			case ErrorCode::NOT_PROGRAMMING: stream << "error, not programming"; break;
			default: stream << "unknown error"; break;
		}
		stream << dec << noshowbase;
	}
	
	bool operator ==(const BootloaderAck &lhs, const BootloaderAck &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.errorCode == rhs.errorCode &&
			lhs.errorAddress == rhs.errorAddress
		;
	}
	
	//
	
	void ListNodes::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(version);
	}
	
	void ListNodes::deserializeSpecific(SerializationBuffer& buffer)
	{
		version = buffer.get<uint16_t>();
	}
	
	void ListNodes::dumpSpecific(std::wostream  &stream) const
	{
		stream << "protocol version " << version;
	}
	
	bool operator ==(const ListNodes &lhs, const ListNodes &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.version == rhs.version
		;
	}
	
	//
	
	void NodePresent::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(version);
	}
	
	void NodePresent::deserializeSpecific(SerializationBuffer& buffer)
	{
		version = buffer.get<uint16_t>();
	}
	
	void NodePresent::dumpSpecific(std::wostream  &stream) const
	{
		stream << "protocol version " << version;
	}
	
	bool operator ==(const NodePresent &lhs, const NodePresent &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.version == rhs.version
		;
	}
	
	//
	
	void GetDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(version);
	}
	
	void GetDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		version = buffer.get<uint16_t>();
	}
	
	void GetDescription::dumpSpecific(std::wostream  &stream) const
	{
		stream << "protocol version " << version;
	}
	
	bool operator ==(const GetDescription &lhs, const GetDescription &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.version == rhs.version
		;
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
		
		version = buffer.get<uint16_t>();
	}
	
	void GetNodeDescription::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "protocol version " << version;
	}
	
	bool operator ==(const GetNodeDescription &lhs, const GetNodeDescription &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.version == rhs.version
		;
	}
	
	//
	
	void Description::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(WStringToUTF8(name));
		buffer.add(static_cast<uint16_t>(protocolVersion));
		
		buffer.add(static_cast<uint16_t>(bytecodeSize));
		buffer.add(static_cast<uint16_t>(stackSize));
		buffer.add(static_cast<uint16_t>(variablesSize));
		
		buffer.add(static_cast<uint16_t>(namedVariables.size()));
		// named variables are sent separately
		
		buffer.add(static_cast<uint16_t>(localEvents.size()));
		// local events are sent separately
		
		buffer.add(static_cast<uint16_t>(nativeFunctions.size()));
		// native functions are sent separately
	}
	
	void Description::deserializeSpecific(SerializationBuffer& buffer)
	{
		name = UTF8ToWString(buffer.get<string>());
		protocolVersion = buffer.get<uint16_t>();
		
		bytecodeSize = buffer.get<uint16_t>();
		stackSize = buffer.get<uint16_t>();
		variablesSize = buffer.get<uint16_t>();
		
		namedVariables.resize(buffer.get<uint16_t>());
		// named variables are received separately
		
		localEvents.resize(buffer.get<uint16_t>());
		// local events are received separately
		
		nativeFunctions.resize(buffer.get<uint16_t>());
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
	
	bool operator ==(const Description &lhs, const Description &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.name == rhs.name &&
			lhs.protocolVersion == rhs.protocolVersion &&
			lhs.bytecodeSize == rhs.bytecodeSize &&
			lhs.stackSize == rhs.stackSize &&
			lhs.variablesSize == rhs.variablesSize &&
			lhs.namedVariables == rhs.namedVariables &&
			lhs.localEvents == rhs.localEvents &&
			lhs.nativeFunctions == rhs.nativeFunctions
		;
	}
	
	//
	
	void NamedVariableDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(static_cast<int16_t>(size));
		buffer.add(WStringToUTF8(name));
	}
	
	void NamedVariableDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		size = buffer.get<uint16_t>();
		name = UTF8ToWString(buffer.get<string>());
	}
	
	void NamedVariableDescription::dumpSpecific(std::wostream  &stream) const
	{
		stream << name << " of size " << size;
	}
	
	bool operator ==(const NamedVariableDescription &lhs, const NamedVariableDescription &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.size == rhs.size &&
			lhs.name == rhs.name
		;
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
	
	bool operator ==(const LocalEventDescription &lhs, const LocalEventDescription &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.name == rhs.name &&
			lhs.description == rhs.description
		;
	}
	
	//
	
	void NativeFunctionDescription::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(WStringToUTF8(name));
		buffer.add(WStringToUTF8(description));
		
		buffer.add(static_cast<uint16_t>(parameters.size()));
		for (const auto& parameter: parameters)
		{
			buffer.add(static_cast<int16_t>(parameter.size));
			buffer.add(WStringToUTF8(parameter.name));
		}
	}
	
	void NativeFunctionDescription::deserializeSpecific(SerializationBuffer& buffer)
	{
		name = UTF8ToWString(buffer.get<string>());
		description = UTF8ToWString(buffer.get<string>());
		
		parameters.resize(buffer.get<uint16_t>());
		for (auto& parameter: parameters)
		{
			parameter.size = buffer.get<int16_t>();
			parameter.name = UTF8ToWString(buffer.get<string>());
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
	
	bool operator ==(const NativeFunctionDescription &lhs, const NativeFunctionDescription &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.name == rhs.name &&
			lhs.description == rhs.description &&
			lhs.parameters == rhs.parameters
		;
	}
	
	//
	
	bool operator ==(const Disconnected &lhs, const Disconnected &rhs)
	{
		return static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs);
	}
	
	//
	
	void Variables::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(start);
		for (const auto variable: variables)
			buffer.add(variable);
	}
	
	void Variables::deserializeSpecific(SerializationBuffer& buffer)
	{
		start = buffer.get<uint16_t>();
		variables.resize((buffer.rawData.size() - buffer.readPos) / 2);
		for (auto& variable: variables)
			variable = buffer.get<int16_t>();
	}
	
	void Variables::dumpSpecific(wostream &stream) const
	{
		stream << "start " << start << ", variables vector of size " << variables.size();
		/*for (size_t i = 0; i < variables.size(); i++)
			stream << "\n " << i << " : " << variables[i];
		stream << "\n";*/
	}
	
	bool operator ==(const Variables &lhs, const Variables &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.start == rhs.start &&
			lhs.variables == rhs.variables
		;
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
		pc = buffer.get<uint16_t>();
		size = buffer.get<uint16_t>();
		index = buffer.get<uint16_t>();
	}
	
	void ArrayAccessOutOfBounds::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << ", size " << size << ", index " << index ;
	}
	
	bool operator ==(const ArrayAccessOutOfBounds &lhs, const ArrayAccessOutOfBounds &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.pc == rhs.pc &&
			lhs.size == rhs.size &&
			lhs.index == rhs.index
		;
	}
	
	//
	
	void DivisionByZero::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
	}
	
	void DivisionByZero::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16_t>();
	}
	
	void DivisionByZero::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc;
	}
	
	bool operator ==(const DivisionByZero &lhs, const DivisionByZero &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.pc == rhs.pc
		;
	}
	
	//
	
	void EventExecutionKilled::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
	}
	
	void EventExecutionKilled::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16_t>();
	}
	
	void EventExecutionKilled::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc;
	}
	
	bool operator ==(const EventExecutionKilled &lhs, const EventExecutionKilled &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.pc == rhs.pc
		;
	}
	
	//
	
	void NodeSpecificError::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
		buffer.add(WStringToUTF8(message));
	}
	
	void NodeSpecificError::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16_t>();
		message = UTF8ToWString(buffer.get<std::string>());
	}
	
	void NodeSpecificError::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << " " << message;
	}
	
	bool operator ==(const NodeSpecificError &lhs, const NodeSpecificError &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.pc == rhs.pc &&
			lhs.message == rhs.message
		;
	}
	
	//
	
	void ExecutionStateChanged::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
		buffer.add(flags);
	}
	
	void ExecutionStateChanged::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16_t>();
		flags = buffer.get<uint16_t>();
	}
	
	void ExecutionStateChanged::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << ", flags ";
		stream << hex << showbase << setw(4) << flags;
		stream << dec << noshowbase;
	}
	
	bool operator ==(const ExecutionStateChanged &lhs, const ExecutionStateChanged &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.pc == rhs.pc &&
			lhs.flags == rhs.flags
		;
	}
	
	//
	
	void BreakpointSetResult::serializeSpecific(SerializationBuffer& buffer) const
	{
		buffer.add(pc);
		buffer.add(success);
	}
	
	void BreakpointSetResult::deserializeSpecific(SerializationBuffer& buffer)
	{
		pc = buffer.get<uint16_t>();
		success = buffer.get<uint16_t>();
	}
	
	void BreakpointSetResult::dumpSpecific(wostream &stream) const
	{
		stream << "pc " << pc << ", success " << success;
	}
	
	bool operator ==(const BreakpointSetResult &lhs, const BreakpointSetResult &rhs)
	{
		return
			static_cast<const Message&>(lhs) == static_cast<const Message&>(rhs) &&
			lhs.pc == rhs.pc &&
			lhs.success == rhs.success
		;
	}
	
	//
	
	bool operator ==(const BootloaderReset &lhs, const BootloaderReset &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
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
		
		pageNumber = buffer.get<uint16_t>();
	}
	
	void BootloaderReadPage::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "page " << pageNumber;
	}
	
	bool operator ==(const BootloaderReadPage &lhs, const BootloaderReadPage &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.pageNumber == rhs.pageNumber
		;
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
		
		pageNumber = buffer.get<uint16_t>();
	}
	
	void BootloaderWritePage::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "page " << pageNumber;
	}
	
	bool operator ==(const BootloaderWritePage &lhs, const BootloaderWritePage &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.pageNumber == rhs.pageNumber
		;
	}
	
	//
	
	void BootloaderPageDataWrite::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		for (const auto word: data)
			buffer.add(word);
	}
	
	void BootloaderPageDataWrite::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		for (auto& word: data)
			word = buffer.get<uint8_t>();
	}
	
	void BootloaderPageDataWrite::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << hex << setfill(wchar_t('0'));
		for (const auto word: data)
			stream << setw(2) << unsigned(word) << " ";
		stream << dec << setfill(wchar_t(' '));
	}
	
	bool operator ==(const BootloaderPageDataWrite &lhs, const BootloaderPageDataWrite &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.data == rhs.data
		;
	}
	
	//
	
	void SetBytecode::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(start);
		for (const auto word: bytecode)
			buffer.add(word);
	}
	
	void SetBytecode::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		start = buffer.get<uint16_t>();
		bytecode.resize((buffer.rawData.size() - buffer.readPos) / 2);
		for (auto& word: bytecode)
			word = buffer.get<uint16_t>();
	}
	
	void SetBytecode::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << bytecode.size() << " words of bytecode of starting at " << start;
	}
	
	bool operator ==(const SetBytecode &lhs, const SetBytecode &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.start == rhs.start &&
			lhs.bytecode == rhs.bytecode
		;
	}
	
	void sendBytecode(Dashel::Stream* stream, uint16_t dest, const std::vector<uint16_t>& bytecode)
	{
		const unsigned bytecodePayloadSize = ASEBA_MAX_EVENT_ARG_COUNT-2;
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
	
	void sendBytecode(std::vector<std::unique_ptr<Message>>& messagesVector, uint16_t dest, const std::vector<uint16_t>& bytecode)
	{
		const unsigned bytecodePayloadSize = ASEBA_MAX_EVENT_ARG_COUNT-2;
		unsigned bytecodeStart = 0;
		unsigned bytecodeCount = bytecode.size();
		
		while (bytecodeCount > bytecodePayloadSize)
		{
			auto setBytecodeMessage = make_unique<SetBytecode>(dest, bytecodeStart);
			setBytecodeMessage->bytecode.resize(bytecodePayloadSize);
			copy(bytecode.begin()+bytecodeStart, bytecode.begin()+bytecodeStart+bytecodePayloadSize, setBytecodeMessage->bytecode.begin());
			messagesVector.push_back(move(setBytecodeMessage));
			
			bytecodeStart += bytecodePayloadSize;
			bytecodeCount -= bytecodePayloadSize;
		}
		
		{
			auto setBytecodeMessage = make_unique<SetBytecode>(dest, bytecodeStart);
			setBytecodeMessage->bytecode.resize(bytecodeCount);
			copy(bytecode.begin()+bytecodeStart, bytecode.end(), setBytecodeMessage->bytecode.begin());
			messagesVector.push_back(move(setBytecodeMessage));
		}
	}
	
	//
	
	bool operator ==(const Reset &lhs, const Reset &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	bool operator ==(const Run &lhs, const Run &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	bool operator ==(const Pause &lhs, const Pause &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	bool operator ==(const Step &lhs, const Step &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	bool operator ==(const Stop &lhs, const Stop &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	bool operator ==(const GetExecutionState &lhs, const GetExecutionState &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
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
		
		pc = buffer.get<uint16_t>();
	}
	
	void BreakpointSet::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "pc " << pc;
	}
	
	bool operator ==(const BreakpointSet &lhs, const BreakpointSet &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.pc == rhs.pc
		;
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
		
		pc = buffer.get<uint16_t>();
	}
	
	void BreakpointClear::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "pc " << pc;
	}
	
	bool operator ==(const BreakpointClear &lhs, const BreakpointClear &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.pc == rhs.pc
		;
	}
	
	//
	
	bool operator ==(const BreakpointClearAll &lhs, const BreakpointClearAll &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	GetVariables::GetVariables(uint16_t dest, uint16_t start, uint16_t length) :
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
		
		start = buffer.get<uint16_t>();
		length = buffer.get<uint16_t>();
	}
	
	void GetVariables::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "start " << start << ", length " << length;
	}
	
	bool operator ==(const GetVariables &lhs, const GetVariables &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.start == rhs.start &&
			lhs.length == rhs.length
		;
	}
	
	//
	
	SetVariables::SetVariables(uint16_t dest, uint16_t start, VariablesVector variables) :
		CmdMessage(ASEBA_MESSAGE_SET_VARIABLES, dest),
		start(start),
		variables(std::move(variables))
	{
	}
	
	void SetVariables::serializeSpecific(SerializationBuffer& buffer) const
	{
		CmdMessage::serializeSpecific(buffer);
		
		buffer.add(start);
		for (const auto variable: variables)
			buffer.add(variable);
	}
	
	void SetVariables::deserializeSpecific(SerializationBuffer& buffer)
	{
		CmdMessage::deserializeSpecific(buffer);
		
		start = buffer.get<uint16_t>();
		variables.resize((buffer.rawData.size() - buffer.readPos) / 2);
		for (auto& variable: variables)
			variable = buffer.get<int16_t>();
	}
	
	void SetVariables::dumpSpecific(wostream &stream) const
	{
		CmdMessage::dumpSpecific(stream);
		
		stream << "start " << start << ", variables vector of size " << variables.size();
	}
	
	bool operator ==(const SetVariables &lhs, const SetVariables &rhs)
	{
		return
			static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs) &&
			lhs.start == rhs.start &&
			lhs.variables == rhs.variables
		;
	}
	
	//
	
	bool operator ==(const WriteBytecode &lhs, const WriteBytecode &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	bool operator ==(const Reboot &lhs, const Reboot &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	//
	
	bool operator ==(const Sleep &lhs, const Sleep &rhs)
	{
		return static_cast<const CmdMessage&>(lhs) == static_cast<const CmdMessage&>(rhs);
	}
	
	
} // namespace Aseba
