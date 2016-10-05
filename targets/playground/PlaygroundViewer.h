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

#ifndef __PLAYGROUND_VIEWER_H
#define __PLAYGROUND_VIEWER_H

#include "../../common/utils/utils.h"
#include <viewer/Viewer.h>
#include <QProcess>

#include <QMainWindow>

#define LOG_COLOR(t,c) Enki::PlaygroundViewer::getInstance()->log(t,c)
#define LOG_INFO(t) Enki::PlaygroundViewer::getInstance()->log(t,Qt::white)
#define LOG_WARN(t) Enki::PlaygroundViewer::getInstance()->log(t,Qt::yellow)
#define LOG_ERR(t) Enki::PlaygroundViewer::getInstance()->log(t,Qt::red)

#define LOG_HISTORY_COUNT 20

namespace Enki
{
	class World;
	
	class PlaygroundViewer : public QMainWindow
	{
		Q_OBJECT
		
	public:
		QFont font;
		QString logText[LOG_HISTORY_COUNT];
		QColor logColor[LOG_HISTORY_COUNT];
		Aseba::UnifiedTime logTime[LOG_HISTORY_COUNT];
		unsigned logPos;
		const int timerPeriodMs;
		unsigned energyPool;

		ViewerWidget* viewer;

	public:
		PlaygroundViewer(World* world);
		virtual ~PlaygroundViewer();
		
		World* getWorld() const;
		ViewerWidget* getViewer();
		static PlaygroundViewer* getInstance();
		
		void log(const QString& entry, const QColor& color);
		void timerEvent(QTimerEvent* event);
		void startSimulation();
		void setCamera(QPointF planPosition, double altitude, double yaw, double pitch);
		void keyPressEvent(QKeyEvent* event);
		
	public slots:
		void processStarted();
		void processError(QProcess::ProcessError error);
		void processReadyRead();
		void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
		
	protected:
		virtual void renderObjectsTypesHook();
		virtual void sceneCompletedHook();
	};
}

#endif // __PLAYGROUND_VIEWER_H
