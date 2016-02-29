/*
	Challenge - Virtual Robot Challenge System
	Copyright (C) 1999--2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
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

#include "../../vm/vm.h"
#include "../../vm/natives.h"
#include "../../common/productids.h"
#include "../../common/consts.h"
#include "../../transport/buffer/vm-buffer.h"
#include <dashel/dashel.h>
#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>
#include <iostream>
#include <QtGui>
#include <QtDebug>
#include "challenge.h"
#include <string.h>

#define SIMPLIFIED_EPUCK

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

// Definition of the aseba glue

namespace Enki
{
	class AsebaFeedableEPuck;
}

// map for aseba glue code
typedef QMap<AsebaVMState*, Enki::AsebaFeedableEPuck*>  VmEPuckMap;
static VmEPuckMap asebaEPuckMap;

static AsebaNativeFunctionPointer nativeFunctions[] =
{
	ASEBA_NATIVES_STD_FUNCTIONS,
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
{
	ASEBA_NATIVES_STD_DESCRIPTIONS,
	0
};

// changed by selection dialog
static QString localName;

extern "C" AsebaVMDescription vmDescription_en;
extern "C" AsebaVMDescription vmDescription_fr;

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
		
		virtual void controlStep(double dt)
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
	
	class AsebaFeedableEPuck : public FeedableEPuck, public Dashel::Hub
	{
	public:
		// public because accessed from a glue function
		Dashel::Stream* stream;
		// all streams that must be disconnected at next step
		std::vector<Dashel::Stream*> toDisconnect;
		AsebaVMState vm;
		std::valarray<unsigned short> bytecode;
		std::valarray<signed short> stack;
		struct Variables
		{
			sint16 productId; // product id
			sint16 speedL; // left motor speed
			sint16 speedR; // right motor speed
			sint16 colorR; // body red [0..100] %
			sint16 colorG; // body green [0..100] %
			sint16 colorB; // body blue [0..100] %
			#ifdef SIMPLIFIED_EPUCK
			sint16 dist_A[8];	// proximity sensors in dist [cm] as variables, normal e-puck order
			sint16 dist_B[8];	// proximity sensors in dist [cm] as array, normal e-puck order
			#else
			sint16 prox[8];		// proximity sensors, normal e-puck order
			#endif
			#ifdef SIMPLIFIED_EPUCK
			sint16 camR_A[3]; // camera red as variables (left, middle, right) [0..100] %
			sint16 camR_B[3]; // camera red as array (left, middle, right) [0..100] %
			sint16 camG_A[3]; // camera green as variables (left, middle, right) [0..100] %
			sint16 camG_B[3]; // camera green as array (left, middle, right) [0..100] %
			sint16 camB_A[3]; // camera blue as variables (left, middle, right) [0..100] %
			sint16 camB_B[3]; // camera blue as array (left, middle, right) [0..100] %
			#else
			sint16 camR[60]; // camera red (left, middle, right) [0..100] %
			sint16 camG[60]; // camera green (left, middle, right) [0..100] %
			sint16 camB[60]; // camera blue (left, middle, right) [0..100] %
			#endif
			sint16 energy;
			sint16 user[1024];
		} variables;
		int port;
		
		uint16 lastMessageSource;
		std::valarray<uint8> lastMessageData;
		
	public:
		AsebaFeedableEPuck(int id) :
			stream(0)
		{
			asebaEPuckMap[&vm] = this;
			
			vm.nodeId = 1;
			
			bytecode.resize(512);
			vm.bytecode = &bytecode[0];
			vm.bytecodeSize = bytecode.size();
			
			stack.resize(64);
			vm.stack = &stack[0];
			vm.stackSize = stack.size();
			
			vm.variables = reinterpret_cast<sint16 *>(&variables);
			vm.variablesSize = sizeof(variables) / sizeof(sint16);
			
			port = PORT_BASE+id;
			try
			{
				Dashel::Hub::connect(QString("tcpin:port=%1").arg(port).toStdString());
			}
			catch (Dashel::DashelException e)
			{
				QMessageBox::critical(0, QApplication::tr("Aseba Challenge"), QApplication::tr("Cannot create listening port %0: %1").arg(port).arg(e.what()));
				abort();
			}
			
			AsebaVMInit(&vm);
			
			variables.productId = ASEBA_PID_CHALLENGE;
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
				// schedule current stream for disconnection
				if (this->stream)
					toDisconnect.push_back(this->stream);
				
				// set new stream as current stream
				this->stream = stream;
				qDebug() << this << " : New client connected.";
			}
		}
		
		void incomingData(Dashel::Stream *stream)
		{
			uint16 temp;
			uint16 len;
			
			stream->read(&temp, 2);
			len = bswap16(temp);
			stream->read(&temp, 2);
			lastMessageSource = bswap16(temp);
			lastMessageData.resize(len+2);
			stream->read(&lastMessageData[0], lastMessageData.size());
			
			if (bswap16(*(uint16*)&lastMessageData[0]) >= 0xA000)
				AsebaProcessIncomingEvents(&vm);
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
		
		void controlStep(double dt)
		{
			// get physical variables
			#ifdef SIMPLIFIED_EPUCK
			variables.dist_A[0] = static_cast<sint16>(infraredSensor0.getDist());
			variables.dist_A[1] = static_cast<sint16>(infraredSensor1.getDist());
			variables.dist_A[2] = static_cast<sint16>(infraredSensor2.getDist());
			variables.dist_A[3] = static_cast<sint16>(infraredSensor3.getDist());
			variables.dist_A[4] = static_cast<sint16>(infraredSensor4.getDist());
			variables.dist_A[5] = static_cast<sint16>(infraredSensor5.getDist());
			variables.dist_A[6] = static_cast<sint16>(infraredSensor6.getDist());
			variables.dist_A[7] = static_cast<sint16>(infraredSensor7.getDist());
			for (size_t i = 0; i < 8; ++i)
				variables.dist_B[i] = variables.dist_A[i];
			#else
			variables.prox[0] = static_cast<sint16>(infraredSensor0.getValue());
			variables.prox[1] = static_cast<sint16>(infraredSensor1.getValue());
			variables.prox[2] = static_cast<sint16>(infraredSensor2.getValue());
			variables.prox[3] = static_cast<sint16>(infraredSensor3.getValue());
			variables.prox[4] = static_cast<sint16>(infraredSensor4.getValue());
			variables.prox[5] = static_cast<sint16>(infraredSensor5.getValue());
			variables.prox[6] = static_cast<sint16>(infraredSensor6.getValue());
			variables.prox[7] = static_cast<sint16>(infraredSensor7.getValue());
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
				variables.camR_A[i] = variables.camR_B[i] = static_cast<sint16>(sumR * 100. / 20.);
				variables.camG_A[i] = variables.camG_B[i] = static_cast<sint16>(sumG * 100. / 20.);
				variables.camB_A[i] = variables.camB_B[i] = static_cast<sint16>(sumB * 100. / 20.);
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
			
			// disconnect old streams
			for (size_t i = 0; i < toDisconnect.size(); ++i)
			{
				closeStream(toDisconnect[i]);
				qDebug() << this << " : Disconnected old client.";
			}
			toDisconnect.clear();
			
			// run VM
			AsebaVMRun(&vm, 65535);
			
			// reschedule a periodic event if we are not in step by step
			if (AsebaMaskIsClear(vm.flags, ASEBA_VM_STEP_BY_STEP_MASK) || AsebaMaskIsClear(vm.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
				AsebaVMSetupEvent(&vm, ASEBA_EVENT_LOCAL_EVENTS_START);
		
			// set physical variables
			leftSpeed = toDoubleClamp(variables.speedL, 1, -13, 13);
			rightSpeed = toDoubleClamp(variables.speedR, 1, -13, 13);
			Color c;
			c.setR(toDoubleClamp(variables.colorR, 0.01, 0, 1));
			c.setG(toDoubleClamp(variables.colorG, 0.01, 0, 1));
			c.setB(toDoubleClamp(variables.colorB, 0.01, 0, 1));
			setColor(c);
			
			// set motion
			FeedableEPuck::controlStep(dt);
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
		
		void objectStep(double dt, World *w, PhysicalObject *po)
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
						owner->setColor(EPUCK_FEEDER_COLOR_INACTIVE);
				}
			}
		}
		
		void finalize(double dt, World *w)
		{
			age += dt;
			if (alive)
			{
				if ((energy < EPUCK_FEEDER_THRESHOLD_SHOW) && (energy+dt >= EPUCK_FEEDER_THRESHOLD_SHOW))
					owner->setColor(EPUCK_FEEDER_COLOR_ACTIVE);
				energy += EPUCK_FEEDER_RECHARGE_RATE * dt;
				if (energy > EPUCK_FEEDER_MAX_ENERGY)
					energy = EPUCK_FEEDER_MAX_ENERGY;
				
				if (age > EPUCK_FEEDER_LIFE_SPAN)
				{
					alive = false;
					age = 0;
					owner->setColor(EPUCK_FEEDER_COLOR_DEAD);
					owner->setCylindric(EPUCK_FEEDER_RADIUS_DEAD, owner->getHeight(), owner->getMass());
				}
			}
			else
			{
				if (age > EPUCK_FEEDER_DEATH_SPAN)
				{
					alive = true;
					age = 0;
					owner->setColor(EPUCK_FEEDER_COLOR_ACTIVE);
					owner->setCylindric(EPUCK_FEEDER_RADIUS, owner->getHeight(), owner->getMass());
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
			setCylindric(EPUCK_FEEDER_RADIUS, 5, -1);
			addLocalInteraction(&feeding);
			setColor(EPUCK_FEEDER_COLOR_ACTIVE);
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
			glColor3d(0.6+object->getColor().components[0]-0.3*object->getColor().components[1]-0.3*object->getColor().components[2], 0.6+object->getColor().components[1]-0.3*object->getColor().components[0]-0.3*object->getColor().components[2], 0.6+object->getColor().components[2]-0.3*object->getColor().components[0]-0.3*object->getColor().components[1]);
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
			glEnable(GL_POLYGON_OFFSET_FILL);
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
			glDisable(GL_POLYGON_OFFSET_FILL);
			glDepthMask( GL_TRUE );
			glPopMatrix();
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_BLEND);
			glEnable(GL_LIGHTING);
			
			glDisable(GL_TEXTURE_2D);
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

	// Challenge Viewer
	
	ChallengeViewer::ChallengeViewer(World* world, int ePuckCount) : ViewerWidget(world), ePuckCount(ePuckCount)
	{
		savingVideo = false;
		autoCamera = false;
		initTexturesResources();
		
		int res = QFontDatabase::addApplicationFont(":/fonts/SF Old Republic SC.ttf");
		Q_ASSERT(res != -1);
		Q_UNUSED(res);
		//qDebug() << QFontDatabase::applicationFontFamilies(res);
		titleFont = QFont("SF Old Republic SC", 20);
		entryFont = QFont("SF Old Republic SC", 23);
		labelFont = QFont("SF Old Republic SC", 16);
		
		setMouseTracking(true);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setAttribute(Qt::WA_NoSystemBackground);
		setAutoFillBackground(false);
		//resize(780, 560);
	}
	
	void ChallengeViewer::addNewRobot()
	{
		bool ok;
		QString eventName = QInputDialog::getText(this, tr("Add a new robot"), tr("Robot name:"), QLineEdit::Normal, "", &ok);
		if (ok && !eventName.isEmpty())
		{
			// TODO change ePuckCount to port
			Enki::AsebaFeedableEPuck* epuck = new Enki::AsebaFeedableEPuck(ePuckCount++);
			epuck->pos.x = Enki::random.getRange(120)+10;
			epuck->pos.y = Enki::random.getRange(120)+10;
			epuck->name = eventName;
			world->addObject(epuck);
		}
	}
	
	void ChallengeViewer::removeRobot()
	{
		std::set<AsebaFeedableEPuck *> toFree;
		// TODO: for now, remove all robots; later, show a gui box to choose which robot to remove
		for (World::ObjectsIterator it = world->objects.begin(); it != world->objects.end(); ++it)
		{
			AsebaFeedableEPuck *epuck = dynamic_cast<AsebaFeedableEPuck*>(*it);
			if (epuck)
				toFree.insert(epuck);
		}
		
		for (std::set<AsebaFeedableEPuck *>::iterator it = toFree.begin(); it != toFree.end(); ++it)
		{
			world->removeObject(*it);
			delete *it;
		}
		ePuckCount = 0;
	}

	void ChallengeViewer::autoCameraStateChanged(bool state)
	{
		autoCamera = state;
	}
	
	void ChallengeViewer::timerEvent(QTimerEvent * event)
	{
		if (autoCamera)
		{
			altitude = 70;
			yaw += 0.002;
			pos = QPointF(-world->w/2 + 120*sin(yaw+M_PI/2), -world->h/2 + 120*cos(yaw+M_PI/2));
			if (yaw > 2*M_PI)
				yaw -= 2*M_PI;
			pitch = M_PI/7	;
		}
		ViewerWidget::timerEvent(event);
	}
/*
	void ChallengeViewer::mouseMoveEvent ( QMouseEvent * event )
	{
		#ifndef Q_WS_MAC

		bool isInButtonArea = event->y() < addRobotButton->y() + addRobotButton->height() + 10;
		if (hideButtons->isChecked())
		{
			if (isInButtonArea && !addRobotButton->isVisible())
			{
				menuFrame->show();
			}
			if (!isInButtonArea && addRobotButton->isVisible())
			{
				menuFrame->hide();
			}
		}

		#endif // Q_WS_MAC
		
		ViewerWidget::mouseMoveEvent(event);
	}
*/
	void ChallengeViewer::keyPressEvent ( QKeyEvent * event )
	{
		if (event->key() == Qt::Key_V)
			savingVideo = true;
		else
			ViewerWidget::keyPressEvent(event);
	}
	
	void ChallengeViewer::keyReleaseEvent ( QKeyEvent * event )
	{
		if (event->key() == Qt::Key_V)
			savingVideo = false;
		else
			ViewerWidget::keyReleaseEvent (event);
	}
	
	void ChallengeViewer::drawQuad2D(double x, double y, double w, double ar)
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
	
	void ChallengeViewer::initializeGL()
	{
		ViewerWidget::initializeGL();
	}
	
	void ChallengeViewer::renderObjectsTypesHook()
	{
		// render vrcs specific static types
		managedObjects[&typeid(EPuckFeeder)] = new FeederModel(this);
		managedObjectsAliases[&typeid(AsebaFeedableEPuck)] = &typeid(EPuck);
	}
	
	void ChallengeViewer::displayObjectHook(PhysicalObject *object)
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
	
	void ChallengeViewer::sceneCompletedHook()
	{
		// create a map with names and scores
		qglColor(Qt::black);
		QMultiMap<int, QStringList> scores;
		for (World::ObjectsIterator it = world->objects.begin(); it != world->objects.end(); ++it)
		{
			AsebaFeedableEPuck *epuck = dynamic_cast<AsebaFeedableEPuck*>(*it);
			if (epuck)
			{
				QStringList entry;
				entry << epuck->name << QString::number(epuck->port) << QString::number((int)epuck->energy) << QString::number((int)epuck->score);
				scores.insert((int)epuck->score, entry);
				renderText(epuck->pos.x, epuck->pos.y, 10, epuck->name, labelFont);
			}
		}
		
		// build score texture
		QImage scoreBoard(512, 256, QImage::Format_ARGB32);
		scoreBoard.setDotsPerMeterX(2350);
		scoreBoard.setDotsPerMeterY(2350);
		QPainter painter(&scoreBoard);
		//painter.fillRect(scoreBoard.rect(), QColor(224,224,255,196));
		painter.fillRect(scoreBoard.rect(), QColor(224,255,224,196));
		
		// draw lines
		painter.setBrush(Qt::NoBrush);
		QPen pen(Qt::black);
		pen.setWidth(2);
		painter.setPen(pen);
		painter.drawRect(scoreBoard.rect());
		pen.setWidth(1);
		painter.setPen(pen);
		painter.drawLine(22, 34, 504, 34);
		painter.drawLine(312, 12, 312, 247);
		painter.drawLine(312, 240, 504, 240);
		
		// draw title
		painter.setFont(titleFont);
		painter.drawText(35, 28, "name");
		painter.drawText(200, 28, "port");
		painter.drawText(324, 28, "energy");
		painter.drawText(430, 28, "points");
		
		// display entries
		QMapIterator<int, QStringList> it(scores);
		
		it.toBack();
		int pos = 61;
		while (it.hasPrevious())
		{
			it.previous();
			painter.drawText(200, pos, it.value().at(1));
			pos += 24;
		}
		
		it.toBack();
		painter.setFont(entryFont);
		pos = 61;
		while (it.hasPrevious())
		{
			it.previous();
			painter.drawText(35, pos, it.value().at(0));
			painter.drawText(335, pos, it.value().at(2));
			painter.drawText(445, pos, it.value().at(3));
			pos += 24;
		}
		
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		GLuint tex = bindTexture(scoreBoard, GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glCullFace(GL_FRONT);
		glColor4d(1, 1, 1, 0.75);
		for (int i = 0; i < 4; i++)
		{
			glPushMatrix();
			glTranslated(world->w/2, world->h/2, 50);
			glRotated(90*i, 0, 0, 1);
			glBegin(GL_QUADS);
			glTexCoord2d(0, 0);
			glVertex3d(-20, -20, 0);
			glTexCoord2d(1, 0);
			glVertex3d(20, -20, 0);
			glTexCoord2d(1, 1);
			glVertex3d(20, -20, 20);
			glTexCoord2d(0, 1);
			glVertex3d(-20, -20, 20);
			glEnd();
			glPopMatrix();
		}
		
		glCullFace(GL_BACK);
		glColor3d(1, 1, 1);
		for (int i = 0; i < 4; i++)
		{
			glPushMatrix();
			glTranslated(world->w/2, world->h/2, 50);
			glRotated(90*i, 0, 0, 1);
			glBegin(GL_QUADS);
			glTexCoord2d(0, 0);
			glVertex3d(-20, -20, 0);
			glTexCoord2d(1, 0);
			glVertex3d(20, -20, 0);
			glTexCoord2d(1, 1);
			glVertex3d(20, -20, 20);
			glTexCoord2d(0, 1);
			glVertex3d(-20, -20, 20);
			glEnd();
			glPopMatrix();
		}
		
		deleteTexture(tex);
		
		glDisable(GL_TEXTURE_2D);
		glColor4d(7./8.,7./8.,1,0.75);
		glPushMatrix();
		glTranslated(world->w/2, world->h/2, 50);
		glBegin(GL_QUADS);
		glVertex3d(-20,-20,20);
		glVertex3d(20,-20,20);
		glVertex3d(20,20,20);
		glVertex3d(-20,20,20);
		
		glVertex3d(-20,20,0);
		glVertex3d(20,20,0);
		glVertex3d(20,-20,0);
		glVertex3d(-20,-20,0);
		glEnd();
		glPopMatrix();
		
		// save image
		static int imageCounter = 0;
		if (savingVideo)
			grabFrameBuffer().save(QString("frame%0.bmp").arg(imageCounter++), "BMP");
	}


	ChallengeApplication::ChallengeApplication(World* world, int ePuckCount) :
		viewer(world, ePuckCount)
	{
		// help viewer
		helpViewer = new QTextBrowser();
		helpViewer->setReadOnly(true);
		helpViewer->resize(600, 500);
		// help files generated by txt2tags, xhtml mode, with TOC
		if (localName.left(2) == "fr")
			helpViewer->setSource(QString("qrc:/doc/challenge.fr.html"));
		else if (localName.left(2) == "es")
			helpViewer->setSource(QString("qrc:/doc/challenge.es.html"));
		else if (localName.left(2) == "it")
			helpViewer->setSource(QString("qrc:/doc/challenge.it.html"));
		else
			helpViewer->setSource(QString("qrc:/doc/challenge.en.html"));
		helpViewer->moveCursor(QTextCursor::Start);
		helpViewer->setWindowTitle(tr("Aseba Challenge Help"));

		connect(this, SIGNAL(windowClosed()), helpViewer, SLOT(close()));

		// main windows layout
		QVBoxLayout *vLayout = new QVBoxLayout;
		QHBoxLayout *hLayout = new QHBoxLayout;

		hLayout->addStretch();

		// construction of menu frame
		QFrame* menuFrame = new QFrame();
		//menuFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
		menuFrame->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
		QHBoxLayout *frameLayout = new QHBoxLayout;
		QPushButton* addRobotButton = new QPushButton(tr("Add a new robot"));
		frameLayout->addWidget(addRobotButton);
		QPushButton* delRobotButton = new QPushButton(tr("Remove all robots"));
		frameLayout->addWidget(delRobotButton);
		QCheckBox* autoCamera = new QCheckBox(tr("Auto camera"));
		frameLayout->addWidget(autoCamera);
		QCheckBox* fullScreen = new QCheckBox(tr("Full screen"));
		frameLayout->addWidget(fullScreen);
		//hideButtons = new QCheckBox(tr("Auto hide"));
		//frameLayout->addWidget(hideButtons);
		QPushButton* helpButton = new QPushButton(tr("Help"));
		frameLayout->addWidget(helpButton);
		QPushButton* quitButton = new QPushButton(tr("Quit"));
		frameLayout->addWidget(quitButton);
		menuFrame->setLayout(frameLayout);

//		menuFrame->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		viewer.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
		resize(780, 560);

		// construction of the screen layout
		hLayout->addWidget(menuFrame);
		hLayout->addStretch();

		vLayout->addLayout(hLayout);
		vLayout->addWidget(&viewer);
		vLayout->setContentsMargins(0,4,0,0);
		setLayout(vLayout);

		connect(addRobotButton, SIGNAL(clicked()), &viewer, SLOT(addNewRobot()));
		connect(delRobotButton, SIGNAL(clicked()), &viewer, SLOT(removeRobot()));
		connect(autoCamera, SIGNAL(toggled(bool)), &viewer, SLOT(autoCameraStateChanged(bool)));
		connect(fullScreen, SIGNAL(toggled(bool)), SLOT(fullScreenStateChanged(bool)));
		connect(helpButton, SIGNAL(clicked()), helpViewer, SLOT(show()));
		connect(quitButton, SIGNAL(clicked()), SLOT(close()));
		
		autoCamera->setCheckState(Qt::Checked);

		/*
		#else // Q_WS_MAC

		QMenuBar *menuBar = new QMenuBar(0);
		QMenu *menu = menuBar->addMenu(tr("Simulator control"));
		menu->addAction(tr("Add a new robot"), this, SLOT(addNewRobot()));
		menu->addAction(tr("Remove all robots"), this, SLOT(removeRobot()));
		menu->addSeparator();
		autoCamera = new QAction(tr("Auto camera"), 0);
		autoCamera->setCheckable(true);
		menu->addAction(autoCamera);
		menu->addSeparator();
		menu->addAction(tr("Help"), helpViewer, SLOT(show()));
		
		#endif // Q_WS_MAC
		*/
	}
	
	void ChallengeApplication::fullScreenStateChanged(bool fullScreen)
	{
		if (fullScreen)
			showFullScreen();
		else
			showNormal();
	}

	void ChallengeApplication::closeEvent ( QCloseEvent * event )
	{
		if (event->isAccepted())
			emit windowClosed();
	}
}

