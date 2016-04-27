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

#ifdef HAVE_DBUS

#include "PlaygroundDBusAdaptors.h"
#include "PlaygroundViewer.h"
#include <QDBusConnection>
#include <QDBusMessage>

namespace Enki
{
	PhysicalObjectInterface::PhysicalObjectInterface(EnkiWorldInterface* enkiWorldInterface, PhysicalObject* physicalObject):
		QObject(enkiWorldInterface),
		enkiWorldInterface(enkiWorldInterface),
		physicalObject(physicalObject)
	{
	}
	
	double PhysicalObjectInterface::getX() const
	{
		if (!enkiWorldInterface->isPointerValid(physicalObject))
			return 0; // in the future, do an error and delete this interface
		return physicalObject->pos.x;
	}
	
	void PhysicalObjectInterface::setX(double x)
	{
		if (!enkiWorldInterface->isPointerValid(physicalObject))
			return; // in the future, do an error and delete this interface
		physicalObject->pos.x = x;
	}
	
	double PhysicalObjectInterface::getY() const
	{
		if (!enkiWorldInterface->isPointerValid(physicalObject))
			return 0; // in the future, do an error and delete this interface
		return physicalObject->pos.y;
	}
	
	void PhysicalObjectInterface::setY(double y)
	{
		if (!enkiWorldInterface->isPointerValid(physicalObject))
			return; // in the future, do an error and delete this interface
		physicalObject->pos.y = y;
	}
	
	double PhysicalObjectInterface::getAngle() const
	{
		if (!enkiWorldInterface->isPointerValid(physicalObject))
			return 0; // in the future, do an error and delete this interface
		return physicalObject->angle;
	}
	
	void PhysicalObjectInterface::setAngle(double angle)
	{
		if (!enkiWorldInterface->isPointerValid(physicalObject))
			return; // in the future, do an error and delete this interface
		physicalObject->angle = angle;
	}
	
	void PhysicalObjectInterface::Free()
	{
		deleteLater();
	}
	
	
	EnkiWorldInterface::EnkiWorldInterface(PlaygroundViewer *playground):
		QDBusAbstractAdaptor(playground),
		playground(playground)
	{
		
	}
	
	static QString ptrToString(void *ptr)
	{
		return QString("%1").arg((quintptr)ptr, QT_POINTER_SIZE * 2, 16, QChar('0'));
	}
	
	QStringList EnkiWorldInterface::PhysicalObjectsByType(QString type) const
	{
		QStringList objectNames;
		const World* world(playground->getWorld());
		for (World::ObjectsIterator it(world->objects.begin()); it != world->objects.end(); ++it)
		{
			Enki::PhysicalObject* object(*it);
			if (QString(typeid(*object).name()).contains(type))
				objectNames.append(ptrToString(*it));
		}
		return objectNames;
	}
	
	QStringList EnkiWorldInterface::AllPhysicalObjects() const
	{
		return PhysicalObjectsByType("");
	}
	
	QDBusObjectPath EnkiWorldInterface::PhysicalObject(QString number, const QDBusMessage &message)
	{
		const World* world(playground->getWorld());
		for (World::ObjectsIterator it(world->objects.begin()); it != world->objects.end(); ++it)
		{
			if (ptrToString(*it) == number)
			{
				QDBusObjectPath path(QString("/ch/epfl/mobots/AsebaPlayground/world/objects/%0").arg(number));
				QDBusConnection::sessionBus().registerObject(path.path(), new PhysicalObjectInterface(this, *it), QDBusConnection::ExportAllContents);
				return path;
			}
		}
		QDBusConnection::sessionBus().send(message.createErrorReply(QDBusError::InvalidArgs, QString("object %0 does not exists").arg(number)));
		return QDBusObjectPath("/");
	}
	
	bool EnkiWorldInterface::isPointerValid(Enki::PhysicalObject* physicalObject) const
	{
		const World* world(playground->getWorld());
		return world->objects.find(physicalObject) != world->objects.end();
	}
}

#endif // HAVE_DBUS
