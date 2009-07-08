/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
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
#include <QRegExp>
#include "../compiler/compiler.h"


namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	struct TargetDescription;
	class VariablesViewPlugin;
	
	class TargetVariablesModel: public QAbstractItemModel
	{
		Q_OBJECT
	
	public:
		// variables
		struct Variable
		{
			QString name;
			unsigned pos;
			VariablesDataVector value;
		};
		
	public:
		TargetVariablesModel() { setSupportedDragActions(Qt::CopyAction); }
		virtual ~TargetVariablesModel();
		
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
		QModelIndex parent(const QModelIndex &index) const;
		
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		
		bool setData(const QModelIndex &index, const QVariant &value, int role);
		
		QStringList mimeTypes () const;
		QMimeData * mimeData ( const QModelIndexList & indexes ) const;
		
		const QList<Variable>& getVariables() { return variables; }
		
	public slots:
		void updateVariablesStructure(const Compiler::VariablesMap *variablesMap);
		void setVariablesData(unsigned start, const VariablesDataVector &data);
		bool setVariableValues(const QString& name, const VariablesDataVector& values);
	
	signals:
		//! Emitted on setData, when the user change the data, not when nodes have sent updated variables
		void variableValuesChanged(unsigned index, const VariablesDataVector &values);

	private:
		friend class VariablesViewPlugin;
		friend class LinearCameraViewPlugin;
		// VariablesViewPlugin API 
		
		//! Unsubscribe the plugin from any variables it is listening to
		void unsubscribeViewPlugin(VariablesViewPlugin* plugin);
		//! Subscribe to a variable of interest, return true if variable exists, false otherwise
		bool subscribeToVariableOfInterest(VariablesViewPlugin* plugin, const QString& name);
		//! Unsubscribe to a variable of interest
		void unsubscribeToVariableOfInterest(VariablesViewPlugin* plugin, const QString& name);
		
	private:
		
		QList<Variable> variables;
		
		// VariablesViewPlugin API 
		typedef QMap<VariablesViewPlugin*, QStringList> ViewPlugInToVariablesNameMap;
		ViewPlugInToVariablesNameMap viewPluginsMap;
	};
	
	class TargetFunctionsModel: public QAbstractItemModel
	{
		Q_OBJECT
		
	public:
		struct TreeItem;
		
	public:
		TargetFunctionsModel(const TargetDescription *descriptionRead, QObject *parent = 0);
		~TargetFunctionsModel();
		
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;
		
		QModelIndex parent(const QModelIndex &index) const;
		QModelIndex index(int row, int column, const QModelIndex &parent) const;
		
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		Qt::ItemFlags flags(const QModelIndex & index) const;
		
		QStringList mimeTypes () const;
		QMimeData * mimeData ( const QModelIndexList & indexes ) const;
		
	public slots:
		void recreateTreeFromDescription(bool showHidden);
		
	private:
		TreeItem *getItem(const QModelIndex &index) const;
		QString getToolTip(const TargetDescription::NativeFunction& function) const;
		
		TreeItem* root; //!< root of functions description tree
		const TargetDescription *descriptionRead; //!< description for read access
		QRegExp regExp;
	};
	
	/*@}*/
}; // Aseba

#endif
