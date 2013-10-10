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

#include <viewer/Viewer.h>

namespace Enki
{
	class World;
	
	class PlaygroundViewer : public ViewerWidget
	{
	public:
		QFont font;
		unsigned energyPool;
		
	public:
		PlaygroundViewer(World* world);
		virtual ~PlaygroundViewer();
		
		World* getWorld() const;
		static PlaygroundViewer* getInstance();
		
	protected:
		virtual void renderObjectsTypesHook();
		virtual void sceneCompletedHook();
	};
}

#endif // __PLAYGROUND_VIEWER_H
