/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
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

#include "TargetModels.h"
#include "VariablesViewPlugin.h"
#include <QtDebug>
#include <QtGui>

#include <TargetModels.moc>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	TargetVariablesModel::~TargetVariablesModel()
	{
		for (ViewPlugInToVariablesNameMap::iterator it = viewPluginsMap.begin(); it != viewPluginsMap.end(); ++it)
		{
			it.key()->invalidateVariableModel();
		}
	}
	
	int TargetVariablesModel::rowCount(const QModelIndex &parent) const
	{
		if (parent.isValid())
		{
			if (parent.parent().isValid() || (variables.at(parent.row()).value.size() == 1))
				return 0;
			else
				return variables.at(parent.row()).value.size();
		}
		else
			return variables.size();
	}
	
	int TargetVariablesModel::columnCount(const QModelIndex & parent) const
	{
		return 2;
	}
	
	QModelIndex TargetVariablesModel::index(int row, int column, const QModelIndex &parent) const
	{
		//if (!hasIndex(row, column, parent))
		//	return QModelIndex();
		
		if (parent.isValid())
			return createIndex(row, column, parent.row());
		else
			return createIndex(row, column, -1);
	}
	
	QModelIndex TargetVariablesModel::parent(const QModelIndex &index) const
	{
		if (index.isValid() && (index.internalId() != -1))
			return createIndex(index.internalId(), 0, -1);
		else
			return QModelIndex();
	}
	
	QVariant TargetVariablesModel::data(const QModelIndex &index, int role) const
	{
		if (index.parent().isValid())
		{
			if (role != Qt::DisplayRole)
				return QVariant();
			
			if (index.column() == 0)
				return index.row();
			else
				return variables.at(index.parent().row()).value[index.row()];
		}
		else
		{
			if (index.column() == 0)
			{
				if (role != Qt::DisplayRole)
					return QVariant();
			
				return variables.at(index.row()).name;
			}
			else
			{
				if (role == Qt::DisplayRole)
				{
					if (variables.at(index.row()).value.size() == 1)
						return variables.at(index.row()).value[0];
					else
						return QString("(%0)").arg(variables.at(index.row()).value.size());
				}
				else if (role == Qt::ForegroundRole)
				{
					if (variables.at(index.row()).value.size() == 1)
						return QVariant();
					else
						return QApplication::palette().color(QPalette::Disabled, QPalette::Text);
				}
				else
					return QVariant();
			}
		}
	}
	
	QVariant TargetVariablesModel::headerData(int section, Qt::Orientation orientation, int role) const
	{
		//Q_UNUSED(section)
		Q_UNUSED(orientation)
		Q_UNUSED(role)
		if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
			if (section == 0)
				return tr("variables");
			else
				return tr("values");
		return QVariant();
	}
	
	Qt::ItemFlags TargetVariablesModel::flags(const QModelIndex &index) const
	{
		if (!index.isValid())
			return 0;
		
		if (index.column() == 1)
		{
			if (index.parent().isValid())
				return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
			else if (variables.at(index.row()).value.size() == 1)
				return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
			else
				return 0;
		}
		else
			return Qt::ItemIsEnabled; //| Qt::ItemIsSelectable;
	}
	
	bool TargetVariablesModel::setData(const QModelIndex &index, const QVariant &value, int role)
	{
		if (index.isValid() && role == Qt::EditRole)
		{
			if (index.parent().isValid())
			{
				int variableValue;
				bool ok;
				variableValue = value.toInt(&ok);
				Q_ASSERT(ok);
				
				variables[index.parent().row()].value[index.row()] = variableValue;
				emit variableValueChanged(variables[index.parent().row()].pos + index.row(), variableValue);
				
				return true;
			}
			else if (variables.at(index.row()).value.size() == 1)
			{
				int variableValue;
				bool ok;
				variableValue = value.toInt(&ok);
				Q_ASSERT(ok);
				
				variables[index.row()].value[0] = variableValue;
				emit variableValueChanged(variables[index.row()].pos, variableValue);
				
				return true;
			}
		}
		return false;
	}
	
	void TargetVariablesModel::updateVariablesStructure(const Compiler::VariablesMap *variablesMap)
	{
		// TODO: make this function more intelligent: keep track of unchanged variables
		variables.clear();
		for (Compiler::VariablesMap::const_iterator it = variablesMap->begin(); it != variablesMap->end(); ++it)
		{
			// create new variable
			Variable var;
			var.name = QString::fromUtf8(it->first.c_str());
			var.pos = it->second.first;
			var.value.resize(it->second.second);
			
			// find its right place in the array
			int i;
			for (i = 0; i < variables.size(); ++i)
			{
				if (var.pos < variables[i].pos)
					break;
			}
			variables.insert(i, var);
		}
		
		reset();
	}
	
	void TargetVariablesModel::setVariablesData(unsigned start, const VariablesDataVector &data)
	{
		size_t dataLength = data.size();
		for (int i = 0; i < variables.size(); ++i)
		{
			Variable &var = variables[i];
			int varLen = (int)var.value.size();
			int varStart = (int)start - (int)var.pos;
			int copyLen = (int)dataLength;
			int copyStart = 0;
			// crop data before us
			if (varStart < 0)
			{
				copyLen += varStart;
				copyStart -= varStart;
				varStart = 0;
			}
			// if nothing to copy, continue
			if (copyLen <= 0)
				continue;
			// crop data after us
			if (varStart + copyLen > varLen)
			{
				copyLen = varLen - varStart;
			}
			// if nothing to copy, continue
			if (copyLen <= 0)
				continue;
			
			// copy
			copy(data.begin() + copyStart, data.begin() + copyStart + copyLen, var.value.begin() + varStart);
			// and notify gui
			QModelIndex parentIndex = index(i, 0);
			emit dataChanged(index(varStart, 0, parentIndex), index(varStart + copyLen, 0, parentIndex));
			
			// and notify view plugins
			for (ViewPlugInToVariablesNameMap::iterator it = viewPluginsMap.begin(); it != viewPluginsMap.end(); ++it)
			{
				QStringList &list = it.value();
				for (int v = 0; v < list.size(); v++)
				{
					if (list[v] == var.name)
						it.key()->variableValueUpdated(var.name, var.value);
				}
			}
		}
	}
	
	void TargetVariablesModel::unsubscribeViewPlugin(VariablesViewPlugin* plugin)
	{
		viewPluginsMap.remove(plugin);
	}
	
	bool TargetVariablesModel::subscribeToVariableOfInterest(VariablesViewPlugin* plugin, const QString& name)
	{
		QStringList &list = viewPluginsMap[plugin];
		list.push_back(name);
		for (int i = 0; i < variables.size(); i++)
			if (variables[i].name == name)
				return true;
		return false;
	}
	
	void TargetVariablesModel::unsubscribeToVariableOfInterest(VariablesViewPlugin* plugin, const QString& name)
	{
		QStringList &list = viewPluginsMap[plugin];
		list.removeAll(name);
	}
	
	
	
	struct TargetFunctionsModel::TreeItem
	{
		TreeItem* parent;
		QList<TreeItem*> children;
		QString name;
		QString toolTip;
		
		TreeItem() :
			parent(0),
			name("root")
		{ }
		
		TreeItem(TreeItem* parent, const QString& name) :
			parent(parent),
			name(name)
		{ }
		
		TreeItem(TreeItem* parent, const QString& name, const QString& toolTip) :
			parent(parent),
			name(name),
			toolTip(toolTip)
		{ }
		
		~TreeItem()
		{
			for (int i = 0; i < children.size(); i++)
				delete children[i];
		}
		
		TreeItem *getEntry(const QString& name)
		{
			for (int i = 0; i < children.size(); i++)
				if (children[i]->name == name)
					return children[i];
			
			children.push_back(new TreeItem(this, name));
			return children.last();
		}
	};
	
	
	
	TargetFunctionsModel::TargetFunctionsModel(const TargetDescription *descriptionRead, QObject *parent) :
		QAbstractTableModel(parent),
		root(0),
		descriptionRead(descriptionRead)
	{
		Q_ASSERT(descriptionRead);
		createTreeFromDescription();
	}
	
	TargetFunctionsModel::~TargetFunctionsModel()
	{
		delete root;
	}
	
	TargetFunctionsModel::TreeItem *TargetFunctionsModel::getItem(const QModelIndex &index) const
	{
		if (index.isValid())
		{
			TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
			if (item)
				return item;
		}
		return root;
	}
	
	QString TargetFunctionsModel::getToolTip(const TargetDescription::NativeFunction& function) const
	{
		// tooltip, display detailed information with pretty print of template parameters
		QString text;
		text += QString("<b>%0</b>(").arg(QString::fromUtf8(function.name.c_str()));
		for (size_t i = 0; i < function.parameters.size(); i++)
		{
			text += QString("%0").arg(QString::fromUtf8(function.parameters[i].name.c_str()));
			if (function.parameters[i].size > 0)
				text += QString("<%0>").arg(function.parameters[i].size);
			else if (function.parameters[i].size < 0)
				text += QString("<T%0>").arg(-function.parameters[i].size);
			
			if (i + 1 < function.parameters.size())
				text += QString(", ");
		}
		
		text += QString(")<br/>") + QString::fromUtf8(function.description.c_str());
		
		return text;
	}
	
	int TargetFunctionsModel::rowCount(const QModelIndex & parent) const
	{
		return getItem(parent)->children.count();
	}
	
	int TargetFunctionsModel::columnCount(const QModelIndex & /* parent */) const
	{
		return 1;
	}
	
	void TargetFunctionsModel::createTreeFromDescription()
	{
		if (root)
			delete root;
		root = new TreeItem;
		for (size_t i = 0; i < descriptionRead->nativeFunctions.size(); i++)
		{
			// get the name, split it, and managed hidden
			QString name = QString::fromUtf8(descriptionRead->nativeFunctions[i].name.c_str());
			QStringList splittedName = name.split(".", QString::SkipEmptyParts);
			
			// ignore functions with no name at all
			if (splittedName.isEmpty())
				continue;
			
			// get first, and iterate
			TreeItem* entry = root->getEntry(splittedName[0]);
			for (int j = 1; j < splittedName.size() - 1; ++j)
			{
				if (splittedName[j].isEmpty())
					entry = entry->getEntry(tr("hidden"));
				else
					entry = entry->getEntry(splittedName[j]);
			}
			
			// for last entry
			entry->children.push_back(new TreeItem(entry, name, getToolTip(descriptionRead->nativeFunctions[i])));
		}
	}
	
	QModelIndex TargetFunctionsModel::parent(const QModelIndex &index) const
	{
		if (!index.isValid())
			return QModelIndex();
	
		TreeItem *childItem = getItem(index);
		TreeItem *parentItem = childItem->parent;
	
		if (parentItem == root)
			return QModelIndex();
	
		return createIndex(parentItem->children.size(), 0, parentItem);
	}
	
	QModelIndex TargetFunctionsModel::index(int row, int column, const QModelIndex &parent) const
	{
		TreeItem *parentItem = getItem(parent);
		TreeItem *childItem = parentItem->children.value(row);
		
		if (childItem)
			return createIndex(row, column, childItem);
		else
			return QModelIndex();
	}
	
	QVariant TargetFunctionsModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() ||
			(role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::WhatsThisRole))
			return QVariant();
		
		if (role == Qt::DisplayRole)
		{
			return getItem(index)->name;
		}
		else
		{
			return getItem(index)->toolTip;
		}
	}
	
	QVariant TargetFunctionsModel::headerData(int section, Qt::Orientation orientation, int role) const
	{
		Q_UNUSED(section)
		Q_UNUSED(orientation)
		Q_UNUSED(role)
		return QVariant();
	}
	
	Qt::ItemFlags TargetFunctionsModel::flags(const QModelIndex & index) const
	{
		return Qt::ItemIsEnabled;
	}
	
	void TargetFunctionsModel::dataChangedExternally(const QModelIndex &index)
	{
		emit dataChanged(index, index);
	}
	
	/*@}*/
}; // Aseba
