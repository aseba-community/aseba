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

#include "CustomWidgets.h"
#include <QMimeData>
#include <QResizeEvent>
#include <QPainter>

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

	void FixedWidthTableView::startDrag(Qt::DropActions supportedActions)
	{
		QModelIndex item = currentIndex();
		QModelIndexList list;
		list.append(item);

		QMimeData* mimeData = model()->mimeData(list);

		QDrag *drag = new QDrag(this);
		drag->setMimeData(mimeData);
		QPixmap pixmap = getDragPixmap(mimeData->text());
		drag->setPixmap(pixmap);
		QPoint hotpoint(pixmap.width()+10, pixmap.height()/2);
		drag->setHotSpot(hotpoint);

		if (drag->exec(Qt::CopyAction|Qt::MoveAction, Qt::CopyAction) == Qt::MoveAction)
		{
			// item moved
		}
		else
		{
			// item copied
		}
	}

	void FixedWidthTableView::dragEnterEvent(QDragEnterEvent *event)
	{
		if (modelMatchMimeFormat(event->mimeData()->formats()))
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
		else
			event->ignore();
	}

	void FixedWidthTableView::dragMoveEvent(QDragMoveEvent *event)
	{
		event->accept();
	}

	void FixedWidthTableView::dropEvent(QDropEvent *event)
	{
		int row = rowAt(event->pos().y());
		int col = columnAt(event->pos().x());

		if (model()->dropMimeData(event->mimeData(), event->dropAction(), row, col, QModelIndex()))
		{
			event->accept();
		}
		else
			event->ignore();
	}

	QPixmap FixedWidthTableView::getDragPixmap(QString text)
	{
		// From the "Fridge Magnets" example
		QFontMetrics metric(font());
		QSize size = metric.size(Qt::TextSingleLine, text);

		QImage image(size.width() + 12, size.height() + 12, QImage::Format_ARGB32_Premultiplied);
		image.fill(qRgba(0, 0, 0, 0));

		QFont font;
		font.setStyleStrategy(QFont::ForceOutline);

		QPainter painter;
		painter.begin(&image);
		painter.setRenderHint(QPainter::Antialiasing);

		painter.setFont(font);
		painter.setBrush(Qt::black);
		painter.drawText(QRect(QPoint(6, 6), size), Qt::AlignCenter, text);
		painter.end();

		return QPixmap::fromImage(image);
	}

	bool FixedWidthTableView::modelMatchMimeFormat(QStringList candidates)
	{
		QStringList acceptedMime = model()->mimeTypes();
		for (QStringList::ConstIterator it = acceptedMime.constBegin(); it != acceptedMime.constEnd(); it++)
		{
			if (candidates.contains(*it))
				return true;
		}
		return false;
	}
	
	/*@}*/
} // namespace Aseba