// Implementation of aseba glue code

extern "C" void AsebaPutVmToSleep(AsebaVMState *vm) 
{
}

extern "C" void AsebaSendBuffer(AsebaVMState *vm, const uint8* data, uint16 length)
{
	Dashel::Stream* stream = asebaEPuckMap[vm]->stream;
	assert(stream);

	try
	{
		uint16 temp;
		temp = bswap16(length - 2);
		stream->write(&temp, 2);
		temp = bswap16(vm->nodeId);
		stream->write(&temp, 2);
		stream->write(data, length);
		stream->flush();
	}
	catch (Dashel::DashelException e)
	{
		std::cerr << "Cannot write to socket: " << stream->getFailReason() << std::endl;
	}
}

extern "C" uint16 AsebaGetBuffer(AsebaVMState *vm, uint8* data, uint16 maxLength, uint16* source)
{
	if (asebaEPuckMap[vm]->lastMessageData.size())
	{
		*source = asebaEPuckMap[vm]->lastMessageSource;
		memcpy(data, &asebaEPuckMap[vm]->lastMessageData[0], asebaEPuckMap[vm]->lastMessageData.size());
	}
	return asebaEPuckMap[vm]->lastMessageData.size();
}

extern "C" const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	if (localName == "fr")
		return &vmDescription_fr;
	else
		return &vmDescription_en;
}

