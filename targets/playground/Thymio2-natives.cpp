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
#include "PlaygroundViewer.h"
#include "../../common/consts.h"

using namespace Enki;

// utility functions

static sint16 clampValueTo32(sint16 v)
{
	if (v > 32)
		return 32;
	else if (v < 0)
		return 32;
	else
		return v;
}

// simulated native functions

// sound

extern "C" void PlaygroundThymio2Native_sound_record(AsebaVMState *vm)
{
	const sint16 number(vm->variables[AsebaNativePopArg(vm)]);
	
	// do nothing for now
	ASEBA_UNUSED(number);
}

extern "C" void PlaygroundThymio2Native_sound_play(AsebaVMState *vm)
{
	const sint16 number(vm->variables[AsebaNativePopArg(vm)]);
	
	// do nothing for now
	ASEBA_UNUSED(number);
}

extern "C" void PlaygroundThymio2Native_sound_replay(AsebaVMState *vm)
{
	const sint16 number(vm->variables[AsebaNativePopArg(vm)]);

	// do nothing for now
	ASEBA_UNUSED(number);
}

extern "C" void PlaygroundThymio2Native_sound_system(AsebaVMState *vm)
{
	const sint16 number(vm->variables[AsebaNativePopArg(vm)]);
	
	// do nothing for now
	ASEBA_UNUSED(number);
}


extern "C" void PlaygroundThymio2Native_sound_freq(AsebaVMState * vm)
{
	const sint16 freq(vm->variables[AsebaNativePopArg(vm)]);
	const sint16 time(vm->variables[AsebaNativePopArg(vm)]);
	
	// do nothing for now
	ASEBA_UNUSED(freq);
	ASEBA_UNUSED(time);
}

extern "C" void PlaygroundThymio2Native_sound_wave(AsebaVMState * vm)
{
	const uint16 waveAddr(AsebaNativePopArg(vm));
	
	// do nothing for now
	ASEBA_UNUSED(waveAddr);
}

// leds

extern "C" void PlaygroundThymio2Native_leds_circle(AsebaVMState *vm)
{
	const sint16 l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l2(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l3(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l4(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l5(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l6(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l7(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(l0);
	ASEBA_UNUSED(l1);
	ASEBA_UNUSED(l2);
	ASEBA_UNUSED(l3);
	ASEBA_UNUSED(l4);
	ASEBA_UNUSED(l5);
	ASEBA_UNUSED(l6);
	ASEBA_UNUSED(l7);
}

static double paramToColor(sint16 param)
{
	return (param*5.46875+80) / 255.;
}

static double _fmax(double x, double y)
{
	if (x > y)
		return x;
	else
		return y;
}

extern "C" void PlaygroundThymio2Native_leds_top(AsebaVMState *vm)
{
	const double r(paramToColor(clampValueTo32(vm->variables[AsebaNativePopArg(vm)])));
	const double g(paramToColor(clampValueTo32(vm->variables[AsebaNativePopArg(vm)])));
	const double b(paramToColor(clampValueTo32(vm->variables[AsebaNativePopArg(vm)])));
	const double a(_fmax(_fmax(r, g), b));
	
	ASEBA_UNUSED(r);
	ASEBA_UNUSED(g);
	ASEBA_UNUSED(b);
	
	// find related VM
	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaThymio2 *thymio2 = dynamic_cast<AsebaThymio2*>(*objectIt);
		if (thymio2 && (&(thymio2->vm) == vm))
		{
			// change color of Enki object
			thymio2->setColor(Color(1.- a + a*r, 1.- a + a*g, 1. - a + a*b));
		}
	}
}

extern "C" void PlaygroundThymio2Native_leds_bottom_right(AsebaVMState *vm)
{
	const sint16 r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(r);
	ASEBA_UNUSED(g);
	ASEBA_UNUSED(b);
}

extern "C" void PlaygroundThymio2Native_leds_bottom_left(AsebaVMState *vm)
{
	const sint16 r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(r);
	ASEBA_UNUSED(g);
	ASEBA_UNUSED(b);
}

extern "C" void PlaygroundThymio2Native_leds_buttons(AsebaVMState *vm)
{
	const sint16 l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l2(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l3(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(l0);
	ASEBA_UNUSED(l1);
	ASEBA_UNUSED(l2);
	ASEBA_UNUSED(l3);
}

extern "C" void PlaygroundThymio2Native_leds_prox_h(AsebaVMState *vm)
{
	const sint16 l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l2(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l3(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l4(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l5(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l6(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l7(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(l0);
	ASEBA_UNUSED(l1);
	ASEBA_UNUSED(l2);
	ASEBA_UNUSED(l3);
	ASEBA_UNUSED(l4);
	ASEBA_UNUSED(l5);
	ASEBA_UNUSED(l6);
	ASEBA_UNUSED(l7);
}

extern "C" void PlaygroundThymio2Native_leds_prox_v(AsebaVMState *vm)
{
	const sint16 l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(l0);
	ASEBA_UNUSED(l1);
}

extern "C" void PlaygroundThymio2Native_leds_rc(AsebaVMState *vm)
{
	const sint16 l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(l0);
}

extern "C" void PlaygroundThymio2Native_leds_sound(AsebaVMState *vm)
{
	const sint16 l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	
	// do nothing for now
	ASEBA_UNUSED(l0);
}

extern "C" void PlaygroundThymio2Native_leds_temperature(AsebaVMState *vm)
{
	const sint16 r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));

	// do nothing for now
	ASEBA_UNUSED(r);
	ASEBA_UNUSED(b);
}

extern "C" void PlaygroundThymio2Native_prox_comm_enable(AsebaVMState *vm)
{
	const sint16 enable(vm->variables[AsebaNativePopArg(vm)]);
	
	// do nothing for now
	ASEBA_UNUSED(enable);
}

extern "C" void PlaygroundThymio2Native_sd_open(AsebaVMState *vm)
{
	const sint16 number(vm->variables[AsebaNativePopArg(vm)]);
	const uint16 statusAddr(AsebaNativePopArg(vm));
	
	// do nothing for now
	ASEBA_UNUSED(number);
	ASEBA_UNUSED(statusAddr);
}

extern "C" void PlaygroundThymio2Native_sd_write(AsebaVMState *vm)
{
	const uint16 dataAddr(AsebaNativePopArg(vm));
	const uint16 statusAddr(AsebaNativePopArg(vm));
	const uint16 dataLength(AsebaNativePopArg(vm));
	
	// do nothing for now
	ASEBA_UNUSED(dataAddr);
	ASEBA_UNUSED(statusAddr);
	ASEBA_UNUSED(dataLength);
}

extern "C" void PlaygroundThymio2Native_sd_read(AsebaVMState *vm)
{
	const uint16 dataAddr(AsebaNativePopArg(vm));
	const uint16 statusAddr(AsebaNativePopArg(vm));
	const uint16 dataLength(AsebaNativePopArg(vm));
	
	// do nothing for now
	ASEBA_UNUSED(dataAddr);
	ASEBA_UNUSED(statusAddr);
	ASEBA_UNUSED(dataLength);
}

extern "C" void PlaygroundThymio2Native_sd_seek(AsebaVMState *vm)
{
	const sint16 seek(vm->variables[AsebaNativePopArg(vm)]);
	const sint16 statusAddr(AsebaNativePopArg(vm));
	
	// do nothing for now
	ASEBA_UNUSED(seek);
	ASEBA_UNUSED(statusAddr);
}
