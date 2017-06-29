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

#include "../../common/msg/msg.h"
#include <iostream>

using namespace Aseba;
using namespace std;

//! Test serialization/deserialization of message type T, initialized with args,
//! with additional members set by initFunc, to test for equality and
//! modified by N times by modifyFuncs to test for inequality
template<typename T, typename... Args>
void testMessage(function<void(T&)> initFunc, initializer_list<function<void(T&)>> modifyFuncs, Args&&... args)
{
	// create first object
	unique_ptr<T> m1(new T(forward<Args>(args)...));
	
	// set values
	initFunc(*m1);
	
	// serialize and deserialize...
	unique_ptr<T> m2;
	{
		Message::SerializationBuffer buffer;
		static_cast<Message*>(m1.get())->serializeSpecific(buffer);
		unique_ptr<Message> m2super(Message::create(m1->source, m1->type, buffer));
		if (!dynamic_cast<T*>(m2super.get()))
		{
			cerr << "Message type " << typeid(T).name() << " did not serialize-deserialize to itself" << endl;
			throw logic_error("Serialization failed");
		}
		m2.reset(dynamic_cast<T*>(m2super.release()));
	}
	
	// check for equality
	if (!(*m1 == *m2))
	{
		cerr << "Message type " << typeid(T).name() << " changed content after serialization" << endl;
		throw logic_error("Serialization failed");
	}
	
	// count the current function, to display proper error message
	unsigned count(0);
	for (auto& modifyFunc: modifyFuncs)
	{
		// create a new object
		m2.reset(new T(forward<Args>(args)...));
		
		// set values
		initFunc(*m2);
		
		// modify values
		modifyFunc(*m2);
		
		// serialize and deserialize...
		{
			Message::SerializationBuffer buffer;
			static_cast<Message*>(m2.get())->serializeSpecific(buffer);
			unique_ptr<Message> m2super(Message::create(m2->source, m2->type, buffer));
			if (!dynamic_cast<T*>(m2super.get()))
			{
				cerr << "Message type " << typeid(T).name() << " did not serialize-deserialize to itself" << endl;
				throw logic_error("Serialization failed");
			}
			m2.reset(dynamic_cast<T*>(m2super.release()));
		}
		
		// check for inequality
		if (*m1 == *m2)
		{
			cerr << "Message type " << typeid(T).name() << " are still equal while content changed (function " << count << ") after serialization" << endl;
			throw logic_error("Serialization failed");
		}
		
		++count;
	}
}

//! Test serialization/deserialization of message type T, initialized with args
template<typename T, typename... Args>
void testMessageNoInit(initializer_list<function<void(T&)>> modifyFuncs, Args&&... args)
{
	testMessage<T>([](T& ){}, modifyFuncs, args...);
}

//! Test serialization/deserialization of message type T, initialized with args, no modify function
template<typename T, typename... Args>
void testMessageNoInitNoFunc(Args&&... args)
{
	testMessage<T>([](T& ){}, {}, args...);
}

