#include <QTimer>
#include <cassert>

#include "ResizingView.h"

namespace Aseba { namespace ThymioVPL
{
	ResizingView::ResizingView(QGraphicsScene * scene, QWidget * parent):
		QGraphicsView(scene, parent),
		wasResized(false),
		computedScale(1)
	{
		assert(scene);
		connect(scene, SIGNAL(sceneSizeChanged()), SLOT(recomputeScale()));
	}
	
	void ResizingView::resizeEvent(QResizeEvent * event)
	{
		QGraphicsView::resizeEvent(event);
		if (!wasResized)
			recomputeScale();
	}
	
	void ResizingView::recomputeScale()
	{
		// set transform
		resetTransform();
		const qreal widthScale(0.95*qreal(viewport()->width())/qreal(scene()->width()));
		const qreal heightScale(qreal(viewport()->height()) / qreal(80+410*3));
		computedScale = qMin(widthScale, heightScale);
		scale(computedScale, computedScale);
		wasResized = true;
		QTimer::singleShot(0, this, SLOT(resetResizedFlag()));
	}
	
	void ResizingView::resetResizedFlag()
	{
		wasResized = false;
	}
	
} } // namespace ThymioVPL / namespace Aseba
