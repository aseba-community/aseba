#include <sstream>
#include "ThymioIntermediateRepresentation.h"

namespace Aseba
{
	// Visitor base //
	void ThymioIRVisitor::visit(ThymioIRButton *button)
	{
		errorCode = THYMIO_NO_ERROR;
	}

	void ThymioIRVisitor::visit(ThymioIRButtonSet *buttonSet)
	{
		errorCode = THYMIO_NO_ERROR;
	}

	ThymioIRErrorCode ThymioIRVisitor::getErrorCode() const
	{
		return errorCode;
	}
	
	bool ThymioIRVisitor::isSuccessful() const
	{
		return ( errorCode == THYMIO_NO_ERROR ? true : false );
	}

	wstring ThymioIRVisitor::toWstring(int val)
	{
		wstringstream streamVal;
		streamVal << val;
		
		return streamVal.str();	
	}

	// Type Checker //	
	ThymioIRTypeChecker::~ThymioIRTypeChecker()
	{
		clear();
	}
	
	void ThymioIRTypeChecker::reset()
	{
		clear();
	}
	
	void ThymioIRTypeChecker::clear()
	{
		moveHash.clear();
		colorHash.clear();
		circleHash.clear();
		soundHash.clear();
		memoryHash.clear();
		
		tapSeenActions.clear();
		clapSeenActions.clear();
		
		activeActionName = THYMIO_BUTTONS_IR;
	}

	void ThymioIRTypeChecker::visit(ThymioIRButton *button)
	{
		if( button == 0 ) return;

		errorCode = THYMIO_NO_ERROR;
		
		multimap<wstring, ThymioIRButton*> *currentHash;
		multimap<wstring, ThymioIRButton*> tempHash;
		
		switch( activeActionName )
		{
		case THYMIO_MOVE_IR:
			currentHash = &moveHash;
			tempHash = moveHash;
			break;
		case THYMIO_COLOR_IR:
			currentHash = &colorHash;
			tempHash = colorHash;
			break;
		case THYMIO_CIRCLE_IR:
			currentHash = &circleHash;
			tempHash = circleHash;
			break;
		case THYMIO_SOUND_IR:
			currentHash = &soundHash;
			tempHash = soundHash;
			break;
		case THYMIO_MEMORY_IR:
			currentHash = &memoryHash;
			tempHash = memoryHash;
			break;
		default:
			return;
			break;
		}

		currentHash->clear();
	
		for(multimap<wstring, ThymioIRButton*>::iterator itr = tempHash.begin();
			itr != tempHash.end(); ++itr)
		{
			if( itr->second != button )
				currentHash->insert(*itr);
		}

		wstring buttonname = button->getBasename() + L"_" + toWstring(button->getMemoryState());;
		for(int i=0; i<button->size(); i++)
		{	
			if( button->isClicked(i) > 0 )
			{
				buttonname += L"_";
				buttonname += toWstring(i);
				buttonname += L"_";
				buttonname += toWstring(button->isClicked(i));
			}
		}

		pair< multimap<wstring, ThymioIRButton*>::iterator,
			  multimap<wstring, ThymioIRButton*>::iterator > ret;
		ret = currentHash->equal_range(buttonname);		

		for(multimap<wstring, ThymioIRButton*>::iterator itr = ret.first; 
			itr != ret.second; ++itr )				
			if( itr->second != button )
				errorCode = THYMIO_EVENT_MULTISET;
				
		currentHash->insert( pair<wstring, ThymioIRButton*>(buttonname, button) );
		tempHash.clear();
	}

	void ThymioIRTypeChecker::visit(ThymioIRButtonSet *buttonSet)
	{
		if( !buttonSet->hasEventButton() || !buttonSet->hasActionButton() ) return;
				
		errorCode = THYMIO_NO_ERROR;

		activeActionName = buttonSet->getActionButton()->getName();

		visit(buttonSet->getEventButton());			
	}

	// Syntax Checker //
	void ThymioIRSyntaxChecker::visit(ThymioIRButton *button)
	{
		errorCode = THYMIO_NO_ERROR;
	}
	
	void ThymioIRSyntaxChecker::visit(ThymioIRButtonSet *buttonSet)
	{
		errorCode = THYMIO_NO_ERROR;		
		
		if( !(buttonSet->hasEventButton()) )
		{
			if( buttonSet->hasActionButton() )
				errorCode = THYMIO_MISSING_EVENT;
		} 
		else if( !(buttonSet->hasActionButton()) )
			errorCode = THYMIO_MISSING_ACTION;
		else
			visit(buttonSet->getEventButton());
	}

	// Code Generator //
	ThymioIRCodeGenerator::ThymioIRCodeGenerator() : 
		ThymioIRVisitor(),
		generatedCode(),
		currentBlock(0),
		inIfBlock(false),
		insertLoc(0)
	{
		editor[THYMIO_BUTTONS_IR] = -1;	
		editor[THYMIO_PROX_IR] = -1;
		editor[THYMIO_PROX_GROUND_IR] = -1;
		editor[THYMIO_TAP_IR] = -1;
		editor[THYMIO_CLAP_IR] = -1;
		
		directions.push_back(L"forward");
		directions.push_back(L"left");
		directions.push_back(L"backward");
		directions.push_back(L"right");
		directions.push_back(L"center");
	}
	
	ThymioIRCodeGenerator::~ThymioIRCodeGenerator()
	{
		clear();
		directions.clear();
	}
	
	void ThymioIRCodeGenerator::clear()
	{
		editor.clear();
		generatedCode.clear();
		inIfBlock = false;
	}
	
	void ThymioIRCodeGenerator::reset() 
	{
		editor[THYMIO_BUTTONS_IR] = -1;	
		editor[THYMIO_PROX_IR] = -1;
		editor[THYMIO_PROX_GROUND_IR] = -1;
		editor[THYMIO_TAP_IR] = -1;
		editor[THYMIO_CLAP_IR] = -1;
		generatedCode.clear();
		inIfBlock = false;
	}	

	void ThymioIRCodeGenerator::visit(ThymioIRButton *button)
	{
		if( button == 0 ) return;
		
		wstring text;

		if( button->isEventButton() )
		{
			insertLoc = generatedCode[currentBlock].end();
			bool flag=false;
			if( button->isSet() )
			{
				text = L"\tif ";
				switch( button->getName() )
				{
				case THYMIO_BUTTONS_IR:
					for(int i=0; i<5; ++i)
					{
						if(button->isClicked(i) > 0)
						{
							text += (flag ? L" and " : L"");
							text += L"button.";
							text += directions[i];
							text += L" == 1";
							flag = true;
						}
					}
					if( button->getMemoryState() >= 0 )
					{
						text += L" and state == ";
						text += toWstring(button->getMemoryState());
					}
					break;
				case THYMIO_PROX_IR:
					for(int i=0; i<button->size(); ++i)
					{
						if(button->isClicked(i) == 1)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.horizontal[";
							text += toWstring(i);
							text += L"] < 400";
							flag = true;
						} 
						else if(button->isClicked(i) == 2)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.horizontal[";
							text += toWstring(i);
							text += L"] > 500";
							flag = true;
						} 
					}
					if( button->getMemoryState() >= 0 )
					{
						text += L" and state == ";
						text += toWstring(button->getMemoryState());
					}					
					break;		
				case THYMIO_PROX_GROUND_IR:
					for(int i=0; i<button->size(); ++i)
					{
						if(button->isClicked(i) == 1)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.ground.reflected[";
							text += toWstring(i);
							text += L"] < 150";
							flag = true;
						}
						else if(button->isClicked(i) == 2)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.ground.reflected[";
							text += toWstring(i);
							text += L"] > 300";
							flag = true;
						}						
					}
					if( button->getMemoryState() >= 0 )
					{
						text += L" and state == ";
						text += toWstring(button->getMemoryState());
					}						
					break;			
				case THYMIO_TAP_IR:
				case THYMIO_CLAP_IR:
				default:
					errorCode = THYMIO_INVALID_CODE;
					break;			
				}

				text += L" then\n";
				inIfBlock = true;
					
				generatedCode[currentBlock].insert(insertLoc, text.begin(), text.end());
				insertLoc = generatedCode[currentBlock].end();	
			}
			else 
			{
				if( button->getMemoryState() >= 0 )
				{
					text += L"\tif state == ";
					text += toWstring(button->getMemoryState());
					text += L" then\n";
					inIfBlock = true;
					
					generatedCode[currentBlock].insert(insertLoc, text.begin(), text.end());
					insertLoc = generatedCode[currentBlock].end();
				}
				else
				{
					inIfBlock = false;
					
					size_t found =  generatedCode[currentBlock].find(L"if");
					if( found == wstring::npos )
						insertLoc = generatedCode[currentBlock].end();
					else
						insertLoc = generatedCode[currentBlock].begin() + found - 1;
				}
			}
		}
		else
		{
			text = (inIfBlock ? L"\t\t" : L"\t");
			int val=0;
			switch( button->getName() )
			{
			case THYMIO_MOVE_IR:
				text += L"motor.left.target = ";
				text += toWstring(button->isClicked(0));
				text += (inIfBlock ? L"\n\t\t" : L"\n\t");
				text += L"motor.right.target = ";
				text += toWstring(button->isClicked(1));
				text += L"\n";				
				break;
			case THYMIO_COLOR_IR:
				text += L"call leds.top(";
				text += toWstring(button->isClicked(0));
				text += L",";
				text += toWstring(button->isClicked(1));
				text += L",";								
				text += toWstring(button->isClicked(2));
				text += L")\n";
				break;
			case THYMIO_CIRCLE_IR:
				text += L"call leds.circle(";
				for(int i=0; i < button->size(); ++i)
				{
					text += toWstring((32*button->isClicked(i))/(button->getNumStates()-1)); 
					text += L",";
				}
				text.replace(text.length()-1, 2,L")\n");
				break;
			case THYMIO_SOUND_IR:
				if( button->isClicked(0) ) // friendly
					text += L"call sound.system(7)\n";
				else if( button->isClicked(1) ) // okay
					text += L"call sound.system(6)\n";
				else
					text += L"call sound.system(4)\n"; // scared
				break;
			case THYMIO_MEMORY_IR:
				text += L"new_state = ";
				for(int i=0; i<button->size(); ++i)
					val += (button->isClicked(i)*(0x01<<i));
				text += toWstring(val);
				text += L"\n";
				break;
			default:
				errorCode = THYMIO_INVALID_CODE;
				break;
			}
			text += (inIfBlock ? L"\tend\n" : L"");
			inIfBlock = false;
		
			generatedCode[currentBlock].insert(insertLoc, text.begin(), text.end());
		}
	
	}

