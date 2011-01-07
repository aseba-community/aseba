/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

AsebaVMDescription vmLeftMotorDescription = {
	"left motor",
	{
		{ 1, "id" },
		{ 1, "source" },
		{ 32, "args" },
		{ 1, "speed" },
		{ 2, "odo" },
		{ 0, NULL }
	}
};

AsebaVMDescription vmRightMotorDescription = {
	"right motor",
	{
		{ 1, "id" },
		{ 1, "source" },
		{ 32, "args" },
		{ 1, "speed" },
		{ 2, "odo" },
		{ 0, NULL }
	}
};

AsebaVMDescription vmProximitySensorsDescription = {
	"proximity sensors",
	{
		{ 1, "id" },
		{ 1, "source" },
		{ 32, "args" },
		{ 24, "bumpers" },
		{ 12, "ground" },
		{ 0, NULL }
	}
};

AsebaVMDescription vmDistanceSensorsDescription = {
	"distance sensors",
	{
		{ 1, "id" },
		{ 1, "source" },
		{ 32, "args" },
		{ 180, "distances" },
		{ 0, NULL }
	}
};
