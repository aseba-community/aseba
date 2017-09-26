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

#include "EnkiGlue.h"
#include "../../common/utils/utils.h"
#include <viewer/Viewer.h>
#include <QProcess>

#define LOG_HISTORY_COUNT 20

namespace Enki
{
	class World;
	
	class PlaygroundViewer : public ViewerWidget
	{
		Q_OBJECT
		
	public:
		QFont font;
		QString logText[LOG_HISTORY_COUNT];
		QColor logColor[LOG_HISTORY_COUNT];
		Aseba::UnifiedTime logTime[LOG_HISTORY_COUNT];
		bool energyScoringSystemEnabled;
		unsigned logPos;
		unsigned energyPool;
		
	public:
		PlaygroundViewer(World* world, bool energyScoringSystemEnabled = false);
		virtual ~PlaygroundViewer();
		
		void log(const QString& entry, const QColor& color);
		void log(const std::string& entry, const QColor& color);
		void log(const char* entry, const QColor& color);
		
	public slots:
		void processStarted();
		void processError(QProcess::ProcessError error);
		void processReadyRead();
		void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
	
	protected:
		virtual void renderObjectsTypesHook();
		virtual void sceneCompletedHook();
		virtual void mouseMoveEvent(QMouseEvent*);
		virtual void timerEvent(QTimerEvent * event);
	
	protected:
		friend class PlaygroundSimulatorEnvironment;
		friend class EnkiWorldInterface;
		World* getWorld() const;
		void notifyAsebaEnvironment(const EnvironmentNotificationType type, const std::string& description, const strings& arguments);
	};
}

#endif // __PLAYGROUND_VIEWER_H
