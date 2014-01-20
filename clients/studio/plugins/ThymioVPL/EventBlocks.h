#ifndef VPL_EVENT_BLOCKS_H
#define VPL_EVENT_BLOCKS_H

#include "Block.h"

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

	class TapEventBlock : public BlockWithNoValues
	{
	public:
		TapEventBlock(QGraphicsItem *parent=0);
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