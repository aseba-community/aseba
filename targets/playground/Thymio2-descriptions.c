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

#include "../../vm/natives.h"
#include "../../common/productids.h"


AsebaVMDescription PlaygroundThymio2VMDescription = {
	"thymio-II",
	{
		{ 1, "_id" },	// Do not touch it
		{ 1, "event.source" }, // nor this one
		{ 32, "event.args" },	// neither this one
		{ 1, ASEBA_PID_VAR_NAME },
		
		// TODO: add additional Thymio2 variables here
		
		{ 0, NULL }
	}
};

// TODO: add the description of the Thymio2 native functions here


