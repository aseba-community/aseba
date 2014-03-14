#ifndef VPL_STATE_BLOCKS_H
#define VPL_STATE_BLOCKS_H

#include "Block.h"

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class StateFilterCheckBlock : public StateFilterBlock
	{
	public:
		StateFilterCheckBlock(QGraphicsItem *parent=0);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif