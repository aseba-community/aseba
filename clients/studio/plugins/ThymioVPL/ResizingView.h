#ifndef THYMIO_RESIZING_VIEW_H
#define THYMIO_RESIZING_VIEW_H

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
		
	protected:
		virtual void resizeEvent(QResizeEvent * event);
		
	protected slots:
		void recomputeScale();
		void resetResizedFlag();
		
	protected:
		bool wasResized;
		qreal computedScale;
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif // THYMIO_RESIZING_VIEW_H

