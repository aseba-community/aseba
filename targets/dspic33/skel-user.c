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


/* Descriptors */

const AsebaVMDescription vmDescription = {
	"myasebaname", 	// Name of the microcontroller
	{
		{ 1, "_id" },	// Do not touch it
		{ 1, "event.source" }, // nor this one
		{ VM_VARIABLES_ARG_SIZE, "event.args" },	// neither this one
		
		
		 /******
          ---> PUT YOUR VARIABLES DESCRIPTIONS HERE <---
         first value is the number of element in the array (1 if not an array)
         second value is the name of the variable which will be displayed in aseba studio
         ******/


		{ 0, NULL }	// null terminated
	}
};


static const AsebaLocalEventDescription localEvents[] = { 
	/*******
	---> PUT YOUR EVENT DESCRIPTIONS HERE <---
	First value is event "name" (will be used as "onvent name" in asebastudio
	second value is the event description)
	*******/
	{ NULL, NULL }
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescription[] = {
	&AsebaNativeDescription__system_reboot,
	&AsebaNativeDescription__system_settings_read,
	&AsebaNativeDescription__system_settings_write,
	&AsebaNativeDescription__system_settings_flash,
	
	ASEBA_NATIVES_STD_DESCRIPTIONS,
	0	// null terminated
};

static AsebaNativeFunctionPointer nativeFunctions[] = {
	AsebaResetIntoBootloader,
	AsebaNative__system_settings_read,
	AsebaNative__system_settings_write,
	AsebaNative__system_settings_flash,

	ASEBA_NATIVES_STD_FUNCTIONS,
};



