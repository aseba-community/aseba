#ifndef VPL_EVENT_CARDS_H
#define VPL_EVENT_CARDS_H

#include "Card.h"

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class ArrowButtonsEventCard : public CardWithButtons
	{
	public:
		ArrowButtonsEventCard(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	class ProxEventCard : public CardWithButtons
	{
	public:
		ProxEventCard(QGraphicsItem *parent=0, bool advanced=false);
	};

	class ProxGroundEventCard : public CardWithButtons
	{
	public:
		ProxGroundEventCard(QGraphicsItem *parent=0, bool advanced=false);
	};

	class TapEventCard : public CardWithNoValues
	{
	public:
		TapEventCard(QGraphicsItem *parent=0, bool advanced=false);
	};

	class ClapEventCard : public CardWithNoValues
	{
	public:
		ClapEventCard(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif