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

#ifndef TARGET_MODELS_H
#define TARGET_MODELS_H

#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QStringListModel>
#include <QVector>
#include <QList>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include "../../compiler/compiler.h"


namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	struct TargetDescription;
	class TargetVariablesModel;
	
	//! Classes that want to listen to variable changes should inherit from this class
	class VariableListener
	{
	protected:
		TargetVariablesModel *variablesModel;
		
	public:
		VariableListener(TargetVariablesModel* variablesModel);
		virtual ~VariableListener();
		
		bool subscribeToVariableOfInterest(const QString& name);
		void unsubscribeToVariableOfInterest(const QString& name);
		void unsubscribeToVariablesOfInterest();
		void invalidateVariableModel();
		
	protected:
		friend class TargetVariablesModel;
		//! New values are available for a variable this plugin is interested in
		virtual void variableValueUpdated(const QString& name, const VariablesDataVector& values) = 0;
	};
	
	class TargetVariablesModel: public QAbstractItemModel
	{
		Q_OBJECT
		
		// FIXME: uses map for fast variable access
	
	public:
		// variables
		struct Variable
		{
			QString name;
			unsigned pos;
			VariablesDataVector value;
		};
		
	public:
		TargetVariablesModel(QObject *parent = 0);
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
		
		const QList<Variable>& getVariables() const { return variables; }
		unsigned getVariablePos(const QString& name) const;
		unsigned getVariableSize(const QString& name) const;
		VariablesDataVector getVariableValue(const QString& name) const;
		
	public slots:
		void updateVariablesStructure(const VariablesMap *variablesMap);
		void setVariablesData(unsigned start, const VariablesDataVector &data);
		bool setVariableValues(const QString& name, const VariablesDataVector& values);
	
	signals:
		//! Emitted on setData, when the user change the data, not when nodes have sent updated variables
		void variableValuesChanged(unsigned index, const VariablesDataVector &values);

	private:
		friend class VariableListener;
		// VariableListener API 
		
		//! Unsubscribe the plugin from any variables it is listening to
		void unsubscribeViewPlugin(VariableListener* plugin);
		//! Subscribe to a variable of interest, return true if variable exists, false otherwise
		bool subscribeToVariableOfInterest(VariableListener* plugin, const QString& name);
		//! Unsubscribe to a variable of interest
		void unsubscribeToVariableOfInterest(VariableListener* plugin, const QString& name);
		//! Unsubscribe to all variables of interest for a given plugin
		void unsubscribeToVariablesOfInterest(VariableListener* plugin);
		
	private:
		QList<Variable> variables;
		
		// VariablesViewPlugin API 
		typedef QMap<VariableListener*, QStringList> VariableListenersNameMap;
		VariableListenersNameMap variableListenersMap;
	};
	
	class TargetFunctionsModel: public QAbstractItemModel
	{
		Q_OBJECT
		
	public:
		struct TreeItem;
		
	public:
		TargetFunctionsModel(const TargetDescription *descriptionRead, bool showHidden, QObject *parent = 0);
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
		friend class AeslEditor;
		friend class StudioAeslEditor;
		TreeItem *getItem(const QModelIndex &index) const;
		QString getToolTip(const TargetDescription::NativeFunction& function) const;
		
		TreeItem* root; //!< root of functions description tree
		const TargetDescription *descriptionRead; //!< description for read access
		QRegExp regExp;
	};
	
	class TargetSubroutinesModel: public QStringListModel 
	{
	public:
		TargetSubroutinesModel(QObject * parent = 0);
	
		void updateSubroutineTable(const Compiler::SubroutineTable& subroutineTable);
	};
	
	/*@}*/
} // namespace Aseba

#endif
