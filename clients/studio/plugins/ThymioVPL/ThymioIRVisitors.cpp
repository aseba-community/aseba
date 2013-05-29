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
		colorTopHash.clear();
		colorBottomHash.clear();
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
		case THYMIO_COLOR_TOP_IR:
			currentHash = &colorTopHash;
			tempHash = colorTopHash;
			break;
		case THYMIO_COLOR_BOTTOM_IR:
			currentHash = &colorBottomHash;
			tempHash = colorBottomHash;
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

		wstring buttonname = button->getBasename() + L"_" + toWstring(button->getStateFilter());;
		for(int i=0; i<button->valuesCount(); i++)
		{	
			if( button->getValue(i) > 0 )
			{
				buttonname += L"_";
				buttonname += toWstring(i);
				buttonname += L"_";
				buttonname += toWstring(button->getValue(i));
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
		advancedMode(false),
		useMicrophone(false),
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
		advancedMode = false;
		useMicrophone = false;
		inIfBlock = false;
		buttonToCodeMap.clear();
	}
	
	void ThymioIRCodeGenerator::addInitialisation()
	{
		// if no code, no need for initialisation
		if (generatedCode.empty())
			return;
		
		// build initialisation code
		wstring text;
		if (advancedMode)
		{
			text += L"var state[4] = [0,0,0,0]\n";
			text += L"var new_state[4] = [0,0,0,0]\n";
			text += L"\n";
		}
		if (useMicrophone)
		{
			text += L"mic.threshold = 250\n";
		}
		text += L"call sound.system(-1)\n";
		text += L"call leds.top(0,0,0)\n";
		text += L"call leds.bottom.left(0,0,0)\n";
		text += L"call leds.bottom.right(0,0,0)\n";
		text += L"call leds.circle(0,0,0,0,0,0,0,0)\n";
		if (advancedMode)
		{
			text += L"\nsub display_state\n";
			text += L"\tcall leds.circle(0,state[1]*32,0,state[3]*32,0,state[2]*32,0,state[0]*32)\n";
		}
		
		generatedCode[0] = text + generatedCode[0];
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
						if(button->getValue(i) > 0)
						{
							text += (flag ? L" and " : L"");
							text += L"button.";
							text += directions[i];
							text += L" == 1";
							flag = true;
						}
					}
					break;
				case THYMIO_PROX_IR:
					for(int i=0; i<button->valuesCount(); ++i)
					{
						if(button->getValue(i) == 1)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.horizontal[";
							text += toWstring(i);
							text += L"] < 400";
							flag = true;
						} 
						else if(button->getValue(i) == 2)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.horizontal[";
							text += toWstring(i);
							text += L"] > 500";
							flag = true;
						} 
					}
					break;
				case THYMIO_PROX_GROUND_IR:
					for(int i=0; i<button->valuesCount(); ++i)
					{
						if(button->getValue(i) == 1)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.ground.reflected[";
							text += toWstring(i);
							text += L"] < 150";
							flag = true;
						}
						else if(button->getValue(i) == 2)
						{
							text += (flag ? L" and " : L"");
							text += L"prox.ground.reflected[";
							text += toWstring(i);
							text += L"] > 300";
							flag = true;
						}
					}
					break;
				case THYMIO_TAP_IR:
				case THYMIO_CLAP_IR:
				default:
					errorCode = THYMIO_INVALID_CODE;
					break;
				}
				
				if( button->getStateFilter() > 0 )
				{
					for (int i=0; i<4; ++i)
					{
						switch (button->getStateFilter(i))
						{
							case 1: text += L" and state[" + toWstring(i) + L"] == 1"; break;
							case 2: text += L" and state[" + toWstring(i) + L"] == 0"; break;
							default: break;
						}
					}
				}
				
				text += L" then\n";
				inIfBlock = true;
				
				generatedCode[currentBlock] = text;
			}
			else 
			{
				if( button->getStateFilter() > 0 )
				{
					text += L"\tif ";
					bool first(true);
					for (int i=0; i<4; ++i)
					{
						switch (button->getStateFilter(i))
						{
							case 1:
								if (!first)
									text += L" and ";
								text += L"state[" + toWstring(i) + L"] == 1";
								first = false;
							break;
							case 2:
								if (!first)
									text += L" and ";
								text += L"state[" + toWstring(i) + L"] == 0";
								first = false;
							break;
							default:
							break;
						}
					}
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
			const wstring indString(inIfBlock ? L"\t\t" : L"\t");
			switch( button->getName() )
			{
			case THYMIO_MOVE_IR:
				text += indString;
				text += L"motor.left.target = ";
				text += toWstring(button->getValue(0));
				text += L"\n";
				text += indString;
				text += L"motor.right.target = ";
				text += toWstring(button->getValue(1));
				text += L"\n";
				break;
			case THYMIO_COLOR_TOP_IR:
				text += indString;
				text += L"call leds.top(";
				text += toWstring(button->getValue(0));
				text += L",";
				text += toWstring(button->getValue(1));
				text += L",";
				text += toWstring(button->getValue(2));
				text += L")\n";
				break;
			case THYMIO_COLOR_BOTTOM_IR:
				text += indString;
				text += L"call leds.bottom.left(";
				text += toWstring(button->getValue(0));
				text += L",";
				text += toWstring(button->getValue(1));
				text += L",";
				text += toWstring(button->getValue(2));
				text += L")\n";
				text += indString;
				text += L"call leds.bottom.right(";
				text += toWstring(button->getValue(0));
				text += L",";
				text += toWstring(button->getValue(1));
				text += L",";
				text += toWstring(button->getValue(2));
				text += L")\n";
				break;
			case THYMIO_SOUND_IR:
				text += indString;
				if( button->getValue(0) ) // friendly
					text += L"call sound.system(7)\n";
				else if( button->getValue(1) ) // okay
					text += L"call sound.system(6)\n";
				else
					text += L"call sound.system(4)\n"; // scared
				break;
			case THYMIO_MEMORY_IR:
				for(int i=0; i<button->valuesCount(); ++i)
				{
					switch (button->getValue(i))
					{
						case 1: text += indString + L"new_state[" + toWstring(i) + L"] = 1\n"; break;
						case 2: text += indString + L"new_state[" + toWstring(i) + L"] = 0\n"; break;
						default: break; // do nothing
					}
				}
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
		
		advancedMode = advancedMode || (buttonSet->getEventButton()->getStateFilter() >= 0);
		errorCode = THYMIO_NO_ERROR;
		
		ThymioIRButtonName name = buttonSet->getEventButton()->getName();
		if( name == THYMIO_PROX_GROUND_IR )
			name = THYMIO_PROX_IR;
		
		int block = editor[name].first;
		int size = editor[name].second;
		bool mflag = (buttonSet->getEventButton()->getStateFilter() >= 0 ? true : false);
			
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
				useMicrophone = true;
				generatedCode.push_back(L"\nonevent mic\n");
				size++;
				break;
			default:
				errorCode = THYMIO_INVALID_CODE;
				return;
				break;
			}

			if (mflag)
			{
				generatedCode.push_back(
					L"\n\tcall math.copy(state, new_state)"
					L"\n\tcallsub display_state\n"
				);
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
