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
		buttonToCodeMap()
	{
		reset();
		
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
		editor[THYMIO_BUTTONS_IR] = make_pair(-1,0);
		editor[THYMIO_PROX_IR] = make_pair(-1,0);
		editor[THYMIO_TAP_IR] = make_pair(-1,0);
		editor[THYMIO_CLAP_IR] = make_pair(-1,0);
		generatedCode.clear();
		inIfBlock = false;
		buttonToCodeMap.clear();
	}	

	int ThymioIRCodeGenerator::buttonToCode(int id) const
	{
		if( errorCode != THYMIO_NO_ERROR )
			return -1;

		return (id < (int)buttonToCodeMap.size() ? buttonToCodeMap[id] : -1);
	}

	void ThymioIRCodeGenerator::visit(ThymioIRButton *button)
	{
		if( button == 0 ) return;
		
		wstring text;

		if( button->isEventButton() )
		{
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
					
				generatedCode[currentBlock] = text;
			}
			else 
			{
				if( button->getMemoryState() >= 0 )
				{
					text += L"\tif state == ";
					text += toWstring(button->getMemoryState());
					text += L" then\n";
					
					generatedCode[currentBlock] = text;
					inIfBlock = true;
				}
				else
					inIfBlock = false;
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
		
			generatedCode[currentBlock].append(text);
		}
	
	}

	void ThymioIRCodeGenerator::visit(ThymioIRButtonSet *buttonSet)
	{
		if( !buttonSet->hasEventButton() || !buttonSet->hasActionButton() ) 
		{
			buttonToCodeMap.push_back(-1);
			return;
		}
		
		errorCode = THYMIO_NO_ERROR;
		
		if( generatedCode.empty() )
		{
			if( buttonSet->getEventButton()->getMemoryState() >= 0 )
				generatedCode.push_back(L"var state = 0\nvar new_state = 0\n");			
			generatedCode.push_back(L"call sound.system(-1)\ncall leds.temperature(0,0)\ncall leds.top(0,0,0)\ncall leds.circle(0,0,0,0,0,0,0,0)\n");
		}
		
		ThymioIRButtonName name = buttonSet->getEventButton()->getName();
		if( name == THYMIO_PROX_GROUND_IR )
			name = THYMIO_PROX_IR;
		
		int block = editor[name].first;
		int size = editor[name].second;
		bool mflag = (buttonSet->getEventButton()->getMemoryState() >= 0 ? true : false);
			
		if( block < 0 )
		{
			block = generatedCode.size();
			
			switch(name) 
			{
			case THYMIO_BUTTONS_IR:
				generatedCode.push_back(L"\nonevent buttons\n");
				size++;
				break;
			case THYMIO_PROX_IR:
				generatedCode.push_back(L"\nonevent prox\n");
				size++;
				break;
			case THYMIO_TAP_IR:
				generatedCode.push_back(L"\nonevent tap\n");
				size++;
				break;
			case THYMIO_CLAP_IR:
				if( generatedCode.empty() || generatedCode[0].find(L"var new_state = 0\n") == wstring::npos )
				{
					generatedCode.insert(generatedCode.begin(),L"mic.threshold = 250\n");
					for(map<ThymioIRButtonName, pair<int, int> >::iterator itr = editor.begin();
						itr != editor.end(); ++itr)
						if( (itr->second).first >= 0 )
							(itr->second).first++;

					for(int i=0; i<(int)buttonToCodeMap.size(); i++)
						if( buttonToCodeMap[i]>=0 ) 
							buttonToCodeMap[i]++;

					block += 1;				
				}
				else
					generatedCode[0].append(L"\nmic.threshold = 250\n");

				generatedCode.push_back(L"\nonevent mic\n");
				size++;
				break;	
			default:
				errorCode = THYMIO_INVALID_CODE;
				return;
				break;
			}

			if(mflag)
			{
				generatedCode.push_back(L"\n\tstate = new_state\n\tcall leds.temperature(((state>>0) & 1)*10+((state>>1) & 1)*22,((state>>2) & 1)*10+((state>>3) & 1)*22)\n");
				editor[name] = make_pair(block, size+1);
			}
			else
				editor[name] = make_pair(block, size);

			currentBlock = block + size;
			generatedCode.insert(generatedCode.begin() + currentBlock, L"");
			buttonToCodeMap.push_back(currentBlock);
		}
		else
		{
			currentBlock = (mflag ? block + size : block + size + 1);
			editor[name].second++;
			for(map<ThymioIRButtonName, pair<int, int> >::iterator itr = editor.begin(); itr != editor.end(); ++itr)
				if( (itr->second).first > block )
					(itr->second).first++;

			if( !mflag && !buttonSet->getEventButton()->isSet() )
			{
				// find where "if" block starts for the current event
				for(unsigned int i=0; i<buttonToCodeMap.size(); i++)
					if( buttonToCodeMap[i] >= block && buttonToCodeMap[i] < currentBlock &&
						generatedCode[buttonToCodeMap[i]].find(L"if ") != wstring::npos )				
						currentBlock = buttonToCodeMap[i];
			}
			
			for( unsigned int i=0; i<buttonToCodeMap.size(); i++)
				if(buttonToCodeMap[i] >= currentBlock)
					buttonToCodeMap[i]++;
				
			generatedCode.insert(generatedCode.begin() + currentBlock, L"");
			buttonToCodeMap.push_back(currentBlock);
		}

		
		visit(buttonSet->getEventButton());
		visit(buttonSet->getActionButton());
	}

}; // Aseba
