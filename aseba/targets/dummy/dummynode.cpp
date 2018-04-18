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

#ifndef ASEBA_ASSERT
#    define ASEBA_ASSERT
#endif

#include <iostream>
#include <sstream>
#include <valarray>
#include <cassert>
#include <cstring>

#include "vm/vm.h"
#include "vm/natives.h"
#include "common/productids.h"
#include "common/consts.h"
#include "common/utils/utils.h"
#include "common/utils/FormatableString.h"
#ifdef ZEROCONF_SUPPORT
#    include "common/zeroconf/zeroconf-dashelhub.h"
#endif  // ZEROCONF_SUPPORT
#include "transport/buffer/vm-buffer.h"
#include <dashel/dashel.h>

static AsebaVMDescription nodeDescription;

class AsebaNode : public Dashel::Hub {
private:
    AsebaVMState vm;
    std::valarray<unsigned short> bytecode;
    std::valarray<signed short> stack;
    struct Variables {
        int16_t id;
        int16_t source;
        int16_t args[32];
        int16_t productId;
        int16_t timerPeriod;
        int16_t user[1024];
    } variables;
    char mutableName[12];

    // stream for listening to incoming connections
    Dashel::Stream* listenStream;

public:
    // public because accessed from a glue function
    uint16_t lastMessageSource;
    std::valarray<uint8_t> lastMessageData;
    // this must be public because of bindings to C functions
    Dashel::Stream* stream;
    // all streams that must be disconnected at next step
    std::vector<Dashel::Stream*> toDisconnect;
#ifdef ZEROCONF_SUPPORT
    // to advertise
    Aseba::DashelhubZeroconf zeroconf;
#endif  // ZEROCONF_SUPPORT

public:
    AsebaNode()
#ifdef ZEROCONF_SUPPORT
        : zeroconf(*this)
#endif  // ZEROCONF_SUPPORT
    {
        // setup variables
        vm.nodeId = 1;

        bytecode.resize(512);
        vm.bytecode = &bytecode[0];
        vm.bytecodeSize = bytecode.size();

        stack.resize(64);
        vm.stack = &stack[0];
        vm.stackSize = stack.size();

        vm.variables = reinterpret_cast<int16_t*>(&variables);
        vm.variablesSize = sizeof(variables) / sizeof(int16_t);
    }

    Dashel::Stream* listen(const int port, const int deltaNodeId) {
        vm.nodeId = 1 + deltaNodeId;
        strncpy(mutableName, "dummynode-0", 12);
        mutableName[10] = '0' + deltaNodeId;
        nodeDescription.name = mutableName;

        // connect network
        try {
            std::ostringstream oss;
            oss << "tcpin:port=" << port;
            listenStream = Dashel::Hub::connect(oss.str());
        } catch(Dashel::DashelException e) {
            std::cerr << "Cannot create listening port " << port << ": " << e.what() << std::endl;
            abort();
        }

        // init VM
        AsebaVMInit(&vm);

#ifdef ZEROCONF_SUPPORT
        // advertise our status
        updateZeroconfStatus();
#endif  // ZEROCONF_SUPPORT

        // return stream
        return listenStream;
    }

#ifdef ZEROCONF_SUPPORT
    void updateZeroconfStatus() {
        // we need a valid listen stream to advertise
        if(!listenStream)
            return;
        // advertise target
        Aseba::Zeroconf::TxtRecord txt{ASEBA_PROTOCOL_VERSION, "Dummy Node", stream != nullptr, {vm.nodeId}, {
                                           static_cast<unsigned int>(variables.productId)
                                       }};
        try {
            if(stream)
                zeroconf.forget(Aseba::FormatableString("Dummy Node %0").arg(vm.nodeId - 1), listenStream);
            else
                zeroconf.advertise(Aseba::FormatableString("Dummy Node %0").arg(vm.nodeId - 1), listenStream, txt);
        } catch(const std::runtime_error& e) {
            std::cerr << "Can't advertise stream " << listenStream->getTargetName() << ": " << e.what() << std::endl;
        }
    }
#endif  // ZEROCONF_SUPPORT

