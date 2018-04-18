/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

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
#include "TargetDescription.h"
#include <utility>
#include <vector>
#include <string>
#include <array>
#include <memory>

namespace Dashel {
class Stream;
}

namespace Aseba {
/**
\defgroup msg Messages exchanged over the network

    Aseba messages payload data must be 512 bytes or smaller, which means that the total
    packets size (len + source + type + payload) must be 518 bytes or smaller.
*/
/*@{*/

// comparison operators for inner classes of compiler, we need them here because messages use them
bool operator==(const TargetDescription::NamedVariable& lhs, const TargetDescription::NamedVariable& rhs);
bool operator==(const TargetDescription::LocalEvent& lhs, const TargetDescription::LocalEvent& rhs);
bool operator==(const TargetDescription::NativeFunctionParameter& lhs,
                const TargetDescription::NativeFunctionParameter& rhs);
bool operator==(const TargetDescription::NativeFunction& lhs, const TargetDescription::NativeFunction& rhs);

//! Vector of data of variables
using VariablesDataVector = std::vector<int16_t>;

//! Parent class of any message exchanged over the network
class Message {
public:
    //! Helper class that contains the raw data being (de-)serialialized
    struct SerializationBuffer {
        std::vector<uint8_t> rawData;
        size_t readPos = 0;

        template <typename T>
        void add(const T& val);
        template <typename T>
        T get();
        void dump(std::wostream& stream) const;
    };

    // data members

    uint16_t source = ASEBA_DEST_DEBUG;
    uint16_t type;

    // constructor and auto-generated members

    constexpr Message(uint16_t type) noexcept : type(type) {}

    Message(const Message&) = default;
    Message& operator=(const Message&) = default;

    Message(Message&&) = default;
    Message& operator=(Message&&) = default;

    virtual ~Message() = default;

    // (de-)serialization methods

    void serialize(Dashel::Stream* stream) const;
    static Message* receive(Dashel::Stream* stream);
    static Message* create(uint16_t source, uint16_t type, SerializationBuffer& buffer);
    Message* clone() const;
    void dump(std::wostream& stream) const;

    // purely-virtual methods for children

    virtual void serializeSpecific(SerializationBuffer& buffer) const = 0;
    virtual void deserializeSpecific(SerializationBuffer& buffer) = 0;
    virtual void dumpSpecific(std::wostream& stream) const = 0;

protected:
    virtual operator const char*() const {
        return "message super class";
    }
};

bool operator==(const Message& lhs, const Message& rhs);

//! Any message sent by a script on a node
class UserMessage : public Message {
public:
    VariablesDataVector data;

public:
    UserMessage() : Message(ASEBA_MESSAGE_INVALID) {}
    UserMessage(uint16_t type, VariablesDataVector data = VariablesDataVector())
        : Message(type), data(std::move(data)) {}
    UserMessage(uint16_t type, const int16_t* data, const size_t length);

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "user message";
    }
};

bool operator==(const UserMessage& lhs, const UserMessage& rhs);

//! Commands messages talk to a specific node
class CmdMessage : public Message {
public:
    uint16_t dest;

protected:
    CmdMessage(uint16_t type, uint16_t dest) : Message(type), dest(dest) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "command message super class";
    }
};

bool operator==(const CmdMessage& lhs, const CmdMessage& rhs);

//! Message for bootloader: description of the flash memory layout
class BootloaderDescription : public Message {
public:
    uint16_t pageSize;
    uint16_t pagesStart;
    uint16_t pagesCount;

public:
    BootloaderDescription() : Message(ASEBA_MESSAGE_BOOTLOADER_DESCRIPTION) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "bootloader description";
    }
};

bool operator==(const BootloaderDescription& lhs, const BootloaderDescription& rhs);

//! Message for bootloader: data from the flash
class BootloaderDataRead : public Message {
public:
    std::array<uint8_t, 4> data;

public:
    BootloaderDataRead() : Message(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_READ) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "bootloader page data read";
    }
};

bool operator==(const BootloaderDataRead& lhs, const BootloaderDataRead& rhs);

//! Message for bootloader: acknowledge, with optional error code
class BootloaderAck : public Message {
public:
    enum class ErrorCode : uint16_t { SUCCESS = 0, INVALID_FRAME_SIZE, PROGRAMMING_FAILED, NOT_PROGRAMMING };

public:
    ErrorCode errorCode;
    uint16_t errorAddress;

public:
    BootloaderAck() : Message(ASEBA_MESSAGE_BOOTLOADER_ACK) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "bootloader ack";
    }
};

