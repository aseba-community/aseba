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

#ifndef __CHALLENGE_ASEBA_GLUE_H
#define __CHALLENGE_ASEBA_GLUE_H

#include "../../common/types.h"
#include "../../common/consts.h"
#include "../../vm/natives.h"
#include <dashel/dashel.h>
#include <valarray>
#include <vector>
#include <QMap>
#include <QPair>

namespace Aseba
{
	// Abstractions to virtualise VM and connection

	struct AbstractNodeGlue
	{
		virtual const AsebaVMDescription* getDescription() const = 0;
		virtual const AsebaLocalEventDescription * getLocalEventsDescriptions() const = 0;
		virtual const AsebaNativeFunctionDescription * const * getNativeFunctionsDescriptions() const = 0;
		virtual void callNativeFunction(uint16 id) = 0;
	};

	struct AbstractNodeConnection
	{
		virtual void sendBuffer(uint16 nodeId, const uint8* data, uint16 length) = 0;
		virtual uint16 getBuffer(uint8* data, uint16 maxLength, uint16* source) = 0;
	};

	// Mapping so that Aseba C callbacks can dispatch to the right objects
	
	typedef QPair<AbstractNodeGlue*, AbstractNodeConnection*> NodeEnvironment;
	typedef QMap<AsebaVMState*, NodeEnvironment> VMStateToEnvironment;

	extern VMStateToEnvironment vmStateToEnvironment;
	
	// Implementation of the connection using Dashel

	class SimpleDashelConnection: public AbstractNodeConnection, public Dashel::Hub
	{
		Dashel::Stream* stream;
		std::vector<Dashel::Stream*> toDisconnect; // all streams that must be disconnected at next step
		uint16 lastMessageSource;
		std::valarray<uint8> lastMessageData;

	public:
		SimpleDashelConnection(unsigned port);
		
		virtual void sendBuffer(uint16 nodeId, const uint8* data, uint16 length);
		virtual uint16 getBuffer(uint8* data, uint16 maxLength, uint16* source);
		
		virtual void connectionCreated(Dashel::Stream *stream);
		virtual void incomingData(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
		
		void closeOldStreams();
	};
	
} // Aseba

#endif // __CHALLENGE_ASEBA_GLUE_H