    virtual void connectionCreated(Dashel::Stream* stream) {
        std::string targetName = stream->getTargetName();
        if(targetName.substr(0, targetName.find_first_of(':')) == "tcp") {
            // schedule current stream for disconnection
            if(this->stream)
                toDisconnect.push_back(this->stream);

            // set new stream as current stream
            this->stream = stream;
            std::cerr << this << " : New client connected." << std::endl;
#ifdef ZEROCONF_SUPPORT
            // we are not available any more
            updateZeroconfStatus();
#endif  // ZEROCONF_SUPPORT
        }
    }

    virtual void connectionClosed(Dashel::Stream* stream, bool abnormal) {
#ifdef ZEROCONF_SUPPORT
        zeroconf.dashelConnectionClosed(stream);
#endif  // ZEROCONF_SUPPORT
        this->stream = nullptr;
        // clear breakpoints
        vm.breakpointsCount = 0;

        if(abnormal)
            std::cerr << this << " : Client has disconnected unexpectedly." << std::endl;
        else
            std::cerr << this << " : Client has disconnected properly." << std::endl;
#ifdef ZEROCONF_SUPPORT
        // we are now available again
        updateZeroconfStatus();
#endif  // ZEROCONF_SUPPORT
    }

    virtual void incomingData(Dashel::Stream* stream) {
#ifdef ZEROCONF_SUPPORT
        zeroconf.dashelIncomingData(stream);
#endif  // ZEROCONF_SUPPORT
        // only process data for the current stream
        if(stream != this->stream)
            return;

        uint16_t temp;
        uint16_t len;

        stream->read(&temp, 2);
        len = bswap16(temp);
        stream->read(&temp, 2);
        lastMessageSource = bswap16(temp);
        lastMessageData.resize(len + 2);
        stream->read(&lastMessageData[0], lastMessageData.size());

        AsebaProcessIncomingEvents(&vm);

        // run VM
        AsebaVMRun(&vm, 1000);
    }

    void run() {
        // wait a given time, return if stop was called
        Aseba::UnifiedTime startTime;
        int timeout(variables.timerPeriod > 0 ? variables.timerPeriod : -1);
#ifdef ZEROCONF_SUPPORT
        while(zeroconf.dashelStep(timeout))
#else   // ZEROCONF_SUPPORT
        while(step(timeout))
#endif  // ZEROCONF_SUPPORT
        {
            if(variables.timerPeriod > 0) {
                if(int((Aseba::UnifiedTime() - startTime).value) >= variables.timerPeriod) {
                    // reschedule a periodic event if we are not in step by step
                    if(AsebaMaskIsClear(vm.flags, ASEBA_VM_STEP_BY_STEP_MASK) ||
                       AsebaMaskIsClear(vm.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
                        AsebaVMSetupEvent(&vm, ASEBA_EVENT_LOCAL_EVENTS_START - 0);

                    // run VM
                    AsebaVMRun(&vm, 1000);

                    // save current time for next iteration
                    Aseba::UnifiedTime currentTime;
                    timeout = 2 * int(variables.timerPeriod) - int((currentTime - startTime).value);
                    if(timeout < 0)
                        timeout = 0;
                    startTime = currentTime;
                }
            } else {
                timeout = -1;
            }

            // disconnect old streams
            for(size_t i = 0; i < toDisconnect.size(); ++i) {
                closeStream(toDisconnect[i]);
                std::cerr << this << " : Old client disconnected by new client." << std::endl;
            }
            toDisconnect.clear();
        }
    }
} node;

// Implementation of aseba glue code

extern "C" void AsebaPutVmToSleep(AsebaVMState* vm) {
    std::cerr << "Received request to go into sleep" << std::endl;
}

extern "C" void AsebaSendBuffer(AsebaVMState* vm, const uint8_t* data, uint16_t length) {
    Dashel::Stream* stream = node.stream;
    if(stream) {
        try {
            uint16_t temp;
            temp = bswap16(length - 2);
            stream->write(&temp, 2);
            temp = bswap16(vm->nodeId);
            stream->write(&temp, 2);
            stream->write(data, length);
            stream->flush();
        } catch(Dashel::DashelException e) {
            std::cerr << "Cannot write to socket: " << stream->getFailReason() << std::endl;
        }
    }
}

extern "C" uint16_t AsebaGetBuffer(AsebaVMState* vm, uint8_t* data, uint16_t maxLength, uint16_t* source) {
    if(node.lastMessageData.size()) {
        *source = node.lastMessageSource;
        memcpy(data, &node.lastMessageData[0], node.lastMessageData.size());
    }
    return node.lastMessageData.size();
}

extern AsebaVMDescription nodeDescription;

extern "C" const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState* vm) {
    return &nodeDescription;
}

static AsebaNativeFunctionPointer nativeFunctions[] = {
    ASEBA_NATIVES_STD_FUNCTIONS,
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] = {ASEBA_NATIVES_STD_DESCRIPTIONS, 0};

extern "C" const AsebaNativeFunctionDescription* const* AsebaGetNativeFunctionsDescriptions(AsebaVMState* vm) {
    return nativeFunctionsDescriptions;
}

extern "C" void AsebaNativeFunction(AsebaVMState* vm, uint16_t id) {
    nativeFunctions[id](vm);
}


static const AsebaLocalEventDescription localEvents[] = {{"timer", "periodic timer at a given frequency"},
                                                         {nullptr, nullptr}};

extern "C" const AsebaLocalEventDescription* AsebaGetLocalEventsDescriptions(AsebaVMState* vm) {
    return localEvents;
}

extern "C" void AsebaWriteBytecode(AsebaVMState* vm) {
    std::cerr << "Received request to write bytecode into flash" << std::endl;
}

extern "C" void AsebaResetIntoBootloader(AsebaVMState* vm) {
    std::cerr << "Received request to reset into bootloader" << std::endl;
}

extern "C" void AsebaAssert(AsebaVMState* vm, AsebaAssertReason reason) {
    std::cerr << "\nFatal error; exception: ";
    switch(reason) {
        case ASEBA_ASSERT_UNKNOWN: std::cerr << "undefined"; break;
        case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator"; break;
        case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode"; break;
        case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow"; break;
        case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow"; break;
        case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds"; break;
        case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds"; break;
        case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run"; break;
        case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds"; break;
        default: std::cerr << "unknown exception"; break;
    }
    std::cerr << ".\npc = " << vm->pc << ", sp = " << vm->sp;
    abort();
    std::cerr << "\nResetting VM" << std::endl;
    AsebaVMInit(vm);
}

int usage(char* program) {
    std::cerr << "Usage: " << program << " [--port|-p PORT] [ID, from 0 to 9]" << std::endl;
    std::cerr << "Usage: " << program << " --help|-h" << std::endl;
    std::cerr << "Creates one node dummynode-ID with node id ID+1 listening on port:" << std::endl;
    std::cerr << " - a dynamically chosen port, if PORT == 0" << std::endl;
    std::cerr << " - PORT, if PORT != 0 and PORT is available" << std::endl;
    std::cerr << " - 33333+ID, if PORT is not set and 33333+ID is available." << std::endl;
    std::cerr << "The Dashel target is printed on stdout." << std::endl;
    return 1;
}

int main(int argc, char* argv[]) {
    int port(ASEBA_DEFAULT_PORT);
    bool do_delta(true);
    int deltaNodeId(0);

    int argCounter = 1;
    while(argCounter < argc) {
        const char* arg = argv[argCounter++];
        if((strcmp(arg, "-p") == 0) || (strcmp(arg, "--port") == 0))
            do_delta = false, port = atoi(argv[argCounter++]);
        else if((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
            return usage(argv[0]);
        else {
            deltaNodeId = atoi(arg);
            if(deltaNodeId < 0 || deltaNodeId > 9)
                return usage(argv[0]);
        }
    }

    Dashel::Stream* listen = node.listen(do_delta ? port + deltaNodeId : port, deltaNodeId);

    std::cout << "tcp:port=" << listen->getTargetParameter("port") << std::endl;

    node.run();
}