bool operator==(const BootloaderAck& lhs, const BootloaderAck& rhs);

//! Request nodes to notify their presence
class ListNodes : public Message {
public:
    uint16_t version = ASEBA_PROTOCOL_VERSION;

public:
    ListNodes() : Message(ASEBA_MESSAGE_LIST_NODES) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream&) const override;
    operator const char*() const override {
        return "list nodes";
    }
};

bool operator==(const ListNodes& lhs, const ListNodes& rhs);

//! Answer of a node notifying its presence
class NodePresent : public Message {
public:
    uint16_t version = ASEBA_PROTOCOL_VERSION;

public:
    NodePresent() : Message(ASEBA_MESSAGE_NODE_PRESENT) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream&) const override;
    operator const char*() const override {
        return "node present";
    }
};

bool operator==(const NodePresent& lhs, const NodePresent& rhs);

//! Request all nodes to send their description
class GetDescription : public Message {
public:
    uint16_t version = ASEBA_PROTOCOL_VERSION;

public:
    GetDescription() : Message(ASEBA_MESSAGE_GET_DESCRIPTION) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream&) const override;
    operator const char*() const override {
        return "presence";
    }
};

bool operator==(const GetDescription& lhs, const GetDescription& rhs);

//! Request a specific node to send its description
class GetNodeDescription : public CmdMessage {
public:
    uint16_t version = ASEBA_PROTOCOL_VERSION;

public:
    GetNodeDescription(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_GET_NODE_DESCRIPTION, dest) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream&) const override;
    operator const char*() const override {
        return "get node description";
    }
};

bool operator==(const GetNodeDescription& lhs, const GetNodeDescription& rhs);

//! Description of a node, local events and native functions are omitted and further received by
//! other messages
class Description : public Message, public TargetDescription {
public:
    Description() : Message(ASEBA_MESSAGE_DESCRIPTION) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "description";
    }
};

bool operator==(const Description& lhs, const Description& rhs);

//! Description of a named variable available on a node, the description of the node itself must
//! have been sent first
class NamedVariableDescription : public Message, public TargetDescription::NamedVariable {
public:
    NamedVariableDescription() : Message(ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "named variable";
    }
};

bool operator==(const NamedVariableDescription& lhs, const NamedVariableDescription& rhs);

//! Description of a local event available on a node, the description of the node itself must have
//! been sent first
class LocalEventDescription : public Message, public TargetDescription::LocalEvent {
public:
    LocalEventDescription() : Message(ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "local event";
    }
};

bool operator==(const LocalEventDescription& lhs, const LocalEventDescription& rhs);

//! Description of a native function available on a node, the description of the node itself must
//! have been sent first
class NativeFunctionDescription : public Message, public TargetDescription::NativeFunction {
public:
    NativeFunctionDescription() : Message(ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "native function description";
    }
};

bool operator==(const NativeFunctionDescription& lhs, const NativeFunctionDescription& rhs);

//! A node has disconnected from the network
class Disconnected : public Message {
public:
    Disconnected() : Message(ASEBA_MESSAGE_DISCONNECTED) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override {}
    void deserializeSpecific(SerializationBuffer& buffer) override {}
    void dumpSpecific(std::wostream&) const override {}
    operator const char*() const override {
        return "disconnected";
    }
};

bool operator==(const Disconnected& lhs, const Disconnected& rhs);

//! Content of some variables
class Variables : public Message {
public:
    uint16_t start;
    VariablesDataVector variables;

public:
    Variables() : Message(ASEBA_MESSAGE_VARIABLES) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "variables";
    }
};

bool operator==(const Variables& lhs, const Variables& rhs);

//! Exception: an array acces attempted to read past memory
class ArrayAccessOutOfBounds : public Message {
public:
    uint16_t pc;
    uint16_t size;
    uint16_t index;

public:
    ArrayAccessOutOfBounds() : Message(ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "array access out of bounds";
    }
};

bool operator==(const ArrayAccessOutOfBounds& lhs, const ArrayAccessOutOfBounds& rhs);

//! Exception: division by zero
class DivisionByZero : public Message {
public:
    uint16_t pc;

public:
    DivisionByZero() : Message(ASEBA_MESSAGE_DIVISION_BY_ZERO) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "division by zero";
    }
};

bool operator==(const DivisionByZero& lhs, const DivisionByZero& rhs);

