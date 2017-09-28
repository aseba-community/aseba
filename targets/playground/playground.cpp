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

#include "../../common/utils/FormatableString.h"
#include "DashelAsebaGlue.h"
#include "Door.h"
#include "PlaygroundViewer.h"
#include "Robots.h"
#include <QtXml>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QDesktopServices>
#include <QDir>
#include <QHash>

#ifdef HAVE_DBUS
#include "PlaygroundDBusAdaptors.h"
#endif // HAVE_DBUS

#ifdef ZEROCONF_SUPPORT
#include "../../common/zeroconf/zeroconf-qt.h"
#endif // ZEROCONF_SUPPORT

namespace Enki
{
	//! The simulator environment for playground
	class PlaygroundSimulatorEnvironment: public SimulatorEnvironment
	{
	public:
		const QString sceneFileName;
		PlaygroundViewer& viewer;
		
	public:
		PlaygroundSimulatorEnvironment(const QString& sceneFileName, PlaygroundViewer& viewer):
			sceneFileName(sceneFileName),
			viewer(viewer)
		{}
		
		virtual void notify(const EnvironmentNotificationType type, const std::string& description, const strings& arguments) override
		{
			viewer.notifyAsebaEnvironment(type, description, arguments);
		}
		
		virtual std::string getSDFilePath(const std::string& robotName, unsigned fileNumber) const override
		{
			auto fileName(QString("%1/%2/%3/U%4.DAT")
				.arg(QDesktopServices::storageLocation(QDesktopServices::DataLocation))
				.arg(qHash(QDir(sceneFileName).canonicalPath()), 8, 16, QChar('0'))
				.arg(QString::fromStdString(robotName))
				.arg(fileNumber)
			);
			QDir().mkpath(QFileInfo(fileName).absolutePath());
			// dump
			std::cerr << fileName.toStdString() << std::endl;
			return fileName.toStdString();
		}
		
		virtual World* getWorld() const override
		{
			return viewer.getWorld();
		}
	};
}

// support for robot factory
struct RobotInstance
{
	Enki::Robot* robot;
#ifdef ZEROCONF_SUPPORT
	const Aseba::Zeroconf::TxtRecord zeroconfProperties;
#endif // ZEROCONF_SUPPORT
};

template<typename RobotT>
RobotInstance createRobotSingleVMNode(unsigned port, std::string robotName, std::string typeName, int16_t nodeId)
{
	auto robot(new RobotT(port, std::move(robotName), nodeId));
#ifdef ZEROCONF_SUPPORT
	const Aseba::Zeroconf::TxtRecord txt
	{
		ASEBA_PROTOCOL_VERSION,
		std::move(typeName),
		false,
		{ robot->vm.nodeId },
		{ static_cast<unsigned int>(robot->variables.productId) }
	};
	return { robot, txt };
#else // ZEROCONF_SUPPORT
	return { robot };
#endif // ZEROCONF_SUPPORT
}

