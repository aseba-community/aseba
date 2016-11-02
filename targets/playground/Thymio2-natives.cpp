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

	ASEBA_UNUSED(l0);
	ASEBA_UNUSED(l1);
	ASEBA_UNUSED(l2);
	ASEBA_UNUSED(l3);
	ASEBA_UNUSED(l4);
	ASEBA_UNUSED(l5);
	ASEBA_UNUSED(l6);
	ASEBA_UNUSED(l7);

	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaThymio2 *thymio2 = dynamic_cast<AsebaThymio2*>(*objectIt);
		if (thymio2 && (&(thymio2->vm) == vm))
		{
			thymio2->setLedIntensity(Thymio2::RING_0, l0/32.);
			thymio2->setLedIntensity(Thymio2::RING_1, l1/32.);
			thymio2->setLedIntensity(Thymio2::RING_2, l2/32.);
			thymio2->setLedIntensity(Thymio2::RING_3, l3/32.);
			thymio2->setLedIntensity(Thymio2::RING_4, l4/32.);
			thymio2->setLedIntensity(Thymio2::RING_5, l5/32.);
			thymio2->setLedIntensity(Thymio2::RING_6, l6/32.);
			thymio2->setLedIntensity(Thymio2::RING_7, l7/32.);
			return;
		}
	}
}

static sint16 _imax(sint16 x, sint16 y)
{
	if (x > y)
		return x;
	else
		return y;
}

extern "C" void PlaygroundThymio2Native_leds_top(AsebaVMState *vm)
{
	const sint16 r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 a(_imax(_imax(r, g), b));
	const double param(1./_imax(_imax(r, g),_imax(1,b)));

	ASEBA_UNUSED(r);
	ASEBA_UNUSED(g);
	ASEBA_UNUSED(b);
	
	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaThymio2 *thymio2 = dynamic_cast<AsebaThymio2*>(*objectIt);
		if (thymio2 && (&(thymio2->vm) == vm))
		{
			thymio2->setLedColor(Thymio2::TOP, Color(param*r,param*g,param*b,a/32.));
			return;
		}
	}
}

extern "C" void PlaygroundThymio2Native_leds_bottom_right(AsebaVMState *vm)
{
	const sint16 r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 a(_imax(_imax(r, g), b));
	const double param(1./_imax(_imax(r, g),_imax(1,b)));

	ASEBA_UNUSED(r);
	ASEBA_UNUSED(g);
	ASEBA_UNUSED(b);

	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaThymio2 *thymio2 = dynamic_cast<AsebaThymio2*>(*objectIt);
		if (thymio2 && (&(thymio2->vm) == vm))
		{
			thymio2->setLedColor(Thymio2::BOTTOM_RIGHT, Color(param*r,param*g,param*b,a/32.));
			return;
		}
	}
}

extern "C" void PlaygroundThymio2Native_leds_bottom_left(AsebaVMState *vm)
{
	const sint16 r(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 g(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 b(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 a(_imax(_imax(r, g), b));
	const double param(1./_imax(_imax(r, g),_imax(1,b)));

	ASEBA_UNUSED(r);
	ASEBA_UNUSED(g);
	ASEBA_UNUSED(b);

	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaThymio2 *thymio2 = dynamic_cast<AsebaThymio2*>(*objectIt);
		if (thymio2 && (&(thymio2->vm) == vm))
		{
			thymio2->setLedColor(Thymio2::BOTTOM_LEFT, Color(param*r,param*g,param*b,a/32.));
			return;
		}
	}
}

extern "C" void PlaygroundThymio2Native_leds_buttons(AsebaVMState *vm)
{
	const sint16 l0(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l1(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l2(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));
	const sint16 l3(clampValueTo32(vm->variables[AsebaNativePopArg(vm)]));

	ASEBA_UNUSED(l0);
	ASEBA_UNUSED(l1);
	ASEBA_UNUSED(l2);
	ASEBA_UNUSED(l3);

	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaThymio2 *thymio2 = dynamic_cast<AsebaThymio2*>(*objectIt);
		if (thymio2 && (&(thymio2->vm) == vm))
		{
			thymio2->setLedIntensity(Thymio2::BUTTON_UP,    l0/32.);
			thymio2->setLedIntensity(Thymio2::BUTTON_RIGHT, l1/32.);
			thymio2->setLedIntensity(Thymio2::BUTTON_DOWN,  l2/32.);
			thymio2->setLedIntensity(Thymio2::BUTTON_LEFT,  l3/32.);
			return;
		}
	}
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

	ASEBA_UNUSED(l0);
	ASEBA_UNUSED(l1);
	ASEBA_UNUSED(l2);
	ASEBA_UNUSED(l3);
	ASEBA_UNUSED(l4);
	ASEBA_UNUSED(l5);
	ASEBA_UNUSED(l6);
	ASEBA_UNUSED(l7);

	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaThymio2 *thymio2 = dynamic_cast<AsebaThymio2*>(*objectIt);
		if (thymio2 && (&(thymio2->vm) == vm))
		{
			thymio2->setLedIntensity(Thymio2::IR_FRONT_0, l0/32.);
			thymio2->setLedIntensity(Thymio2::IR_FRONT_1, l1/32.);
			thymio2->setLedIntensity(Thymio2::IR_FRONT_2, l2/32.);
			thymio2->setLedIntensity(Thymio2::IR_FRONT_3, l3/32.);
			thymio2->setLedIntensity(Thymio2::IR_FRONT_4, l4/32.);
			thymio2->setLedIntensity(Thymio2::IR_FRONT_5, l5/32.);
			thymio2->setLedIntensity(Thymio2::IR_BACK_0,  l6/32.);
			thymio2->setLedIntensity(Thymio2::IR_BACK_1,  l7/32.);
			return;
		}
	}
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
