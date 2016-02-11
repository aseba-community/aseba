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

#ifndef VPL_RESIZING_VIEW_H
#define VPL_RESIZING_VIEW_H

#include <QGraphicsView>

namespace Aseba { namespace ThymioVPL
{
	class Scene;
	class BlockButton;
	
	/** \addtogroup studio */
	/*@{*/
	
	class ResizingView: public QGraphicsView
	{
		Q_OBJECT
	
	public:
		ResizingView(QGraphicsScene * scene, QWidget * parent = 0);
		qreal getScale() const { return computedScale; }
		
	public slots:
		void recomputeScale();
		
	/*protected slots:
		void clearIgnoreResize();*/
		
	protected:
		virtual void resizeEvent(QResizeEvent * event);
		
	protected:
		qreal computedScale;
		//bool ignoreResize;
	
		QTimer *recomputeTimer;
		//QTimer *ignoreResizeTimer;
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif // VPL_RESIZING_VIEW_H