using RobotFactory = std::function<RobotInstance(unsigned, std::string, std::string, int16_t)>;
struct RobotType
{
	RobotType(std::string prettyName, RobotFactory factory):
		prettyName(std::move(prettyName)),
		factory(std::move(factory))
	{}
	const std::string prettyName;
	const RobotFactory factory;
	unsigned number = 0;
};


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setOrganizationName("Aseba"); // FIXME: we should be consistent here
	app.setApplicationName("Playground");
	
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
	QString sceneFileName;
	
	// Get cmd line arguments
	bool ask = true;
	if (argc > 1)
	{
		sceneFileName = argv[1];
		ask = false;
	}
	
	// Try to load xml config file
	do
	{
		if (ask)
		{
			QString lastFileName = QSettings("EPFL-LSRO-Mobots", "Aseba Playground").value("last file").toString();
			sceneFileName = QFileDialog::getOpenFileName(0, app.tr("Open Scenario"), lastFileName, app.tr("playground scenario (*.playground)"));
		}
		ask = true;
		
		if (sceneFileName.isEmpty())
		{
			std::cerr << "You must specify a valid setup scenario on the command line or choose one in the file dialog" << std::endl;
			exit(1);
		}
		
		QFile file(sceneFileName);
		if (file.open(QIODevice::ReadOnly))
		{
			QString errorStr;
			int errorLine, errorColumn;
			if (!domDocument.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
			{
				QMessageBox::information(0, "Aseba Playground",
										app.tr("Parse error at file %1, line %2, column %3:\n%4")
										.arg(sceneFileName)
										.arg(errorLine)
										.arg(errorColumn)
										.arg(errorStr));
			}
			else
			{
				QSettings("EPFL-LSRO-Mobots", "Aseba Playground").setValue("last file", sceneFileName);
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
	typedef QMap<QString, Enki::Polygon> AreasMap;
	AreasMap areasMap;
	QDomElement areaE = domDocument.documentElement().firstChildElement("area");
	while (!areaE.isNull())
	{
		Enki::Polygon p;
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
		const QString groundTextureFileName(QFileInfo(sceneFileName).absolutePath() + QDir::separator() + worldE.attribute("groundTexture"));
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
	if (Enki::simulatorEnvironment)
		qDebug() << "A simulator environment already exists, replacing";
	Enki::simulatorEnvironment.reset(new Enki::PlaygroundSimulatorEnvironment(sceneFileName, viewer));
	
	// Zeroconf support to advertise targets
#ifdef ZEROCONF_SUPPORT
	Aseba::QtZeroconf zeroconf;
#endif // ZEROCONF_SUPPORT
	
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
		
		const Enki::Polygon& area = *areasMap.find(activationE.attribute("area"));
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
	
	// load all robots in one loop
	std::map<std::string, RobotType> robotTypes {
		{ "thymio2", { "Thymio II", createRobotSingleVMNode<Enki::DashelAsebaThymio2> } },
		{ "e-puck", { "E-Puck", createRobotSingleVMNode<Enki::DashelAsebaFeedableEPuck> } },
	};
	QDomElement robotE = domDocument.documentElement().firstChildElement("robot");
	unsigned asebaServerCount(0);
	while (!robotE.isNull())
	{
		const auto type(robotE.attribute("type", "thymio2"));
		auto typeIt(robotTypes.find(type.toStdString()));
		if (typeIt != robotTypes.end())
		{
			// retrieve informations
			const auto& cppTypeName(typeIt->second.prettyName);
			const auto qTypeName(QString::fromStdString(cppTypeName));
			auto& countOfThisType(typeIt->second.number);
			const auto qRobotName(robotE.attribute("name", QString("%1 %2").arg(qTypeName).arg(countOfThisType)));
			const auto cppRobotName(qRobotName.toStdString());
			const unsigned port(robotE.attribute("port", QString("%1").arg(ASEBA_DEFAULT_PORT+asebaServerCount)).toUInt());
			const int16_t nodeId(robotE.attribute("nodeId", "1").toInt());
			
			// create
			const auto& creator(typeIt->second.factory);
			auto instance(creator(port, cppRobotName, cppTypeName, nodeId));
			auto robot(instance.robot);
			asebaServerCount++;
			countOfThisType++;
			
			// setup in the world
			robot->pos.x = robotE.attribute("x").toDouble();
			robot->pos.y = robotE.attribute("y").toDouble();
			robot->angle = robotE.attribute("angle").toDouble();
			world.addObject(robot);
			
			// advertise
			viewer.log(app.tr("New robot %0 of type %1 on port %2").arg(qRobotName).arg(qTypeName).arg(port), Qt::white);
#ifdef ZEROCONF_SUPPORT
			zeroconf.advertise(cppRobotName, port, instance.zeroconfProperties);
#endif // ZEROCONF_SUPPORT
		}
		else
			viewer.log("Error, unknown robot type " + type, Qt::red);
		
		robotE = robotE.nextSiblingElement ("robot");
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
	QDBusConnection::sessionBus().registerObject("/world", &viewer);
	QDBusConnection::sessionBus().registerService("ch.epfl.mobots.AsebaPlayground");
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