static const AsebaLocalEventDescription localEvents[] = { { "timer", "periodic timer at 50 Hz" }, { NULL, NULL }};

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


LanguageSelectionDialog::LanguageSelectionDialog()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	
	QLabel* text = new QLabel(tr("Please choose your language"));
	layout->addWidget(text);
	
	languageSelectionBox = new QComboBox(this);
	languageSelectionBox->addItem(QString::fromUtf8("English"), "en");
	languageSelectionBox->addItem(QString::fromUtf8("Français"), "fr");
	languageSelectionBox->addItem(QString::fromUtf8("German"), "de");
	languageSelectionBox->addItem(QString::fromUtf8("Español"), "es");
	languageSelectionBox->addItem(QString::fromUtf8("Italiano"), "it");
	languageSelectionBox->addItem(QString::fromUtf8("日本語"), "ja");
	/* insert translation here (DO NOT REMOVE -> for automated script) */
	//qDebug() << "locale is " << QLocale::system().name();
	for (int i = 0; i < languageSelectionBox->count(); ++i)
	{
		if (QLocale::system().name().startsWith(languageSelectionBox->itemData(i).toString()))
		{
			languageSelectionBox->setCurrentIndex(i);
			break;
		}
	}
	layout->addWidget(languageSelectionBox);
	
	QPushButton* okButton = new QPushButton(QIcon(":/images/ok.png"), tr("Ok"));
	connect(okButton, SIGNAL(clicked(bool)), SLOT(accept()));
	layout->addWidget(okButton);
	
	setWindowTitle(tr("Language selection"));
}


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	// Translation support
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);
	
	//qDebug() << QLocale::system().name();
	QTranslator translator;
	translator.load(QString(":/asebachallenge_") + QLocale::system().name());
	app.installTranslator(&translator);
	
	// choose the language
	{
		LanguageSelectionDialog languageSelectionDialog;
		languageSelectionDialog.show();
		languageSelectionDialog.exec();
		
		localName = languageSelectionDialog.languageSelectionBox->itemData(languageSelectionDialog.languageSelectionBox->currentIndex()).toString();
		qtTranslator.load(QString("qt_") + localName, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
		translator.load(QString(":/asebachallenge_") + localName);
	}
	
	// Create the world
	Enki::World world(140, 140, Enki::Color(0.4, 0.4, 0.4));
	
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
	int ePuckCount = 0;
	for (int i = 1; i < argc; i++)
	{
		Enki::AsebaFeedableEPuck* epuck = new Enki::AsebaFeedableEPuck(i-1);
		epuck->pos.x = Enki::random.getRange(120)+10;
		epuck->pos.y = Enki::random.getRange(120)+10;
		epuck->name = argv[i];
		world.addObject(epuck);
		ePuckCount++;
	}
	
	// Create viewer
	Enki::ChallengeApplication viewer(&world, ePuckCount);
	
	// Show and run
	viewer.setWindowTitle("ASEBA Challenge - Stephane Magnenat (code) - Basilio Noris (gfx)");
	viewer.setWindowIcon(QIcon(":/textures/asebachallenge.svgz"));
	viewer.show();
	
	return app.exec();
}
