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

#ifndef __PLAYGROUND_ASEBA_GLUE_H
#define __PLAYGROUND_ASEBA_GLUE_H

#include "../../common/types.h"
#include "../../common/consts.h"
#include "../../vm/natives.h"
#include <valarray>
#include <vector>
#include <map>
#include <string>

namespace Aseba
{
	// Abstractions to virtualise VM and connection
	
	struct AbstractNodeGlue
	{
		// default virtual destructor
		virtual ~AbstractNodeGlue() = default;
		
		// to be implemente by robots
		virtual const AsebaVMDescription* getDescription() const = 0;
		virtual const AsebaLocalEventDescription * getLocalEventsDescriptions() const = 0;
		virtual const AsebaNativeFunctionDescription * const * getNativeFunctionsDescriptions() const = 0;
		virtual void callNativeFunction(uint16_t id) = 0;
		
		// to be implemented by subclasses of robots for communicating with the external world
		virtual void externalInputStep(double dt) = 0;
	};
	
	struct NamedRobot
	{
		// Robot name
		const std::string robotName;
		
		NamedRobot(std::string robotName);
	};
	
	struct SingleVMNodeGlue: NamedRobot, AbstractNodeGlue
	{
		// VM implementation
		AsebaVMState vm;
		std::valarray<unsigned short> bytecode;
		std::valarray<signed short> stack;
		
		SingleVMNodeGlue(std::string robotName, int16_t nodeId);
	};

	struct AbstractNodeConnection
	{
		virtual void sendBuffer(uint16_t nodeId, const uint8_t* data, uint16_t length) = 0;
		virtual uint16_t getBuffer(uint8_t* data, uint16_t maxLength, uint16_t* source) = 0;
	};

	// Mapping so that Aseba C callbacks can dispatch to the right objects
	
	typedef std::pair<AbstractNodeGlue*, AbstractNodeConnection*> NodeEnvironment;
	typedef std::map<AsebaVMState*, NodeEnvironment> VMStateToEnvironment;

	extern VMStateToEnvironment vmStateToEnvironment;
	
	// Buffer for data reception
	
	class RecvBufferNodeConnection: public AbstractNodeConnection
	{
	protected:
		uint16_t lastMessageSource;
		std::valarray<uint8_t> lastMessageData;
		
	public:
		virtual uint16_t getBuffer(uint8_t* data, uint16_t maxLength, uint16_t* source);
	};
	
} // Aseba

#endif // __PLAYGROUND_ASEBA_GLUE_H
