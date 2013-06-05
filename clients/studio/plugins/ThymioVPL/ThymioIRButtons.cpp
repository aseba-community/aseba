#include "ThymioIntermediateRepresentation.h"

namespace Aseba { namespace ThymioVPL
{
	using namespace std;
	
	ThymioIRButton::ThymioIRButton(int size, ThymioIRButtonName n) : 
		values(size),
		memory(-1),
		name(n)
	{
	}

	ThymioIRButton::~ThymioIRButton() 
	{ 
		values.clear();
	}
	
	int ThymioIRButton::valuesCount() const
	{ 
		return values.size(); 
	}

	int ThymioIRButton::getValue(int i) const
	{ 
		if( size_t(i)<values.size() ) 
			return values[i]; 
		return -1;
	}
	
	void ThymioIRButton::setValue(int i, int status) 
	{ 
		if( size_t(i)<values.size() ) 
			values[i] = status; 
	}
	
	int ThymioIRButton::getStateFilter() const
	{
		return memory;
	}
	
	int ThymioIRButton::getStateFilter(int i) const
	{
		return ((memory >> (i*2)) & 0x3);
	}

	void ThymioIRButton::setStateFilter(int s)
	{
		memory = s;
	}

	void ThymioIRButton::setBasename(wstring n)
	{
		basename = n;
	}

	ThymioIRButtonName ThymioIRButton::getName() const
	{ 
		return name; 
	}
	
	wstring ThymioIRButton::getBasename() const
	{
		return basename;
	}
	
	bool ThymioIRButton::isEventButton() const
	{ 
		return (name < 5 ? true : false); 
	}
	
	bool ThymioIRButton::isSet() const
	{
		for(int i=0; i<valuesCount(); ++i)
		{
			if( values[i] > 0 )
				return true;
		}
	
		return false;
	}

	void ThymioIRButton::accept(ThymioIRVisitor *visitor) 
	{ 
		visitor->visit(this); 
	}

	ThymioIRButtonSet::ThymioIRButtonSet(ThymioIRButton *event, ThymioIRButton *action) : 
		eventButton(event), 
		actionButton(action) 
	{
	}
	
	//! Set event button IR; if exists, replace but do not delete pointer as membership is hold by the caller
	void ThymioIRButtonSet::setEventButton(ThymioIRButton *event) 
	{
		eventButton = event; 
	}
	
	//! Set action button IR; if exists, replace but do not delete pointer as membership is hold by the caller
	void ThymioIRButtonSet::setActionButton(ThymioIRButton *action) 
	{
		actionButton = action; 
	}
	
	ThymioIRButton *ThymioIRButtonSet::getEventButton() 
	{ 
		return eventButton; 
	}
	
	ThymioIRButton *ThymioIRButtonSet::getActionButton() 
	{ 
		return actionButton;
	}

	bool ThymioIRButtonSet::hasEventButton() const
	{ 
		return (eventButton == 0 ? false : true); 
	}
	
	bool ThymioIRButtonSet::hasActionButton() const
	{ 
		return (actionButton == 0 ? false : true); 
	}
	
	void ThymioIRButtonSet::accept(ThymioIRVisitor *visitor) 
	{ 
		visitor->visit(this); 
	}

} } // namespace ThymioVPL / namespace Aseba
