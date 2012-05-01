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

#include "CustomWidgets.h"
#include <QMimeData>
#include <QResizeEvent>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	
	QStringList DraggableListWidget::mimeTypes () const
	{
		QStringList types;
		types << "text/plain";
		return types;
	}
	
	QMimeData * DraggableListWidget::mimeData ( const QList<QListWidgetItem *> items ) const
	{
		QString texts;
		foreach (QListWidgetItem *item, items)
		{
			texts += item->text();
		}
		
		QMimeData *mimeData = new QMimeData();
		mimeData->setText(texts);
		return mimeData;
	}
	
	//////
		
	FixedWidthTableView::FixedWidthTableView()
	{
		col1Width = 50;
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}
	
	void FixedWidthTableView::setSecondColumnLongestContent(const QString& content)
	{
		Q_ASSERT((model ()->columnCount() == 2) || (model ()->columnCount() == 3));
		QFontMetrics fm(font());
		col1Width = fm.width(content);
	}
	
	void FixedWidthTableView::resizeEvent ( QResizeEvent * event )
	{
		Q_ASSERT((model ()->columnCount() == 2) || (model ()->columnCount() == 3));
		if (model()->columnCount() == 2)
		{
			int col0Width = event->size().width() - col1Width;
			setColumnWidth(0, col0Width);
			setColumnWidth(1, col1Width);
		}
		else if (model()->columnCount() == 3)
		{
			int col0Width = event->size().width() - col1Width - 22;
			setColumnWidth(0, col0Width);
			setColumnWidth(1, col1Width);
			setColumnWidth(2, 22);
		}
	}
	
	/*@}*/
}; // Aseba
