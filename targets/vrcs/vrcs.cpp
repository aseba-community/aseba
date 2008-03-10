/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASEBA_ASSERT
#define ASEBA_ASSERT
#endif

#include <../../vm/vm.h>
#include <../../common/consts.h>
#include <dashel/dashel.h>
#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>
#include <viewer/Viewer.h>
#include <iostream>
#include <QtGui>
#include <QtDebug>


//#define SIMPLIFIED_EPUCK

// Definition of the aseba glue

namespace Enki
{
	class AsebaFeedableEPuck;
}

// map for aseba glue code
typedef QMap<AsebaVMState*, Enki::AsebaFeedableEPuck*>  VmEPuckMap;
static VmEPuckMap asebaEPuckMap;


namespace Enki
{
	#define EPUCK_FEEDER_INITIAL_ENERGY		10
	#define EPUCK_FEEDER_THRESHOLD_HIDE		2
	#define EPUCK_FEEDER_THRESHOLD_SHOW		4
	#define EPUCK_FEEDER_RADIUS				6
	#define EPUCK_FEEDER_RANGE				12
	#define EPUCK_FEEDER_COLOR_ACTIVE		Color::blue
	#define EPUCK_FEEDER_COLOR_INACTIVE		Color::red
	#define EPUCK_FEEDER_D_ENERGY			4
	#define EPUCK_FEEDER_RECHARGE_RATE		0.5
	#define EPUCK_FEEDER_MAX_ENERGY			100
	
	#define EPUCK_INITIAL_ENERGY			10
	#define EPUCK_ENERGY_CONSUMPTION_RATE	1
	
	#define DEATH_ANIMATION_STEPS			30
	#define PORT_BASE						ASEBA_DEFAULT_PORT
	
	class FeedableEPuck: public EPuck
	{
	public:
		double energy;
		double score;
		QString name;
		int diedAnimation;
	
	public:
		FeedableEPuck() : EPuck(CAPABILITY_BASIC_SENSORS | CAPABILITY_CAMERA), energy(EPUCK_INITIAL_ENERGY), score(0), diedAnimation(-1) { }
		
