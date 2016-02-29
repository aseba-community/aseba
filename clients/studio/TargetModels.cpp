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

#include "TargetModels.h"
#include <QtDebug>
#include <QtGui>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	VariableListener::VariableListener(TargetVariablesModel* variablesModel) :
		variablesModel(variablesModel)
	{
		
	}
	
	VariableListener::~VariableListener()
	{
		if (variablesModel)
			variablesModel->unsubscribeViewPlugin(this);
	}
	
	bool VariableListener::subscribeToVariableOfInterest(const QString& name)
	{
		return variablesModel->subscribeToVariableOfInterest(this, name);
	}
	
	void VariableListener::unsubscribeToVariableOfInterest(const QString& name)
	{
		variablesModel->unsubscribeToVariableOfInterest(this, name);
	}
	
	void VariableListener::unsubscribeToVariablesOfInterest()
	{
		variablesModel->unsubscribeToVariablesOfInterest(this);
	}
	
	void VariableListener::invalidateVariableModel()
	{
		variablesModel = 0;
	}
	
	
	TargetVariablesModel::~TargetVariablesModel()
	{
		for (VariableListenersNameMap::iterator it = variableListenersMap.begin(); it != variableListenersMap.end(); ++it)
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
	
	TargetVariablesModel::TargetVariablesModel(QObject *parent) :
		QAbstractItemModel(parent)
	{
		setSupportedDragActions(Qt::CopyAction);
	}
	
	int TargetVariablesModel::columnCount(const QModelIndex & parent) const
	{
		return 2;
	}
	
	QModelIndex TargetVariablesModel::index(int row, int column, const QModelIndex &parent) const
	{
		if (parent.isValid())
			return createIndex(row, column, parent.row());
		else
		{
			// top-level indices shall not point outside the variable array
			if (row < 0 || row >= variables.length())
				return QModelIndex();
			else
				return createIndex(row, column, -1);
		}
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
			QString name = variables.at(index.row()).name;
			// hidden variable
			if ((name.left(1) == "_") || name.contains(QString("._")))
			{
				if (role == Qt::ForegroundRole)
					return QApplication::palette().color(QPalette::Disabled, QPalette::Text);
				else if (role == Qt::FontRole)
				{
					QFont font;
					font.setItalic(true);
					return font;
				}
			}
			if (index.column() == 0)
			{
				if (role == Qt::DisplayRole)
					return name;
				return QVariant();
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
		{
			if (section == 0)
				return tr("names");
			else
				return tr("values");
		}
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
		{
			if (index.parent().isValid())
				return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
			else
				return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
		}
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
				emit variableValuesChanged(variables[index.parent().row()].pos + index.row(), VariablesDataVector(1, variableValue));
				
				return true;
			}
			else if (variables.at(index.row()).value.size() == 1)
			{
				int variableValue;
				bool ok;
				variableValue = value.toInt(&ok);
				Q_ASSERT(ok);
				
				variables[index.row()].value[0] = variableValue;
				emit variableValuesChanged(variables[index.row()].pos, VariablesDataVector(1, variableValue));
				
				return true;
			}
		}
		return false;
	}
	
	QStringList TargetVariablesModel::mimeTypes () const
	{
		QStringList types;
		types << "text/plain";
		return types;
	}
	
	QMimeData * TargetVariablesModel::mimeData ( const QModelIndexList & indexes ) const
	{
		QString texts;
		foreach (QModelIndex index, indexes)
		{
			if (index.isValid())
			{
				const QString text = data(index, Qt::DisplayRole).toString();
				if (index.parent().isValid())
				{
					const QString varName = data(index.parent(), Qt::DisplayRole).toString();
					texts += varName + "[" + text + "]";
				}
				else
					texts += text;
			}
		}
		
		QMimeData *mimeData = new QMimeData();
		mimeData->setText(texts);
		return mimeData;
	}
	
	unsigned TargetVariablesModel::getVariablePos(const QString& name) const
	{
		for (int i = 0; i < variables.size(); ++i)
		{
			const Variable& variable(variables[i]);
			if (variable.name == name)
				return variable.pos;
		}
		return 0;
	}
	
	unsigned TargetVariablesModel::getVariableSize(const QString& name) const
	{
		for (int i = 0; i < variables.size(); ++i)
		{
			const Variable& variable(variables[i]);
			if (variable.name == name)
				return variable.value.size();
		}
		return 0;
	}
	
	VariablesDataVector TargetVariablesModel::getVariableValue(const QString& name) const
	{
		for (int i = 0; i < variables.size(); ++i)
		{
			const Variable& variable(variables[i]);
			if (variable.name == name)
				return variable.value;
		}
		return VariablesDataVector();
	}
	
	void TargetVariablesModel::updateVariablesStructure(const VariablesMap *variablesMap)
	{
		// Build a new list of variables
		QList<Variable> newVariables;
		for (VariablesMap::const_iterator it = variablesMap->begin(); it != variablesMap->end(); ++it)
		{
			// create new variable
			Variable var;
			var.name = QString::fromStdWString(it->first);
			var.pos = it->second.first;
			var.value.resize(it->second.second);
			
			// find its right place in the array
			int i;
			for (i = 0; i < newVariables.size(); ++i)
			{
				if (var.pos < newVariables[i].pos)
					break;
			}
			newVariables.insert(i, var);
		}
		
		// compute the difference
		int i(0);
		int count(std::min(variables.length(), newVariables.length()));
		while (
			i < count && 
			variables[i].name == newVariables[i].name && 
			variables[i].pos == newVariables[i].pos &&
			variables[i].value.size() == newVariables[i].value.size()
		)
			++i;
		
		// update starting from the first change point
		//qDebug() << "change from " << i << " to " << variables.length();
		if (i != variables.length())
		{
			beginRemoveRows(QModelIndex(), i, variables.length()-1);
			int removeCount(variables.length() - i);
			for (int j = 0; j < removeCount; ++j)
				variables.removeLast();
			endRemoveRows();
		}
		
		//qDebug() << "size: " << variables.length();
		
		if (i != newVariables.length())
		{
			beginInsertRows(QModelIndex(), i, newVariables.length()-1);
			for (int j = i; j < newVariables.length(); ++j)
				variables.append(newVariables[j]);
			endInsertRows();
		}

		/*variables.clear();
		for (Compiler::VariablesMap::const_iterator it = variablesMap->begin(); it != variablesMap->end(); ++it)
		{
			// create new variable
			Variable var;
			var.name = QString::fromStdWString(it->first);
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
		
		reset();*/
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
			
			// notify gui
			QModelIndex parentIndex = index(i, 0);
			emit dataChanged(index(varStart, 0, parentIndex), index(varStart + copyLen, 0, parentIndex));
			
			// and notify view plugins
			for (VariableListenersNameMap::iterator it = variableListenersMap.begin(); it != variableListenersMap.end(); ++it)
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
	
	bool TargetVariablesModel::setVariableValues(const QString& name, const VariablesDataVector& values)
	{
		for (int i = 0; i < variables.size(); ++i)
		{
			Variable& variable(variables[i]);
			if (variable.name == name)
			{
// 				setVariablesData(variable.pos, values);
				emit variableValuesChanged(variable.pos, values);
				return true;
			}
		}
		return false;
	}
	
	void TargetVariablesModel::unsubscribeViewPlugin(VariableListener* listener)
	{
		variableListenersMap.remove(listener);
	}
	
	bool TargetVariablesModel::subscribeToVariableOfInterest(VariableListener* listener, const QString& name)
	{
		QStringList &list = variableListenersMap[listener];
		list.push_back(name);
		for (int i = 0; i < variables.size(); i++)
			if (variables[i].name == name)
				return true;
		return false;
	}
	
	void TargetVariablesModel::unsubscribeToVariableOfInterest(VariableListener* listener, const QString& name)
	{
		QStringList &list = variableListenersMap[listener];
		list.removeAll(name);
	}
	
	void TargetVariablesModel::unsubscribeToVariablesOfInterest(VariableListener* plugin)
	{
		if (variableListenersMap.contains(plugin))
			variableListenersMap.remove(plugin);
	}
	
	struct TargetFunctionsModel::TreeItem
	{
		TreeItem* parent;
		QList<TreeItem*> children;
		QString name;
		QString toolTip;
		bool enabled;
		bool draggable;
		
		TreeItem() :
			parent(0),
			name("root"),
			enabled(true),
			draggable(false)
		{ }
		
		TreeItem(TreeItem* parent, const QString& name, bool enabled, bool draggable) :
			parent(parent),
			name(name),
			enabled(enabled),
			draggable(draggable)
		{ }
		
		TreeItem(TreeItem* parent, const QString& name, const QString& toolTip, bool enabled, bool draggable) :
			parent(parent),
			name(name),
			toolTip(toolTip),
			enabled(enabled),
			draggable(draggable)
		{ }
		
		~TreeItem()
		{
			for (int i = 0; i < children.size(); i++)
				delete children[i];
		}
		
		TreeItem *getEntry(const QString& name, bool enabled = true)
		{
			for (int i = 0; i < children.size(); i++)
				if (children[i]->name == name)
					return children[i];
			
			children.push_back(new TreeItem(this, name, enabled, draggable));
			return children.last();
		}
	};
	
	
	
	TargetFunctionsModel::TargetFunctionsModel(const TargetDescription *descriptionRead, bool showHidden, QObject *parent) :
		QAbstractItemModel(parent),
		root(0),
		descriptionRead(descriptionRead),
		regExp("\\b")
	{
		Q_ASSERT(descriptionRead);
		setSupportedDragActions(Qt::CopyAction);
		recreateTreeFromDescription(showHidden);
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
		QSet<QString> variablesNames;
		
		text += QString("<b>%1</b>(").arg(QString::fromStdWString(function.name));
		for (size_t i = 0; i < function.parameters.size(); i++)
		{
			QString variableName(QString::fromStdWString(function.parameters[i].name));
			variablesNames.insert(variableName);
			text += variableName;
			if (function.parameters[i].size > 1)
				text += QString("[%1]").arg(function.parameters[i].size);
			else if (function.parameters[i].size < 0)
			{
				text += QString("[&lt;T%1&gt;]").arg(-function.parameters[i].size);
			}
			
			if (i + 1 < function.parameters.size())
				text += QString(", ");
		}
		
		QString description = QString::fromStdWString(function.description);
		QStringList descriptionWords = description.split(regExp);
		for (int i = 0; i < descriptionWords.size(); ++i)
			if (variablesNames.contains(descriptionWords.at(i)))
				descriptionWords[i] = QString("<tt>%1</tt>").arg(descriptionWords[i]);
		
		text += QString(")<br/>") + descriptionWords.join(" ");
		
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
	
	void TargetFunctionsModel::recreateTreeFromDescription(bool showHidden)
	{
		beginResetModel();

		if (root)
			delete root;
		root = new TreeItem;
		
		if (showHidden)
			root->getEntry(tr("hidden"), false);
		
		for (size_t i = 0; i < descriptionRead->nativeFunctions.size(); i++)
		{
			// get the name, split it, and managed hidden
			QString name = QString::fromStdWString(descriptionRead->nativeFunctions[i].name);
			QStringList splittedName = name.split(".", QString::SkipEmptyParts);
			
			// ignore functions with no name at all
			if (splittedName.isEmpty())
				continue;
			
			// get first, check whether hidden, and then iterate
			TreeItem* entry = root;
			Q_ASSERT(!splittedName[0].isEmpty());
			if (name.at(0) == '_' || name.contains(QString("._")))
			{
				if (!showHidden)
					continue;
				entry = entry->getEntry(tr("hidden"), false);
			}
			
			for (int j = 0; j < splittedName.size() - 1; ++j)
				entry = entry->getEntry(splittedName[j], entry->enabled);
			
			// for last entry
			entry->children.push_back(new TreeItem(entry, name, getToolTip(descriptionRead->nativeFunctions[i]), entry->enabled, true));
		}
		
		endResetModel();
	}
	
	QModelIndex TargetFunctionsModel::parent(const QModelIndex &index) const
	{
		if (!index.isValid())
			return QModelIndex();
	
		TreeItem *childItem = getItem(index);
		TreeItem *parentItem = childItem->parent;
	
		if (parentItem == root)
			return QModelIndex();
		
		if (parentItem->parent)
			return createIndex(parentItem->parent->children.indexOf(const_cast<TreeItem*>(parentItem)), 0, parentItem);
		else
			return createIndex(0, 0, parentItem);
	}
	
	QModelIndex TargetFunctionsModel::index(int row, int column, const QModelIndex &parent) const
	{
		TreeItem *parentItem = getItem(parent);
		TreeItem *childItem = parentItem->children.value(row);
		Q_ASSERT(childItem);
		
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
		TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
		if (item)
		{
			QFlags<Qt::ItemFlag> flags;
			flags |= item->enabled ? Qt::ItemIsEnabled : QFlags<Qt::ItemFlag>();
			flags |= item->draggable ? Qt::ItemIsDragEnabled | Qt::ItemIsSelectable : QFlags<Qt::ItemFlag>();
			return flags;
		}
		else
			return Qt::ItemIsEnabled;
	}
	
	QStringList TargetFunctionsModel::mimeTypes () const
	{
		QStringList types;
		types << "text/plain";
		return types;
	}
	
	QMimeData * TargetFunctionsModel::mimeData ( const QModelIndexList & indexes ) const
	{
		QString texts;
		foreach (QModelIndex index, indexes)
		{
			if (index.isValid())
			{
				QString text = data(index, Qt::DisplayRole).toString();
				texts += text;
			}
		}
		
		QMimeData *mimeData = new QMimeData();
		mimeData->setText(texts);
		return mimeData;
	}
	
	
	
	TargetSubroutinesModel::TargetSubroutinesModel(QObject * parent):
		QStringListModel(parent)
	{}
	
	void TargetSubroutinesModel::updateSubroutineTable(const Compiler::SubroutineTable& subroutineTable)
	{
		QStringList subroutineNames;
		for (Compiler::SubroutineTable::const_iterator it = subroutineTable.begin(); it != subroutineTable.end(); ++it)
			subroutineNames.push_back(QString::fromStdWString(it->name));
		setStringList(subroutineNames);
	}
	
	/*@}*/
} // namespace Aseba
