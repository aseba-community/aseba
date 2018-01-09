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

#include "Thymio2-natives.h"
#include "Thymio2.h"
#include "../../EnkiGlue.h"
#include "../../../../common/consts.h"

using namespace Enki;

// utility functions

static int16_t clampValueTo32(int16_t v)
{
	if (v > 32)
		return 32;
	else if (v < 0)
		return 32;
	else
		return v;
}

void notifyMissingFeature()
{
	SEND_NOTIFICATION(DISPLAY_INFO, "missing Thymio2 feature");
}

void logNativeFromThymio2(AsebaThymio2& thymio2, unsigned id, std::vector<int16_t>&& args)
{
	if (thymio2.logThymioNativeCalls)
		thymio2.thymioNativeCallLog.emplace_back(AsebaThymio2::NativeCallLogEntry(id, std::move(args)));
}

void logNativeFromVM(AsebaVMState *vm, unsigned id, std::vector<int16_t>&& args)
{
	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
		logNativeFromThymio2(*thymio2, id, std::move(args));
}

// simulated native functions

// sound

extern "C" void PlaygroundThymio2Native_sound_record(AsebaVMState *vm)
{
	const int16_t number(vm->variables[AsebaNativePopArg(vm)]);
	
	logNativeFromVM(vm, 0, { number });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_sound_play(AsebaVMState *vm)
{
	const int16_t number(vm->variables[AsebaNativePopArg(vm)]);
	
	logNativeFromVM(vm, 1, { number });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_sound_replay(AsebaVMState *vm)
{
	const int16_t number(vm->variables[AsebaNativePopArg(vm)]);

	logNativeFromVM(vm, 2, { number });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_sound_duration(AsebaVMState *vm)
{
	const int16_t number(vm->variables[AsebaNativePopArg(vm)]);
	const uint16_t durationAddr(AsebaNativePopArg(vm));
	
	vm->variables[durationAddr] = 0;

	logNativeFromVM(vm, 21, { number, static_cast<int16_t>(durationAddr) });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_sound_system(AsebaVMState *vm)
{
	const int16_t number(vm->variables[AsebaNativePopArg(vm)]);
	
	logNativeFromVM(vm, 3, { number });
	
	if (number != -1)
		notifyMissingFeature();
}


extern "C" void PlaygroundThymio2Native_sound_freq(AsebaVMState * vm)
{
	const int16_t freq(vm->variables[AsebaNativePopArg(vm)]);
	const int16_t time(vm->variables[AsebaNativePopArg(vm)]);
	
	logNativeFromVM(vm, 8, { freq, time });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_sound_wave(AsebaVMState * vm)
{
	const uint16_t waveAddr(AsebaNativePopArg(vm));
	
	// Note: if necessary, copy data of the wave form to the log
	logNativeFromVM(vm, 15, { static_cast<int16_t>(waveAddr) });
	
	notifyMissingFeature();
}

// leds

extern "C" void PlaygroundThymio2Native_leds_circle(AsebaVMState *vm)
{
	const int16_t l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l2(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l3(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l4(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l5(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l6(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l7(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));

	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		thymio2->setLedIntensity(Thymio2::RING_0, l0/32.);
		thymio2->setLedIntensity(Thymio2::RING_1, l1/32.);
		thymio2->setLedIntensity(Thymio2::RING_2, l2/32.);
		thymio2->setLedIntensity(Thymio2::RING_3, l3/32.);
		thymio2->setLedIntensity(Thymio2::RING_4, l4/32.);
		thymio2->setLedIntensity(Thymio2::RING_5, l5/32.);
		thymio2->setLedIntensity(Thymio2::RING_6, l6/32.);
		thymio2->setLedIntensity(Thymio2::RING_7, l7/32.);
		
		logNativeFromThymio2(*thymio2, 4, { l0, l1, l2, l3, l4, l5, l6, l7 });
	}
}

extern "C" void PlaygroundThymio2Native_leds_top(AsebaVMState *vm)
{
	const int16_t r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t a(std::max(std::max(r, g), b));
	const double param(1./std::max(std::max(r, g),std::max((int16_t)1,b)));

	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		thymio2->setLedColor(Thymio2::TOP, Color(param*r,param*g,param*b,a/32.));
		
		logNativeFromThymio2(*thymio2, 5, { r, g, b });
	}
}

extern "C" void PlaygroundThymio2Native_leds_bottom_right(AsebaVMState *vm)
{
	const int16_t r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t a(std::max(std::max(r, g), b));
	const double param(1./std::max(std::max(r, g),std::max((int16_t)1,b)));

	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		thymio2->setLedColor(Thymio2::BOTTOM_RIGHT, Color(param*r,param*g,param*b,a/32.));
		
		logNativeFromThymio2(*thymio2, 6, { r, g, b });
	}
}

extern "C" void PlaygroundThymio2Native_leds_bottom_left(AsebaVMState *vm)
{
	const int16_t r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t a(std::max(std::max(r, g), b));
	const double param(1./std::max(std::max(r, g),std::max((int16_t)1,b)));

	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		thymio2->setLedColor(Thymio2::BOTTOM_LEFT, Color(param*r,param*g,param*b,a/32.));
		
		logNativeFromThymio2(*thymio2, 7, { r, g, b });
	}
}

extern "C" void PlaygroundThymio2Native_leds_buttons(AsebaVMState *vm)
{
	const int16_t l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l2(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l3(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));

	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		thymio2->setLedIntensity(Thymio2::BUTTON_UP,    l0/32.);
		thymio2->setLedIntensity(Thymio2::BUTTON_RIGHT, l1/32.);
		thymio2->setLedIntensity(Thymio2::BUTTON_DOWN,  l2/32.);
		thymio2->setLedIntensity(Thymio2::BUTTON_LEFT,  l3/32.);
		
		logNativeFromThymio2(*thymio2, 9, { l0, l1, l2, l3 });
	}
}

extern "C" void PlaygroundThymio2Native_leds_prox_h(AsebaVMState *vm)
{
	const int16_t l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l2(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l3(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l4(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l5(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l6(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l7(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));

	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		thymio2->setLedIntensity(Thymio2::IR_FRONT_0, l0/32.);
		thymio2->setLedIntensity(Thymio2::IR_FRONT_1, l1/32.);
		thymio2->setLedIntensity(Thymio2::IR_FRONT_2, l2/32.);
		thymio2->setLedIntensity(Thymio2::IR_FRONT_3, l3/32.);
		thymio2->setLedIntensity(Thymio2::IR_FRONT_4, l4/32.);
		thymio2->setLedIntensity(Thymio2::IR_FRONT_5, l5/32.);
		thymio2->setLedIntensity(Thymio2::IR_BACK_0,  l6/32.);
		thymio2->setLedIntensity(Thymio2::IR_BACK_1,  l7/32.);
		
		logNativeFromThymio2(*thymio2, 10, { l0, l1, l2, l3, l4, l5, l6, l7 });
	}
}

extern "C" void PlaygroundThymio2Native_leds_prox_v(AsebaVMState *vm)
{
	const int16_t l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));

	logNativeFromVM(vm, 11, { l0, l1 });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_leds_rc(AsebaVMState *vm)
{
	const int16_t l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	logNativeFromVM(vm, 12, { l0 });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_leds_sound(AsebaVMState *vm)
{
	const int16_t l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	logNativeFromVM(vm, 13, { l0 });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_leds_temperature(AsebaVMState *vm)
{
	const int16_t r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const int16_t b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));

	logNativeFromVM(vm, 14, { r, b });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_prox_comm_enable(AsebaVMState *vm)
{
	const int16_t enable(vm->variables[AsebaNativePopArg(vm)]);
	
	logNativeFromVM(vm, 16, { enable });
	
	notifyMissingFeature();
}

extern "C" void PlaygroundThymio2Native_sd_open(AsebaVMState *vm)
{
	const int16_t number(vm->variables[AsebaNativePopArg(vm)]);
	const uint16_t statusAddr(AsebaNativePopArg(vm));
	
	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		int16_t result(0);
		// number must be [0:32767], or -1
		if (number < -1)
		{
			result = -1;
		}
		// try to open
		else if (!thymio2->openSDCardFile(number))
		{
			result = -1;
		}
		
		vm->variables[statusAddr] = result;
		
		logNativeFromThymio2(*thymio2, 17, { number, result });
	}
}

extern "C" void PlaygroundThymio2Native_sd_write(AsebaVMState *vm)
{
	const uint16_t dataAddr(AsebaNativePopArg(vm));
	const uint16_t statusAddr(AsebaNativePopArg(vm));
	const uint16_t dataLength(AsebaNativePopArg(vm));
	
	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		int16_t result(0);
		
		if (thymio2->sdCardFile)
			thymio2->sdCardFile.write(reinterpret_cast<const char*>(&vm->variables[dataAddr]), dataLength*2);
		
		if (thymio2->sdCardFile)
			result = dataLength;
		// a failed write will report 0. It is unlikely that a partial write occurs, but if so it is not handled properly.
		
		vm->variables[statusAddr] = result;
		
		// log the data written and the status
		std::vector<int16_t> data(&vm->variables[dataAddr], &vm->variables[dataAddr+dataLength]);
		data.push_back(result);
		logNativeFromThymio2(*thymio2, 18, std::move(data));
	}
}

extern "C" void PlaygroundThymio2Native_sd_read(AsebaVMState *vm)
{
	const uint16_t dataAddr(AsebaNativePopArg(vm));
	const uint16_t statusAddr(AsebaNativePopArg(vm));
	const uint16_t dataLength(AsebaNativePopArg(vm));
	
	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		int16_t result(0);
		
		if (thymio2->sdCardFile)
			thymio2->sdCardFile.read(reinterpret_cast<char*>(&vm->variables[dataAddr]), dataLength*2);
		
		if (thymio2->sdCardFile)
			result = dataLength;
		else
			result = thymio2->sdCardFile.gcount() / 2;
		
		vm->variables[statusAddr] = result;
		
		// log the data read and the status
		std::vector<int16_t> data(&vm->variables[dataAddr], &vm->variables[dataAddr+dataLength]);
		data.push_back(result);
		logNativeFromThymio2(*thymio2, 19, std::move(data));
	}
}

extern "C" void PlaygroundThymio2Native_sd_seek(AsebaVMState *vm)
{
	const int16_t seek(vm->variables[AsebaNativePopArg(vm)]);
	const int16_t statusAddr(AsebaNativePopArg(vm));
	
	AsebaThymio2* thymio2(getEnkiObject<AsebaThymio2>(vm));
	if (thymio2)
	{
		int16_t result(0);
		
		// if bad, try to close and re-open
		if (!thymio2->sdCardFile)
			thymio2->openSDCardFile(thymio2->sdCardFileNumber);
		
		// if good, try to seek
		if (thymio2->sdCardFile)
		{
			thymio2->sdCardFile.seekp(seek);
			thymio2->sdCardFile.seekg(seek);
		}
		
		if (!thymio2->sdCardFile)
			result = -1;
		
		vm->variables[statusAddr] = result;
		
		logNativeFromThymio2(*thymio2, 20, { seek, result } );
	}
}
