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

#include "TargetModels.h"
#include <QtDebug>
#include <QtGui>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	TargetVariablesModel::TargetVariablesModel(const TargetDescription *descriptionRead, TargetDescription *descriptionWrite, QObject *parent) :
		QAbstractTableModel(parent),
		descriptionRead(descriptionRead),
		descriptionWrite(descriptionWrite)
	{
		Q_ASSERT(descriptionRead);
	}
	
	int TargetVariablesModel::rowCount(const QModelIndex & /* parent */) const
	{
		return descriptionRead->namedVariables.size();
	}
	
	int TargetVariablesModel::columnCount(const QModelIndex & /* parent */) const
	{
		return 2;
	}
	
	QVariant TargetVariablesModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() || role != Qt::DisplayRole)
			return QVariant();
		if (index.column() == 0)
			return QString::fromUtf8(descriptionRead->namedVariables[index.row()].name.c_str());
		else
			return descriptionRead->namedVariables[index.row()].size;
	}
	
	QVariant TargetVariablesModel::headerData(int section, Qt::Orientation orientation, int role) const
	{
		Q_UNUSED(section)
		Q_UNUSED(orientation)
		Q_UNUSED(role)
		return QVariant();
	}
	
	Qt::ItemFlags TargetVariablesModel::flags(const QModelIndex & /*index*/) const
	{
		if (descriptionWrite)
			return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
		else
			return Qt::ItemIsEnabled;
	}
	
	bool TargetVariablesModel::setData(const QModelIndex &index, const QVariant &value, int role)
	{
		Q_ASSERT(descriptionWrite);
		if (index.isValid() && role == Qt::EditRole)
		{
			// check for valid identifiers
			if (index.column() == 0)
			{
				QString name = value.toString();
				bool ok = !name.isEmpty();
				if (ok && (name[0].isLetter() || name[0] == '_'))
				{
					for (int i = 1; i < name.size(); i++)
						if (!(name[i].isLetterOrNumber() || name[i] == '_'))
							return false;
					
					descriptionWrite->namedVariables[index.row()].name = name.toUtf8().data();
					emit dataChanged(index, index);
					return true;
				}
			}
			else
			{
				unsigned size;
				bool ok;
				size = value.toUInt(&ok);
				if (ok)
				{
					descriptionWrite->namedVariables[index.row()].size = size;
					emit dataChanged(index, index);
					return true;
				}
			}
			
			return false;
		}
		return false;
	}
	
	void TargetVariablesModel::addVariable()
	{
		Q_ASSERT(descriptionWrite);
		
		unsigned position = descriptionWrite->namedVariables.size();
		beginInsertRows(QModelIndex(), position, position);
		
		TargetDescription::NamedVariable newVariable("unnamed", 1);
		descriptionWrite->namedVariables.insert(descriptionWrite->namedVariables.begin() + position, 1, newVariable);
	
		endInsertRows();
	}
	
	void TargetVariablesModel::delVariable(int index)
	{
		beginRemoveRows(QModelIndex(), index, index);
		
		descriptionWrite->namedVariables.erase(descriptionWrite->namedVariables.begin() + index);
		
		endRemoveRows();
	}
	
	
	
	TargetFunctionsModel::TargetFunctionsModel(const TargetDescription *descriptionRead, TargetDescription *descriptionWrite, QObject *parent) :
		QAbstractTableModel(parent),
		descriptionRead(descriptionRead),
		descriptionWrite(descriptionWrite)
	{
		Q_ASSERT(descriptionRead);
	}
	
	int TargetFunctionsModel::rowCount(const QModelIndex & /* parent */) const
	{
		return descriptionRead->nativeFunctions.size();
	}
	
	int TargetFunctionsModel::columnCount(const QModelIndex & /* parent */) const
	{
		return 1;
	}
	
	QVariant TargetFunctionsModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() ||
			(role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::WhatsThisRole))
			return QVariant();
		
		if (role == Qt::DisplayRole)
		{
			return QString::fromUtf8(descriptionRead->nativeFunctions[index.row()].name.c_str());
		}
		else
		{
			// tooltip, display detailed information with pretty print of template parameters
			const TargetDescription::NativeFunction& function = descriptionRead->nativeFunctions[index.row()];
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
			
			QMap<QString, QString> dict;
			dict["&"] = tr("and");
			dict["MAC"] = tr("Multiply and Accumulate");
			dict["RS"] = tr("Right Shift");
			
			QStringList phrases = QString::fromUtf8(function.description.c_str()).split(' ');
			
			for (int i = 0; i < phrases.size(); ++i)
				if (dict.contains(phrases[i]))
					phrases[i] = dict[phrases[i]];
			
			text += QString(")<br/>") + phrases.join(" ");
			
			return text;
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
		if (descriptionWrite)
		{
			if (index.column() == 0)
				return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
			else
				return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		}
		else
			return Qt::ItemIsEnabled;
	}
	
	bool TargetFunctionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
	{
		Q_ASSERT(descriptionWrite);
		if (index.isValid() && role == Qt::EditRole)
		{
			// check for valid identifiers
			if (index.column() == 0)
			{
				QString name = value.toString();
				bool ok = !name.isEmpty();
				if (ok && (name[0].isLetter() || name[0] == '_'))
				{
					for (int i = 1; i < name.size(); i++)
						if (!(name[i].isLetterOrNumber() || name[i] == '_'))
							return false;
					
					descriptionWrite->nativeFunctions[index.row()].name = name.toUtf8().data();
					emit dataChanged(index, index);
					return true;
				}
			}
			else
			{
				// do nothing there, see setParameterData(...)
			}
			
			return false;
		}
		return false;
	}
	
	void TargetFunctionsModel::dataChangedExternally(const QModelIndex &index)
	{
		emit dataChanged(index, index);
	}
	
	void TargetFunctionsModel::addFunction()
	{
		Q_ASSERT(descriptionWrite);
		
		unsigned position = descriptionWrite->nativeFunctions.size();
		beginInsertRows(QModelIndex(), position, position);
		
		TargetDescription::NativeFunction newFunction;
		newFunction.name = "unnamed";
		descriptionWrite->nativeFunctions.insert(descriptionWrite->nativeFunctions.begin() + position, 1, newFunction);
	
		endInsertRows();
	}
	
	void TargetFunctionsModel::delFunction(int index)
	{
		beginRemoveRows(QModelIndex(), index, index);
		
		descriptionWrite->nativeFunctions.erase(descriptionWrite->nativeFunctions.begin() + index);
		
		endRemoveRows();
	}
	
	
	TargetMemoryModel::TargetMemoryModel(QObject *parent) :
		QAbstractTableModel(parent)
	{
		
	}
	
	int TargetMemoryModel::rowCount(const QModelIndex & /* parent */) const
	{
		return variablesData.size();
	}
	
	int TargetMemoryModel::columnCount(const QModelIndex & /* parent */) const
	{
		return 2;
	}
	
	QVariant TargetMemoryModel::data(const QModelIndex &index, int role) const
	{
		Q_ASSERT(variablesNames.size() == variablesData.size());
		if (!index.isValid() || role != Qt::DisplayRole)
			return QVariant();
		switch (index.column())
		{
			case 0: return variablesNames[index.row()];
			case 1: return variablesData[index.row()];
			default: return QVariant();
		}
	}
	
	QVariant TargetMemoryModel::headerData(int section, Qt::Orientation orientation, int role) const
	{
		Q_UNUSED(section)
		Q_UNUSED(orientation)
		Q_UNUSED(role)
		return QVariant();
	}
	
	Qt::ItemFlags TargetMemoryModel::flags(const QModelIndex & index) const
	{
		switch (index.column())
		{
			case 0: return 0;
			case 1: return Qt::ItemIsEnabled;
			default: return 0;
		}
	}
	
	void TargetMemoryModel::setVariablesNames(const VariablesNamesVector &names)
	{
		variablesNames.resize(names.size());
		
		QString name;
		unsigned counter = 0;
		for (size_t i = 0; i < names.size(); i++)
		{
			QString newName = QString::fromUtf8(names[i].c_str());
			bool singleVariable = false;
			if (newName != name)
			{
				name = newName;
				if ((i+1 < names.size()) && (QString::fromUtf8(names[i+1].c_str()) != newName))
					singleVariable = true;
				counter = 0;
			}
			else
				counter++;
			if (!name.isEmpty())
			{
				if (singleVariable)
					variablesNames[i] = name;
				else
					variablesNames[i] = QString("%0[%1]").arg(name).arg(counter);
			}
		}
		
		variablesData.resize(variablesNames.size());
		reset();
	}
	
	void TargetMemoryModel::setVariablesData(unsigned start, const VariablesDataVector &data)
	{
		Q_ASSERT(start + data.size() <= variablesData.size());
		
		if (data.size() > 0)
		{
			copy(data.begin(), data.end(), variablesData.begin() + start);
			emit dataChanged(index(start, 1), index(start + data.size() - 1, 1));
		}
	}
	
	/*@}*/
}; // Aseba
