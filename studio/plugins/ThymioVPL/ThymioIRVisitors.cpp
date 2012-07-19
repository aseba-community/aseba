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

	ThymioIRErrorCode ThymioIRVisitor::getErrorCode()
	{
		return errorCode;
	}
	
	bool ThymioIRVisitor::isSuccessful()
	{
		return ( errorCode == THYMIO_NO_ERROR ? true : false );
	}

	wstring ThymioIRVisitor::getErrorMessage()
	{
		switch(errorCode) {
		case THYMIO_MISSING_EVENT:
			return L"Missing event button";
			break;
		case THYMIO_MISSING_ACTION:
			return L"Missing action button";
			break;
		case THYMIO_EVENT_MULTISET:
			return L"Redeclaration of event button.";
			break;
		case THYMIO_INVALID_CODE:
			return L"Unknown button type.";
			break;
		case THYMIO_NO_ERROR:
			return L"Compilation success.";
		default:
			break;
		}
		
		return L"";
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
//		resetHash.clear();
		
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
//		case THYMIO_RESET_IR:
//			currentHash = &resetHash;
//			tempHash = resetHash;
//			break;
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

		wstring buttonname = button->getBasename();
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

		ThymioIRButtonName eventName = buttonSet->getEventButton()->getName();
		ThymioIRButtonName actionName = buttonSet->getActionButton()->getName();

		if( eventName == THYMIO_TAP_IR ) 
		{
			if( tapSeenActions.find(actionName) != tapSeenActions.end() ) 
				errorCode = THYMIO_EVENT_MULTISET;
			else
				tapSeenActions.insert(actionName);
		}
		else if( eventName == THYMIO_CLAP_IR )
		{
			if( clapSeenActions.find(actionName) != clapSeenActions.end() )
				errorCode = THYMIO_EVENT_MULTISET;
			else
				clapSeenActions.insert(actionName);
		}
		else
		{
			activeActionName = actionName;
			visit(buttonSet->getEventButton());	
		}
		
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
			switch( button->getName() )
			{
			case THYMIO_BUTTONS_IR:
				if( button->isSet() )
				{
					text = L"\tif ";
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
					text += L" then\n";
					inIfBlock = true;
					
					generatedCode[currentBlock].insert(insertLoc, text.begin(), text.end());
					insertLoc = generatedCode[currentBlock].end();
				}
				else
				{
					size_t found =  generatedCode[currentBlock].find(L"if");
					if( found == wstring::npos )
						insertLoc = generatedCode[currentBlock].end();
					else
						insertLoc = generatedCode[currentBlock].begin() + found - 1;
				}	
				break;
			case THYMIO_PROX_IR:
				if( button->isSet() )
				{
					text = L"\tif ";
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
					text += L" then\n";
					inIfBlock = true;
					
					generatedCode[currentBlock].insert(insertLoc, text.begin(), text.end());
					insertLoc = generatedCode[currentBlock].end();
				}	
				else 
				{
					size_t found =  generatedCode[currentBlock].find(L"if");
					if( found == wstring::npos )
						insertLoc = generatedCode[currentBlock].end();
					else
						insertLoc = generatedCode[currentBlock].begin() + found - 1;
				}
				break;		
			case THYMIO_PROX_GROUND_IR:
				if( button->isSet() )
				{
					text = L"\tif ";
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
					text += L" then\n";
					inIfBlock = true;
					
					generatedCode[currentBlock].insert(insertLoc, text.begin(), text.end());
					insertLoc = generatedCode[currentBlock].end();
				} 
				else 
				{
					size_t found =  generatedCode[currentBlock].find(L"if");
					if( found == wstring::npos )
						insertLoc = generatedCode[currentBlock].end();
					else
						insertLoc = generatedCode[currentBlock].begin() + found - 1;
				}		
						
				break;			
			case THYMIO_TAP_IR:
			case THYMIO_CLAP_IR:
				inIfBlock = false;

				generatedCode[currentBlock].insert(insertLoc, text.begin(), text.end());
				insertLoc = generatedCode[currentBlock].end();
				break;
			default:
				errorCode = THYMIO_INVALID_CODE;
				break;			
			}
		}
		else
		{
			text = (inIfBlock ? L"\t\t" : L"\t");
			switch( button->getName() )
			{
			case THYMIO_MOVE_IR:
				text += L"motor.left.target = ";
				text += ( button->isClicked(0) > 0 ? L"500\n" : 
						  button->isClicked(1) > 0 ? L"200\n" :
						  button->isClicked(2) > 0 ? L"0\n" : 
						  button->isClicked(3) > 0 ? L"-200\n" : L"-500\n" );
				text += (inIfBlock ? L"\t\t" : L"\t");
				text += L"motor.right.target = ";
				text += ( button->isClicked(5) > 0 ? L"500\n" : 
						  button->isClicked(6) > 0 ? L"200\n" :
						  button->isClicked(7) > 0 ? L"0\n" : 
						  button->isClicked(8) > 0 ? L"-200\n" : L"-500\n" );
				break;
			case THYMIO_COLOR_IR:
				text += L"call leds.top(";
				text += (button->isClicked(0) > 0 ? L"0," : button->isClicked(1) > 0 ? L"16," : L"32,");
				text += (button->isClicked(3) > 0 ? L"0," : button->isClicked(4) > 0 ? L"16," : L"32,");
				text += (button->isClicked(6) > 0 ? L"0)\n" : button->isClicked(7) > 0 ? L"16)\n" : L"32)\n");				
				break;
			case THYMIO_CIRCLE_IR:
				text += L"call leds.circle(";
				for(int i=0; i < button->size(); ++i)
				{
					text += toWstring((32*button->isClicked(i))/(button->getNumStates()-1)); 
					text += L",";//(button->isClicked(i)? L"32," : L"0,");
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
//			case THYMIO_RESET_IR:
//				text += L"motor.left.target = 0\n";
//				if( inIfBlock )
//				{
//					text += L"\t\tmotor.right.target = 0\n";
//					text += L"\t\tcall leds.top(0,0,0)\n";
//					text += L"\t\tcall leds.circle(0,0,0,0,0,0,0,0)\n";
//					text += L"\t\tcall sound.system(-1)\n";
//				}
//				else
//				{
//					text += L"\tmotor.right.target = 0\n";
//					text += L"\tcall leds.top(0,0,0)\n";
//					text += L"\tcall leds.circle(0,0,0,0,0,0,0,0)\n";
//					text += L"\tcall sound.system(-1)\n";
//				}
//				break;
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
		
//		if( generatedCode.empty() )
//			generatedCode.push_back(L"onevent vplthymioreset\n\tmotor.right.target = 0\n\tcall leds.top(0,0,0)\n\tcall leds.circle(0,0,0,0,0,0,0,0)\n\tcall sound.system(-1)\n");
		
		ThymioIRButtonName name = buttonSet->getEventButton()->getName();
		int block = editor[name];

		if( block < 0 )
		{
			block = generatedCode.size();
			
			switch(name) 
			{
			case THYMIO_BUTTONS_IR:
				generatedCode.push_back(L"onevent buttons\n");
				break;
			case THYMIO_PROX_IR:
				if( editor[THYMIO_PROX_GROUND_IR] < 0 )
					generatedCode.push_back(L"onevent prox\n");
				else
					block = editor[THYMIO_PROX_GROUND_IR];
				break;
			case THYMIO_PROX_GROUND_IR:
				if( editor[THYMIO_PROX_IR] < 0 )
					generatedCode.push_back(L"onevent prox\n");
				else
					block = editor[THYMIO_PROX_IR];
				break;
			case THYMIO_TAP_IR:
				generatedCode.push_back(L"onevent tap\n");
				break;
			case THYMIO_CLAP_IR:
				generatedCode.insert(generatedCode.begin(),L"mic.threshold = 250\n");
				generatedCode.push_back(L"onevent mic\n");
				block += 1;
				for(map<ThymioIRButtonName, int>::iterator itr = editor.begin();
					itr != editor.end(); ++itr)
					if( itr->second >= 0 )
						itr->second += 1;
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