	void ThymioIRCodeGenerator::visit(ThymioIRButtonSet *buttonSet)
	{
		if( !buttonSet->hasEventButton() || !buttonSet->hasActionButton() ) return;
		
		errorCode = THYMIO_NO_ERROR;
		
		if( generatedCode.empty() )
		{
			if( buttonSet->getEventButton()->getMemoryState() >= 0 )
				generatedCode.push_back(L"var state = 0\nvar new_state = 0\n\ncall sound.system(2)\ncall sound.system(3)\ncall sound.system(-1)\n");
			else
				generatedCode.push_back(L"call sound.system(2)\ncall sound.system(3)\ncall sound.system(-1)\n");
		}
		
		ThymioIRButtonName name = buttonSet->getEventButton()->getName();
		int block = editor[name];

		if( block < 0 )
		{
			block = generatedCode.size();
			
			switch(name) 
			{
			case THYMIO_BUTTONS_IR:
				generatedCode.push_back(L"onevent buttons\n");
				if(buttonSet->getEventButton()->getMemoryState() >= 0)
					generatedCode.push_back(L"\tstate = new_state\n\tcall leds.buttons(((state>>0) & 1)*32,((state>>1) & 1)*32,((state>>2) & 1)*32,((state>>3) & 1)*32)\n");
				break;
			case THYMIO_PROX_IR:
				if( editor[THYMIO_PROX_GROUND_IR] < 0 )
				{
					generatedCode.push_back(L"onevent prox\n");
					if(buttonSet->getEventButton()->getMemoryState() >= 0)
						generatedCode.push_back(L"\tstate = new_state\n\tcall leds.buttons(((state>>0) & 1)*32,((state>>1) & 1)*32,((state>>2) & 1)*32,((state>>3) & 1)*32)\n");
				}
				else
					block = editor[THYMIO_PROX_GROUND_IR];
				break;
			case THYMIO_PROX_GROUND_IR:
				if( editor[THYMIO_PROX_IR] < 0 )
				{
					generatedCode.push_back(L"onevent prox\n");
					if(buttonSet->getEventButton()->getMemoryState() >= 0)
						generatedCode.push_back(L"\tstate = new_state\n\tcall leds.buttons(((state>>0) & 1)*32,((state>>1) & 1)*32,((state>>2) & 1)*32,((state>>3) & 1)*32)\n");
				}
				else
					block = editor[THYMIO_PROX_IR];
				break;
			case THYMIO_TAP_IR:
				generatedCode.push_back(L"onevent tap\n");
				if(buttonSet->getEventButton()->getMemoryState() >= 0)
					generatedCode.push_back(L"\tstate = new_state\n\tcall leds.buttons(((state>>0) & 1)*32,((state>>1) & 1)*32,((state>>2) & 1)*32,((state>>3) & 1)*32)\n");				
				break;
			case THYMIO_CLAP_IR:
				if( generatedCode.empty() || generatedCode[0].find(L"var new_state = 0\n") == wstring::npos )
				{
					generatedCode.insert(generatedCode.begin(),L"mic.threshold = 250\n");
					for(map<ThymioIRButtonName, int>::iterator itr = editor.begin();
						itr != editor.end(); ++itr)
						if( itr->second >= 0 )
							itr->second += 1;				
					block += 1;
				}
				else
					generatedCode[0].append(L"\nmic.threshold = 250\n");
				generatedCode.push_back(L"onevent mic\n");
				if(buttonSet->getEventButton()->getMemoryState() >= 0)
					generatedCode.push_back(L"\tstate = new_state\n\tcall leds.buttons(((state>>0) & 1)*32,((state>>1) & 1)*32,((state>>2) & 1)*32,((state>>3) & 1)*32)");				
				break;	
			default:
				errorCode = THYMIO_INVALID_CODE;
				return;
				break;
			}

			editor[name] = block;
		}
		currentBlock = block;
				
		visit(buttonSet->getEventButton());
		visit(buttonSet->getActionButton());
	}

}; // Aseba
