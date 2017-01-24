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


#include "DashelAsebaGlue.h"
#include "Door.h"
#include "PlaygroundViewer.h"
#include <QtXml>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>

#ifdef HAVE_DBUS
#include "PlaygroundDBusAdaptors.h"
#endif // HAVE_DBUS

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	/*
	// Translation support
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name());
	app.installTranslator(&qtTranslator);
	
	QTranslator translator;
	translator.load(QString(":/asebachallenge_") + QLocale::system().name());
	app.installTranslator(&translator);
	*/
	
	// create document
	QDomDocument domDocument("aseba-playground");
	
	// Get cmd line arguments
	QString fileName;
	bool ask = true;
	if (argc > 1)
	{
		fileName = argv[1];
		ask = false;
	}
	
	// Try to load xml config file
	do
	{
		if (ask)
		{
			QString lastFileName = QSettings("EPFL-LSRO-Mobots", "Aseba Playground").value("last file").toString();
			fileName = QFileDialog::getOpenFileName(0, app.tr("Open Scenario"), lastFileName, app.tr("playground scenario (*.playground)"));
		}
		ask = true;
		
		if (fileName.isEmpty())
		{
			std::cerr << "You must specify a valid setup scenario on the command line or choose one in the file dialog" << std::endl;
			exit(1);
		}
		
		QFile file(fileName);
		if (file.open(QIODevice::ReadOnly))
		{
			QString errorStr;
			int errorLine, errorColumn;
			if (!domDocument.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
			{
				QMessageBox::information(0, "Aseba Playground",
										app.tr("Parse error at file %1, line %2, column %3:\n%4")
										.arg(fileName)
										.arg(errorLine)
										.arg(errorColumn)
										.arg(errorStr));
			}
			else
			{
				QSettings("EPFL-LSRO-Mobots", "Aseba Playground").setValue("last file", fileName);
				break;
			}
		}
	}
	while (true);
	
	// Scan for colors
	typedef QMap<QString, Enki::Color> ColorsMap;
	ColorsMap colorsMap;
	QDomElement colorE = domDocument.documentElement().firstChildElement("color");
	while (!colorE.isNull())
	{
		colorsMap[colorE.attribute("name")] = Enki::Color(
			colorE.attribute("r").toDouble(),
			colorE.attribute("g").toDouble(),
			colorE.attribute("b").toDouble()
		);
		
		colorE = colorE.nextSiblingElement ("color");
	}
	
	// Scan for areas
	typedef QMap<QString, Enki::Polygone> AreasMap;
	AreasMap areasMap;
	QDomElement areaE = domDocument.documentElement().firstChildElement("area");
	while (!areaE.isNull())
	{
		Enki::Polygone p;
		QDomElement pointE = areaE.firstChildElement("point");
		while (!pointE.isNull())
		{
			p.push_back(Enki::Point(
				pointE.attribute("x").toDouble(),
				pointE.attribute("y").toDouble()
			));
			pointE = pointE.nextSiblingElement ("point");
		}
		areasMap[areaE.attribute("name")] = p;
		areaE = areaE.nextSiblingElement ("area");
	}
	
	// Create the world
	QDomElement worldE = domDocument.documentElement().firstChildElement("world");
	Enki::Color worldColor(Enki::Color::gray);
	if (!colorsMap.contains(worldE.attribute("color")))
		std::cerr << "Warning, world walls color " << worldE.attribute("color").toStdString() << " undefined\n";
	else
		worldColor = colorsMap[worldE.attribute("color")];
	Enki::World::GroundTexture groundTexture;
	if (worldE.hasAttribute("groundTexture"))
	{
		const QString groundTextureFileName(QFileInfo(fileName).absolutePath() + QDir::separator() + worldE.attribute("groundTexture"));
		QImage image(groundTextureFileName);
		if (!image.isNull())
		{
			// flip vertically as y-coordinate is inverted in an image
			image = image.mirrored();
			// convert to a specific format and copy the underlying data to Enki
			image = image.convertToFormat(QImage::Format_ARGB32);
			groundTexture.width = image.width();
			groundTexture.height = image.height();
			const uint32_t* imageData(reinterpret_cast<const uint32_t*>(image.constBits()));
			std::copy(imageData, imageData+image.width()*image.height(), std::back_inserter(groundTexture.data));
			// Note: this works in little endian, in big endian data should be swapped
		}
		else
		{
			qDebug() << "Could not load ground texture file named" << groundTextureFileName;
		}
	}
	Enki::World world(
		worldE.attribute("w").toDouble(),
		worldE.attribute("h").toDouble(),
		worldColor,
		groundTexture
	);
	
	// Create viewer
	Enki::PlaygroundViewer viewer(&world, worldE.attribute("energyScoringSystemEnabled", "false").toLower() == "true");
	
	// Scan for camera
	QDomElement cameraE = domDocument.documentElement().firstChildElement("camera");
	if (!cameraE.isNull())
	{
		const double largestDim(qMax(world.h, world.w));
		viewer.setCamera(
			QPointF(
				cameraE.attribute("x", QString::number(world.w / 2)).toDouble(),
				cameraE.attribute("y", QString::number(0)).toDouble()
			),
			cameraE.attribute("altitude", QString::number(0.85 * largestDim)).toDouble(),
			cameraE.attribute("yaw", QString::number(-M_PI/2)).toDouble(),
			cameraE.attribute("pitch", QString::number((3*M_PI)/8)).toDouble()
		);
	}
	
	// Scan for walls
	QDomElement wallE = domDocument.documentElement().firstChildElement("wall");
	while (!wallE.isNull())
	{
		Enki::PhysicalObject* wall = new Enki::PhysicalObject();
		if (!colorsMap.contains(wallE.attribute("color")))
			std::cerr << "Warning, color " << wallE.attribute("color").toStdString() << " undefined\n";
		else
			wall->setColor(colorsMap[wallE.attribute("color")]);
		wall->pos.x = wallE.attribute("x").toDouble();
		wall->pos.y = wallE.attribute("y").toDouble();
		wall->setRectangular(
			wallE.attribute("l1").toDouble(),
			wallE.attribute("l2").toDouble(),
			wallE.attribute("h").toDouble(),
			!wallE.attribute("mass").isNull() ? wallE.attribute("mass").toDouble() : -1 // normally -1 because immobile
		);
		if (! wallE.attribute("angle").isNull())
			wall->angle = wallE.attribute("angle").toDouble(); // radians
		world.addObject(wall);
		
		wallE  = wallE.nextSiblingElement ("wall");
	}
	
	// Scan for cylinders
	QDomElement cylinderE = domDocument.documentElement().firstChildElement("cylinder");
	while (!cylinderE.isNull())
	{
		Enki::PhysicalObject* cylinder = new Enki::PhysicalObject();
		if (!colorsMap.contains(cylinderE.attribute("color")))
			std::cerr << "Warning, color " << cylinderE.attribute("color").toStdString() << " undefined\n";
		else
			cylinder->setColor(colorsMap[cylinderE.attribute("color")]);
		cylinder->pos.x = cylinderE.attribute("x").toDouble();
		cylinder->pos.y = cylinderE.attribute("y").toDouble();
		cylinder->setCylindric(
			cylinderE.attribute("r").toDouble(), 
			cylinderE.attribute("h").toDouble(),
			!cylinderE.attribute("mass").isNull() ? cylinderE.attribute("mass").toDouble() : -1 // normally -1 because immobile
		);
		world.addObject(cylinder);
		
		cylinderE = cylinderE.nextSiblingElement("cylinder");
	}
	
	// Scan for feeders
	QDomElement feederE = domDocument.documentElement().firstChildElement("feeder");
	while (!feederE.isNull())
	{
		Enki::EPuckFeeder* feeder = new Enki::EPuckFeeder;
		feeder->pos.x = feederE.attribute("x").toDouble();
		feeder->pos.y = feederE.attribute("y").toDouble();
		world.addObject(feeder);
	
		feederE = feederE.nextSiblingElement ("feeder");
	}
	// TODO: if needed, custom color to feeder
	
	// Scan for doors
	typedef QMap<QString, Enki::SlidingDoor*> DoorsMap;
	DoorsMap doorsMap;
	QDomElement doorE = domDocument.documentElement().firstChildElement("door");
	while (!doorE.isNull())
	{
		Enki::SlidingDoor *door = new Enki::SlidingDoor(
			Enki::Point(
				doorE.attribute("closedX").toDouble(),
				doorE.attribute("closedY").toDouble()
			),
			Enki::Point(
				doorE.attribute("openedX").toDouble(),
				doorE.attribute("openedY").toDouble()
			),
			Enki::Point(
				doorE.attribute("l1").toDouble(),
				doorE.attribute("l2").toDouble()
			),
			doorE.attribute("h").toDouble(),
			doorE.attribute("moveDuration").toDouble()
		);
		if (!colorsMap.contains(doorE.attribute("color")))
			std::cerr << "Warning, door color " << doorE.attribute("color").toStdString() << " undefined\n";
		else
			door->setColor(colorsMap[doorE.attribute("color")]);
		doorsMap[doorE.attribute("name")] = door;
		world.addObject(door);
		
		doorE = doorE.nextSiblingElement ("door");
	}
	
	// Scan for activation, and link them with areas and doors
	QDomElement activationE = domDocument.documentElement().firstChildElement("activation");
	while (!activationE.isNull())
	{
		if (areasMap.find(activationE.attribute("area")) == areasMap.end())
		{
			std::cerr << "Warning, area " << activationE.attribute("area").toStdString() << " undefined\n";
			activationE = activationE.nextSiblingElement ("activation");
			continue;
		}
		
		if (doorsMap.find(activationE.attribute("door")) == doorsMap.end())
		{
			std::cerr << "Warning, door " << activationE.attribute("door").toStdString() << " undefined\n";
			activationE = activationE.nextSiblingElement ("activation");
			continue;
		}
		
		const Enki::Polygone& area = *areasMap.find(activationE.attribute("area"));
		Enki::Door* door = *doorsMap.find(activationE.attribute("door"));
		
		Enki::DoorButton* activation = new Enki::DoorButton(
			Enki::Point(
				activationE.attribute("x").toDouble(),
				activationE.attribute("y").toDouble()
			),
			Enki::Point(
				activationE.attribute("l1").toDouble(),
				activationE.attribute("l2").toDouble()
			),
			area,
			door
		);
		
		world.addObject(activation);
		
		activationE = activationE.nextSiblingElement ("activation");
	}
	
	// Scan for e-puck
	// TODO: make sure we do not try to open twice the same ports
	//QSet<unsigned> portUsed;
	QDomElement ePuckE = domDocument.documentElement().firstChildElement("e-puck");
	unsigned asebaServerCount(0);
	while (!ePuckE.isNull())
	{
		const unsigned port(ePuckE.attribute("port", QString("%0").arg(ASEBA_DEFAULT_PORT+asebaServerCount)).toUInt());	
		Enki::AsebaFeedableEPuck* epuck(new Enki::DashelAsebaFeedableEPuck(port, asebaServerCount + 1));
		asebaServerCount++;
		epuck->pos.x = ePuckE.attribute("x").toDouble();
		epuck->pos.y = ePuckE.attribute("y").toDouble();
		epuck->angle = ePuckE.attribute("angle").toDouble();
		world.addObject(epuck);
		viewer.log(app.tr("New e-puck on port %0").arg(port), Qt::white);
		ePuckE = ePuckE.nextSiblingElement ("e-puck");
	}
	
	// Scan for Thymio 2
	QDomElement thymioE(domDocument.documentElement().firstChildElement("thymio2"));
	while (!thymioE.isNull())
	{
		const unsigned port(thymioE.attribute("port", QString("%0").arg(ASEBA_DEFAULT_PORT+asebaServerCount)).toUInt());
		Enki::AsebaThymio2* thymio(new Enki::DashelAsebaThymio2(port));
		asebaServerCount++;
		thymio->pos.x = thymioE.attribute("x").toDouble();
		thymio->pos.y = thymioE.attribute("y").toDouble();
		thymio->angle = thymioE.attribute("angle").toDouble();
		world.addObject(thymio);
		viewer.log(app.tr("New Thymio II on port %0").arg(port), Qt::white);
		thymioE = thymioE.nextSiblingElement ("thymio2");
	}
	
	// Scan for external processes
	QList<QProcess*> processes;
	QDomElement procssE(domDocument.documentElement().firstChildElement("process"));
	while (!procssE.isNull())
	{
		QString command(procssE.attribute("command"));
		// create process
		processes.push_back(new QProcess());
		processes.back()->setProcessChannelMode(QProcess::MergedChannels);
		// make sure it is killed when we close the window
		QObject::connect(processes.back(), SIGNAL(started()), &viewer, SLOT(processStarted()));
		QObject::connect(processes.back(), SIGNAL(error(QProcess::ProcessError)), &viewer, SLOT(processError(QProcess::ProcessError)));
		QObject::connect(processes.back(), SIGNAL(readyReadStandardOutput()), &viewer, SLOT(processReadyRead()));
		QObject::connect(processes.back(), SIGNAL(finished(int, QProcess::ExitStatus)), &viewer, SLOT(processFinished(int, QProcess::ExitStatus)));
		// check whether it is a relative command
		bool isRelative(false);
		if (!command.isEmpty() && command[0] == ':')
		{
			isRelative = true;
			command = command.mid(1);
		}
		// process the command into its components
		QStringList args(command.split(" ", QString::SkipEmptyParts));
		if (args.size() == 0)
		{
			viewer.log(app.tr("Missing program in command"), Qt::red);
		}
		else
		{
			const QString program(QDir::toNativeSeparators(args[0]));
			args.pop_front();
			if (isRelative)
				processes.back()->start(QCoreApplication::applicationDirPath() + QDir::separator() + program, args, QIODevice::ReadOnly);
			else
				processes.back()->start(program, args, QIODevice::ReadOnly);
		}
		procssE = procssE.nextSiblingElement("process");
	}
	
	// Show and run
	viewer.setWindowTitle(app.tr("Aseba Playground - Simulate your robots!"));
	viewer.show();
	
	// If D-Bus is used, register the viewer object
	#ifdef HAVE_DBUS
	new Enki::EnkiWorldInterface(&viewer);
	QDBusConnection::sessionBus().registerObject("/world", &viewer);	QDBusConnection::sessionBus().registerService("ch.epfl.mobots.AsebaPlayground");
	#endif // HAVE_DBUS
	
	// Run the application
	const int exitValue(app.exec());
	
	// Stop and delete ongoing processes
	foreach(QProcess*process,processes)
	{
		process->terminate();
		if (!process->waitForFinished(1000))
			process->kill();
		delete process;
	}
	
	return exitValue;
}
