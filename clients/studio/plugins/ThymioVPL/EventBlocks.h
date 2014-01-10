#ifndef VPL_EVENT_CARDS_H
#define VPL_EVENT_CARDS_H

#include "Block.h"

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class ArrowButtonsEventBlock : public BlockWithButtons
	{
	public:
		ArrowButtonsEventBlock(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	class ProxEventBlock : public BlockWithButtonsAndRange
	{
	public:
		ProxEventBlock(QGraphicsItem *parent=0, bool advanced=false);
	};

	class ProxGroundEventBlock : public BlockWithButtonsAndRange
	{
	public:
		ProxGroundEventBlock(QGraphicsItem *parent=0, bool advanced=false);
	};

	class TapEventBlock : public BlockWithNoValues
	{
	public:
		TapEventBlock(QGraphicsItem *parent=0, bool advanced=false);
	};

	class ClapEventBlock : public BlockWithNoValues
	{
	public:
		ClapEventBlock(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	class TimeoutEventBlock : public BlockWithNoValues
	{
	public:
		TimeoutEventBlock(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif