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

#ifndef __CHALLENGE_THYMIO2_H
#define __CHALLENGE_THYMIO2_H

#include "AsebaGlue.h"
#include <enki/PhysicalEngine.h>
// TODO: switch to Thymio2 when robot implemented in Enki
#include <enki/robots/e-puck/EPuck.h>
//#include <enki/robots/thymio2/Thymio2.h>

namespace Enki
{
	// TODO: remove this placeholder once the Thymio2 exists in Enki
	typedef EPuck Thymio2;
	
	class AsebaThymio2 : public Thymio2, public Aseba::AbstractNodeGlue, public Aseba::SimpleDashelConnection
	{
	public:
		AsebaVMState vm;
		std::valarray<unsigned short> bytecode;
		std::valarray<signed short> stack;
		struct Variables
		{
			sint16 id;
			sint16 source;
			sint16 args[32];
			sint16 productId; 
			// TODO: put additional aseba vm variables here
			sint16 freeSpace[512];
		} variables;
		
		
	public:
		AsebaThymio2(unsigned port);
		virtual ~AsebaThymio2();
		
		// from THymio2
		
		virtual void controlStep(double dt);
		
		// from AbstractNodeGlue
		
		virtual const AsebaVMDescription* getDescription() const;
		virtual const AsebaLocalEventDescription * getLocalEventsDescriptions() const;
		virtual const AsebaNativeFunctionDescription * const * getNativeFunctionsDescriptions() const;
		virtual void callNativeFunction(uint16 id);
	};
} // Enki

#endif // __CHALLENGE_EPUCK_H
