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

#include "Thymio2.h"
#include "Parameters.h"
#include "PlaygroundViewer.h"
#include "../../common/productids.h"
#include "../../common/utils/utils.h"

// Native functions

// specific to this robot, and their descriptions (in the companion C file)

using namespace Enki;

// TODO: put Thymio2 native functions here

// objects in Enki

namespace Enki
{
	using namespace Aseba;
	
	// AsebaThymio2
	
	AsebaThymio2::AsebaThymio2(unsigned port):
		SimpleDashelConnection(port)
	{
		vm.nodeId = 1;
		
		bytecode.resize(766+768);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(32);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
		
		vm.variables = reinterpret_cast<sint16 *>(&variables);
		vm.variablesSize = sizeof(variables) / sizeof(sint16);
		
		AsebaVMInit(&vm);
		
		variables.id = vm.nodeId;
		variables.productId = ASEBA_PID_THYMIO2;
		
		vmStateToEnvironment[&vm] = qMakePair((Aseba::AbstractNodeGlue*)this, (Aseba::AbstractNodeConnection *)this);
	}
	
	AsebaThymio2::~AsebaThymio2()
	{
		vmStateToEnvironment.remove(&vm);
	}
	
	void AsebaThymio2::controlStep(double dt)
	{
		// do a network step
		Hub::step();
		
		// disconnect old streams
		closeOldStreams();
		
		// get physical variables
		// TODO: implement
		
		// run VM
		AsebaVMRun(&vm, 1000);
		
		// TODO: change execution policy of local events to match the one of Thymio2
		// reschedule local events if we are not in step by step
		if (AsebaMaskIsClear(vm.flags, ASEBA_VM_STEP_BY_STEP_MASK) || AsebaMaskIsClear(vm.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			// TODO: implement events
		}
		
		// set physical variables
		// TODO: implement
		
		// set motion
		Thymio2::controlStep(dt);
	}
	
	
	// robot description
	
	extern "C" AsebaVMDescription PlaygroundThymio2VMDescription;
	
	const AsebaVMDescription* AsebaThymio2::getDescription() const
	{
		return &PlaygroundThymio2VMDescription;
	}
	
	
	// local events, static so only visible in this file
	
	static const AsebaLocalEventDescription localEvents[] = {
		// TODO: add description of implemented events
		{ NULL, NULL }
	};
	
	const AsebaLocalEventDescription * AsebaThymio2::getLocalEventsDescriptions() const
	{
		return localEvents;
	}
	
	
	// array of descriptions of native functions, static so only visible in this file
	
	static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
	{
		ASEBA_NATIVES_STD_DESCRIPTIONS,
		// TODO: add Thymio-specific native function descriptions
		0
	};

	const AsebaNativeFunctionDescription * const * AsebaThymio2::getNativeFunctionsDescriptions() const
	{
		return nativeFunctionsDescriptions;
	}
	
	// array of native functions, static so only visible in this file
	
	static AsebaNativeFunctionPointer nativeFunctions[] =
	{
		ASEBA_NATIVES_STD_FUNCTIONS,
		// TODO: add Thymio-specific native functions
	};
	
	void AsebaThymio2::callNativeFunction(uint16 id)
	{
		nativeFunctions[id](&vm);
	}
	
} // Enki
