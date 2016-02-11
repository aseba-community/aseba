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

#include <QTimer>
#include <QDebug>
#include <cassert>

#include "ResizingView.h"

namespace Aseba { namespace ThymioVPL
{
	ResizingView::ResizingView(QGraphicsScene * scene, QWidget * parent):
		QGraphicsView(scene, parent),
		computedScale(1)/*,
		ignoreResize(false)*/
	{
		assert(scene);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		recomputeTimer = new QTimer(this);
		//ignoreResizeTimer = new QTimer(this);
		connect(recomputeTimer, SIGNAL(timeout()), SLOT(recomputeScale()));
		//connect(ignoreResizeTimer, SIGNAL(timeout()), SLOT(clearIgnoreResize()));
		connect(scene, SIGNAL(sceneSizeChanged()), recomputeTimer, SLOT(start()));
	}
	
	void ResizingView::resizeEvent(QResizeEvent * event)
	{
		//qDebug() << "resive event";
		QGraphicsView::resizeEvent(event);
		
		/*if (ignoreResize)
			return;*/
		recomputeTimer->start();
	}
	
	void ResizingView::recomputeScale()
	{
		//qDebug() << "recompute scale";
		
		disconnect(scene(), SIGNAL(sceneSizeChanged()), this, SLOT(recomputeScale()));
		
		// set transform
		resetTransform();
		const qreal widthScale(0.95*qreal(viewport()->width())/qreal(scene()->width()));
		const qreal heightScale(qreal(viewport()->height()) / qreal(80+410*3));
		computedScale = qMin(widthScale, heightScale);
		scale(computedScale, computedScale);
		
		connect(scene(), SIGNAL(sceneSizeChanged()), this, SLOT(recomputeScale()));
		
		recomputeTimer->stop();
		/*ignoreResize = true;
		ignoreResizeTimer->start();*/
	}
	/*
	void ResizingView::clearIgnoreResize()
	{
		ignoreResize = false;
		ignoreResizeTimer->stop();
	}*/
	
} } // namespace ThymioVPL / namespace Aseba
