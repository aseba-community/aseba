#include "ThymioIntermediateRepresentation.h"

namespace Aseba
{

	ThymioIRButton::ThymioIRButton(int size, ThymioIRButtonName n, int states) : 
		name(n),
		numStates(states),
		buttons(size),
		memory(-1)
	{
		cout << "thymio ir button -- size, buttons.size: " << size << ", " << buttons.size() << endl; 
	}

	ThymioIRButton::~ThymioIRButton() 
	{ 
		buttons.clear();
	}	

	void ThymioIRButton::setClicked(int i, int status) 
	{ 
		cout << "in set clicked .. " << i << ", " << buttons.size() << flush;
		if( i<size() ) 
			buttons[i] = status; 
		cout << " .. done\n" << flush;
	}
	
	int ThymioIRButton::isClicked(int i) const
	{ 
		if( i<size() ) 
			return buttons[i]; 

		return -1;
	}
	
	int ThymioIRButton::getNumStates() const
	{
		return numStates;
	}
	
	int ThymioIRButton::size() const
	{ 
		return buttons.size(); 
	}

	void ThymioIRButton::setMemoryState(int s)
	{
		memory = s;
	}

	int ThymioIRButton::getMemoryState() const
	{
		return memory;
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
		for(int i=0; i<size(); ++i)
		{
			if( buttons[i] > 0 )
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
