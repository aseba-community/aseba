/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
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

#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>
#include <viewer/Viewer.h>
#include "AsebaMarxbot.h"
#include <iostream>
#include <QtGui>
#include <QtDebug>

namespace Enki
{
	class MarxbotViewer: public ViewerWidget
	{
	public:
		MarxbotViewer(World* world) : ViewerWidget(world) {}
		
		void renderObjectsTypesHook()
		{
			managedObjectsAliases[&typeid(AsebaMarxbot)] = &typeid(Marxbot);
		}
	};
}


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	// Create the world
	Enki::World world(200, 200);
	
	// Create viewer
	Enki::MarxbotViewer viewer(&world);
	
	// Create a Khepera and position it
	Enki::AsebaMarxbot *marXbot = new Enki::AsebaMarxbot();
	marXbot->pos = Enki::Point(100, 100);
	
	// objects are garbage collected by the world on destruction
	world.addObject(marXbot);
	
	viewer.setWindowTitle("Aseba Enki");
	viewer.show();
	
	return app.exec();
}

