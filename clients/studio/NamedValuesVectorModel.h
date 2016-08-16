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

#ifndef NAMED_VALUES_VECTOR_MODEL_H
#define NAMED_VALUES_VECTOR_MODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include "../../compiler/compiler.h"


namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	template<typename V>
	class NamedValuesVectorModel: public QAbstractTableModel
	{
		// moved all QObject-related features to subclasses, working around no-template limitation with QObjects
		//Q_OBJECT
		
	public:
		typedef NamedValuesVector<V> NamedValuesVectorV;
		
	public:
		NamedValuesVectorModel(NamedValuesVectorV* namedValues, const QString &tooltipText, QObject *parent = 0);
		NamedValuesVectorModel(NamedValuesVectorV* namedValues, QObject *parent = 0);
		
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		
		QStringList mimeTypes () const;
		void setExtraMimeType(QString mime) { privateMimeType = mime; }
		QMimeData * mimeData ( const QModelIndexList & indexes ) const;
		bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
		Qt::DropActions supportedDragActions();
		Qt::DropActions supportedDropActions() const;
		
		bool setData(const QModelIndex &index, const QVariant &value, int role);
		bool checkIfModified() { return wasModified; }
		void clearWasModified() { wasModified = false; }
		void setEditable(bool editable);
		
		virtual bool moveRow(int oldRow, int& newRow);
		
		virtual bool validateName(const QString& name) const;

	protected:
		// implementation for slots, working around no-template limitation with QObjects
		void _addNamedValue(const NamedValue& namedValue, int index = -1);
		void _delNamedValue(int index);
		void _clear();
		
	protected:
		// callbacks for signals, working around no-template limitation with QObjects
		virtual void publicRowsInsertedCallback() = 0;
		virtual void publicRowsRemovedCallback() = 0;
		
	protected:
		NamedValuesVectorV* namedValues;
		bool wasModified;
		QString privateMimeType;

	private:
		QString tooltipText;
		bool editable;
	};
	
	class ConstantsModel: public NamedValuesVectorModel<ConstantDefinition>
	{
		Q_OBJECT
	
	public:
		ConstantsModel(ConstantsDefinitions* namedValues, const QString &tooltipText, QObject *parent = 0);
		ConstantsModel(ConstantsDefinitions* namedValues, QObject *parent = 0);
		
		virtual bool validateName(const QString& name) const;
		
	// working around no-template limitation with QObjects
	
	public slots:
		void addNamedValue(const NamedValue& namedValue, int index = -1);
		void delNamedValue(int index);
		void clear();

	signals:
		void publicRowsInserted();
		void publicRowsRemoved();
		
	protected:
		virtual void publicRowsInsertedCallback();
		virtual void publicRowsRemovedCallback();
	};

	
	class EventsModel: public NamedValuesVectorModel<EventDescription>
	{
		Q_OBJECT

	public:
		EventsModel(EventsDescriptions* namedValues, const QString &tooltipText, QObject *parent = 0);
		EventsModel(EventsDescriptions* namedValues, QObject *parent = 0);

		int columnCount(const QModelIndex &parent = QModelIndex()) const;

		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;

		bool isVisible(const unsigned id);

		virtual bool moveRow(int oldRow, int& newRow);

	public slots:
		void addNamedValue(const NamedValue& namedValue);
		void delNamedValue(int index);
		void toggle(const QModelIndex &index);

	private:
		std::vector<bool> viewEvent;
		
	// working around no-template limitation with QObjects
	
	public slots:
		void clear();

	signals:
		void publicRowsInserted();
		void publicRowsRemoved();
		
	protected:
		virtual void publicRowsInsertedCallback();
		virtual void publicRowsRemovedCallback();
	};
	
	/*@}*/
} // namespace Aseba

#endif
