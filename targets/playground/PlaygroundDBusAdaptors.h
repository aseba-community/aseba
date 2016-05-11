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

#ifndef __PLAYGROUND_DBUS_ADAPTORS_H
#define __PLAYGROUND_DBUS_ADAPTORS_H

#ifdef HAVE_DBUS

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusMessage>
#include <QStringList>

namespace Enki
{
	class PlaygroundViewer;
	class EnkiWorldInterface;
	class PhysicalObject;
	class AsebaThymio2;
	
	class PhysicalObjectInterface: public QObject
	{
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.EnkiPhysicalObject")
		Q_PROPERTY(double x READ getX WRITE setX)
		Q_PROPERTY(double y READ getY WRITE setY)
		Q_PROPERTY(double angle READ getAngle WRITE setAngle)
		
	private:
		EnkiWorldInterface* enkiWorldInterface;
		PhysicalObject* physicalObject;
			
	public:
		PhysicalObjectInterface(EnkiWorldInterface* enkiWorldInterface, PhysicalObject* physicalObject);
		
		double getX() const;
		void setX(double x);
		double getY() const;
		void setY(double Y);
		double getAngle() const;
		void setAngle(double angle);
	
	public slots:
		void Free();
	};
	
	class Thymio2Interface: public QObject
	{
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.PlaygroundThymio2")
		
	private:
		EnkiWorldInterface* enkiWorldInterface;
		AsebaThymio2* thymio;
	
	public:
		Thymio2Interface(EnkiWorldInterface* enkiWorldInterface, AsebaThymio2* thymio);
	
	public slots:
		void Clap();
		void Tap();
		void SetButton(unsigned number, bool value, const QDBusMessage &message);
		void Free();
	};

	class EnkiWorldInterface: public QDBusAbstractAdaptor
	{
		Q_OBJECT
		Q_CLASSINFO("D-Bus Interface", "ch.epfl.mobots.EnkiWorld")
		
	private:
		PlaygroundViewer *playground;
	
	public:
		EnkiWorldInterface(PlaygroundViewer *playground);
		
	public slots:
		QStringList PhysicalObjectsByType(QString type) const;
		QStringList AllPhysicalObjects() const;
		QDBusObjectPath PhysicalObject(QString number, const QDBusMessage &message);
		
	protected:
		friend class PhysicalObjectInterface;
		friend class Thymio2Interface;
		bool isPointerValid(Enki::PhysicalObject* physicalObject) const;
	};
}

#endif // HAVE_DBUS

#endif // __PLAYGROUND_DBUS_ADAPTORS_H