//! Exception: an event execution was killed by a new event
class EventExecutionKilled : public Message {
public:
    uint16_t pc;

public:
    EventExecutionKilled() : Message(ASEBA_MESSAGE_EVENT_EXECUTION_KILLED) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "event execution killed";
    }
};

bool operator==(const EventExecutionKilled& lhs, const EventExecutionKilled& rhs);

//! A node as produced an error specific to it
class NodeSpecificError : public Message {
public:
    uint16_t pc;
    std::wstring message;

public:
    NodeSpecificError() : Message(ASEBA_MESSAGE_NODE_SPECIFIC_ERROR) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream&) const override;
    operator const char*() const override {
        return "node specific error";
    }
};

bool operator==(const NodeSpecificError& lhs, const NodeSpecificError& rhs);

//! The Program Counter or the flags have changed
class ExecutionStateChanged : public Message {
public:
    uint16_t pc;
    uint16_t flags;

public:
    ExecutionStateChanged() : Message(ASEBA_MESSAGE_EXECUTION_STATE_CHANGED) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "execution state";
    }
};

bool operator==(const ExecutionStateChanged& lhs, const ExecutionStateChanged& rhs);

//! A breakpoint has been set
class BreakpointSetResult : public Message {
public:
    uint16_t pc;
    uint16_t success;

public:
    BreakpointSetResult() : Message(ASEBA_MESSAGE_BREAKPOINT_SET_RESULT) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "breakpoint set result";
    }
};

bool operator==(const BreakpointSetResult& lhs, const BreakpointSetResult& rhs);

//! Message for bootloader: reset node
class BootloaderReset : public CmdMessage {
public:
    BootloaderReset(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_RESET, dest) {}

protected:
    operator const char*() const override {
        return "bootloader reset";
    }
};

bool operator==(const BootloaderReset& lhs, const BootloaderReset& rhs);

//! Message for bootloader: read a page of flash
class BootloaderReadPage : public CmdMessage {
public:
    uint16_t pageNumber;

public:
    BootloaderReadPage(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_READ_PAGE, dest) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "bootloader read page";
    }
};

bool operator==(const BootloaderReadPage& lhs, const BootloaderReadPage& rhs);

//! Message for bootloader: write a page of flash
class BootloaderWritePage : public CmdMessage {
public:
    uint16_t pageNumber;

public:
    BootloaderWritePage(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE, dest) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "bootloader write page";
    }
};

bool operator==(const BootloaderWritePage& lhs, const BootloaderWritePage& rhs);

//! Message for bootloader: data for flash
class BootloaderPageDataWrite : public CmdMessage {
public:
    std::array<uint8_t, 4> data;

public:
    BootloaderPageDataWrite(uint16_t dest = ASEBA_DEST_INVALID)
        : CmdMessage(ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE, dest) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "bootloader page data write";
    }
};

bool operator==(const BootloaderPageDataWrite& lhs, const BootloaderPageDataWrite& rhs);

//! Upload bytecode to a node
class SetBytecode : public CmdMessage {
public:
    uint16_t start = 0;
    std::vector<uint16_t> bytecode;

public:
    SetBytecode() : CmdMessage(ASEBA_MESSAGE_SET_BYTECODE, ASEBA_DEST_INVALID) {}
    SetBytecode(uint16_t dest, uint16_t start) : CmdMessage(ASEBA_MESSAGE_SET_BYTECODE, dest), start(start) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "set bytecode";
    }
};

bool operator==(const SetBytecode& lhs, const SetBytecode& rhs);

//! Call the SetBytecode multiple time in order to send all the bytecode
void sendBytecode(Dashel::Stream* stream, uint16_t dest, const std::vector<uint16_t>& bytecode);
//! Call the SetBytecode multiple time in order to send all the bytecode
void sendBytecode(std::vector<std::unique_ptr<Message>>& messagesVector, uint16_t dest,
                  const std::vector<uint16_t>& bytecode);

//! Reset a node
class Reset : public CmdMessage {
public:
    Reset(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_RESET, dest) {}

protected:
    operator const char*() const override {
        return "reset";
    }
};

bool operator==(const Reset& lhs, const Reset& rhs);

//! Run a node
class Run : public CmdMessage {
public:
    Run(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_RUN, dest) {}

protected:
    operator const char*() const override {
        return "run";
    }
};

bool operator==(const Run& lhs, const Run& rhs);

//! Pause a node
class Pause : public CmdMessage {
public:
    Pause(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_PAUSE, dest) {}

protected:
    operator const char*() const override {
        return "pause";
    }
};

