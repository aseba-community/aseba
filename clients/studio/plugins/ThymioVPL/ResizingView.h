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
		
	protected:
		virtual void resizeEvent(QResizeEvent * event);
		
	protected:
		qreal computedScale;
	
		QTimer *recomputeTimer;
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif // VPL_RESIZING_VIEW_H

