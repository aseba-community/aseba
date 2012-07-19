/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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

#include <NamedValuesVectorModel.moc>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	NamedValuesVectorModel::NamedValuesVectorModel(NamedValuesVector* namedValues, const QString &tooltipText, QObject *parent) :
		QAbstractTableModel(parent),
		namedValues(namedValues),
		tooltipText(tooltipText),
		wasModified(false),
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
		
		if (role == Qt::DisplayRole)
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
		if (index.column() == 0)
		{
			if (editable)
				return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
			else
				return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
		}
		else
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
	}
	
	QStringList NamedValuesVectorModel::mimeTypes () const
	{
		QStringList types;
		types << "text/plain";
		return types;
	}
	
	QMimeData * NamedValuesVectorModel::mimeData ( const QModelIndexList & indexes ) const
	{
		QString texts;
		foreach (QModelIndex index, indexes)
		{
			if (index.isValid() && (index.column() == 0))
			{
				QString text = data(index, Qt::DisplayRole).toString();
				texts += text;
			}
		}
		
		QMimeData *mimeData = new QMimeData();
		mimeData->setText(texts);
		return mimeData;
	}
	
	bool NamedValuesVectorModel::setData(const QModelIndex &index, const QVariant &value, int role)
	{
		Q_ASSERT(namedValues);
		if (index.isValid() && role == Qt::EditRole)
		{
			if (index.column() == 0)
			{
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

	void NamedValuesVectorModel::addNamedValue(const NamedValue& namedValue)
	{
		Q_ASSERT(namedValues);
		
		unsigned position = namedValues->size();
		
		beginInsertRows(QModelIndex(), position, position);
		
		namedValues->push_back(namedValue);
		wasModified = true;
		
		endInsertRows();
	}
	
	void NamedValuesVectorModel::delNamedValue(int index)
	{
		Q_ASSERT(namedValues);
		Q_ASSERT(index < (int)namedValues->size());
		
		beginRemoveRows(QModelIndex(), index, index);
		
		namedValues->erase(namedValues->begin() + index);
		wasModified = true;
		
		endRemoveRows();
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

	void MaskableNamedValuesVectorModel::toggle(const QModelIndex &index)
	{
		Q_ASSERT(namedValues);
		Q_ASSERT(index.row() < (int)namedValues->size());

		viewEvent[index.row()] = !viewEvent[index.row()];
		wasModified = true;
	}

	/*@}*/
}; // Aseba