		void step(double dt)
		{
			EPuck::step(dt);
			
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
	
	class AsebaFeedableEPuck : public FeedableEPuck, public Dashel::Hub
	{
	public:
		Dashel::Stream* stream;
		AsebaVMState vm;
		std::valarray<unsigned short> bytecode;
		std::valarray<signed short> stack;
		struct Variables
		{
			sint16 speedL; // left motor speed
			sint16 speedR; // right motor speed
			sint16 colorR; // body red [0..100] %
			sint16 colorG; // body green [0..100] %
			sint16 colorB; // body blue [0..100] %
			sint16 prox[8];	// 
			#ifdef SIMPLIFIED_EPUCK
			sint16 camR[3]; // camera red (left, middle, right) [0..100] %
			sint16 camG[3]; // camera green (left, middle, right) [0..100] %
			sint16 camB[3]; // camera blue (left, middle, right) [0..100] %
			#else
			sint16 camR[60]; // camera red (left, middle, right) [0..100] %
			sint16 camG[60]; // camera green (left, middle, right) [0..100] %
			sint16 camB[60]; // camera blue (left, middle, right) [0..100] %
			#endif
			sint16 energy;
			sint16 user[1024];
		} variables;
		
		//std::deque<Event> eventsQueue;
		//unsigned amountOfTimerEventInQueue;
		
			
	public:
		AsebaFeedableEPuck(int id) :
			stream(0)
		{
			asebaEPuckMap[&vm] = this;
			
			bytecode.resize(512);
			vm.bytecode = &bytecode[0];
			vm.bytecodeSize = bytecode.size();
			
			stack.resize(64);
			vm.stack = &stack[0];
			vm.stackSize = stack.size();
			
			vm.variables = reinterpret_cast<sint16 *>(&variables);
			vm.variablesSize = sizeof(variables) / sizeof(sint16);
			
			Dashel::Hub::connect(QString("tcpin:port=%1").arg(PORT_BASE+id).toStdString());
			
			AsebaVMInit(&vm, 1);
			
			variables.colorG = 100;
		}
		
		virtual ~AsebaFeedableEPuck()
		{
			asebaEPuckMap.remove(&vm);
		}
		
	public:
		void connectionCreated(Dashel::Stream *stream)
		{
			std::string targetName = stream->getTargetName();
			if (targetName.substr(0, targetName.find_first_of(':')) == "tcp")
			{
				qDebug() << this << " : New client connected.";
				if (this->stream)
				{
					closeStream(this->stream);
					qDebug() << this << " : Disconnected old client.";
				}
				this->stream = stream;
			}
		}
		
		void incomingData(Dashel::Stream *stream)
		{
			unsigned short len;
			unsigned short type;
			unsigned short source;
			stream->read(&len, 2);
			stream->read(&source, 2);
			stream->read(&type, 2);
			std::valarray<unsigned char> buffer(static_cast<size_t>(len));
			stream->read(&buffer[0], buffer.size());
			
			signed short *dataPtr = reinterpret_cast<signed short *>(&buffer[0]);
			
			// we only handle debug message
			if (type >= 0xA000)
			{
				// not bootloader
				if (buffer.size() % 2 != 0)
				{
					qDebug() << this << " : ";
					std::cerr << std::hex << std::showbase;
					std::cerr << "AsebaMarxbot::incomingData() : fatal error: received event: " << type;
					std::cerr << std::dec << std::noshowbase;
					std::cerr << " of size " << buffer.size();
					std::cerr << ", which is not a multiple of two." << std::endl;
					assert(false);
				}
				AsebaVMDebugMessage(&vm, type, reinterpret_cast<uint16 *>(dataPtr), buffer.size() / 2);
			}
			else
				qDebug() << this << " : Non debug event dropped.";
		}
		
		void connectionClosed(Dashel::Stream *stream, bool abnormal)
		{
			if (stream == this->stream)
				this->stream = 0;
			qDebug() << this << " : Client has disconnected.";
		}
		
		double toDoubleClamp(sint16 val, double mul, double min, double max)
		{
			double v = static_cast<double>(val) * mul;
			if (v > max)
				v = max;
			else if (v < min)
				v = min;
			return v;
		}
		
		void step(double dt)
		{
			// set physical variables
			leftSpeed = toDoubleClamp(variables.speedL, 1, -13, 13);
			rightSpeed = toDoubleClamp(variables.speedR, 1, -13, 13);
			color.setR(toDoubleClamp(variables.colorR, 0.01, 0, 1));
			color.setG(toDoubleClamp(variables.colorG, 0.01, 0, 1));
			color.setB(toDoubleClamp(variables.colorB, 0.01, 0, 1));
			
			// do motion
			FeedableEPuck::step(dt);
			
			// get physical variables
			#ifdef SIMPLIFIED_EPUCK
			variables.prox[0] = static_cast<sint16>(infraredSensor0.getDist());
			variables.prox[1] = static_cast<sint16>(infraredSensor1.getDist());
			variables.prox[2] = static_cast<sint16>(infraredSensor2.getDist());
			variables.prox[3] = static_cast<sint16>(infraredSensor3.getDist());
			variables.prox[4] = static_cast<sint16>(infraredSensor4.getDist());
			variables.prox[5] = static_cast<sint16>(infraredSensor5.getDist());
			variables.prox[6] = static_cast<sint16>(infraredSensor6.getDist());
			variables.prox[7] = static_cast<sint16>(infraredSensor7.getDist());
			#else
			variables.prox[0] = static_cast<sint16>(infraredSensor0.finalValue);
			variables.prox[1] = static_cast<sint16>(infraredSensor1.finalValue);
			variables.prox[2] = static_cast<sint16>(infraredSensor2.finalValue);
			variables.prox[3] = static_cast<sint16>(infraredSensor3.finalValue);
			variables.prox[4] = static_cast<sint16>(infraredSensor4.finalValue);
			variables.prox[5] = static_cast<sint16>(infraredSensor5.finalValue);
			variables.prox[6] = static_cast<sint16>(infraredSensor6.finalValue);
			variables.prox[7] = static_cast<sint16>(infraredSensor7.finalValue);
			#endif
			
			#ifdef SIMPLIFIED_EPUCK
			for (size_t i = 0; i < 3; i++)
			{
				double sumR = 0;
				double sumG = 0;
				double sumB = 0;
				for (size_t j = 0; j < 20; j++)
				{
					size_t index = 59 - (i * 20 + j);
					sumR += camera.image[index].r();
					sumG += camera.image[index].g();
					sumB += camera.image[index].b();
				}
				variables.camR[i] = static_cast<sint16>(sumR * 100. / 20.);
				variables.camG[i] = static_cast<sint16>(sumG * 100. / 20.);
				variables.camB[i] = static_cast<sint16>(sumB * 100. / 20.);
			}
			#else
			for (size_t i = 0; i < 60; i++)
			{
				variables.camR[i] = static_cast<sint16>(camera.image[i].r() * 100.);
				variables.camG[i] = static_cast<sint16>(camera.image[i].g() * 100.);
				variables.camB[i] = static_cast<sint16>(camera.image[i].b() * 100.);
			}
			#endif
			
			variables.energy = static_cast<sint16>(energy);
			
			// do a network step
			Hub::step();
			
			// run VM with a periodic event if nothing else is currently running
			if (!AsebaVMIsExecutingThread(&vm))
				AsebaVMSetupEvent(&vm, ASEBA_EVENT_PERIODIC);
			AsebaVMRun(&vm, 65535);
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
		
		void objectStep (double dt, PhysicalObject *po, World *w)
		{
			FeedableEPuck *epuck = dynamic_cast<FeedableEPuck *>(po);
			if (epuck && energy > 0)
			{
				double dEnergy = dt * EPUCK_FEEDER_D_ENERGY;
				epuck->energy += dEnergy;
				energy -= dEnergy;
				if (energy < EPUCK_FEEDER_THRESHOLD_HIDE)
					owner->color = EPUCK_FEEDER_COLOR_INACTIVE;
			}
		}
		
		void finalize(double dt)
		{
			if ((energy < EPUCK_FEEDER_THRESHOLD_SHOW) && (energy+dt >= EPUCK_FEEDER_THRESHOLD_SHOW))
				owner->color = EPUCK_FEEDER_COLOR_ACTIVE;
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
			r = EPUCK_FEEDER_RADIUS;
			height = 5;
			mass = -1;
			addLocalInteraction(&feeding);
			//color = EPUCK_FEEDER_COLOR_ACTIVE;
		}
	};

	class VRCSViewer : public ViewerWidget
	{
	protected:
		GLuint mobotsLogo;
		
	public:
		VRCSViewer(World* world) : ViewerWidget(world) { }
		
	protected:
		void drawQuad2D(double x, double y, double w, double ar)
		{
			double thisAr = (double)width() / (double)height();
			double h = (w * thisAr) / ar;
			glBegin(GL_QUADS);
			glTexCoord2d(0, 1);
			glVertex2d(x, y);
			glTexCoord2d(1, 1);
			glVertex2d(x+w, y);
			glTexCoord2d(1, 0);
			glVertex2d(x+w, y+h);
			glTexCoord2d(0, 0);
			glVertex2d(x, y+h);
			glEnd();
		}
		
		void initializeGL()
		{
			ViewerWidget::initializeGL();
			mobotsLogo = bindTexture(QPixmap(QString("mobots.png")), GL_TEXTURE_2D);
		}
		
		void displayObjectHook(PhysicalObject *object)
		{
			FeedableEPuck *epuck = dynamic_cast<FeedableEPuck*>(object);
			if ((epuck) && (epuck->diedAnimation >= 0))
			{
				ViewerUserData *userData = dynamic_cast<ViewerUserData *>(epuck->userData);
				assert(userData);
				
				double dist = (double)(DEATH_ANIMATION_STEPS - epuck->diedAnimation);
				double coeff =  (double)(epuck->diedAnimation) / DEATH_ANIMATION_STEPS;
				glColor3d(0.2*coeff, 0.2*coeff, 0.2*coeff);
				glTranslated(0, 0, 2. * dist);
				userData->drawSpecial(object);
			}
		}
		
		void sceneCompletedHook()
		{
			qglColor(Qt::black);
			QFont font;
			font.setPixelSize(18);
			
			// create a map with names and scores
			QMultiMap<int, QPair<QString, int> > scores;
			for (World::ObjectsIterator it = world->objects.begin(); it != world->objects.end(); ++it)
			{
				FeedableEPuck *epuck = dynamic_cast<FeedableEPuck*>(*it);
				if (epuck)
				{
					scores.insert((int)epuck->score, qMakePair<QString, int>(epuck->name, (int)epuck->energy));
					renderText(epuck->pos.x, epuck->pos.y, 10, epuck->name, font);
				}
			}
			
			// display this map
			QMapIterator<int, QPair<QString, int> > it(scores);
			it.toBack();
			int pos = 0;
			while (it.hasPrevious())
			{
				it.previous();
				renderText(5, 22+pos, QString("%0\tpt. %1\tenergy. %2").arg(it.value().first).arg(it.key()).arg(it.value().second), font);
				pos += 22;
			}
			
			// display logos
			glDisable(GL_DEPTH_TEST);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, 1, 1, 0);
			
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			
			glEnable(GL_TEXTURE_2D);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			
			glBindTexture(GL_TEXTURE_2D, mobotsLogo);
			drawQuad2D(0.05, 0.05, 0.2, 1.48125);
			
			glDisable(GL_BLEND);
			glEnable(GL_CULL_FACE);
			glDisable(GL_TEXTURE_2D);
			
			glEnable(GL_DEPTH_TEST);
		}
	};
}


// Implementation of aseba glue code

extern "C" void AsebaSendMessage(AsebaVMState *vm, uint16 id, void *data, uint16 size)
{
	Dashel::Stream* stream = asebaEPuckMap[vm]->stream;
	assert(stream);
	
	// write message
	stream->write(&size, 2);
	stream->write(&vm->nodeId, 2);
	stream->write(&id, 2);
	stream->write(data, size);
	stream->flush();
}

extern "C" void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length)
{
	Dashel::Stream* stream = asebaEPuckMap[vm]->stream;
	assert(stream);
	
	// write message
	uint16 size = length * 2 + 2;
	uint16 id = ASEBA_MESSAGE_VARIABLES;
	stream->write(&size, 2);
	stream->write(&vm->nodeId, 2);
	stream->write(&id, 2);
	stream->write(&start, 2);
	stream->write(vm->variables + start, length * 2);
	stream->flush();
}

void AsebaWriteString(Dashel::Stream* stream, const char *s)
{
	size_t len = strlen(s);
	uint8 lenUint8 = static_cast<uint8>(strlen(s));
	stream->write(&lenUint8, 1);
	stream->write(s, len);
}

extern "C" void AsebaSendDescription(AsebaVMState *vm)
{
	Dashel::Stream* stream = asebaEPuckMap[vm]->stream;
	
	if (stream)
	{
		// write sizes (basic + nodeName + sizes + native functions)
		uint16 size;
		sint16 ssize;
		
		size = 7 + (8 + 87) + 2;
		stream->write(&size, 2);
		stream->write(&vm->nodeId, 2);
		uint16 id = ASEBA_MESSAGE_DESCRIPTION;
		stream->write(&id, 2);
		
		// write node name
		AsebaWriteString(stream, "e-puck");
		
		// write sizes
		stream->write(&vm->bytecodeSize, 2);
		stream->write(&vm->stackSize, 2);
		stream->write(&vm->variablesSize, 2);
		
		size = 10;
		stream->write(&size, 2);
		
		// 80
		
		// 12
		size = 1;
		stream->write(&size, 2);
		AsebaWriteString(stream, "leftSpeed");
		
		// 13
		size = 1;
		stream->write(&size, 2);
		AsebaWriteString(stream, "rightSpeed");
		
		// 9
		size = 1;
		stream->write(&size, 2);
		AsebaWriteString(stream, "colorR");
		
		// 9
		size = 1;
		stream->write(&size, 2);
		AsebaWriteString(stream, "colorG");
		
		// 9
		size = 1;
		stream->write(&size, 2);
		AsebaWriteString(stream, "colorB");
		
		// 5
		size = 8;
		stream->write(&size, 2);
		AsebaWriteString(stream, "ir");
		
		// 7
		#ifdef SIMPLIFIED_EPUCK
		size = 3;
		#else
		size = 60;
		#endif
		stream->write(&size, 2);
		AsebaWriteString(stream, "camR");
		
		// 7
		#ifdef SIMPLIFIED_EPUCK
		size = 3;
		#else
		size = 60;
		#endif
		stream->write(&size, 2);
		AsebaWriteString(stream, "camG");
		
		// 7
		#ifdef SIMPLIFIED_EPUCK
		size = 3;
		#else
		size = 60;
		#endif
		stream->write(&size, 2);
		AsebaWriteString(stream, "camB");
		
		// 9
		size = 1;
		stream->write(&size, 2);
		AsebaWriteString(stream, "energy");
		
		// write native functions
		size = 0;
		stream->write(&size, 2);
		
		stream->flush();
	}
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	// no native functions
	assert(false);
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
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode"; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow"; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow"; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds"; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds"; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run"; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds"; break;
		default: std::cerr << "unknown exception"; break;
	}
	std::cerr << ".\npc = " << vm->pc << ", sp = " << vm->sp;
	std::cerr << "\nResetting VM" << std::endl;
	assert(false);
	AsebaVMInit(vm, vm->nodeId);
}


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	// Create the world
	Enki::World world(140, 140);
	
	// Add feeders
	Enki::EPuckFeeder* feeders[4];
	
	feeders[0] = new Enki::EPuckFeeder;
	feeders[0]->pos.x = 40;
	feeders[0]->pos.y = 40;
	world.addObject(feeders[0]);
	
	feeders[1] = new Enki::EPuckFeeder;
	feeders[1]->pos.x = 100;
	feeders[1]->pos.y = 40;
	world.addObject(feeders[1]);
	
	feeders[2] = new Enki::EPuckFeeder;
	feeders[2]->pos.x = 40;
	feeders[2]->pos.y = 100;
	world.addObject(feeders[2]);
	
	feeders[3] = new Enki::EPuckFeeder;
	feeders[3]->pos.x = 100;
	feeders[3]->pos.y = 100;
	world.addObject(feeders[3]);
	
	// Add e-puck
	for (int i = 1; i < argc; i++)
	{
		Enki::AsebaFeedableEPuck* epuck = new Enki::AsebaFeedableEPuck(i-1);
		epuck->pos.x = Enki::random.getRange(120)+10;
		epuck->pos.y = Enki::random.getRange(120)+10;
		epuck->name = argv[i];
		world.addObject(epuck);
	}
	
	// Create viewer
	Enki::VRCSViewer viewer(&world);
	
	// Show and run
	viewer.setWindowTitle("VRCS - Stephane Magnenat (code) - Basilio Noris (gfx)");
	viewer.show();
	
	return app.exec();
}
