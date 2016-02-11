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

#include "NamedValuesVectorModel.h"
#include <QtDebug>
#include <QtGui>
#include <QMessageBox>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	NamedValuesVectorModel::NamedValuesVectorModel(NamedValuesVector* namedValues, const QString &tooltipText, QObject *parent) :
		QAbstractTableModel(parent),
		namedValues(namedValues),
		wasModified(false),
		tooltipText(tooltipText),
		editable(false)
	{
		Q_ASSERT(namedValues);
	}
	
	NamedValuesVectorModel::NamedValuesVectorModel(NamedValuesVector* namedValues, QObject *parent) :
		QAbstractTableModel(parent),
		namedValues(namedValues),
		wasModified(false),
		editable(false)
	{
		Q_ASSERT(namedValues);
	}
	
	int NamedValuesVectorModel::rowCount(const QModelIndex & parent) const
	{
		Q_UNUSED(parent)
		return namedValues->size();
	}
	
	int NamedValuesVectorModel::columnCount(const QModelIndex & parent) const
	{
		Q_UNUSED(parent)
		return 2;
	}
	
	QVariant NamedValuesVectorModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid())
			return QVariant();
		
		if (role == Qt::DisplayRole || role == Qt::EditRole)
		{
			if (index.column() == 0)
				return QString::fromStdWString(namedValues->at(index.row()).name);
			else
				return namedValues->at(index.row()).value;
		}
		else if (role == Qt::ToolTipRole && !tooltipText.isEmpty())
		{
			return tooltipText.arg(index.row());
		}
		else
			return QVariant();
	}
	
	QVariant NamedValuesVectorModel::headerData(int section, Qt::Orientation orientation, int role) const
	{
		Q_UNUSED(section)
		Q_UNUSED(orientation)
		Q_UNUSED(role)
		return QVariant();
	}
	
	Qt::ItemFlags NamedValuesVectorModel::flags(const QModelIndex & index) const
	{
		if (!index.isValid())
			return Qt::ItemIsDropEnabled;

		Qt::ItemFlags commonFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
		if (index.column() == 0)
		{
			if (editable)
				return commonFlags | Qt::ItemIsEditable;
			else
				return commonFlags;
		}
		else
			return commonFlags | Qt::ItemIsEditable;
	}

	QStringList NamedValuesVectorModel::mimeTypes () const
	{
		QStringList types;
		types << "text/plain";
		if (privateMimeType != "")
			types << privateMimeType;
		return types;
	}
	
	QMimeData * NamedValuesVectorModel::mimeData ( const QModelIndexList & indexes ) const
	{
		QMimeData *mimeData = new QMimeData();

		// "text/plain"
		QString texts;
		foreach (QModelIndex index, indexes)
		{
			if (index.isValid() && (index.column() == 0))
			{
				QString text = data(index, Qt::DisplayRole).toString();
				texts += text;
			}
		}
		mimeData->setText(texts);

		if (privateMimeType == "")
			return mimeData;

		// privateMimeType
		QByteArray itemData;
		QDataStream dataStream(&itemData, QIODevice::WriteOnly);
		foreach (QModelIndex itemIndex, indexes)
		{
			if (!itemIndex.isValid())
				continue;
			QString name = data(index(itemIndex.row(), 0), Qt::DisplayRole).toString();
			int nbArgs = data(index(itemIndex.row(), 1), Qt::DisplayRole).toInt();
			dataStream << name << nbArgs;
		}
		mimeData->setData(privateMimeType, itemData);

		return mimeData;
	}

	bool NamedValuesVectorModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
	{
		if (action == Qt::IgnoreAction)
			return true;

		if (!data->hasFormat(privateMimeType))
			return false;

		// decode mime data
		QByteArray itemData = data->data(privateMimeType);
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);
		QString name;
		int value;
		dataStream >> name >> value;

		// search for this element
		int oldIndex = 0;
		for (NamedValuesVector::iterator it = namedValues->begin(); it != namedValues->end(); it++, oldIndex++)
			if ((*it).name == name.toStdWString() && (*it).value == value)
			{
				// found! move it
				moveRow(oldIndex, row);
				return true;
			}

		// element not found
		return false;
	}

	Qt::DropActions NamedValuesVectorModel::supportedDragActions()
	{
		return Qt::CopyAction | Qt::MoveAction;
	}

	Qt::DropActions NamedValuesVectorModel::supportedDropActions() const
	{
		return Qt::CopyAction | Qt::MoveAction;
	}
	
	bool NamedValuesVectorModel::setData(const QModelIndex &index, const QVariant &value, int role)
	{
		Q_ASSERT(namedValues);
		if (index.isValid() && role == Qt::EditRole)
		{
			if (index.column() == 0)
			{
				if (!validateName(value.toString()))
					return false;
				namedValues->at(index.row()).name = value.toString().toStdWString();
				emit dataChanged(index, index);
				wasModified = true;
				return true;
			}
			if (index.column() == 1)
			{
				namedValues->at(index.row()).value = value.toInt();
				emit dataChanged(index, index);
				wasModified = true;
				return true;
			}
		}
		return false;
	}

	void NamedValuesVectorModel::setEditable(bool editable)
	{
		this->editable = editable;
	}

	void NamedValuesVectorModel::addNamedValue(const NamedValue &namedValue, int index)
	{
		Q_ASSERT(namedValues);
		Q_ASSERT(index < (int)namedValues->size());

		if (index < 0)
		{
			// insert at the end
			beginInsertRows(QModelIndex(), namedValues->size(), namedValues->size());
			namedValues->push_back(namedValue);
			endInsertRows();
		}
		else
		{
			beginInsertRows(QModelIndex(), index, index);
			NamedValuesVector::iterator it = namedValues->begin() + index;
			namedValues->insert(it, namedValue);
			endInsertRows();
		}

		wasModified = true;
		emit publicRowsInserted();
	}
	
	void NamedValuesVectorModel::delNamedValue(int index)
	{
		Q_ASSERT(namedValues);
		Q_ASSERT(index < (int)namedValues->size());
		
		beginRemoveRows(QModelIndex(), index, index);
		
		namedValues->erase(namedValues->begin() + index);
		wasModified = true;
		
		endRemoveRows();
		emit publicRowsRemoved();
	}
	
	bool NamedValuesVectorModel::moveRow(int oldRow, int& newRow)
	{
		if (oldRow == newRow || namedValues->size() <= 1)
			return false;

		// get values
		NamedValue value = namedValues->at(oldRow);

		delNamedValue(oldRow);

		// update index for the new model
		if (newRow > oldRow && newRow > 0)
			newRow--;

		addNamedValue(value, newRow);

		return true;
	}

	void NamedValuesVectorModel::clear()
	{
		Q_ASSERT(namedValues);
		
		if (namedValues->size() == 0)
			return;
		
		beginRemoveRows(QModelIndex(), 0, namedValues->size()-1);
		
		namedValues->clear();
		wasModified = true;

		endRemoveRows();
	}
	
	//! Validate name, returns true if valid, false and displays an error message otherwise
	bool NamedValuesVectorModel::validateName(const QString& name) const
	{
		return true;
	}
	
	// ****************************************************************************** //
	
	ConstantsModel::ConstantsModel(NamedValuesVector* namedValues, const QString &tooltipText, QObject *parent):
		NamedValuesVectorModel(namedValues, tooltipText, parent)
	{
		
	}
	
	ConstantsModel::ConstantsModel(NamedValuesVector* namedValues, QObject *parent):
		NamedValuesVectorModel(namedValues, parent)
	{
		
	}
	
	//! Name is valid if not already existing and not a keyword
	bool ConstantsModel::validateName(const QString& name) const
	{
		Q_ASSERT(namedValues);
		
		if (namedValues->contains(name.toStdWString()))
		{
			QMessageBox::warning(0, tr("Constant already defined"), tr("Constant %0 is already defined.").arg(name));
			return false;
		}
		
		if (Compiler::isKeyword(name.toStdWString()))
		{
			QMessageBox::warning(0, tr("The name is a keyword"), tr("The name <tt>%0</tt> cannot be used as a constant, because it is a language keyword.").arg(name));
			return false;
		}
		
		return true;
	}

	// ****************************************************************************** //

	MaskableNamedValuesVectorModel::MaskableNamedValuesVectorModel(NamedValuesVector* namedValues, const QString &tooltipText, QObject *parent) :
		NamedValuesVectorModel(namedValues, tooltipText, parent),
		viewEvent()
	{
	}

	MaskableNamedValuesVectorModel::MaskableNamedValuesVectorModel(NamedValuesVector* namedValues, QObject *parent) :
		NamedValuesVectorModel(namedValues, parent),
		viewEvent()
	{
	}

	int MaskableNamedValuesVectorModel::columnCount(const QModelIndex & parent) const
	{
		return 3;
	}

	QVariant MaskableNamedValuesVectorModel::data(const QModelIndex &index, int role) const
	{
		if (index.column() != 2)
			return NamedValuesVectorModel::data(index, role);

		if (role == Qt::DisplayRole)
		{
			return QVariant();
		}
		else if (role == Qt::DecorationRole)
		{
			return viewEvent[index.row()] ?
				QPixmap(QString(":/images/eye.png")) :
				QPixmap(QString(":/images/eyeclose.png"));
		}
		else if (role == Qt::ToolTipRole)
		{
			return viewEvent[index.row()] ?
				tr("Hide") :
				tr("View");
		}
		else
			return QVariant();
	}

	Qt::ItemFlags MaskableNamedValuesVectorModel::flags(const QModelIndex & index) const
	{
		if (index.column() == 2)
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
		else
			return NamedValuesVectorModel::flags(index);

	}

	bool MaskableNamedValuesVectorModel::isVisible(const unsigned id)
	{
		if (id >= viewEvent.size() || viewEvent[id])
			return true;

		return false;
	}

	void MaskableNamedValuesVectorModel::addNamedValue(const NamedValue& namedValue)
	{
		NamedValuesVectorModel::addNamedValue(namedValue);
		viewEvent.push_back(true);
	}

	void MaskableNamedValuesVectorModel::delNamedValue(int index)
	{
		viewEvent.erase(viewEvent.begin() + index);
		NamedValuesVectorModel::delNamedValue(index);
	}

	bool MaskableNamedValuesVectorModel::moveRow(int oldRow, int& newRow)
	{
		if (!NamedValuesVectorModel::moveRow(oldRow, newRow))
			return false;

		// get value
		bool value = viewEvent.at(oldRow);

		viewEvent.erase(viewEvent.begin() + oldRow);

		if (newRow < 0)
			viewEvent.push_back(value);
		else
			viewEvent.insert(viewEvent.begin() + newRow, value);

		return true;
	}

	void MaskableNamedValuesVectorModel::toggle(const QModelIndex &index)
	{
		Q_ASSERT(namedValues);
		Q_ASSERT(index.row() < (int)namedValues->size());

		viewEvent[index.row()] = !viewEvent[index.row()];
		wasModified = true;
	}

	/*@}*/
} // namespace Aseba
