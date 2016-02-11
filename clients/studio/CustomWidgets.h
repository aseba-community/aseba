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

#ifndef CUSTOM_WIDGETS_H
#define CUSTOM_WIDGETS_H

#include <QListWidget>
#include <QTableView>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class DraggableListWidget: public QListWidget
	{
	protected:
		virtual QStringList mimeTypes () const;
		virtual QMimeData * mimeData ( const QList<QListWidgetItem *> items ) const;
	};
	
	class FixedWidthTableView : public QTableView
	{
	protected:
		int col1Width;
		
	public:
		FixedWidthTableView();
		void setSecondColumnLongestContent(const QString& content);
	
	protected:
		virtual void resizeEvent ( QResizeEvent * event );

		void startDrag(Qt::DropActions supportedActions);
		void dragEnterEvent(QDragEnterEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dropEvent(QDropEvent *event);

		QPixmap getDragPixmap(QString text);
		bool modelMatchMimeFormat(QStringList candidates);
	};
	
	/*@}*/
} // namespace Aseba

#endif	