int main()
{
	// Test the serialization and deserialization of all messages
	
	// The concept is that, for each message, we create and instance
	// and call an init function.
	// Then, we serialize and deserialize and check for equality.
	// Then, for every modifier function in a list, we re-create and re-init
	// an object, modify it, we serialize and deserialize, and check for inequality.
	
	testMessageNoInit<UserMessage>(
		{
			[](UserMessage& m) { m.data[0] = 4; },
			[](UserMessage& m) { m.data[1] = 3; },
			[](UserMessage& m) { m.data[2] = 2; }
		},
		0, UserMessage::DataVector{1, 2, 3}
	);
	
	testMessage<BootloaderDescription>(
		[](BootloaderDescription& m) { m.pageSize = 128; m.pagesStart = 10; m.pagesCount = 20; },
		{
			[](BootloaderDescription& m) { m.pageSize = 64; },
			[](BootloaderDescription& m) { m.pagesStart = 20; },
			[](BootloaderDescription& m) { m.pagesCount = 10; }
		}
	);
	
	testMessage<BootloaderDataRead>(
		[](BootloaderDataRead& m) { m.data = {{1, 2, 3, 4}}; },
		{
			[](BootloaderDataRead& m) { m.data[0] = 4; },
			[](BootloaderDataRead& m) { m.data[1] = 3; },
			[](BootloaderDataRead& m) { m.data[2] = 2; },
			[](BootloaderDataRead& m) { m.data[3] = 1; }
		}
	);
	
	testMessage<BootloaderAck>(
		[](BootloaderAck& m) { m.errorCode = BootloaderAck::ErrorCode::PROGRAMMING_FAILED; m.errorAddress = 0xff; },
		{
			[](BootloaderAck& m) { m.errorCode = BootloaderAck::ErrorCode::SUCCESS; m.errorAddress = 0; }
		}
	);
	
	testMessageNoInit<ListNodes>(
		{
			[](ListNodes& m) { m.version = 1; }
		}
	);
	
	testMessageNoInit<NodePresent>(
		{
			[](NodePresent& m) { m.version = 1; }
		}
	);
	
	testMessageNoInit<GetDescription>(
		{
			[](GetDescription& m) { m.version = 1; }
		}
	);
	
	testMessage<GetNodeDescription>(
		[](GetNodeDescription& m) {
			m.dest = 1;
		},
		{
			[](GetNodeDescription& m) { m.dest = 3; },
			[](GetNodeDescription& m) { m.version = 1; }
		}
	);
	
	testMessage<Description>(
		[](Description& m) {
			m.name = L"test";
			m.protocolVersion = ASEBA_PROTOCOL_VERSION;
			m.bytecodeSize = 1024;
			m.stackSize = 64;
			m.variablesSize = 1024;
		},
		{
			[](Description& m) { m.name = L"titi"; },
			[](Description& m) { m.protocolVersion = 1; },
			[](Description& m) { m.bytecodeSize = 512; },
			[](Description& m) { m.stackSize = 32; },
			[](Description& m) { m.variablesSize = 512; }
		}
	);
	
	testMessage<NamedVariableDescription>(
		[](NamedVariableDescription& m) {
			m.name = L"test";
			m.size = 2;
		},
		{
			[](NamedVariableDescription& m) { m.name = L"titi"; },
			[](NamedVariableDescription& m) { m.size = 1; }
		}
	);
	
	testMessage<LocalEventDescription>(
		[](LocalEventDescription& m) {
			m.name = L"ontimer0";
			m.description = L"test event";
		},
		{
			[](LocalEventDescription& m) { m.name = L"ontimer1"; },
			[](LocalEventDescription& m) { m.description = L"another test event"; }
		}
	);
	
	testMessage<NativeFunctionDescription>(
		[](NativeFunctionDescription& m) {
			m.name = L"math.null";
			m.description = L"do nothing";
		},
		{
			[](NativeFunctionDescription& m) { m.name = L"math.ignore"; },
			[](NativeFunctionDescription& m) { m.description = L"still do nothing"; },
			[](NativeFunctionDescription& m) { m.parameters.emplace_back(L"param0", 1); }
		}
	);
	
	testMessageNoInitNoFunc<Disconnected>();
	
	testMessage<Variables>(
		[](Variables& m) {
			m.start = 10;
			m.variables = {1, 2};
		},
		{
			[](Variables& m) { m.start = 20; },
			[](Variables& m) { m.variables[0] = 3; },
			[](Variables& m) { m.variables[1] = 4; },
			[](Variables& m) { m.variables.push_back(5); }
		}
	);
	
	testMessage<ArrayAccessOutOfBounds>(
		[](ArrayAccessOutOfBounds& m) {
			m.pc = 10;
			m.size = 10;
			m.index = 10;
		},
		{
			[](ArrayAccessOutOfBounds& m) { m.pc = 5; },
			[](ArrayAccessOutOfBounds& m) { m.size = 5; },
			[](ArrayAccessOutOfBounds& m) { m.index = 20; }
		}
	);
	
	testMessage<DivisionByZero>(
		[](DivisionByZero& m) {
			m.pc = 10;
		},
		{
			[](DivisionByZero& m) { m.pc = 5; }
		}
	);
	
	testMessage<EventExecutionKilled>(
		[](EventExecutionKilled& m) {
			m.pc = 10;
		},
		{
			[](EventExecutionKilled& m) { m.pc = 5; }
		}
	);
	
	testMessage<NodeSpecificError>(
		[](NodeSpecificError& m) {
			m.pc = 10;
			m.message = L"my error";
		},
		{
			[](NodeSpecificError& m) { m.pc = 5; },
			[](NodeSpecificError& m) { m.message = L"my other error"; }
		}
	);
	
	testMessage<ExecutionStateChanged>(
		[](ExecutionStateChanged& m) {
			m.pc = 10;
			m.flags = 3;
		},
		{
			[](ExecutionStateChanged& m) { m.pc = 5; },
			[](ExecutionStateChanged& m) { m.flags = 1; }
		}
	);
	
	testMessage<BreakpointSetResult>(
		[](BreakpointSetResult& m) {
			m.pc = 10;
			m.success = 1;
		},
		{
			[](BreakpointSetResult& m) { m.pc = 5; },
			[](BreakpointSetResult& m) { m.success = 0; }
		}
	);
	
	testMessage<BootloaderReset>(
		[](BootloaderReset& m) {
			m.dest = 1;
		},
		{
			[](BootloaderReset& m) { m.dest = 3; }
		}
	);
	
	testMessage<BootloaderReadPage>(
		[](BootloaderReadPage& m) {
			m.dest = 1;
			m.pageNumber = 10;
		},
		{
			[](BootloaderReadPage& m) { m.dest = 3; },
			[](BootloaderReadPage& m) { m.pageNumber = 1; }
		}
	);
	
	testMessage<BootloaderWritePage>(
		[](BootloaderWritePage& m) {
			m.dest = 1;
			m.pageNumber = 10;
		},
		{
			[](BootloaderWritePage& m) { m.dest = 3; },
			[](BootloaderWritePage& m) { m.pageNumber = 1; }
		}
	);
	
	testMessage<BootloaderPageDataWrite>(
		[](BootloaderPageDataWrite& m) {
			m.dest = 1;
			m.data = {{1, 2, 3, 4}};
		},
		{
			[](BootloaderPageDataWrite& m) { m.dest = 3; },
			[](BootloaderPageDataWrite& m) { m.data[0] = 4; },
			[](BootloaderPageDataWrite& m) { m.data[1] = 3; },
			[](BootloaderPageDataWrite& m) { m.data[2] = 2; },
			[](BootloaderPageDataWrite& m) { m.data[3] = 1; }
		}
	);
	
	testMessage<SetBytecode>(
		[](SetBytecode& m) {
			m.dest = 1;
			m.bytecode = {1, 2};
		},
		{
			[](SetBytecode& m) { m.dest = 3; },
			[](SetBytecode& m) { m.bytecode[0] = 3; },
			[](SetBytecode& m) { m.bytecode[1] = 4; },
			[](SetBytecode& m) { m.bytecode.push_back(5); }
		}
	);
	
	testMessage<Reset>(
		[](Reset& m) {
			m.dest = 1;
		},
		{
			[](Reset& m) { m.dest = 3; }
		}
	);
	
	testMessage<Run>(
		[](Run& m) {
			m.dest = 1;
		},
		{
			[](Run& m) { m.dest = 3; }
		}
	);
	
	testMessage<Pause>(
		[](Pause& m) {
			m.dest = 1;
		},
		{
			[](Pause& m) { m.dest = 3; }
		}
	);
	
	testMessage<Step>(
		[](Step& m) {
			m.dest = 1;
		},
		{
			[](Step& m) { m.dest = 3; }
		}
	);
	
	testMessage<Stop>(
		[](Stop& m) {
			m.dest = 1;
		},
		{
			[](Stop& m) { m.dest = 3; }
		}
	);
	
	testMessage<GetExecutionState>(
		[](GetExecutionState& m) {
			m.dest = 1;
		},
		{
			[](GetExecutionState& m) { m.dest = 3; }
		}
	);
	
	testMessage<BreakpointSet>(
		[](BreakpointSet& m) {
			m.dest = 1;
			m.pc = 10;
		},
		{
			[](BreakpointSet& m) { m.dest = 3; },
			[](BreakpointSet& m) { m.pc = 20; }
		}
	);
	
	testMessage<BreakpointClear>(
		[](BreakpointClear& m) {
			m.dest = 1;
			m.pc = 10;
		},
		{
			[](BreakpointClear& m) { m.dest = 3; },
			[](BreakpointClear& m) { m.pc = 20; }
		}
	);
	
	testMessage<BreakpointClearAll>(
		[](BreakpointClearAll& m) {
			m.dest = 1;
		},
		{
			[](BreakpointClearAll& m) { m.dest = 3; }
		}
	);
	
	testMessage<GetVariables>(
		[](GetVariables& m) {
			m.dest = 1;
			m.start = 10;
			m.length = 10;
		},
		{
			[](GetVariables& m) { m.dest = 3; },
			[](GetVariables& m) { m.start = 20; },
			[](GetVariables& m) { m.length = 20; }
		}
	);
	
	testMessage<SetVariables>(
		[](SetVariables& m) {
			m.dest = 1;
			m.start = 10;
			m.variables = {1, 2};
		},
		{
			[](SetVariables& m) { m.dest = 3; },
			[](SetVariables& m) { m.start = 20; },
			[](SetVariables& m) { m.variables[0] = 3; },
			[](SetVariables& m) { m.variables[1] = 4; },
			[](SetVariables& m) { m.variables.push_back(5); }
		}
	);
	
	testMessage<WriteBytecode>(
		[](WriteBytecode& m) {
			m.dest = 1;
		},
		{
			[](WriteBytecode& m) { m.dest = 3; }
		}
	);
	
	testMessage<Reboot>(
		[](Reboot& m) {
			m.dest = 1;
		},
		{
			[](Reboot& m) { m.dest = 3; }
		}
	);
	testMessage<Sleep>(
		[](Sleep& m) {
			m.dest = 1;
		},
		{
			[](Sleep& m) { m.dest = 3; }
		}
	);
	
	return 0;
}
