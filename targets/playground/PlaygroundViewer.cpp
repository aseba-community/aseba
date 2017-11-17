/*
	Playground - An active arena to learn multi-robots programming
	Copyright (C) 1999--2013:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
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


#include "PlaygroundViewer.h"
#include "Parameters.h"
#include "Robots.h"
#include "../../common/utils/utils.h"
#include <QMessageBox>
#include <QApplication>
#include <QMouseEvent>
#include <QToolTip>
#include <QtDebug>

#ifdef Q_OS_WIN32
	#define Q_PID_PRINT size_t
#else // Q_OS_WIN32
	#define Q_PID_PRINT qint64
#endif // Q_OS_WIN32

namespace Enki
{
	using namespace Aseba;
	
	const std::map<EnvironmentNotificationType, QColor> notificationLogTypeToColor = {
		{ EnvironmentNotificationType::LOG_INFO, Qt::white },
		{ EnvironmentNotificationType::LOG_WARNING, Qt::yellow },
		{ EnvironmentNotificationType::LOG_ERROR, Qt::red }
	};
	
	PlaygroundViewer::PlaygroundViewer(World* world, bool energyScoringSystemEnabled) : 
		ViewerWidget(world),
		font("Courier", 10),
		energyScoringSystemEnabled(energyScoringSystemEnabled),
		logPos(0),
		energyPool(INITIAL_POOL_ENERGY)
	{
		setMouseTracking(true);
	}
	
	PlaygroundViewer::~PlaygroundViewer()
	{
		
	}
	
	World* PlaygroundViewer::getWorld() const
	{
		return world;
	}
	
	void PlaygroundViewer::notifyAsebaEnvironment(const EnvironmentNotificationType type, const std::string& description, const strings& arguments)
	{
		if (type == EnvironmentNotificationType::DISPLAY_INFO)
		{
			if (description == "missing Thymio2 feature")
			{
				addInfoMessage(tr("You are using a feature not available in the simulator, click here to buy a real Thymio"), 5.0, Qt::blue, QUrl(tr("https://www.thymio.org/en:thymiobuy")));
			}
			else
				qDebug() << "Unknown description for notifying DISPLAY_INFO" << QString::fromStdString(description);
		}
		else if ((type == EnvironmentNotificationType::LOG_INFO) ||
				 (type == EnvironmentNotificationType::LOG_WARNING) ||
				 (type == EnvironmentNotificationType::LOG_ERROR))
		{
			if (description == "cannot read from socket")
			{
				log(tr("Target %0, cannot read from socket: %1").arg(QString::fromStdString(arguments.at(0))).arg(QString::fromStdString(arguments.at(1))), notificationLogTypeToColor.at(type));
			}
			else if (description == "new client connected")
			{
				log(tr("New client connected from %0").arg(QString::fromStdString(arguments.at(0))), notificationLogTypeToColor.at(type));
			}
			else if (description == "cannot read from socket")
			{
				log(tr("Target %0, cannot read from socket: %1").arg(QString::fromStdString(arguments.at(0))).arg(QString::fromStdString(arguments.at(1))), notificationLogTypeToColor.at(type));
			}
			else if (description == "client disconnected properly")
			{
				log(tr("Client disconnected properly from %0").arg(QString::fromStdString(arguments.at(0))), notificationLogTypeToColor.at(type));
			}
			else if (description == "client disconnected abnormally")
			{
				log(tr("Client disconnected abnormally from %0").arg(QString::fromStdString(arguments.at(0))), notificationLogTypeToColor.at(type));
			}
			else if (description == "old client disconnected")
			{
				log(tr("Old client disconnected from %0").arg(QString::fromStdString(arguments.at(0))), notificationLogTypeToColor.at(type));
			}
			else
			{
				QString fullDescription(QString::fromStdString(description));
				for (const auto& argument: arguments)
					fullDescription += " " + QString::fromStdString(argument);
				log(fullDescription, notificationLogTypeToColor.at(type));
			}
		}
		else if (type == EnvironmentNotificationType::FATAL_ERROR)
		{
			if (description == "cannot create listening port")
			{
				QMessageBox::critical(0, tr("Aseba Playground"), tr("Cannot create listening port %0: %1").arg(QString::fromStdString(arguments.at(0))).arg(QString::fromStdString(arguments.at(1))));
				abort();
			}
			else
				qDebug() << "Unknown description for notifying FATAL_ERROR" << QString::fromStdString(description);
		}
	}
	
	void PlaygroundViewer::log(const QString& entry, const QColor& color)
	{
		logText[logPos] = entry;
		logColor[logPos] = color;
		logTime[logPos] = Aseba::UnifiedTime();
		logPos = (logPos+1) % LOG_HISTORY_COUNT;
	}
	
	void PlaygroundViewer::log(const std::string& entry, const QColor& color)
	{
		log(QString::fromStdString(entry), color);
	}
	
	void PlaygroundViewer::log(const char* entry, const QColor& color)
	{
		log(QString(entry), color);
	}
	
	void PlaygroundViewer::processStarted()
	{
		QProcess* process(polymorphic_downcast<QProcess*>(sender()));
		// do not display to avoid clutter
		//log(tr("%0: Process started").arg((Q_PID_PRINT)process->pid()), Qt::white);
		Q_UNUSED(process);
	}
	
	void PlaygroundViewer::processError(QProcess::ProcessError error)
	{
		QProcess* process(polymorphic_downcast<QProcess*>(sender()));
		switch (error)
		{
			case QProcess::FailedToStart:
				log(tr("%0: Process failed to start").arg((Q_PID_PRINT)process->pid()), Qt::red);
				break;
			case QProcess::Crashed:
				log(tr("%0: Process crashed").arg((Q_PID_PRINT)process->pid()), Qt::red);
				break;
			case QProcess::WriteError:
				log(tr("%0: Write error").arg((Q_PID_PRINT)process->pid()), Qt::red);
				break;
			case QProcess::ReadError:
				log(tr("%0: Read error").arg((Q_PID_PRINT)process->pid()), Qt::red);
				break;
			case QProcess::UnknownError:
				log(tr("%0: Unknown error").arg((Q_PID_PRINT)process->pid()), Qt::red);
				break;
			default:
				break;
		}
		
	}
	
	void PlaygroundViewer::processReadyRead()
	{
		QProcess* process(polymorphic_downcast<QProcess*>(sender()));
		while (process->canReadLine())
			log(tr("%0: %1").arg((Q_PID_PRINT)process->pid()).arg(process->readLine().constData()), Qt::yellow);
	}
	
	void PlaygroundViewer::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
	{
		QProcess* process(polymorphic_downcast<QProcess*>(sender()));
		// do not display to avoid clutter
		/*if (exitStatus == QProcess::NormalExit)
			log(tr("Process finished").arg((Q_PID_PRINT)process->pid()), Qt::yellow);
		else
			log(tr("Process finished abnormally").arg((Q_PID_PRINT)process->pid()), Qt::red);*/
		Q_UNUSED(process);
	}
	
	void PlaygroundViewer::renderObjectsTypesHook()
	{
		managedObjectsAliases[&typeid(DashelAsebaFeedableEPuck)] = &typeid(EPuck);
		managedObjectsAliases[&typeid(DashelAsebaThymio2)] = &typeid(Thymio2);
	}
	
	void PlaygroundViewer::sceneCompletedHook()
	{
		// create a map with names and scores
		//qglColor(QColor::fromRgbF(0, 0.38 ,0.61));
		qglColor(Qt::black);
		
		// TODO: once we have ECS-enabled Enki, use modules for playground as well
		if (energyScoringSystemEnabled)
		{
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
			
			renderText(16, height() * 7 / 8 - 40, scoreString, font);
			
			renderText(16, height() * 7 / 8 - 20, QString("E. in pool: %0 - total score: %1").arg(energyPool).arg(totalScore), font);
		}
		
		// console background
		glEnable(GL_BLEND);
		qglColor(QColor(0,0,0,200));
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glBegin(GL_QUADS);
			glVertex2f(-1,-1);
			glVertex2f(1,-1);
			glVertex2f(1,-0.75);
			glVertex2f(-1,-0.75);
		glEnd();
		glDisable(GL_BLEND);
		
		// console text
		for (unsigned i=0; i<LOG_HISTORY_COUNT; ++i)
		{
			const unsigned j((logPos+LOG_HISTORY_COUNT+LOG_HISTORY_COUNT-1-i) % LOG_HISTORY_COUNT);
			if (!logText[j].isEmpty())
			{
				qglColor(logColor[j]);
				renderText(8,(height()*7)/8 + 14 + i*14, QString::fromStdString(logTime[j].toHumanReadableStringFromEpoch()) + " " + logText[j],font);
			}
		}
	}
	
	void PlaygroundViewer::mouseMoveEvent(QMouseEvent * event)
	{
		ViewerWidget::mouseMoveEvent(event);
		auto* pointedRobot(dynamic_cast<NamedRobot*>(getPointedObject()));
		if (pointedRobot)
			QToolTip::showText(event->globalPos(), QString::fromStdString(pointedRobot->robotName));
		else
			QToolTip::showText(event->globalPos(), "");
	}
	
	void PlaygroundViewer::timerEvent(QTimerEvent * event)
	{
		// if the object being moved is Aseba-enabled, make sure it processes network events
		AbstractNodeGlue* asebaObject(dynamic_cast<AbstractNodeGlue*>(selectedObject));
		if (asebaObject)
			asebaObject->externalInputStep(double(timerPeriodMs)/1000.);
		
		ViewerWidget::timerEvent(event);
	}
} // Enki
