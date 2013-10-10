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

#ifndef __CHALLENGE_EPUCK_H
#define __CHALLENGE_EPUCK_H

#include "AsebaGlue.h"
#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>

namespace Enki
{
	class EPuckFeeding : public LocalInteraction
	{
	public:
		double energy;

	public :
		EPuckFeeding(Robot *owner);
		
		virtual void objectStep(double dt, World *w, PhysicalObject *po);
		
		virtual void finalize(double dt, World *w);
	};
	
	class EPuckFeeder : public Robot
	{
	public:
		EPuckFeeding feeding;
	
	public:
		EPuckFeeder();
	};
	
	class ScoreModifier: public GlobalInteraction
	{
	public:
		ScoreModifier(Robot* owner) : GlobalInteraction(owner) {}
		
		virtual void step(double dt, World *w);
	};
	
	class FeedableEPuck: public EPuck
	{
	public:
		double energy;
		double score;
		int diedAnimation;
		ScoreModifier scoreModifier;
	
	public:
		FeedableEPuck();
		
		virtual void controlStep(double dt);
	};
	
	class AsebaFeedableEPuck : public FeedableEPuck, public Aseba::AbstractNodeGlue, public Aseba::SimpleDashelConnection
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
			sint16 productId; 
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
		AsebaFeedableEPuck(unsigned port, int id);
		virtual ~AsebaFeedableEPuck();
		
		// from FeedableEPuck
		
		virtual void controlStep(double dt);
		
		// from AbstractNodeGlue
		
		virtual const AsebaVMDescription* getDescription() const;
		virtual const AsebaLocalEventDescription * getLocalEventsDescriptions() const;
		virtual const AsebaNativeFunctionDescription * const * getNativeFunctionsDescriptions() const;
		virtual void callNativeFunction(uint16 id);
	};
} // Enki

#endif // __CHALLENGE_EPUCK_H
