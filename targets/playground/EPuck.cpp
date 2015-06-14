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

#include "EPuck.h"
#include "Parameters.h"
#include "PlaygroundViewer.h"
#include "../../common/productids.h"
#include "../../common/utils/utils.h"

// Native functions

// specific to this robot, and their descriptions (in the companion C file)

using namespace Enki;

extern "C" void PlaygroundEPuckNative_energysend(AsebaVMState *vm)
{
	int index = AsebaNativePopArg(vm);
	
	// find related VM
	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaFeedableEPuck *epuck = dynamic_cast<AsebaFeedableEPuck*>(*objectIt);
		if (epuck && (&(epuck->vm) == vm) && (epuck->energy > EPUCK_INITIAL_ENERGY))
		{
			uint16 amount = vm->variables[index];
			
			unsigned toSend = std::min((unsigned)amount, (unsigned)epuck->energy);
			playgroundViewer->energyPool += toSend;
			epuck->energy -= toSend;
		}
	}
}

extern "C" AsebaNativeFunctionDescription PlaygroundEPuckNativeDescription_energysend;


extern "C" void PlaygroundEPuckNative_energyreceive(AsebaVMState *vm)
{
	int index = AsebaNativePopArg(vm);
	
	// find related VM
	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	World* world(playgroundViewer->getWorld());
	for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
	{
		AsebaFeedableEPuck *epuck = dynamic_cast<AsebaFeedableEPuck*>(*objectIt);
		if (epuck && (&(epuck->vm) == vm))
		{
			uint16 amount = vm->variables[index];
			
			unsigned toReceive = std::min((unsigned)amount, (unsigned)playgroundViewer->energyPool);
			playgroundViewer->energyPool -= toReceive;
			epuck->energy += toReceive;
		}
	}
}

extern "C" AsebaNativeFunctionDescription PlaygroundEPuckNativeDescription_energyreceive;


extern "C" void PlaygroundEPuckNative_energyamount(AsebaVMState *vm)
{
	int index = AsebaNativePopArg(vm);
	
	PlaygroundViewer* playgroundViewer(PlaygroundViewer::getInstance());
	vm->variables[index] = playgroundViewer->energyPool;
}

extern "C" AsebaNativeFunctionDescription PlaygroundEPuckNativeDescription_energyamount;


// objects in Enki

namespace Enki
{
	using namespace Aseba;
	
	// EPuckFeeding
	
	EPuckFeeding::EPuckFeeding(Robot *owner) : energy(EPUCK_FEEDER_INITIAL_ENERGY)
	{
		r = EPUCK_FEEDER_RANGE;
		this->owner = owner;
	}
	
	void EPuckFeeding::objectStep(double dt, World *w, PhysicalObject *po)
	{
		FeedableEPuck *epuck = dynamic_cast<FeedableEPuck *>(po);
		if (epuck && energy > 0)
		{
			double dEnergy = dt * EPUCK_FEEDER_D_ENERGY;
			epuck->energy += dEnergy;
			energy -= dEnergy;
			if (energy < EPUCK_FEEDER_THRESHOLD_HIDE)
				owner->setColor(EPUCK_FEEDER_COLOR_INACTIVE);
		}
	}
	
	void EPuckFeeding::finalize(double dt, World *w)
	{
		if ((energy < EPUCK_FEEDER_THRESHOLD_SHOW) && (energy+dt >= EPUCK_FEEDER_THRESHOLD_SHOW))
			owner->setColor(EPUCK_FEEDER_COLOR_ACTIVE);
		energy += EPUCK_FEEDER_RECHARGE_RATE * dt;
		if (energy > EPUCK_FEEDER_MAX_ENERGY)
			energy = EPUCK_FEEDER_MAX_ENERGY;
	}
	
	// EPuckFeeder
	
	EPuckFeeder::EPuckFeeder() : feeding(this)
	{
		setRectangular(3.2, 3.2, EPUCK_FEEDER_HEIGHT, -1);
		addLocalInteraction(&feeding);
		setColor(EPUCK_FEEDER_COLOR_ACTIVE);
	}
	
	// ScoreModifier
	
	void ScoreModifier::step(double dt, World *w)
	{
		double x = owner->pos.x;
		double y = owner->pos.y;
		if ((x > 32) && (x < 110.4-32) && (y > 67.2) && (y < 110.4-32))
			polymorphic_downcast<FeedableEPuck*>(owner)->score += dt * SCORE_MODIFIER_COEFFICIENT;
	}
	
	// FeedableEPuck
	
	FeedableEPuck::FeedableEPuck() :
		EPuck(CAPABILITY_BASIC_SENSORS | CAPABILITY_CAMERA), 
		energy(EPUCK_INITIAL_ENERGY),
		score(0),
		diedAnimation(-1),
		scoreModifier(this)
	{
		addGlobalInteraction(&scoreModifier);
	}
	
	void FeedableEPuck::controlStep(double dt)
	{
		EPuck::controlStep(dt);
		
		energy -= dt * EPUCK_ENERGY_CONSUMPTION_RATE;
		score += dt;
		if (energy < 0)
		{
			score /= 2;
			energy = EPUCK_INITIAL_ENERGY;
			diedAnimation = DEATH_ANIMATION_STEPS;
		}
		else if (diedAnimation >= 0)
			diedAnimation--;
	}
	
