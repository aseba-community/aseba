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

// variables memory layout

AsebaVMDescription PlaygroundThymio2VMDescription = {
	"thymio-II",
	{
		{ 1, "_id" },
		{ 1, "event.source" },
		{ 32,"event.args" },
		{ 1, ASEBA_PID_VAR_NAME },
		{ 2, "_fwversion"},
		
		{ 1, "button.backward"},
		{ 1, "button.left"},
		{ 1, "button.center"},
		{ 1, "button.forward"},
		{ 1, "button.right"},
        
		{ 7, "prox.horizontal"},
		
		{ 1, "prox.comm.rx"},
		{ 1, "prox.comm.tx"},
		
		{ 2, "prox.ground.ambiant"},
		{ 2, "prox.ground.reflected"},
		{ 2, "prox.ground.delta"},
        
		{ 1, "motor.left.target"},
		{ 1, "motor.right.target"},
		{ 1, "motor.left.speed"},
		{ 1, "motor.right.speed"},
		{ 1, "motor.left.pwm"},
		{ 1, "motor.right.pwm"},
        
		{ 3, "acc"},
        
		{ 1, "temperature"},
        
		{ 1, "rc5.address"},
		{ 1, "rc5.command"},
        
		{ 1, "mic.intensity"},
		{ 1, "mic.threshold"},
		
		{ 2, "timer.period"},
        
		{ 0, NULL }
	}
};

// native functions

// sound

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sound_record = {
	"sound.record",
	"Start recording of rN.wav",
	{
		{1,"N"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sound_play = {
	"sound.play",
	"Start playback of pN.wav",
	{
		{1,"N"},
		{0,0},
	}
};


AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sound_replay = {
	"sound.replay",
	"Start playback of rN.wav",
	{
		{1,"N"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sound_system = {
	"sound.system",
	"Start playback of system sound N",
	{
		{1,"N"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sound_freq = {
	"sound.freq",
	"Play frequency",
	{
		{1,"Hz"},
		{1,"ds"},
		{0,0},
	}
};

#define WAVEFORM_SIZE 142

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sound_wave = {
	"sound.wave",
	"Set the primary wave of the tone generator",
	{
		{WAVEFORM_SIZE, "wave"},
		{0,0},
	}
};

// leds

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_circle = {
	"leds.circle",
	"Set circular ring leds",
	{
		{1,"led 0"},
		{1,"led 1"},
		{1,"led 2"},
		{1,"led 3"},
		{1,"led 4"},
		{1,"led 5"},
		{1,"led 6"},
		{1,"led 7"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_top = {
	"leds.top",
	"Set RGB top led",
	{
		{1,"red"},
		{1,"green"},
		{1,"blue"},
		{0,0},
	}
};


AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_bottom_right = {
	"leds.bottom.right",
	"Set RGB botom right led",
	{
		{1,"red"},
		{1,"green"},
		{1,"blue"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_bottom_left = {
	"leds.bottom.left",
	"Set RGB botom left led",
	{
		{1,"red"},
		{1,"green"},
		{1,"blue"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_buttons = {
	"leds.buttons",
	"Set buttons leds",
	{
		{1,"led 0"},
		{1,"led 1"},
		{1,"led 2"},
		{1,"led 3"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_prox_h = {
	"leds.prox.h",
	"Set horizontal proximity leds",
	{
		{1,"led 0"},
		{1,"led 1"},
		{1,"led 2"},
		{1,"led 3"},
		{1,"led 4"},
		{1,"led 5"},
		{1,"led 6"},
		{1,"led 7"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_prox_v = {
	"leds.prox.v",
	"Set vertical proximity leds",
	{
		{1,"led 0"},
		{1,"led 1"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_rc = {
	"leds.rc",
	"Set rc led",
	{
		{1,"led"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_leds_sound = {
	"leds.sound",
	"Set sound led",
	{
		{1,"led"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_led_temperature = {
	"leds.temperature",
	"Set ntc led",
	{
		{1,"red"},
		{1,"blue"},
		{0,0},
	}
};

// comm

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_prox_comm_enable = {
	"prox.comm.enable",
	"Enable or disable the proximity communication",
	{
		{1, "state"},
		{0,0},
	}
};

// sd

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sd_open = {
	"sd.open",
	"Open a file on the SD card",
	{
		{1, "number"},
		{1, "status"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sd_write = {
	"sd.write",
	"Write data to the opened file",
	{
		{-1, "data"},
		{1, "written"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sd_read = {
	"sd.read",
	"Read data from the opened file",
	{
		{-1, "data"},
		{1, "read"},
		{0,0},
	}
};

AsebaNativeFunctionDescription PlaygroundThymio2NativeDescription_sd_seek = {
	"sd.seek",
	"Seek the opened file",
	{
		{1, "position"},
		{1, "status"},
		{0,0},
	}
};
