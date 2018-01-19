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

#include "DashelAsebaGlue.h"
#include "EnkiGlue.h"
#include "../../common/utils/FormatableString.h"
#include "../../transport/buffer/vm-buffer.h"

namespace Aseba
{
	// SimpleDashelConnection

	SimpleDashelConnection::SimpleDashelConnection(unsigned port)
	{
		try
		{
			listenStream = Dashel::Hub::connect(FormatableString("tcpin:port=%0").arg(port));
		}
		catch (const Dashel::DashelException& e)
		{
			SEND_NOTIFICATION(FATAL_ERROR, "cannot create listening port", std::to_string(port), e.what());
			abort();
		}
	}

	void SimpleDashelConnection::sendBuffer(uint16_t nodeId, const uint8_t* data, uint16_t length)
	{
		if (stream)
		{
			try
			{
				uint16_t temp;

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
				SEND_NOTIFICATION(LOG_ERROR, "cannot read from socket", stream->getTargetName(), e.what());
			}
		}
	}

	void SimpleDashelConnection::connectionCreated(Dashel::Stream* stream)
	{
		const std::string& targetName(stream->getTargetName());
		if (targetName.substr(0, targetName.find_first_of(':')) == "tcp")
		{
			// schedule current stream for disconnection
			if (this->stream)
			{
				toDisconnect.push_back(this->stream);
				clearBreakpoints();
			}

			// set new stream as current stream
			this->stream = stream;
			SEND_NOTIFICATION(LOG_INFO, "new client connected", stream->getTargetName());
		}
	}

	void SimpleDashelConnection::incomingData(Dashel::Stream* stream)
	{
		// if we receive data from an old connection, disregard as we'll close the old stream soon
		if (stream != this->stream)
		{
			// read one byte to avoid deadlock
			char c;
			stream->read(&c, 1);
			return;
		}

		try
		{
			// receive data
			uint16_t temp;
			uint16_t len;

			stream->read(&temp, 2);
			len = bswap16(temp);
			stream->read(&temp, 2);
			lastMessageSource = bswap16(temp);
			lastMessageData.resize(len + 2);
			stream->read(&lastMessageData[0], lastMessageData.size());

			// execute event on all VM that are linked to this connection
			for (auto vmStateToEnvironmentKV : vmStateToEnvironment)
			{
				if (vmStateToEnvironmentKV.second.second == this)
				{
					AsebaProcessIncomingEvents(vmStateToEnvironmentKV.first);
					AsebaVMRun(vmStateToEnvironmentKV.first, 1000);
				}
			}
		}
		catch (Dashel::DashelException e)
		{
			SEND_NOTIFICATION(LOG_ERROR, "cannot read from socket", stream->getTargetName(), e.what());
		}
	}

	void SimpleDashelConnection::connectionClosed(Dashel::Stream* stream, bool abnormal)
	{
		// if the stream being closed is the current one (not old), clear breakpoints and reset current
		if (stream == this->stream)
		{
			clearBreakpoints();
			this->stream = nullptr;
		}
		if (abnormal)
		{
			SEND_NOTIFICATION(LOG_WARNING, "client disconnected abnormally", stream->getTargetName());
		}
		else
		{
			SEND_NOTIFICATION(LOG_INFO, "client disconnected properly", stream->getTargetName());
		}
	}

	//! Clear breakpoints on all VM that are linked to this connection
	void SimpleDashelConnection::clearBreakpoints()
	{
		for (auto vmStateToEnvironmentKV : vmStateToEnvironment)
		{
			if (vmStateToEnvironmentKV.second.second == this)
				vmStateToEnvironmentKV.first->breakpointsCount = 0;
		}
	}

	//! Disconnect old streams
	void SimpleDashelConnection::closeOldStreams()
	{
		for (size_t i = 0; i < toDisconnect.size(); ++i)
		{
			SEND_NOTIFICATION(LOG_WARNING, "old client disconnected", toDisconnect[i]->getTargetName());
			closeStream(toDisconnect[i]);
		}
		toDisconnect.clear();
	}

} // namespace Aseba
