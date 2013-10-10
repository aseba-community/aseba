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

#include "Door.h"
#include "Parameters.h"

namespace Enki
{
	// SlidingDoor
	
	SlidingDoor::SlidingDoor(const Point& closedPos, const Point& openedPos, const Point& size, double height, double moveDuration) :
		closedPos(closedPos),
		openedPos(openedPos),
		moveDuration(moveDuration),
		mode(MODE_CLOSED),
		moveTimeLeft(0)
	{
		setRectangular(size.x, size.y, height, -1);
	}
	
	void SlidingDoor::controlStep(double dt)
	{
		// cos interpolation between positions
		double alpha;
		switch (mode)
		{
			case MODE_CLOSED:
			pos = closedPos;
			break;
			
			case MODE_OPENING:
			alpha = (cos((moveTimeLeft*M_PI)/moveDuration) + 1.) / 2.;
			pos = openedPos * alpha + closedPos * (1 - alpha);
			moveTimeLeft -= dt;
			if (moveTimeLeft < 0)
				mode = MODE_OPENED;
			break;
			
			case MODE_OPENED:
			pos = openedPos;
			break;
			
			case MODE_CLOSING:
			alpha = (cos((moveTimeLeft*M_PI)/moveDuration) + 1.) / 2.;
			pos = closedPos * alpha + openedPos * (1 - alpha);
			moveTimeLeft -= dt;
			if (moveTimeLeft < 0)
				mode = MODE_CLOSED;
			break;
			
			default:
			break;
		}
		PhysicalObject::controlStep(dt);
	}
	
	void SlidingDoor::open(void)
	{
		if (mode == MODE_CLOSED)
		{
			moveTimeLeft = moveDuration;
			mode = MODE_OPENING;
		}
		else if (mode == MODE_CLOSING)
		{
			moveTimeLeft = moveDuration - moveTimeLeft;
			mode = MODE_OPENING;
		}
	}
	
	void SlidingDoor::close(void)
	{
		if (mode == MODE_OPENED)
		{
			moveTimeLeft = moveDuration;
			mode = MODE_CLOSING;
		}
		else if (mode == MODE_OPENING)
		{
			moveTimeLeft = moveDuration - moveTimeLeft;
			mode = MODE_CLOSING;
		}
	}
	
	// AreaActivating
	
	AreaActivating::AreaActivating(Robot *owner, const Polygone& activeArea) :
		activeArea(activeArea),
		active(false)
	{
		r = activeArea.getBoundingRadius();
		this->owner = owner;
	}
	
	void AreaActivating::init(double dt, World *w)
	{
		active = false;
	}
	
	void AreaActivating::objectStep (double dt, World *w, PhysicalObject *po)
	{
		if (po != owner && dynamic_cast<Robot*>(po))
		{
			active |= activeArea.isPointInside(po->pos - owner->pos);
		}
	}
	
	bool AreaActivating::isActive() const
	{
		return active;
	}
	
	// DoorButton
	
	DoorButton::DoorButton(const Point& pos, const Point& size, const Polygone& activeArea, Door* attachedDoor) :
		areaActivating(this, activeArea),
		wasActive(false),
		attachedDoor(attachedDoor)
	{
		this->pos = pos;
		setRectangular(size.x, size.y, ACTIVATION_OBJECT_HEIGHT, -1);
		addLocalInteraction(&areaActivating);
		setColor(ACTIVATION_OBJECT_COLOR);
	}
	
	void DoorButton::controlStep(double dt)
	{
		Robot::controlStep(dt);
		
		if (areaActivating.isActive() != wasActive)
		{
			//std::cerr << "Door activity changed to " << areaActivating.active <<  "\n";
			if (areaActivating.isActive())
				attachedDoor->open();
			else
				attachedDoor->close();
			wasActive = areaActivating.isActive();
		}
	}
} // Enki
