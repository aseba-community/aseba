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
#include "EPuck.h"
#include "../../common/utils/utils.h"

#ifdef Q_OS_WIN32
	#define Q_PID_PRINT size_t
#else // Q_OS_WIN32
	#define Q_PID_PRINT qint64
#endif // Q_OS_WIN32

namespace Enki
{
	using namespace Aseba;
	
	static PlaygroundViewer* playgroundViewer = 0;

	PlaygroundViewer::PlaygroundViewer(World* world) : 
		ViewerWidget(world),
		font("Courier", 10),
		logPos(0),
		energyPool(INITIAL_POOL_ENERGY)
	{
		//font.setPixelSize(14);
		if (playgroundViewer)
			abort();
		playgroundViewer = this;
	}
	
	PlaygroundViewer::~PlaygroundViewer()
	{
		
	}
	
	World* PlaygroundViewer::getWorld() const
	{
		return world;
	}
	
	PlaygroundViewer* PlaygroundViewer::getInstance()
	{
		return playgroundViewer;
	}
	
	void PlaygroundViewer::log(const QString& entry, const QColor& color)
	{
		logText[logPos] = entry;
		logColor[logPos] = color;
		logTime[logPos] = Aseba::UnifiedTime();
		logPos = (logPos+1) % LOG_HISTORY_COUNT;
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
		managedObjectsAliases[&typeid(AsebaFeedableEPuck)] = &typeid(EPuck);
		// TODO: add display of Thymio 2 in EnkiViewer, then add alias here
	}
	
	void PlaygroundViewer::sceneCompletedHook()
	{
		// create a map with names and scores
		//qglColor(QColor::fromRgbF(0, 0.38 ,0.61));
		qglColor(Qt::black);
		
		// TODO: clean-up that
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
			glVertex2f(1,-0.5);
			glVertex2f(-1,-0.5);
		glEnd();
		// console text
		for (unsigned i=0; i<LOG_HISTORY_COUNT; ++i)
		{
			const unsigned j((logPos+LOG_HISTORY_COUNT+LOG_HISTORY_COUNT-1-i) % LOG_HISTORY_COUNT);
			if (!logText[j].isEmpty())
			{
				qglColor(logColor[j]);
				renderText(8,(height()*3)/4 + 14 + i*14, QString::fromStdString(logTime[j].toHumanReadableStringFromEpoch()) + " " + logText[j],font);
			}
		}
	}
} // Enki
