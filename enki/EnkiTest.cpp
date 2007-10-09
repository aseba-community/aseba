/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>
#include <viewer/Viewer.h>
#include "AsebaMarxbot.h"
#include <iostream>
#include <QtGui>
#include <QtDebug>


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	// Create the world
	Enki::World world(200, 200);
	
	// Create viewer
	Enki::ViewerWidget viewer(&world);
	
	// Create a Khepera and position it
	Enki::AsebaMarxbot *marXbot = new Enki::AsebaMarxbot();
	marXbot->pos = Enki::Point(100, 100);
	
	// objects are garbage collected by the world on destruction
	world.addObject(marXbot);
	
	viewer.setWindowTitle("Aseba Enki");
	viewer.show();
	
	return app.exec();
}

