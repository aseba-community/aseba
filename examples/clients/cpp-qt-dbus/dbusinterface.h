/*
	A Qt binding for Aseba that relies on DBus

	Supports event sending/receiving, Aseba scripts Loading, as well as setting/getting an Aseba variable.

	Authors: Frank Bonnet, Stefan Witwicki
	Copyright (C) 2007--2016

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

#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QtDBus/QtDBus>
#include <functional>

Q_DECLARE_METATYPE( QList<qint16> );


typedef QList<qint16> Values;

namespace Aseba
{
	class DBusInterface : public QObject
	{
		Q_OBJECT

	private:

		QDBusConnection bus;
		typedef std::function<void(const Values&)> EventCallback;
		std::map<QString, EventCallback> callbacks;
		QDBusInterface dbusMainInterface;
		QDBusInterface* eventfilterInterface;

	public:

		DBusInterface();

		QList<QString> nodeList;

		static QVariant valuetoVariant(const Values& value);
		static Values dBusMessagetoValues(const QDBusMessage& dbmess, int index);
		static std::string toString(const Values& v);


		bool checkConnection();
		void displayNodeList();
		void loadScript(const QString& script);
		Values getVariable(const QString& node, const QString& variable);
		void setVariable(const QString& node, const QString& variable, const Values& value);
		void connectEvent(const QString& eventName, EventCallback callback);

		void sendEvent(quint16 eventID, const Values& value);
		void sendEventName(const QString& eventName, const Values& value);

	public slots:

		void dispatchEvent(const QDBusMessage& message);
	};
}

#endif // DBUSINTERFACE_H
