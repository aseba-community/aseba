#ifndef VPL_EVENT_CARDS_H
#define VPL_EVENT_CARDS_H

#include "Card.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class ArrowButtonsEventCard : public CardWithButtons
	{
	public:
		ArrowButtonsEventCard(QGraphicsItem *parent=0, bool advanced=false);
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_BUTTONS_IR; }
	};
	
	class ProxEventCard : public CardWithButtons
	{
	public:
		ProxEventCard(QGraphicsItem *parent=0, bool advanced=false);
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_PROX_IR; }
	};

	class ProxGroundEventCard : public CardWithButtons
	{
	public:
		ProxGroundEventCard(QGraphicsItem *parent=0, bool advanced=false);
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_PROX_GROUND_IR; }
	};

	class TapEventCard : public Card
	{
	public:
		TapEventCard(QGraphicsItem *parent=0, bool advanced=false);
		
		virtual int valuesCount() const { return 0; }
		virtual int getValue(int i) const { return -1; }
		virtual void setValue(int i, int status) {}
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_TAP_IR; }
	};

	class ClapEventCard : public Card
	{
	public:
		ClapEventCard(QGraphicsItem *parent=0, bool advanced=false);
		
		virtual int valuesCount() const { return 0; }
		virtual int getValue(int i) const { return -1; }
		virtual void setValue(int i, int status) {}
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_CLAP_IR; }
	};
	
	/*@}*/
}; // Aseba

#endif