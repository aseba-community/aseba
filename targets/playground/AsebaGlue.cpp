/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2013:
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

#ifndef ASEBA_ASSERT
#define ASEBA_ASSERT
#endif

#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <string>
#include <typeinfo>
#include <algorithm>
#include <cassert>
#include "AsebaGlue.h"
#include "PlaygroundViewer.h"
#include "../../transport/buffer/vm-buffer.h"
#include "../../common/utils/FormatableString.h"
// #include "../../vm/vm.h"


namespace Aseba
{
	// Mapping so that Aseba C callbacks can dispatch to the right objects
	VMStateToEnvironment vmStateToEnvironment;

	// SimpleDashelConnection

	SimpleDashelConnection::SimpleDashelConnection(unsigned port):
		stream(0)
	{
		try
		{
			Dashel::Hub::connect(QString("tcpin:port=%1").arg(port).toStdString());
		}
		catch (Dashel::DashelException e)
		{
			QMessageBox::critical(0, QApplication::tr("Aseba Playground"), QApplication::tr("Cannot create listening port %0: %1").arg(port).arg(e.what()));
			abort();
		}
	}

	void SimpleDashelConnection::sendBuffer(uint16 nodeId, const uint8* data, uint16 length)
	{
		if (stream)
		{
			try
			{
				uint16 temp;
				
				// this may happen if target has disconnected
				temp = bswap16(length - 2);
				stream->write(&temp, 2);
				temp = bswap16(nodeId);
				stream->write(&temp, 2);
				stream->write(data, length);
				stream->flush();
			}
			catch (Dashel::DashelException e)
			{
				LOG_ERR(QString("Target %0, cannot read from socket: %1").arg(stream->getTargetName().c_str()).arg(e.what()));
			}
		}
	}

	uint16 SimpleDashelConnection::getBuffer(uint8* data, uint16 maxLength, uint16* source)
	{
		if (lastMessageData.size())
		{
			*source = lastMessageSource;
			size_t len(std::min<size_t>(maxLength, lastMessageData.size()));
			memcpy(data, &lastMessageData[0], len);
			return len;
		}
		return 0;
	}

	void SimpleDashelConnection::connectionCreated(Dashel::Stream *stream)
	{
		const std::string& targetName(stream->getTargetName());
		if (targetName.substr(0, targetName.find_first_of(':')) == "tcp")
		{
			// schedule current stream for disconnection
			if (this->stream)
				toDisconnect.push_back(this->stream);
			
			// set new stream as current stream
			this->stream = stream;
			LOG_INFO(QString("New client connected from ") + stream->getTargetName().c_str());
		}
	}

	void SimpleDashelConnection::incomingData(Dashel::Stream *stream)
	{
		assert(stream == this->stream);
		try
		{
			// receive data
			uint16 temp;
			uint16 len;
			
			stream->read(&temp, 2);
			len = bswap16(temp);
			stream->read(&temp, 2);
			lastMessageSource = bswap16(temp);
			lastMessageData.resize(len+2);
			stream->read(&lastMessageData[0], lastMessageData.size());
		
			// execute event on all VM that are linked to this connection
			for (VMStateToEnvironment::iterator it(Aseba::vmStateToEnvironment.begin()); it != vmStateToEnvironment.end(); ++it)
			{
				if (it.value().second == this)
					AsebaProcessIncomingEvents(it.key());
			}
		}
		catch (Dashel::DashelException e)
		{
			LOG_ERR(QString("Target %0, cannot read from socket: %1").arg(stream->getTargetName().c_str()).arg(e.what()));
		}
	}

	void SimpleDashelConnection::connectionClosed(Dashel::Stream *stream, bool abnormal)
	{
		if (stream == this->stream)
		{
			this->stream = 0;
			// clear breakpoints on all VM that are linked to this connection
			for (VMStateToEnvironment::iterator it(Aseba::vmStateToEnvironment.begin()); it != vmStateToEnvironment.end(); ++it)
			{
				if (it.value().second == this)
					it.key()->breakpointsCount = 0;
			}
		}
		LOG_INFO(QString("Client disconnected properly from ") + stream->getTargetName().c_str());
	}
	
	void SimpleDashelConnection::closeOldStreams()
	{
		// disconnect old streams
		for (size_t i = 0; i < toDisconnect.size(); ++i)
		{
			LOG_WARN(QString("Old client disconnected from ") + toDisconnect[i]->getTargetName().c_str());
			closeStream(toDisconnect[i]);
		}
		toDisconnect.clear();
	}

} // Aseba

// implementation of Aseba glue C functions

extern "C" void AsebaPutVmToSleep(AsebaVMState *vm) 
{
	// not implemented in playground
}

extern "C" void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.value(vm));
	Aseba::AbstractNodeConnection* connection(environment.second);
	assert(connection);
	connection->sendBuffer(vm->nodeId, data, length);
}

extern "C" uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.value(vm));
	Aseba::AbstractNodeConnection* connection(environment.second);
	assert(connection);
	return connection->getBuffer(data, maxLength, source);
}

extern "C" const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.value(vm));
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	return glue->getDescription();
}

extern "C" const AsebaLocalEventDescription * AsebaGetLocalEventsDescriptions(AsebaVMState *vm)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.value(vm));
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	return glue->getLocalEventsDescriptions();
}

extern "C" const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.value(vm));
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	return glue->getNativeFunctionsDescriptions();
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.value(vm));
	Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	glue->callNativeFunction(id);
}

extern "C" void AsebaWriteBytecode(AsebaVMState *vm)
{
	// not implemented in playground
}

extern "C" void AsebaResetIntoBootloader(AsebaVMState *vm)
{
	// not implemented in playground
}

extern "C" void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	const Aseba::NodeEnvironment& environment(Aseba::vmStateToEnvironment.value(vm));
	const Aseba::AbstractNodeGlue* glue(environment.first);
	assert(glue);
	qDebug() << QString::fromStdString(Aseba::FormatableString("\nFatal error: glue %0 with node id %1 of type %2 at has produced exception: ").arg(glue).arg(vm->nodeId).arg(typeid(glue).name()));
	switch (reason)
	{
		case ASEBA_ASSERT_UNKNOWN: qDebug() << "undefined"; break;
		case ASEBA_ASSERT_UNKNOWN_UNARY_OPERATOR: qDebug() << "unknown unary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: qDebug() << "unknown binary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: qDebug() << "unknown bytecode"; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: qDebug() << "stack overflow"; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: qDebug() << "stack underflow"; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: qDebug() << "out of variables bounds"; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: qDebug() << "out of bytecode bounds"; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: qDebug() << "step out of run"; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: qDebug() << "breakpoint out of bytecode bounds"; break;
		case ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG: qDebug() << "tried to emit a buffer too long"; break;
		default: qDebug() << "unknown exception"; break;
	}
	qDebug() << ".\npc = " << vm->pc << ", sp = " << vm->sp;
	//qDebug() << "\nResetting VM" << std::endl;
	qDebug() << "\nAborting playground, please report bug to \nhttps://github.com/aseba-community/aseba/issues/new";
	abort();
	AsebaVMInit(vm);
}