bool operator==(const Pause& lhs, const Pause& rhs);

//! Step a node
class Step : public CmdMessage {
public:
    Step(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_STEP, dest) {}

protected:
    operator const char*() const override {
        return "step";
    }
};

bool operator==(const Step& lhs, const Step& rhs);

//! Stop a node
class Stop : public CmdMessage {
public:
    Stop(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_STOP, dest) {}

protected:
    operator const char*() const override {
        return "stop";
    }
};

bool operator==(const Stop& lhs, const Stop& rhs);

//! Request the current execution state of a node
class GetExecutionState : public CmdMessage {
public:
    GetExecutionState(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_GET_EXECUTION_STATE, dest) {}

protected:
    operator const char*() const override {
        return "get execution state";
    }
};

bool operator==(const GetExecutionState& lhs, const GetExecutionState& rhs);

//! Set a breakpoint on a node
class BreakpointSet : public CmdMessage {
public:
    uint16_t pc;

public:
    BreakpointSet() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_SET, ASEBA_DEST_INVALID) {}
    BreakpointSet(uint16_t dest, uint16_t pc) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_SET, dest), pc(pc) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "breakpoint set";
    }
};

bool operator==(const BreakpointSet& lhs, const BreakpointSet& rhs);

//! Clear a breakpoint on a node
class BreakpointClear : public CmdMessage {
public:
    uint16_t pc;

public:
    BreakpointClear() : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR, ASEBA_DEST_INVALID) {}
    BreakpointClear(uint16_t dest, uint16_t pc) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR, dest), pc(pc) {}

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "breakpoint clear";
    }
};

bool operator==(const BreakpointClear& lhs, const BreakpointClear& rhs);

//! Clear all breakpoints on a node
class BreakpointClearAll : public CmdMessage {
public:
    BreakpointClearAll(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL, dest) {}

protected:
    operator const char*() const override {
        return "breakpoint clear all";
    }
};

bool operator==(const BreakpointClearAll& lhs, const BreakpointClearAll& rhs);

//! Read some variables from a node
class GetVariables : public CmdMessage {
public:
    uint16_t start;
    uint16_t length;

public:
    GetVariables() : CmdMessage(ASEBA_MESSAGE_GET_VARIABLES, ASEBA_DEST_INVALID) {}
    GetVariables(uint16_t dest, uint16_t start, uint16_t length);

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "get variables";
    }
};

bool operator==(const GetVariables& lhs, const GetVariables& rhs);

//! Set some variables on a node
class SetVariables : public CmdMessage {
public:
    uint16_t start;
    VariablesDataVector variables;

public:
    SetVariables() : CmdMessage(ASEBA_MESSAGE_SET_VARIABLES, ASEBA_DEST_INVALID) {}
    SetVariables(uint16_t dest, uint16_t start, VariablesDataVector variables);

protected:
    void serializeSpecific(SerializationBuffer& buffer) const override;
    void deserializeSpecific(SerializationBuffer& buffer) override;
    void dumpSpecific(std::wostream& stream) const override;
    operator const char*() const override {
        return "set variables";
    }
};

bool operator==(const SetVariables& lhs, const SetVariables& rhs);

//! Save the current bytecode of a node
class WriteBytecode : public CmdMessage {
public:
    WriteBytecode(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_WRITE_BYTECODE, dest) {}

protected:
    operator const char*() const override {
        return "save bytecode";
    }
};

bool operator==(const WriteBytecode& lhs, const WriteBytecode& rhs);

//! Reboot a node, useful to go into its bootloader, if present
class Reboot : public CmdMessage {
public:
    Reboot(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_REBOOT, dest) {}

protected:
    operator const char*() const override {
        return "reboot";
    }
};

bool operator==(const Reboot& lhs, const Reboot& rhs);

//! Put the node in sleep mode. After receiving this event, the node might go into sleep depending
//! on its internal state. If it goes to sleep, the node will not receive events any more, until it
//! is waken up by an implementation-specific action.
class Sleep : public CmdMessage {
public:
    Sleep(uint16_t dest = ASEBA_DEST_INVALID) : CmdMessage(ASEBA_MESSAGE_SUSPEND_TO_RAM, dest) {}

protected:
    operator const char*() const override {
        return "sleep";
    }
};

bool operator==(const Sleep& lhs, const Sleep& rhs);

/*@}*/
}  // namespace Aseba

#endif
