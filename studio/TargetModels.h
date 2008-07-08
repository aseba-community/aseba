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

#ifndef TARGET_MODELS_H
#define TARGET_MODELS_H

#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QVector>
#include <QList>
#include <QString>
#include "../compiler/compiler.h"


namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class TargetDescription;
	
	class TargetVariablesModel: public QAbstractItemModel
	{
		Q_OBJECT
		
	public:
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
		QModelIndex parent(const QModelIndex &index) const;
		
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		
		bool setData(const QModelIndex &index, const QVariant &value, int role);
	
	public slots:
		void updateVariablesStructure(const Compiler::VariablesMap *variablesMap);
		void setVariablesData(unsigned start, const VariablesDataVector &data);
	
	signals:
		void variableValueChanged(unsigned index, int value);
	
	private:
		struct Variable
		{
			QString name;
			unsigned pos;
			VariablesDataVector value;
		};
		QList<Variable> variables;
	};
	
	class TargetFunctionsModel: public QAbstractTableModel
	{
		Q_OBJECT
	
	public:
		TargetFunctionsModel(const TargetDescription *descriptionRead, TargetDescription *descriptionWrite, QObject *parent = 0);
		
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		
		bool setData(const QModelIndex &index, const QVariant &value, int role);
		//! Data have been by directly accessing the struct from the model index, so we have to emit dataChanged() from here anyway
		void dataChangedExternally(const QModelIndex &index);
		
	public slots:
		void addFunction();
		void delFunction(int index);
		
	private:
		const TargetDescription *descriptionRead; //!< description for read access
		TargetDescription *descriptionWrite; //!< description for write access
	};
	
	/*class TargetMemoryModel: public QAbstractTableModel
	{
		Q_OBJECT
	
	public:
		TargetMemoryModel(QObject *parent = 0);
		
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		
		bool setData(const QModelIndex &index, const QVariant &value, int role);
	
	public slots:
		void setVariablesNames(const VariablesNamesVector &names);
		void setVariablesData(unsigned start, const VariablesDataVector &data);
	
	signals:
		void variableValueChanged(unsigned index, int value);
		
	private:
		friend class MainWindow;
		QVector<QString> variablesNames;
		VariablesDataVector variablesData;
	};*/
	
	/*@}*/
}; // Aseba

#endif
