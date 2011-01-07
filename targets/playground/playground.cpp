/*
	Playground - An active arena to learn multi-robots programming
	Copyright (C) 1999--2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
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

#ifndef ASEBA_ASSERT
#define ASEBA_ASSERT
#endif

#include <../../vm/vm.h>
#include "../../vm/natives.h"
#include <../../common/consts.h>
#include <../../transport/buffer/vm-buffer.h>
#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>
#include <iostream>
#include <QtGui>
#include <QtDebug>
#include <QtXml>
#include "playground.h"
//#include <playground.moc>
#include <string.h>

#ifdef USE_SDL
#include <SDL/SDL.h>
#endif

//! Asserts a dynamic cast.	Similar to the one in boost/cast.hpp
template<typename Derived, typename Base>
inline Derived polymorphic_downcast(Base base)
{
	Derived derived = dynamic_cast<Derived>(base);
	assert(derived);
	return derived;
}

// Definition of the aseba glue

namespace Enki
{
	class AsebaFeedableEPuck;
}

extern "C" void PlaygroundNative_energysend(AsebaVMState *vm);
extern "C" void PlaygroundNative_energyreceive(AsebaVMState *vm);
extern "C" void PlaygroundNative_energyamount(AsebaVMState *vm);

extern "C" AsebaNativeFunctionDescription PlaygroundNativeDescription_energysend;
extern "C" AsebaNativeFunctionDescription PlaygroundNativeDescription_energyreceive;
extern "C" AsebaNativeFunctionDescription PlaygroundNativeDescription_energyamount;


static AsebaNativeFunctionPointer nativeFunctions[] =
{
	ASEBA_NATIVES_STD_FUNCTIONS,
	PlaygroundNative_energysend,
	PlaygroundNative_energyreceive,
	PlaygroundNative_energyamount
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
{
	ASEBA_NATIVES_STD_DESCRIPTIONS,
	&PlaygroundNativeDescription_energysend,
	&PlaygroundNativeDescription_energyreceive,
	&PlaygroundNativeDescription_energyamount,
	0
};

extern "C" AsebaVMDescription vmDescription;

// this uglyness is necessary to glue with C code
Enki::PlaygroundViewer *playgroundViewer = 0;

namespace Enki
{
	#define LEGO_RED						Enki::Color(0.77, 0.2, 0.15)
	#define LEGO_GREEN						Enki::Color(0, 0.5, 0.17)
	#define LEGO_BLUE						Enki::Color(0, 0.38 ,0.61)
	#define LEGO_WHITE						Enki::Color(0.9, 0.9, 0.9)
	
	#define EPUCK_FEEDER_INITIAL_ENERGY		10
	#define EPUCK_FEEDER_THRESHOLD_HIDE		2
	#define EPUCK_FEEDER_THRESHOLD_SHOW		4
	#define EPUCK_FEEDER_RADIUS				5
	#define EPUCK_FEEDER_RANGE				10
	
	#define EPUCK_FEEDER_COLOR_ACTIVE		LEGO_GREEN
	#define EPUCK_FEEDER_COLOR_INACTIVE		LEGO_WHITE
	
	#define EPUCK_FEEDER_D_ENERGY			4
	#define EPUCK_FEEDER_RECHARGE_RATE		0.5
	#define EPUCK_FEEDER_MAX_ENERGY			100
	
	#define EPUCK_FEEDER_HEIGHT				5
	
	#define EPUCK_INITIAL_ENERGY			10
	#define EPUCK_ENERGY_CONSUMPTION_RATE	1
	
	#define SCORE_MODIFIER_COEFFICIENT		0.2
	
	#define INITIAL_POOL_ENERGY				10
	
	#define ACTIVATION_OBJECT_COLOR			LEGO_RED
	#define ACTIVATION_OBJECT_HEIGHT		6
	
	#define DEATH_ANIMATION_STEPS			30
	
	extern GLint GenFeederBase();
	extern GLint GenFeederCharge0();
	extern GLint GenFeederCharge1();
	extern GLint GenFeederCharge2();
	extern GLint GenFeederCharge3();
	extern GLint GenFeederRing();
	
	class ScoreModifier: public GlobalInteraction
	{
	public:
		ScoreModifier(Robot* owner) : GlobalInteraction(owner) {}
		
		virtual void step(double dt, World *w);
	};
	
	class FeedableEPuck: public EPuck
	{
	public:
		double energy;
		double score;
		QString name;
		int diedAnimation;
		ScoreModifier scoreModifier;
	
	public:
		FeedableEPuck() :
			EPuck(CAPABILITY_BASIC_SENSORS | CAPABILITY_CAMERA), 
			energy(EPUCK_INITIAL_ENERGY),
			score(0),
			diedAnimation(-1),
			scoreModifier(this)
		{
			addGlobalInteraction(&scoreModifier);
		}
		
		void controlStep(double dt)
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
	};
	
	void ScoreModifier::step(double dt, World *w)
	{
		double x = owner->pos.x;
		double y = owner->pos.y;
		if ((x > 32) && (x < 110.4-32) && (y > 67.2) && (y < 110.4-32))
			polymorphic_downcast<FeedableEPuck*>(owner)->score += dt * SCORE_MODIFIER_COEFFICIENT;
	}
	
	class AsebaFeedableEPuck : public FeedableEPuck
	{
	public:
		int joystick;
		AsebaVMState vm;
		std::valarray<unsigned short> bytecode;
		std::valarray<signed short> stack;
		struct Variables
		{
			sint16 id;
			sint16 source;
			sint16 args[32];
			sint16 speedL; // left motor speed
			sint16 speedR; // right motor speed
			sint16 colorR; // body red [0..100] %
			sint16 colorG; // body green [0..100] %
			sint16 colorB; // body blue [0..100] %
			sint16 prox[8];	// 
			sint16 camR[60]; // camera red (left, middle, right) [0..100] %
			sint16 camG[60]; // camera green (left, middle, right) [0..100] %
			sint16 camB[60]; // camera blue (left, middle, right) [0..100] %
			sint16 energy;
			sint16 user[256];
		} variables;
		
	public:
		AsebaFeedableEPuck(int id) :
			joystick(-1)
		{
			vm.nodeId = id;
			
			bytecode.resize(512);
			vm.bytecode = &bytecode[0];
			vm.bytecodeSize = bytecode.size();
			
			stack.resize(64);
			vm.stack = &stack[0];
			vm.stackSize = stack.size();
			
			vm.variables = reinterpret_cast<sint16 *>(&variables);
			vm.variablesSize = sizeof(variables) / sizeof(sint16);
			
			AsebaVMInit(&vm);
			
			variables.id = id;
		}
		
		virtual ~AsebaFeedableEPuck()
		{
			
		}
		
	public:
		double toDoubleClamp(sint16 val, double mul, double min, double max)
		{
			double v = static_cast<double>(val) * mul;
			if (v > max)
				v = max;
			else if (v < min)
				v = min;
			return v;
		}
		
		void controlStep(double dt)
		{
			// set physical variables
			leftSpeed = (double)(variables.speedL * 12.8) / 1000.;
			rightSpeed = (double)(variables.speedR * 12.8) / 1000.;
			Color c;
			c.setR(toDoubleClamp(variables.colorR, 0.01, 0, 1));
			c.setG(toDoubleClamp(variables.colorG, 0.01, 0, 1));
			c.setB(toDoubleClamp(variables.colorB, 0.01, 0, 1));
			setColor(c);
			
			// if this robot is controlled by a joystick, override the wheel speed commands
			#ifdef USE_SDL
			SDL_JoystickUpdate();
			if (joystick >= 0 && joystick < playgroundViewer->joysticks.size())
			{
				const double SPEED_MAX = 13.;
				double x = SDL_JoystickGetAxis(playgroundViewer->joysticks[joystick], 0) / (32767. / SPEED_MAX);
				double y = -SDL_JoystickGetAxis(playgroundViewer->joysticks[joystick], 1) / (32767. / SPEED_MAX);
				leftSpeed = y + x;
				rightSpeed = y - x;
			}
			#endif // USE_SDL
			
			// do motion
			FeedableEPuck::controlStep(dt);
			
			// get physical variables
			variables.prox[0] = static_cast<sint16>(infraredSensor0.finalValue);
			variables.prox[1] = static_cast<sint16>(infraredSensor1.finalValue);
			variables.prox[2] = static_cast<sint16>(infraredSensor2.finalValue);
			variables.prox[3] = static_cast<sint16>(infraredSensor3.finalValue);
			variables.prox[4] = static_cast<sint16>(infraredSensor4.finalValue);
			variables.prox[5] = static_cast<sint16>(infraredSensor5.finalValue);
			variables.prox[6] = static_cast<sint16>(infraredSensor6.finalValue);
			variables.prox[7] = static_cast<sint16>(infraredSensor7.finalValue);
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
		}
	};
	
	class EPuckFeeding : public LocalInteraction
	{
	public:
		double energy;

	public :
		EPuckFeeding(Robot *owner) : energy(EPUCK_FEEDER_INITIAL_ENERGY)
		{
			r = EPUCK_FEEDER_RANGE;
			this->owner = owner;
		}
		
		void objectStep(double dt, World *w, PhysicalObject *po)
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
		
		void finalize(double dt, World *w)
		{
			if ((energy < EPUCK_FEEDER_THRESHOLD_SHOW) && (energy+dt >= EPUCK_FEEDER_THRESHOLD_SHOW))
				owner->setColor(EPUCK_FEEDER_COLOR_ACTIVE);
			energy += EPUCK_FEEDER_RECHARGE_RATE * dt;
			if (energy > EPUCK_FEEDER_MAX_ENERGY)
				energy = EPUCK_FEEDER_MAX_ENERGY;
		}
	};
	
	class EPuckFeeder : public Robot
	{
	public:
		EPuckFeeding feeding;
	
	public:
		EPuckFeeder() : feeding(this)
		{
			setRectangular(3.2, 3.2, EPUCK_FEEDER_HEIGHT, -1);
			addLocalInteraction(&feeding);
			setColor(EPUCK_FEEDER_COLOR_ACTIVE);
		}
	};
	
	class Door: public PhysicalObject
	{
	public:
		virtual void open() = 0;
		virtual void close() = 0;
	};
	
	class SlidingDoor : public Door
	{
	protected:
		Point closedPos;
		Point openedPos;
		double moveDuration;
		enum Mode
		{
			MODE_CLOSED,
			MODE_OPENING,
			MODE_OPENED,
			MODE_CLOSING
		} mode;
		double moveTimeLeft;
	
	public:
		SlidingDoor(const Point& closedPos, const Point& openedPos, const Point& size, double height, double moveDuration) :
			closedPos(closedPos),
			openedPos(openedPos),
			moveDuration(moveDuration),
			mode(MODE_CLOSED),
			moveTimeLeft(0)
		{
			setRectangular(size.x, size.y, height, -1);
		}
		
		virtual void controlStep(double dt)
		{
			// cos interpolation between positions
			double alpha;
			switch (mode)
			{
				case MODE_CLOSED:
				pos = closedPos;
				break;
				
				case MODE_OPENING:
				alpha = (cos((moveTimeLeft*M_PI)/moveDuration) + 1.) / 2.;
				pos = openedPos * alpha + closedPos * (1 - alpha);
				moveTimeLeft -= dt;
				if (moveTimeLeft < 0)
					mode = MODE_OPENED;
				break;
				
				case MODE_OPENED:
				pos = openedPos;
				break;
				
				case MODE_CLOSING:
				alpha = (cos((moveTimeLeft*M_PI)/moveDuration) + 1.) / 2.;
				pos = closedPos * alpha + openedPos * (1 - alpha);
				moveTimeLeft -= dt;
				if (moveTimeLeft < 0)
					mode = MODE_CLOSED;
				break;
				
				default:
				break;
			}
			PhysicalObject::controlStep(dt);
		}
		
		virtual void open(void)
		{
			if (mode == MODE_CLOSED)
			{
				moveTimeLeft = moveDuration;
				mode = MODE_OPENING;
			}
			else if (mode == MODE_CLOSING)
			{
				moveTimeLeft = moveDuration - moveTimeLeft;
				mode = MODE_OPENING;
			}
		}
		
		virtual void close(void)
		{
			if (mode == MODE_OPENED)
			{
				moveTimeLeft = moveDuration;
				mode = MODE_CLOSING;
			}
			else if (mode == MODE_OPENING)
			{
				moveTimeLeft = moveDuration - moveTimeLeft;
				mode = MODE_CLOSING;
			}
		}
	};
	
	class AreaActivating : public LocalInteraction
	{
	public:
		bool active;
		Polygone activeArea;
		
	public:
		AreaActivating(Robot *owner, const Polygone& activeArea) :
			active(false),
			activeArea(activeArea)
		{
			r = activeArea.getBoundingRadius();
			this->owner = owner;
		}
		
		virtual void init(double dt, World *w)
		{
			active = false;
		}
		
		virtual void objectStep (double dt, World *w, PhysicalObject *po)
		{
			if (po != owner && dynamic_cast<Robot*>(po))
			{
				active |= activeArea.isPointInside(po->pos - owner->pos);
			}
		}
	};
	
	class ActivationObject: public Robot
	{
	protected:
		AreaActivating areaActivating;
		bool wasActive;
		Door* attachedDoor;
	
	public:
		ActivationObject(const Point& pos, const Point& size, const Polygone& activeArea, Door* attachedDoor) :
			areaActivating(this, activeArea),
			wasActive(false),
			attachedDoor(attachedDoor)
		{
			this->pos = pos;
			setRectangular(size.x, size.y, ACTIVATION_OBJECT_HEIGHT, -1);
			addLocalInteraction(&areaActivating);
			setColor(ACTIVATION_OBJECT_COLOR);
		}
		
		virtual void controlStep(double dt)
		{
			Robot::controlStep(dt);
			
			if (areaActivating.active != wasActive)
			{
				//std::cerr << "Door activity changed to " << areaActivating.active <<  "\n";
				if (areaActivating.active)
					attachedDoor->open();
				else
					attachedDoor->close();
				wasActive = areaActivating.active;
			}
		}
	};

	PlaygroundViewer::PlaygroundViewer(World* world) : ViewerWidget(world), stream(0), energyPool(INITIAL_POOL_ENERGY)
	{
		font.setPixelSize(16);
		#if QT_VERSION >= 0x040400
		font.setLetterSpacing(QFont::PercentageSpacing, 130);
		#elif (!defined(_MSC_VER))
		#warning "Some feature have been disabled because you are using Qt < 4.4.0 !"
		#endif
		try
		{
			Dashel::Hub::connect(QString("tcpin:port=%1").arg(ASEBA_DEFAULT_PORT).toStdString());
		}
		catch (Dashel::DashelException e)
		{
			QMessageBox::critical(0, QApplication::tr("Aseba Playground"), QApplication::tr("Cannot create listening port %0: %1").arg(ASEBA_DEFAULT_PORT).arg(e.what()));
			abort();
		}
		playgroundViewer = this;
		
		#ifdef USE_SDL
		if((SDL_Init(SDL_INIT_JOYSTICK)==-1))
		{
			std::cerr << "Error : Could not initialize SDL: " << SDL_GetError() << std::endl;
			return;
		}
		
		int joystickCount = SDL_NumJoysticks();
		for (int i = 0; i < joystickCount; ++i)
		{
			SDL_Joystick* joystick = SDL_JoystickOpen(i);
			if (!joystick)
			{
				std::cerr << "Error: Can't open joystick " << i << std::endl;
				continue;
			}
			if (SDL_JoystickNumAxes(joystick) < 2)
			{
				std::cerr << "Error: not enough axis on joystick" << i<< std::endl;
				SDL_JoystickClose(joystick);
				continue;
			}
			joysticks.push_back(joystick);
		}
		#endif // USE_SDL
	}
	
	PlaygroundViewer::~PlaygroundViewer()
	{
		#ifdef USE_SDL
		for (int i = 0; i < joysticks.size(); ++i)
			SDL_JoystickClose(joysticks[i]);
		SDL_Quit();
		#endif // USE_SDL
	}
	
	void PlaygroundViewer::renderObjectsTypesHook()
	{
		managedObjectsAliases[&typeid(AsebaFeedableEPuck)] = &typeid(EPuck);
	}
	
	void PlaygroundViewer::sceneCompletedHook()
	{
		// create a map with names and scores
		//qglColor(QColor::fromRgbF(0, 0.38 ,0.61));
		qglColor(Qt::black);
		
		
		int i = 0;
		QString scoreString("Id.: E./Score. - ");
		int totalScore = 0;
		for (World::ObjectsIterator it = world->objects.begin(); it != world->objects.end(); ++it)
		{
			AsebaFeedableEPuck *epuck = dynamic_cast<AsebaFeedableEPuck*>(*it);
			if (epuck)
			{
				totalScore += (int)epuck->score;
				if (i != 0)
					scoreString += " - ";
				scoreString += QString("%0: %1/%2").arg(epuck->vm.nodeId).arg(epuck->variables.energy).arg((int)epuck->score);
				renderText(epuck->pos.x, epuck->pos.y, 10, QString("%0").arg(epuck->vm.nodeId), font);
				i++;
			}
		}
		
		renderText(16, 22, scoreString, font);
		
		renderText(16, 42, QString("E. in pool: %0 - total score: %1").arg(energyPool).arg(totalScore), font);
	}
	
	void PlaygroundViewer::connectionCreated(Dashel::Stream *stream)
	{
		std::string targetName = stream->getTargetName();
		if (targetName.substr(0, targetName.find_first_of(':')) == "tcp")
		{
			qDebug() << "New client connected.";
			if (this->stream)
			{
				closeStream(this->stream);
				qDebug() << "Disconnected old client.";
			}
			this->stream = stream;
		}
	}
	
	void PlaygroundViewer::incomingData(Dashel::Stream *stream)
	{
		uint16 len;
		uint16 incomingMessageSource;
		std::valarray<uint8> incomingMessageData;
		
		stream->read(&len, 2);
		stream->read(&incomingMessageSource, 2);
		incomingMessageData.resize(len+2);
		stream->read(&incomingMessageData[0], incomingMessageData.size());
		
		for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
		{
			AsebaFeedableEPuck *epuck = dynamic_cast<AsebaFeedableEPuck*>(*objectIt);
			if (epuck)
			{
				lastMessageSource = incomingMessageSource;
				lastMessageData.resize(incomingMessageData.size());
				memcpy(&lastMessageData[0], &incomingMessageData[0], incomingMessageData.size());
				AsebaProcessIncomingEvents(&(epuck->vm));
			}
		}
	}
	
	void PlaygroundViewer::connectionClosed(Dashel::Stream *stream, bool abnormal)
	{
		if (stream == this->stream)
		{
			this->stream = 0;
			// clear breakpoints
			for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
			{
				AsebaFeedableEPuck *epuck = dynamic_cast<AsebaFeedableEPuck*>(*objectIt);
				if (epuck)
					(epuck->vm).breakpointsCount = 0;
			}
		}
		if (abnormal)
			qDebug() << "Client has disconnected unexpectedly.";
		else
			qDebug() << "Client has disconnected properly.";
	}
	
	void PlaygroundViewer::timerEvent(QTimerEvent * event)
	{
		// do a network step
		Hub::step();
		
		// do a simulator/gui step
		ViewerWidget::timerEvent(event);
	}
}

// Implementation of native function

extern "C" void PlaygroundNative_energysend(AsebaVMState *vm)
{
	int index = AsebaNativePopArg(vm);
	// find related VM
	for (Enki::World::ObjectsIterator objectIt = playgroundViewer->getWorld()->objects.begin(); objectIt != playgroundViewer->getWorld()->objects.end(); ++objectIt)
	{
		Enki::AsebaFeedableEPuck *epuck = dynamic_cast<Enki::AsebaFeedableEPuck*>(*objectIt);
		if (epuck && (&(epuck->vm) == vm) && (epuck->energy > EPUCK_INITIAL_ENERGY))
		{
			uint16 amount = vm->variables[index];
			
			unsigned toSend = std::min((unsigned)amount, (unsigned)epuck->energy);
			playgroundViewer->energyPool += toSend;
			epuck->energy -= toSend;
		}
	}
}

extern "C" void PlaygroundNative_energyreceive(AsebaVMState *vm)
{
	int index = AsebaNativePopArg(vm);
	// find related VM
	for (Enki::World::ObjectsIterator objectIt = playgroundViewer->getWorld()->objects.begin(); objectIt != playgroundViewer->getWorld()->objects.end(); ++objectIt)
	{
		Enki::AsebaFeedableEPuck *epuck = dynamic_cast<Enki::AsebaFeedableEPuck*>(*objectIt);
		if (epuck && (&(epuck->vm) == vm))
		{
			uint16 amount = vm->variables[index];
			
			unsigned toReceive = std::min((unsigned)amount, (unsigned)playgroundViewer->energyPool);
			playgroundViewer->energyPool -= toReceive;
			epuck->energy += toReceive;
		}
	}
}

extern "C" void PlaygroundNative_energyamount(AsebaVMState *vm)
{
	int index = AsebaNativePopArg(vm);
	vm->variables[index] = playgroundViewer->energyPool;
}


// Implementation of aseba glue code

extern "C" void AsebaPutVmToSleep(AsebaVMState *vm) 
{
}

extern "C" void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	Dashel::Stream* stream = playgroundViewer->stream;
	if (stream)
	{
		try
		{
			// this may happen if target has disconnected
			uint16 len = length - 2;
			stream->write(&len, 2);
			stream->write(&vm->nodeId, 2);
			stream->write(data, length);
			stream->flush();
		}
		catch (Dashel::DashelException e)
		{
			qDebug() << "Cannot write to socket: " << e.what();
		}
	}
	
	// send to other robots too
	playgroundViewer->lastMessageSource = vm->nodeId;
	playgroundViewer->lastMessageData.resize(length);
	memcpy(&playgroundViewer->lastMessageData[0], data, length);
	for (Enki::World::ObjectsIterator objectIt = playgroundViewer->getWorld()->objects.begin(); objectIt != playgroundViewer->getWorld()->objects.end(); ++objectIt)
	{
		Enki::AsebaFeedableEPuck *epuck = dynamic_cast<Enki::AsebaFeedableEPuck*>(*objectIt);
		if (epuck && (&(epuck->vm) != vm))
			AsebaProcessIncomingEvents(&(epuck->vm));
	}
}

extern "C" uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	if (playgroundViewer->lastMessageData.size())
	{
		*source = playgroundViewer->lastMessageSource;
		memcpy(data, &playgroundViewer->lastMessageData[0], playgroundViewer->lastMessageData.size());
	}
	return playgroundViewer->lastMessageData.size();
}

// this is really ugly, but a necessary hack
static char ePuckName[9] = "e-puck  ";

extern "C" const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	ePuckName[7] = '0' + vm->nodeId;
	vmDescription.name = ePuckName;
	return &vmDescription;
}

static const AsebaLocalEventDescription localEvents[] = {
	{ "ir_sensors", "New IR sensors values available" },
	{"camera", "New camera picture available"},
	{ NULL, NULL }
};

extern "C" const AsebaLocalEventDescription * AsebaGetLocalEventsDescriptions(AsebaVMState *vm)
{
	return localEvents;
}

extern "C" const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	return nativeFunctionsDescriptions;
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	nativeFunctions[id](vm);
}

extern "C" void AsebaWriteBytecode(AsebaVMState *vm)
{
}

extern "C" void AsebaResetIntoBootloader(AsebaVMState *vm)
{
}

extern "C" void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	std::cerr << "\nFatal error: ";
	switch (vm->nodeId)
	{
		case 1: std::cerr << "left motor module"; break;
		case 2:	std::cerr << "right motor module"; break;
		case 3: std::cerr << "proximity sensors module"; break;
		case 4: std::cerr << "distance sensors module"; break;
		default: std::cerr << "unknown module"; break;
	}
	std::cerr << " has produced exception: ";
	switch (reason)
	{
		case ASEBA_ASSERT_UNKNOWN: std::cerr << "undefined"; break;
		case ASEBA_ASSERT_UNKNOWN_UNARY_OPERATOR: std::cerr << "unknown unary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode"; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow"; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow"; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds"; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds"; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run"; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds"; break;
		case ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG: std::cerr << "tried to emit a buffer too long"; break;
		default: std::cerr << "unknown exception"; break;
	}
	std::cerr << ".\npc = " << vm->pc << ", sp = " << vm->sp;
	std::cerr << "\nResetting VM" << std::endl;
	assert(false);
	AsebaVMInit(vm);
}


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	/*
	// Translation support
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name());
	app.installTranslator(&qtTranslator);
	
	QTranslator translator;
	translator.load(QString(":/asebachallenge_") + QLocale::system().name());
	app.installTranslator(&translator);
	*/
	
	// create document
	QDomDocument domDocument("aseba-playground");
	
	// Get cmd line arguments
	QString fileName;
	bool ask = true;
	if (argc > 1)
	{
		fileName = argv[1];
		ask = false;
	}
	
	// Try to load xml config file
	do
	{
		if (ask)
		{
			QString lastFileName = QSettings("EPFL-LSRO-Mobots", "Aseba Playground").value("last file").toString();
			fileName = QFileDialog::getOpenFileName(0, app.tr("Open Scenario"), lastFileName, app.tr("playground scenario (*.playground)"));
		}
		ask = true;
		
		if (fileName.isEmpty())
		{
			std::cerr << "You must specify a valid setup scenario on the command line or choose one in the file dialog\n";
			exit(1);
		}
		
		QFile file(fileName);
		if (file.open(QIODevice::ReadOnly))
		{
			QString errorStr;
			int errorLine, errorColumn;
			if (!domDocument.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
			{
				QMessageBox::information(0, "Aseba Playground",
										app.tr("Parse error at file %1, line %2, column %3:\n%4")
										.arg(fileName)
										.arg(errorLine)
										.arg(errorColumn)
										.arg(errorStr));
			}
			else
			{
				QSettings("EPFL-LSRO-Mobots", "Aseba Playground").setValue("last file", fileName);
				break;
			}
		}
	}
	while (true);
	
	// Scan for colors
	typedef QMap<QString, Enki::Color> ColorsMap;
	ColorsMap colorsMap;
	QDomElement colorE = domDocument.documentElement().firstChildElement("color");
	while (!colorE.isNull())
	{
		colorsMap[colorE.attribute("name")] = Enki::Color(
			colorE.attribute("r").toDouble(),
			colorE.attribute("g").toDouble(),
			colorE.attribute("b").toDouble()
		);
		
		colorE = colorE.nextSiblingElement ("color");
	}
	
	// Scan for areas
	typedef QMap<QString, Enki::Polygone> AreasMap;
	AreasMap areasMap;
	QDomElement areaE = domDocument.documentElement().firstChildElement("area");
	while (!areaE.isNull())
	{
		Enki::Polygone p;
		QDomElement pointE = areaE.firstChildElement("point");
		while (!pointE.isNull())
		{
			p.push_back(Enki::Point(
				pointE.attribute("x").toDouble(),
				pointE.attribute("y").toDouble()
			));
			pointE = pointE.nextSiblingElement ("point");
		}
		areasMap[areaE.attribute("name")] = p;
		areaE = areaE.nextSiblingElement ("area");
	}
	
	// Create the world
	QDomElement worldE = domDocument.documentElement().firstChildElement("world");
	Enki::Color worldColor(Enki::Color::gray);
	if (!colorsMap.contains(worldE.attribute("color")))
		std::cerr << "Warning, world walls color " << worldE.attribute("color").toStdString() << " undefined\n";
	else
		worldColor = colorsMap[worldE.attribute("color")];
	Enki::World world(
		worldE.attribute("w").toDouble(),
		worldE.attribute("h").toDouble(),
		worldColor
	);
	
	
	// Scan for walls
	QDomElement wallE = domDocument.documentElement().firstChildElement("wall");
	while (!wallE.isNull())
	{
		Enki::PhysicalObject* wall = new Enki::PhysicalObject();
		if (!colorsMap.contains(wallE.attribute("color")))
			std::cerr << "Warning, color " << wallE.attribute("color").toStdString() << " undefined\n";
		else
			wall->setColor(colorsMap[wallE.attribute("color")]);
		wall->pos.x = wallE.attribute("x").toDouble();
		wall->pos.y = wallE.attribute("y").toDouble();
		wall->setRectangular(
			wallE.attribute("l1").toDouble(),
			wallE.attribute("l2").toDouble(),
			wallE.attribute("h").toDouble(),
			-1
		);
		world.addObject(wall);
		
		wallE  = wallE.nextSiblingElement ("wall");
	}
	
	// Scan for feeders
	QDomElement feederE = domDocument.documentElement().firstChildElement("feeder");
	while (!feederE.isNull())
	{
		Enki::EPuckFeeder* feeder = new Enki::EPuckFeeder;
		feeder->pos.x = feederE.attribute("x").toDouble();
		feeder->pos.y = feederE.attribute("y").toDouble();
		world.addObject(feeder);
	
		feederE = feederE.nextSiblingElement ("feeder");
	}
	// TODO: if needed, custom color to feeder
	
	// Scan for doors
	typedef QMap<QString, Enki::SlidingDoor*> DoorsMap;
	DoorsMap doorsMap;
	QDomElement doorE = domDocument.documentElement().firstChildElement("door");
	while (!doorE.isNull())
	{
		Enki::SlidingDoor *door = new Enki::SlidingDoor(
			Enki::Point(
				doorE.attribute("closedX").toDouble(),
				doorE.attribute("closedY").toDouble()
			),
			Enki::Point(
				doorE.attribute("openedX").toDouble(),
				doorE.attribute("openedY").toDouble()
			),
			Enki::Point(
				doorE.attribute("l1").toDouble(),
				doorE.attribute("l2").toDouble()
			),
			doorE.attribute("h").toDouble(),
			doorE.attribute("moveDuration").toDouble()
		);
		if (!colorsMap.contains(doorE.attribute("color")))
			std::cerr << "Warning, door color " << doorE.attribute("color").toStdString() << " undefined\n";
		else
			door->setColor(colorsMap[doorE.attribute("color")]);
		doorsMap[doorE.attribute("name")] = door;
		world.addObject(door);
		
		doorE = doorE.nextSiblingElement ("door");
	}
	
	// Scan for activation, and link them with areas and doors
	QDomElement activationE = domDocument.documentElement().firstChildElement("activation");
	while (!activationE.isNull())
	{
		if (areasMap.find(activationE.attribute("area")) == areasMap.end())
		{
			std::cerr << "Warning, area " << activationE.attribute("area").toStdString() << " undefined\n";
			activationE = activationE.nextSiblingElement ("activation");
			continue;
		}
		
		if (doorsMap.find(activationE.attribute("door")) == doorsMap.end())
		{
			std::cerr << "Warning, door " << activationE.attribute("door").toStdString() << " undefined\n";
			activationE = activationE.nextSiblingElement ("activation");
			continue;
		}
		
		const Enki::Polygone& area = *areasMap.find(activationE.attribute("area"));
		Enki::Door* door = *doorsMap.find(activationE.attribute("door"));
		
		Enki::ActivationObject* activation = new Enki::ActivationObject(
			Enki::Point(
				activationE.attribute("x").toDouble(),
				activationE.attribute("y").toDouble()
			),
			Enki::Point(
				activationE.attribute("l1").toDouble(),
				activationE.attribute("l2").toDouble()
			),
			area,
			door
		);
		
		world.addObject(activation);
		
		activationE = activationE.nextSiblingElement ("activation");
	}
	
	// Scan for e-puck
	QDomElement ePuckE = domDocument.documentElement().firstChildElement("e-puck");
	int ePuckCount = 0;
	while (!ePuckE.isNull())
	{
		char buffer[9];
		strncpy(buffer, "e-puck  ", sizeof(buffer));
		Enki::AsebaFeedableEPuck* epuck = new Enki::AsebaFeedableEPuck(ePuckCount + 1);
		buffer[7] = '0' + ePuckCount++;
		epuck->pos.x = ePuckE.attribute("x").toDouble();
		epuck->pos.y = ePuckE.attribute("y").toDouble();
		if (ePuckE.hasAttribute("joystick"))
			epuck->joystick = ePuckE.attribute("joystick").toInt();
		epuck->name = buffer;
		world.addObject(epuck);
		ePuckE = ePuckE.nextSiblingElement ("e-puck");
	}
	
	// Create viewer
	Enki::PlaygroundViewer viewer(&world);
	
	// Show and run
	viewer.setWindowTitle("Playground - Stephane Magnenat (code) - Basilio Noris (gfx)");
	viewer.show();
	
	return app.exec();
}
