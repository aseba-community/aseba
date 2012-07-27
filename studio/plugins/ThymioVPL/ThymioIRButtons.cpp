#include "ThymioIntermediateRepresentation.h"

namespace Aseba
{

	ThymioIRButton::ThymioIRButton(int size, ThymioIRButtonName n) : 
		name(n)
	{ 
		buttons.resize(size, false); 
	}

	ThymioIRButton::~ThymioIRButton() 
	{ 
		buttons.clear();
	}	

	void ThymioIRButton::setClicked(int i, bool status) 
	{ 
		if( i<size() ) buttons[i] = status; 
	}
	
	bool ThymioIRButton::isClicked(int i) const
	{ 
		if( i<size() ) 
			return buttons[i]; 
	}
	
	int ThymioIRButton::size() const
	{ 
		return buttons.size(); 
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

	bool ThymioIRButton::isValid() const
	{ 
		if( name > 2 )
			return true; 

		for(int i=0; i<size(); ++i)
		{
			if( buttons[i] == true )
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
		
	void ThymioIRButtonSet::addEventButton(ThymioIRButton *event) 
	{ 
		eventButton = event; 
	}
	
	void ThymioIRButtonSet::addActionButton(ThymioIRButton *action) 
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

		
}; // Aseba