	// AsebaFeedableEPuck
	
	AsebaFeedableEPuck::AsebaFeedableEPuck(unsigned port, int id):
		SimpleDashelConnection(port)
	{
		vm.nodeId = id;
		
		bytecode.resize(1024);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(32);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
		
		vm.variables = reinterpret_cast<sint16 *>(&variables);
		vm.variablesSize = sizeof(variables) / sizeof(sint16);
		
		AsebaVMInit(&vm);
		
		variables.id = id;
		variables.productId = ASEBA_PID_PLAYGROUND_EPUCK;
		
		vmStateToEnvironment[&vm] = qMakePair((Aseba::AbstractNodeGlue*)this, (Aseba::AbstractNodeConnection *)this);
	}
	
	AsebaFeedableEPuck::~AsebaFeedableEPuck()
	{
		vmStateToEnvironment.remove(&vm);
	}
	
	void AsebaFeedableEPuck::controlStep(double dt)
	{
		// do a network step
		Hub::step();
		
		// disconnect old streams
		closeOldStreams();
		
		// get physical variables
		variables.prox[0] = static_cast<sint16>(infraredSensor0.getValue());
		variables.prox[1] = static_cast<sint16>(infraredSensor1.getValue());
		variables.prox[2] = static_cast<sint16>(infraredSensor2.getValue());
		variables.prox[3] = static_cast<sint16>(infraredSensor3.getValue());
		variables.prox[4] = static_cast<sint16>(infraredSensor4.getValue());
		variables.prox[5] = static_cast<sint16>(infraredSensor5.getValue());
		variables.prox[6] = static_cast<sint16>(infraredSensor6.getValue());
		variables.prox[7] = static_cast<sint16>(infraredSensor7.getValue());
		for (size_t i = 0; i < 60; i++)
		{
			variables.camR[i] = static_cast<sint16>(camera.image[i].r() * 100.);
			variables.camG[i] = static_cast<sint16>(camera.image[i].g() * 100.);
			variables.camB[i] = static_cast<sint16>(camera.image[i].b() * 100.);
		}
		
		variables.energy = static_cast<sint16>(energy);
		
		// run VM
		AsebaVMRun(&vm, 1000);
		
		// reschedule a IR sensors and camera events if we are not in step by step
		if (AsebaMaskIsClear(vm.flags, ASEBA_VM_STEP_BY_STEP_MASK) || AsebaMaskIsClear(vm.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			AsebaVMSetupEvent(&vm, ASEBA_EVENT_LOCAL_EVENTS_START);
			AsebaVMRun(&vm, 1000);
			AsebaVMSetupEvent(&vm, ASEBA_EVENT_LOCAL_EVENTS_START-1);
			AsebaVMRun(&vm, 1000);
		}
		
		// set physical variables
		leftSpeed = (double)(variables.speedL * 12.8) / 1000.;
		rightSpeed = (double)(variables.speedR * 12.8) / 1000.;
		setColor(Color(
			Aseba::clamp<double>(variables.colorR*0.01, 0, 1),
			Aseba::clamp<double>(variables.colorG*0.01, 0, 1),
			Aseba::clamp<double>(variables.colorB*0.01, 0, 1)
		));
		
		// set motion
		FeedableEPuck::controlStep(dt);
	}
	
	
	// robot description
	
	extern "C" AsebaVMDescription PlaygroundEPuckVMDescription;
	
	// really ugly and *thread unsafe* hack to have e-puck with different names
	static char ePuckName[] = "e-puck0";

	const AsebaVMDescription* AsebaFeedableEPuck::getDescription() const
	{
		const unsigned id(Aseba::clamp<unsigned>(vm.nodeId-1,0,9));
		ePuckName[6] = '0' + id;
		PlaygroundEPuckVMDescription.name = ePuckName;
		return &PlaygroundEPuckVMDescription;
	}
	
	
	// local events, static so only visible in this file
	
	static const AsebaLocalEventDescription localEvents[] = {
		{ "ir_sensors", "IR sensors updated" },
		{"camera", "camera updated"},
		{ NULL, NULL }
	};
	
	const AsebaLocalEventDescription * AsebaFeedableEPuck::getLocalEventsDescriptions() const
	{
		return localEvents;
	}
	
	
	// array of descriptions of native functions, static so only visible in this file
	
	static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
	{
		ASEBA_NATIVES_STD_DESCRIPTIONS,
		&PlaygroundEPuckNativeDescription_energysend,
		&PlaygroundEPuckNativeDescription_energyreceive,
		&PlaygroundEPuckNativeDescription_energyamount,
		0
	};

	const AsebaNativeFunctionDescription * const * AsebaFeedableEPuck::getNativeFunctionsDescriptions() const
	{
		return nativeFunctionsDescriptions;
	}
	
	// array of native functions, static so only visible in this file
	
	static AsebaNativeFunctionPointer nativeFunctions[] =
	{
		ASEBA_NATIVES_STD_FUNCTIONS,
		PlaygroundEPuckNative_energysend,
		PlaygroundEPuckNative_energyreceive,
		PlaygroundEPuckNative_energyamount
	};
	
	void AsebaFeedableEPuck::callNativeFunction(uint16 id)
	{
		nativeFunctions[id](&vm);
	}
	
} // Enki
