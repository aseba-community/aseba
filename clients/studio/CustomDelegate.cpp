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

#include "CustomDelegate.h"
#include <QtGui>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	SpinBoxDelegate::SpinBoxDelegate(int minValue, int maxValue, QObject *parent) :
		QItemDelegate(parent),
		minValue(minValue),
		maxValue(maxValue)
	{
	}
 
	QWidget *SpinBoxDelegate::createEditor(QWidget *parent,
		const QStyleOptionViewItem &/* option */,
		const QModelIndex &/* index */) const
	{
		QSpinBox *editor = new QSpinBox(parent);
		editor->setMinimum(minValue);
		editor->setMaximum(maxValue);
		editor->installEventFilter(const_cast<SpinBoxDelegate*>(this));
	
		return editor;
	}
		
	void SpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
	{
		int value = index.model()->data(index, Qt::DisplayRole).toInt();
		
		QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
		spinBox->setValue(value);
	}
	
	void SpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const
	{
		QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
		spinBox->interpretText();
		int value = spinBox->value();
		
		model->setData(index, value);
	}
	
	void SpinBoxDelegate::updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
	{
		editor->setGeometry(option.rect);
	}
	
	/*@}*/
} // namespace Aseba
