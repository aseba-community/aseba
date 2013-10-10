/*
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

#ifndef __CHALLENGE_DOOR_H
#define __CHALLENGE_DOOR_H

#include <enki/PhysicalEngine.h>

namespace Enki
{
	class Door: public PhysicalObject
	{
	public:
		virtual void open() = 0;
		virtual void close() = 0;
	};
	
	class SlidingDoor: public Door
	{
	public:
		const Point closedPos;
		const Point openedPos;
		const double moveDuration;
		
	protected:
		enum Mode
		{
			MODE_CLOSED,
			MODE_OPENING,
			MODE_OPENED,
			MODE_CLOSING
		} mode;
		double moveTimeLeft;
	
	public:
		SlidingDoor(const Point& closedPos, const Point& openedPos, const Point& size, double height, double moveDuration);
		
		virtual void controlStep(double dt);
		
		virtual void open(void);
		virtual void close(void);
	};
	
	class AreaActivating: public LocalInteraction
	{
	public:
		const Polygone activeArea;
		
	protected:
		bool active;
		
	public:
		AreaActivating(Robot *owner, const Polygone& activeArea);
		
		virtual void init(double dt, World *w);
		virtual void objectStep (double dt, World *w, PhysicalObject *po);
		
		bool isActive() const;
	};
	
	class DoorButton: public Robot
	{
	protected:
		AreaActivating areaActivating;
		bool wasActive;
		Door *const attachedDoor;
	
	public:
		DoorButton(const Point& pos, const Point& size, const Polygone& activeArea, Door* attachedDoor);
		
		virtual void controlStep(double dt);
	};
} // Enki

#endif // __CHALLENGE_DOOR_H
