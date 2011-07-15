/*
	Playground - An active arena to learn multi-robots programming
	Copyright (C) 1999--2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

#ifndef __CHALLENGE_H
#define __CHALLENGE_H

#include <viewer/Viewer.h>
#include <dashel/dashel.h>
#include <QVector>

class QPushButton;
class QCheckBox;
#ifdef USE_SDL
typedef struct _SDL_Joystick SDL_Joystick;
#endif

namespace Enki
{
	class PlaygroundViewer : public ViewerWidget, public Dashel::Hub
	{
	public:
		// these have to be public for the aseba C interface
		Dashel::Stream* stream;
		uint16 lastMessageSource;
		std::valarray<uint8> lastMessageData;
		QFont font;
		unsigned energyPool;
		#ifdef USE_SDL
		QVector<SDL_Joystick *> joysticks;
		#endif // USE_SDL
		
	public:
		PlaygroundViewer(World* world);
		virtual ~PlaygroundViewer();
		World* getWorld() { return world; }
		
	protected:
		virtual void renderObjectsTypesHook();
		virtual void sceneCompletedHook();
		
		virtual void connectionCreated(Dashel::Stream *stream);
		virtual void incomingData(Dashel::Stream *stream);
		virtual void connectionClosed(Dashel::Stream *stream, bool abnormal);
		
		virtual void timerEvent(QTimerEvent * event);
	};
}

#endif

