#ifndef VPL_EVENT_BLOCKS_H
#define VPL_EVENT_BLOCKS_H

#include "Block.h"

class QGraphicsSvgItem;

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class ArrowButtonsEventBlock : public BlockWithButtons
	{
	public:
		ArrowButtonsEventBlock(QGraphicsItem *parent=0);
	};
	
	class ProxEventBlock : public BlockWithButtonsAndRange
	{
	public:
		ProxEventBlock(bool advanced, QGraphicsItem *parent=0);
	};

	class ProxGroundEventBlock : public BlockWithButtonsAndRange
	{
	public:
		ProxGroundEventBlock(bool advanced, QGraphicsItem *parent=0);
	};

	class AccEventBlock : public Block
	{
	public:
		enum
		{
			MODE_TAP = 0,
			MODE_PITCH,
			MODE_ROLL
		};
		static const int resolution;
		
	public:
		AccEventBlock(bool advanced, QGraphicsItem *parent=0);
		
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		virtual unsigned valuesCount() const { return 2; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		
		virtual bool isAnyAdvancedFeature() const;
		virtual void setAdvanced(bool advanced);
		
	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
		
		int orientationFromPos(const QPointF& pos, bool* ok) const;
		void setMode(unsigned mode);
		void setOrientation(int orientation);
		
	protected:
		static const QRectF buttonPoses[3];
		
		unsigned mode;
		int orientation;
		bool dragging;
		
		QGraphicsSvgItem* tapSimpleSvg;
		QGraphicsSvgItem* tapAdvancedSvg;
		QGraphicsSvgItem* quadrantSvg;
		QGraphicsSvgItem* pitchSvg;
		QGraphicsSvgItem* rollSvg;
	};

	class ClapEventBlock : public BlockWithNoValues
	{
	public:
		ClapEventBlock(QGraphicsItem *parent=0);
	};
	
	class TimeoutEventBlock : public BlockWithNoValues
	{
	public:
		TimeoutEventBlock(QGraphicsItem *parent=0);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif