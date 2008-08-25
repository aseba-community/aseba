/*
	Playground - An active arena to learn multi-robots programming
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
#include <../../transport/buffer/vm-buffer.h>
#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>
#include <iostream>
#include <QtGui>
#include <QtDebug>
#include "playground.h"
//#include <playground.moc>
#include <string.h>

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

static AsebaNativeFunctionPointer nativeFunctions[] =
{
	AsebaNative_vecfill,
	AsebaNative_veccopy,
	AsebaNative_vecadd,
	AsebaNative_vecsub,
	AsebaNative_vecmin,
	AsebaNative_vecmax,
	AsebaNative_vecdot,
	AsebaNative_vecstat,
	AsebaNative_mathmuldiv,
	AsebaNative_mathatan2,
	AsebaNative_mathsin,
	AsebaNative_mathcos,
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
{
	&AsebaNativeDescription_vecfill,
	&AsebaNativeDescription_veccopy,
	&AsebaNativeDescription_vecadd,
	&AsebaNativeDescription_vecsub,
	&AsebaNativeDescription_vecmin,
	&AsebaNativeDescription_vecmax,
	&AsebaNativeDescription_vecdot,
	&AsebaNativeDescription_vecstat,
	&AsebaNativeDescription_mathmuldiv,
	&AsebaNativeDescription_mathatan2,
	&AsebaNativeDescription_mathsin,
	&AsebaNativeDescription_mathcos,
	0
};

extern AsebaVMDescription vmDescription;

// this uglyness is necessary to glue with C code
Enki::PlaygroundViewer *playgroundViewer = 0;

namespace Enki
{
	#define EPUCK_FEEDER_INITIAL_ENERGY		10
	#define EPUCK_FEEDER_THRESHOLD_HIDE		2
	#define EPUCK_FEEDER_THRESHOLD_SHOW		4
	#define EPUCK_FEEDER_RADIUS				5
	#define EPUCK_FEEDER_RADIUS_DEAD		6
	#define EPUCK_FEEDER_RANGE				10
	
	#define EPUCK_FEEDER_COLOR_ACTIVE		Color::green
	#define EPUCK_FEEDER_COLOR_INACTIVE		Color::gray
	
	#define EPUCK_FEEDER_D_ENERGY			4
	#define EPUCK_FEEDER_RECHARGE_RATE		0.5
	#define EPUCK_FEEDER_MAX_ENERGY			100
	
	#define EPUCK_FEEDER_LIFE_SPAN			60
	#define EPUCK_FEEDER_DEATH_SPAN			10
	
	#define EPUCK_INITIAL_ENERGY			10
	#define EPUCK_ENERGY_CONSUMPTION_RATE	1
	
	#define DEATH_ANIMATION_STEPS			30
	
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
	
	class AsebaFeedableEPuck : public FeedableEPuck
	{
	public:
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
		AsebaFeedableEPuck(int id)
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
		
		void objectStep (double dt, PhysicalObject *po, World *w)
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
		
		void finalize(double dt)
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
			height = 5;
			mass = -1;
			addLocalInteraction(&feeding);
			color = EPUCK_FEEDER_COLOR_ACTIVE;
			setupBoundingSurface(Polygone() << Point(-1.6, -1.6) <<  Point(1.6, -1.6) << Point(1.6, 1.6) << Point(-1.6, 1.6));
			
			commitPhysicalParameters();
		}
	};

	PlaygroundViewer::PlaygroundViewer(World* world) : ViewerWidget(world), stream(0)
	{
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
	}
	
	void PlaygroundViewer::renderObjectsTypesHook()
	{
		managedObjectsAliases[&typeid(AsebaFeedableEPuck)] = &typeid(EPuck);
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
	/*
	// Playground Viewer
	
	PlaygroundViewer::PlaygroundViewer(World* world, int ePuckCount) : ViewerWidget(world), ePuckCount(ePuckCount)
	{
		savingVideo = false;
		initTexturesResources();
		
		int res = QFontDatabase::addApplicationFont(":/fonts/SF Old Republic SC.ttf");
		Q_ASSERT(res != -1);
		//qDebug() << QFontDatabase::applicationFontFamilies(res);
		
		titleFont = QFont("SF Old Republic SC", 20);
		entryFont = QFont("SF Old Republic SC", 23);
		labelFont = QFont("SF Old Republic SC", 16);
		
		QVBoxLayout *vLayout = new QVBoxLayout;
		QHBoxLayout *hLayout = new QHBoxLayout;
		
		hLayout->addStretch();
		addRobotButton = new QPushButton(tr("Add a new robot"));
		hLayout->addWidget(addRobotButton);
		delRobotButton = new QPushButton(tr("Remove all robots"));
		hLayout->addWidget(delRobotButton);
		autoCameraButtons = new QCheckBox(tr("Auto camera"));
		hLayout->addWidget(autoCameraButtons);
		hideButtons = new QCheckBox(tr("Auto hide"));
		hLayout->addWidget(hideButtons);
		hLayout->addStretch();
		vLayout->addLayout(hLayout);
		vLayout->addStretch();
		setLayout(vLayout);
		
		// TODO: setup transparent with style sheet
		connect(addRobotButton, SIGNAL(clicked()), SLOT(addNewRobot()));
		connect(delRobotButton, SIGNAL(clicked()), SLOT(removeRobot()));
		
		setMouseTracking(true);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setAttribute(Qt::WA_NoSystemBackground);
		resize(780, 560);
	}
	
	void PlaygroundViewer::addNewRobot()
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
	
	void PlaygroundViewer::removeRobot()
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

	void PlaygroundViewer::timerEvent(QTimerEvent * event)
	{
		if (autoCameraButtons->isChecked())
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

	void PlaygroundViewer::mouseMoveEvent ( QMouseEvent * event )
	{
		bool isInButtonArea = event->y() < addRobotButton->y() + addRobotButton->height() + 10;
		if (hideButtons->isChecked())
		{
			if (isInButtonArea && !addRobotButton->isVisible())
			{
				addRobotButton->show();
				delRobotButton->show();
				autoCameraButtons->show();
				hideButtons->show();
			}
			if (!isInButtonArea && addRobotButton->isVisible())
			{
				addRobotButton->hide();
				delRobotButton->hide();
				autoCameraButtons->hide();
				hideButtons->hide();
			}
		}
		
		ViewerWidget::mouseMoveEvent(event);
	}
	
	void PlaygroundViewer::keyPressEvent ( QKeyEvent * event )
	{
		if (event->key() == Qt::Key_V)
			savingVideo = true;
		else
			ViewerWidget::keyPressEvent(event);
	}
	
	void PlaygroundViewer::keyReleaseEvent ( QKeyEvent * event )
	{
		if (event->key() == Qt::Key_V)
			savingVideo = false;
		else
			ViewerWidget::keyReleaseEvent (event);
	}
	
	void PlaygroundViewer::drawQuad2D(double x, double y, double w, double ar)
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
	
	void PlaygroundViewer::initializeGL()
	{
		ViewerWidget::initializeGL();
	}
	
	void PlaygroundViewer::renderObjectsTypesHook()
	{
		// render vrcs specific static types
		managedObjects[&typeid(EPuckFeeder)] = new FeederModel(this);
		managedObjectsAliases[&typeid(AsebaFeedableEPuck)] = &typeid(EPuck);
	}
	
	void PlaygroundViewer::displayObjectHook(PhysicalObject *object)
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
	
	void PlaygroundViewer::sceneCompletedHook()
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
	}*/
}

// Implementation of aseba glue code

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
	
	// Create the world
	Enki::World world(110.4, 110.4);
	
	// Add feeders
	Enki::EPuckFeeder* feeder = new Enki::EPuckFeeder;
	feeder->pos.x = 16;
	feeder->pos.y = 16;
	world.addObject(feeder);
	
	// Add e-puck
	int ePuckCount = 4;
	for (int i = 0; i < ePuckCount; i++)
	{
		char buffer[9];
		strncpy(buffer, "e-puck  ", sizeof(buffer));
		Enki::AsebaFeedableEPuck* epuck = new Enki::AsebaFeedableEPuck(i+1);
		epuck->pos.x = Enki::random.getRange(90)+10;
		epuck->pos.y = Enki::random.getRange(90)+10;
		buffer[7] = '0' + i;
		epuck->name = buffer;
		world.addObject(epuck);
	}
	
	// Create viewer
	Enki::PlaygroundViewer viewer(&world);
	
	// Show and run
	viewer.setWindowTitle("Playground - Stephane Magnenat (code) - Basilio Noris (gfx)");
	viewer.show();
	
	return app.exec();
}
