/*
	VRCS - Virtual Robot Challenge System
	Copyright (C) 1999 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Mobots group (http://mobots.epfl.ch)
		Laboratory of Robotics Systems (http://lsro.epfl.ch)
		EPFL, Lausanne (http://www.epfl.ch)
	
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
#include "../../vm/natives.h"
#include <../../common/consts.h>
#include <dashel/dashel.h>
#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>
#include <viewer/Viewer.h>
#include <iostream>
#include <QtGui>
#include <QtDebug>

static void initTexturesResources()
{
	Q_INIT_RESOURCE(challenge_textures);
}

//! Asserts a dynamic cast.	Similar to the one in boost/cast.hpp
template<typename Derived, typename Base>
inline Derived polymorphic_downcast(Base base)
{
	Derived derived = dynamic_cast<Derived>(base);
	assert(derived);
	return derived;
}


#define SIMPLIFIED_EPUCK

// Definition of the aseba glue


namespace Enki
{
	class AsebaFeedableEPuck;
}

// map for aseba glue code
typedef QMap<AsebaVMState*, Enki::AsebaFeedableEPuck*>  VmEPuckMap;
static VmEPuckMap asebaEPuckMap;

static const unsigned nativeFunctionsCount = 2;
static AsebaNativeFunctionPointer nativeFunctions[nativeFunctionsCount] =
{
	AsebaNative_vecdot,
	AsebaNative_vecstat
};
static AsebaNativeFunctionDescription* nativeFunctionsDescriptions[nativeFunctionsCount] =
{
	&AsebaNativeDescription_vecdot,
	&AsebaNativeDescription_vecstat
};


namespace Enki
{
	#define EPUCK_FEEDER_INITIAL_ENERGY		10
	#define EPUCK_FEEDER_THRESHOLD_HIDE		2
	#define EPUCK_FEEDER_THRESHOLD_SHOW		4
	#define EPUCK_FEEDER_RADIUS				5
	#define EPUCK_FEEDER_RADIUS_DEAD		6
	#define EPUCK_FEEDER_RANGE				10
	
	#define EPUCK_FEEDER_COLOR_ACTIVE		Color::blue
	#define EPUCK_FEEDER_COLOR_INACTIVE		Color::red
	#define EPUCK_FEEDER_COLOR_DEAD			Color::gray
	
	#define EPUCK_FEEDER_D_ENERGY			4
	#define EPUCK_FEEDER_RECHARGE_RATE		0.5
	#define EPUCK_FEEDER_MAX_ENERGY			100
	
	#define EPUCK_FEEDER_LIFE_SPAN			60
	#define EPUCK_FEEDER_DEATH_SPAN			10
	
	#define EPUCK_INITIAL_ENERGY			10
	#define EPUCK_ENERGY_CONSUMPTION_RATE	1
	
	#define DEATH_ANIMATION_STEPS			30
	#define PORT_BASE						ASEBA_DEFAULT_PORT
	
	extern GLint GenFeederBase();
	extern GLint GenFeederCharge0();
	extern GLint GenFeederCharge1();
	extern GLint GenFeederCharge2();
	extern GLint GenFeederCharge3();
	extern GLint GenFeederRing();
	
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
			{
				this->stream = 0;
				// clear breakpoints
				vm.breakpointsCount = 0;
			}
			if (abnormal)
				qDebug() << this << " : Client has disconnected unexpectedly.";
			else
				qDebug() << this << " : Client has disconnected properly.";
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
		double age;
		bool alive;

	public :
		EPuckFeeding(Robot *owner, double age) : energy(EPUCK_FEEDER_INITIAL_ENERGY), age(age)
		{
			r = EPUCK_FEEDER_RANGE;
			this->owner = owner;
			alive = true;
		}
		
		void objectStep (double dt, PhysicalObject *po, World *w)
		{
			if (alive)
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
		}
		
		void finalize(double dt)
		{
			age += dt;
			if (alive)
			{
				if ((energy < EPUCK_FEEDER_THRESHOLD_SHOW) && (energy+dt >= EPUCK_FEEDER_THRESHOLD_SHOW))
					owner->color = EPUCK_FEEDER_COLOR_ACTIVE;
				energy += EPUCK_FEEDER_RECHARGE_RATE * dt;
				if (energy > EPUCK_FEEDER_MAX_ENERGY)
					energy = EPUCK_FEEDER_MAX_ENERGY;
				
				if (age > EPUCK_FEEDER_LIFE_SPAN)
				{
					alive = false;
					age = 0;
					owner->color = EPUCK_FEEDER_COLOR_DEAD;
					owner->r = EPUCK_FEEDER_RADIUS_DEAD;
				}
			}
			else
			{
				if (age > EPUCK_FEEDER_DEATH_SPAN)
				{
					alive = true;
					age = 0;
					owner->color = EPUCK_FEEDER_COLOR_ACTIVE;
					owner->r = EPUCK_FEEDER_RADIUS;
				}
			}
		}
	};
	
	class EPuckFeeder : public Robot
	{
	public:
		EPuckFeeding feeding;
	
	public:
		EPuckFeeder(double age) : feeding(this, age)
		{
			r = EPUCK_FEEDER_RADIUS;
			height = 5;
			mass = -1;
			addLocalInteraction(&feeding);
			color = EPUCK_FEEDER_COLOR_ACTIVE;
		}
	};
	
	class FeederModel : public ViewerWidget::CustomRobotModel
	{
	public:
		FeederModel(ViewerWidget* viewer)
		{
			textures.resize(2);
			textures[0] = viewer->bindTexture(QPixmap(QString(":/textures/feeder.png")), GL_TEXTURE_2D);
			textures[1] = viewer->bindTexture(QPixmap(QString(":/textures/feederr.png")), GL_TEXTURE_2D, GL_LUMINANCE8);
			lists.resize(6);
			lists[0] = GenFeederBase();
			lists[1] = GenFeederCharge0();
			lists[2] = GenFeederCharge1();
			lists[3] = GenFeederCharge2();
			lists[4] = GenFeederCharge3();
			lists[5] = GenFeederRing();
		}
		
		void cleanup(ViewerWidget* viewer)
		{
			for (int i = 0; i < textures.size(); i++)
				viewer->deleteTexture(textures[i]);
			for (int i = 0; i < lists.size(); i++)
				glDeleteLists(lists[i], 1);
		}
		
		virtual void draw(PhysicalObject* object) const
		{
			EPuckFeeder* feeder = polymorphic_downcast<EPuckFeeder*>(object);
			double age = feeder->feeding.age;
			bool alive = feeder->feeding.alive;
			
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			
			glPushMatrix();
			double disp;
			if (age < M_PI/2)
			{
				//glTranslated(0, 0, -0.2);
				if (alive)
					disp = -1 + sin(age);
				else
					disp = -sin(age);
			}
			else
			{
				if (alive)
					disp = 0;
				else
					disp = -1;
			}
			
			glTranslated(0, 0, 4.3*disp-0.2);
			
			// body
			glColor3d(1, 1, 1);
			glCallList(lists[0]);
			
			// ring
			glColor3d(0.6+object->color.components[0]-0.3*object->color.components[1]-0.3*object->color.components[2], 0.6+object->color.components[1]-0.3*object->color.components[0]-0.3*object->color.components[2], 0.6+object->color.components[2]-0.3*object->color.components[0]-0.3*object->color.components[1]);
			glCallList(lists[5]);
			
			// food
			glColor3d(0.3, 0.3, 1);
			int foodAmount = (int)((feeder->feeding.energy * 5) / (EPUCK_FEEDER_MAX_ENERGY + 0.001));
			assert(foodAmount <= 4);
			for (int i = 0; i < foodAmount; i++)
				glCallList(lists[1+i]);
			
			glPopMatrix();
			
			// shadow
			glColor3d(1, 1, 1);
			glBindTexture(GL_TEXTURE_2D, textures[1]);
			glDisable(GL_LIGHTING);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			
			// bottom shadow
			glPushMatrix();
			// disable writing of z-buffer
			glDepthMask( GL_FALSE );
			glTranslated(0, 0, 0.001);
			//glScaled(1-disp*0.1, 1-disp*0.1, 1);
			glBegin(GL_QUADS);
			glTexCoord2f(1.f, 0.f);
			glVertex2f(-7.5f, -7.5f);
			glTexCoord2f(1.f, 1.f);
			glVertex2f(7.5f, -7.5f);
			glTexCoord2f(0.f, 1.f);
			glVertex2f(7.5f, 7.5f);
			glTexCoord2f(0.f, 0.f);
			glVertex2f(-7.5f, 7.5f);
			glEnd();
			glDepthMask( GL_TRUE );
			glPopMatrix();
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_BLEND);
			glEnable(GL_LIGHTING);
			
			glDisable(GL_TEXTURE_2D);
			
			/*glCallList(lists[1]);
			
			//glColor3d(1-object->color.components[0], 1+object->color.components[1], 1+object->color.components[2]);
			glColor3d(0.6+object->color.components[0]-0.3*object->color.components[1]-0.3*object->color.components[2], 0.6+object->color.components[1]-0.3*object->color.components[0]-0.3*object->color.components[2], 0.6+object->color.components[2]-0.3*object->color.components[0]-0.3*object->color.components[1]);
			glCallList(lists[2]);
			
			glColor3d(1, 1, 1);
			
			// wheels
			glPushMatrix();
			glRotated((fmod(dw->leftOdometry, wheelCirc) * 360) / wheelCirc, 0, 1, 0);
			glCallList(lists[3]);
			glPopMatrix();
			
			glPushMatrix();
			glRotated((fmod(dw->rightOdometry, wheelCirc) * 360) / wheelCirc, 0, 1, 0);
			glCallList(lists[4]);
			glPopMatrix();
			
			// shadow
			glBindTexture(GL_TEXTURE_2D, textures[1]);
			glDisable(GL_LIGHTING);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			
			// bottom shadow
			glPushMatrix();
			// disable writing of z-buffer
			glDepthMask( GL_FALSE );
			glTranslated(0, 0, -wheelRadius+0.01);
			glBegin(GL_QUADS);
			glTexCoord2f(0.49f, 0.01f);
			glVertex2f(-5.f, -5.f);
			glTexCoord2f(0.49f, 0.49f);
			glVertex2f(5.f, -5.f);
			glTexCoord2f(0.01f, 0.49f);
			glVertex2f(5.f, 5.f);
			glTexCoord2f(0.01f, 0.01f);
			glVertex2f(-5.f, 5.f);
			glEnd();
			glDepthMask( GL_TRUE );
			glPopMatrix();
			
			// wheel shadow
			glPushMatrix();
			glScaled(radiosityScale, radiosityScale, radiosityScale);
			glTranslated(0, -0.025, 0);
			glCallList(lists[3]);
			glPopMatrix();
			
			glPushMatrix();
			glScaled(radiosityScale, radiosityScale, radiosityScale);
			glTranslated(0, 0.025, 0);
			glCallList(lists[4]);
			glPopMatrix();
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_BLEND);
			glEnable(GL_LIGHTING);
			
			glDisable(GL_TEXTURE_2D);*/
		}
		
		virtual void drawSpecial(PhysicalObject* object, int param) const
		{
			/*glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glDisable(GL_TEXTURE_2D);
			glCallList(lists[0]);
			glDisable(GL_BLEND);*/
		}
	};

	class VRCSViewer : public ViewerWidget
	{
	protected:
		GLuint mobotsLogo;
		
	public:
		VRCSViewer(World* world) : ViewerWidget(world)
		{
			initTexturesResources();
		}
		
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
		
		void renderObjectsTypesHook()
		{
			// render vrcs specific static types
			managedObjects[&typeid(EPuckFeeder)] = new FeederModel(this);
			managedObjectsAliases[&typeid(AsebaFeedableEPuck)] = &typeid(EPuck);
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
			/*FeedableEPuck *epuck = dynamic_cast<FeedableEPuck*>(object);
			{
			}*/
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
			
			/*// display logos
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
			
			glEnable(GL_DEPTH_TEST);*/
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

void AsebaWriteNativeFunctionDescription(Dashel::Stream* stream, const AsebaNativeFunctionDescription* description)
{
	AsebaWriteString(stream, description->name);
	AsebaWriteString(stream, description->doc);
	stream->write(description->argumentCount);
	for (unsigned i = 0; i < description->argumentCount; i++)
	{
		stream->write(description->arguments[i].size);
		AsebaWriteString(stream, description->arguments[i].name);
	}
}

extern "C" void AsebaSendDescription(AsebaVMState *vm)
{
	Dashel::Stream* stream = asebaEPuckMap[vm]->stream;
	
	if (stream)
	{
		// compute the size of all native functions inside description
		unsigned nativeFunctionSizes = 0;
		for (unsigned i = 0; i < nativeFunctionsCount; i++)
			nativeFunctionSizes += AsebaNativeFunctionGetDescriptionSize(nativeFunctionsDescriptions[i]);
	
		// write sizes (basic + nodeName + sizes + native functions)
		uint16 size;
		sint16 ssize;
		
		size = 7 + (8 + 89) + 2 + nativeFunctionSizes;
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
		
		// 82
		
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
		
		// 7
		size = 8;
		stream->write(&size, 2);
		AsebaWriteString(stream, "prox");
		
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
		size = nativeFunctionsCount;
		stream->write(&size, 2);
		
		// write all native functions descriptions
		for (unsigned i = 0; i < nativeFunctionsCount; i++)
			AsebaWriteNativeFunctionDescription(stream, nativeFunctionsDescriptions[i]);
		
		stream->flush();
	}
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	nativeFunctions[id](vm);
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
	
	feeders[0] = new Enki::EPuckFeeder(0);
	feeders[0]->pos.x = 40;
	feeders[0]->pos.y = 40;
	world.addObject(feeders[0]);
	
	feeders[1] = new Enki::EPuckFeeder(15);
	feeders[1]->pos.x = 100;
	feeders[1]->pos.y = 40;
	world.addObject(feeders[1]);
	
	feeders[2] = new Enki::EPuckFeeder(45);
	feeders[2]->pos.x = 40;
	feeders[2]->pos.y = 100;
	world.addObject(feeders[2]);
	
	feeders[3] = new Enki::EPuckFeeder(30);
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
