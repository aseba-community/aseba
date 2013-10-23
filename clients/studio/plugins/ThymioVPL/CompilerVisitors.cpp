#include <sstream>
#include <QtGlobal>
#include <QDebug>
#include <iostream>
#include "Compiler.h"
#include "Card.h"
#include "EventActionPair.h"

namespace Aseba { namespace ThymioVPL
{
	using namespace std;
	
	// support functions //
	
	wstring toWstring(int val)
	{
		wstringstream streamVal;
		streamVal << val;
		
		return streamVal.str();
	}
	
	// Visitor base //
	void Compiler::Visitor::visit(const EventActionPair& p)
	{
		errorCode = NO_ERROR;
	}

	Compiler::ErrorCode Compiler::Visitor::getErrorCode() const
	{
		return errorCode;
	}
	
	bool Compiler::Visitor::isSuccessful() const
	{
		return ( errorCode == NO_ERROR ? true : false );
	}

	// Type Checker //
	void Compiler::TypeChecker::reset()
	{
		clear();
	}
	
	void Compiler::TypeChecker::clear()
	{
		moveHash.clear();
		colorTopHash.clear();
		colorBottomHash.clear();
		soundHash.clear();
		timerHash.clear();
		memoryHash.clear();
	}

	void Compiler::TypeChecker::visit(const EventActionPair& p, int& secondErrorLine)
	{
		if( !p.hasEventCard() || !p.hasActionCard() )
			return;
		
		errorCode = NO_ERROR;

		// we know that p has an action card, so we can dereference it
		multimap<wstring, const Card*> *currentHash;
		multimap<wstring, const Card*> tempHash;
		
		const Card* event(p.getEventCard());
		const Card* action(p.getActionCard());
		if (action->getName() == "move")
		{
			currentHash = &moveHash;
			tempHash = moveHash;
		}
		else if (action->getName() == "colortop")
		{
			currentHash = &colorTopHash;
			tempHash = colorTopHash;
		}
		else if (action->getName() == "colorbottom")
		{
			currentHash = &colorBottomHash;
			tempHash = colorBottomHash;
		}
		else if (action->getName() == "sound")
		{
			currentHash = &soundHash;
			tempHash = soundHash;
		}
		else if (action->getName() == "timer")
		{
			currentHash = &timerHash;
			tempHash = timerHash;
		}
		else if (action->getName() == "statefilter")
		{
			currentHash = &memoryHash;
			tempHash = memoryHash;
		}
		else
		{
			qDebug() << L"Compiler::TypeChecker::visit(): unknown card name: " << action->getName();
			return;
		}

		currentHash->clear();
	
		for(multimap<wstring, const Card*>::iterator itr = tempHash.begin();
			itr != tempHash.end(); ++itr)
		{
			if (itr->second != event)
				currentHash->insert(*itr);
		}

		wstring cardHashText(event->getName().toStdWString() + L"_" + toWstring(event->getStateFilter()));
		for(unsigned i=0; i<event->valuesCount(); i++)
		{	
			if (event->getValue(i) > 0)
			{
				cardHashText += L"_";
				cardHashText += toWstring(i);
				cardHashText += L"_";
				cardHashText += toWstring(event->getValue(i));
			}
		}

		pair< multimap<wstring, const Card*>::iterator,
			  multimap<wstring, const Card*>::iterator > ret;
		ret = currentHash->equal_range(cardHashText);

		secondErrorLine = 0;
		for (multimap<wstring, const Card*>::iterator itr(ret.first); itr != ret.second; ++itr)
		{
			if (itr->second != event)
			{
				errorCode = EVENT_REPEATED;
				break;
			}
			++secondErrorLine;
		}
		
		currentHash->insert(pair<wstring, const Card*>(cardHashText, event));
	}

	// Syntax Checker //
	void Compiler::SyntaxChecker::visit(const EventActionPair& p)
	{
		errorCode = NO_ERROR;
		
		if( !(p.hasEventCard()) )
		{
			if( p.hasActionCard() )
				errorCode = MISSING_EVENT;
		} 
		else if( !(p.hasActionCard()) )
		{
			errorCode = MISSING_ACTION;
		}
	}

	// Code Generator //
	Compiler::CodeGenerator::CodeGenerator() : 
		Compiler::Visitor(),
		advancedMode(false),
		useSound(false),
		useTimer(false),
		useMicrophone(false),
		inIfBlock(false),
		buttonToCodeMap()
	{
		reset();
	}
	
	Compiler::CodeGenerator::~CodeGenerator()
	{
		clear();
	}
	
	void Compiler::CodeGenerator::clear()
	{
		editor.clear();
		generatedCode.clear();
		inIfBlock = false;
	}
	
	void Compiler::CodeGenerator::reset() 
	{
		editor["button"] = make_pair(-1,0);
		editor["prox"] = make_pair(-1,0);
		editor["tap"] = make_pair(-1,0);
		editor["clap"] = make_pair(-1,0);
		editor["timeout"] = make_pair(-1,0);
		generatedCode.clear();
		advancedMode = false;
		useSound = false;
		useTimer= false;
		useMicrophone = false;
		inIfBlock = false;
		buttonToCodeMap.clear();
	}
	
	void Compiler::CodeGenerator::addInitialisation()
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
		if (useSound)
		{
			text += L"# variables for notes\n";
			text += L"var notes[6]\n";
			text += L"var durations[6]\n";
			text += L"var note_index = 6\n";
			text += L"var wave[142]\n";
			text += L"var i\n";
			text += L"var angle\n";
			text += L"var cos_result\n";
			text += L"\n# compute a sinus wave for sound\n";
			text += L"for i in 0:141 do\n";
			text += L"\tangle = (i-70)*468\n";
			text += L"\tcall math.cos(cos_result, angle)\n";
			text += L"\twave[i] = cos_result/256\n";
			text += L"end\n";
			text += L"call sound.wave(wave)\n";
		}
		if (useTimer)
		{
			text += L"timer.period[0] = 0\n";
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
		if (useSound)
		{
			text += L"\nonevent sound.finished\n";
			text += L"\tif note_index != 6 then\n";
			text += L"\t\tcall sound.freq(notes[note_index], durations[note_index])\n";
			text += L"\t\tnote_index += 1\n";
			text += L"\tend\n";
		}
		generatedCode[0] = text + generatedCode[0];
	}

	int Compiler::CodeGenerator::buttonToCode(int id) const
	{
		if( errorCode != NO_ERROR )
			return -1;

		return (id < (int)buttonToCodeMap.size() ? buttonToCodeMap[id] : -1);
	}
	
	void Compiler::CodeGenerator::visit(const EventActionPair& p)
	{
		if( !p.hasEventCard() || !p.hasActionCard() ) 
		{
			buttonToCodeMap.push_back(-1);
			return;
		}
		
		errorCode = NO_ERROR;
		
		// action and event name
		const QString& eventName(p.getEventCard()->getName());
		QString lookupEventName;
		if (eventName == "proxground")
			lookupEventName = "prox";
		else
			lookupEventName = eventName;
		
		int block = editor[lookupEventName].first;
		int size = editor[lookupEventName].second;
		unsigned currentBlock(0);
		const bool isStateFilter(p.getEventCard()->getStateFilter() >= 0 ? true : false);
		
		if( block < 0 )
		{
			block = generatedCode.size();
			
			if (lookupEventName == "button")
			{
				generatedCode.push_back(L"\nonevent buttons\n");
				size++;
			}
			else if (lookupEventName == "prox")
			{
				generatedCode.push_back(L"\nonevent prox\n");
				size++;
			}
			else if (lookupEventName == "tap")
			{
				generatedCode.push_back(L"\nonevent tap\n");
				size++;
			}
			else if (lookupEventName == "clap")
			{
				useMicrophone = true;
				generatedCode.push_back(L"\nonevent mic\n");
				size++;
			}
			else if (lookupEventName == "timeout")
			{
				useTimer = true;
				generatedCode.push_back(
					L"\nonevent timer0"
					L"\n\ttimer.period[0] = 0\n"
				);
				size++;
			}
			else
			{
				errorCode = INVALID_CODE;
				return;
			}

			if (isStateFilter)
			{
				generatedCode.push_back(
					L"\n\tcall math.copy(state, new_state)"
					L"\n\tcallsub display_state\n"
				);
				editor[lookupEventName] = make_pair(block, size+1);
			}
			else
				editor[lookupEventName] = make_pair(block, size);

			currentBlock = block + size;
			Q_ASSERT(currentBlock <= generatedCode.size());
			generatedCode.insert(generatedCode.begin() + currentBlock, L"");
			buttonToCodeMap.push_back(currentBlock);
		}
		else
		{
			currentBlock = (isStateFilter ? block + size : block + size + 1);
			editor[lookupEventName].second++;
			for (EventToCodePosMap::iterator itr(editor.begin()); itr != editor.end(); ++itr)
				if( (itr->second).first > block )
					(itr->second).first++;

			if( !isStateFilter && !p.getEventCard()->isAnyValueSet() )
			{
				// find where "if" block starts for the current event
				for(unsigned int i=0; i<buttonToCodeMap.size(); i++)
					if( buttonToCodeMap[i] >= block && buttonToCodeMap[i] < (int)currentBlock &&
						generatedCode[buttonToCodeMap[i]].find(L"if ") != wstring::npos )
						currentBlock = buttonToCodeMap[i];
			}
			
			for( unsigned int i=0; i<buttonToCodeMap.size(); i++)
				if(buttonToCodeMap[i] >= (int)currentBlock)
					buttonToCodeMap[i]++;
			
			Q_ASSERT(currentBlock <= generatedCode.size());
			generatedCode.insert(generatedCode.begin() + currentBlock, L"");
			buttonToCodeMap.push_back(currentBlock);
		}
		
		visitEvent(*p.getEventCard(), currentBlock);
		visitAction(*p.getActionCard(), currentBlock);
	}

	void Compiler::CodeGenerator::visitEvent(const Card& card, unsigned currentBlock)
	{
		// set the generator in advanced mode if any state filter is set
		advancedMode = advancedMode || (card.getStateFilter() >= 0);
		
		// generate event code
		wstring text;
		if (card.isAnyValueSet())
		{
			text = L"\tif ";
			if (card.getName() == "button")
			{
				text += visitEventArrowButtons(card);
			}
			else if (card.getName() == "prox")
			{
				text += visitEventProx(card);
			}
			else if (card.getName() == "proxground")
			{
				text += visitEventProxGround(card);
			}
			else
			{
				errorCode = INVALID_CODE;
			}
			
			// if set, add state filter tests
			if (card.getStateFilter() > 0)
			{
				for (unsigned i=0; i<card.stateFilterCount(); ++i)
				{
					switch (card.getStateFilter(i))
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
			// if card does not generate a test, see if state filter do
			if( card.getStateFilter() > 0 )
			{
				text += L"\tif ";
				bool first(true);
				for (unsigned i=0; i<card.stateFilterCount(); ++i)
				{
					switch (card.getStateFilter(i))
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
	
	wstring Compiler::CodeGenerator::visitEventArrowButtons(const Card& card)
	{
		wstring text;
		bool first(true);
		for (unsigned i=0; i<card.valuesCount(); ++i)
		{
			if (card.getValue(i) > 0)
			{
				static const wchar_t* directions[] = {
					L"forward",
					L"left",
					L"backward",
					L"right",
					L"center"
				};
				text += (first ? L"": L" and ");
				text += L"button.";
				text += directions[i];
				text += L" == 1";
				first = false;
			}
		}
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitEventProx(const Card& card)
	{
		wstring text;
		bool first(true);
		for(unsigned i=0; i<card.valuesCount(); ++i)
		{
			if (card.getValue(i) == 1)
			{
				text += (first ? L"": L" and ");
				text += L"prox.horizontal[";
				text += toWstring(i);
				text += L"] < 400";
				first = false;
			} 
			else if(card.getValue(i) == 2)
			{
				text += (first ? L"": L" and ");
				text += L"prox.horizontal[";
				text += toWstring(i);
				text += L"] > 500";
				first = false;
			} 
		}
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitEventProxGround(const Card& card)
	{
		wstring text;
		bool first(true);
		for(unsigned i=0; i<card.valuesCount(); ++i)
		{
			if(card.getValue(i) == 1)
			{
				text += (first ? L"": L" and ");
				text += L"prox.ground.reflected[";
				text += toWstring(i);
				text += L"] < 150";
				first = false;
			} 
			else if(card.getValue(i) == 2)
			{
				text += (first ? L"": L" and ");
				text += L"prox.ground.reflected[";
				text += toWstring(i);
				text += L"] > 300";
				first = false;
			} 
		}
		return text;
	}
	
	void Compiler::CodeGenerator::visitAction(const Card& card, unsigned currentBlock)
	{
		wstring text;
		
		if (card.getName() == "move")
		{
			text += visitActionMove(card);
		}
		else if (card.getName() == "colortop")
		{
			text += visitActionTopColor(card);
		}
		else if (card.getName() == "colorbottom")
		{
			text += visitActionBottomColor(card);
		}
		else if (card.getName() == "sound")
		{
			text += visitActionSound(card);
		}
		else if (card.getName() == "timer")
		{
			text += visitActionTimer(card);
		}
		else if (card.getName() == "statefilter")
		{
			text += visitActionStateFilter(card);
		}
		else
		{
			errorCode = INVALID_CODE;
		}
		text += (inIfBlock ? L"\tend\n" : L"");
		inIfBlock = false;
	
		generatedCode[currentBlock].append(text);
	}
	
	wstring Compiler::CodeGenerator::visitActionMove(const Card& card)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"motor.left.target = ";
		text += toWstring(card.getValue(0));
		text += L"\n";
		text += indString;
		text += L"motor.right.target = ";
		text += toWstring(card.getValue(1));
		text += L"\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionTopColor(const Card& card)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"call leds.top(";
		text += toWstring(card.getValue(0));
		text += L",";
		text += toWstring(card.getValue(1));
		text += L",";
		text += toWstring(card.getValue(2));
		text += L")\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionBottomColor(const Card& card)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"call leds.bottom.left(";
		text += toWstring(card.getValue(0));
		text += L",";
		text += toWstring(card.getValue(1));
		text += L",";
		text += toWstring(card.getValue(2));
		text += L")\n";
		text += indString;
		text += L"call leds.bottom.right(";
		text += toWstring(card.getValue(0));
		text += L",";
		text += toWstring(card.getValue(1));
		text += L",";
		text += toWstring(card.getValue(2));
		text += L")\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionSound(const Card& card)
	{
		static const int noteTable[5] = { 262, 311, 370, 440, 524 };
		static const int durationTable[3] = {-1, 8, 15};
		
		useSound = true;
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"call math.copy(notes, [";
		for (unsigned i = 0; i<card.valuesCount(); ++i)
		{
			const unsigned note(card.getValue(i) & 0xff);
			text += toWstring(noteTable[note]);
			if (i+1 != card.valuesCount())
				text += L", ";
		}
		text += L"])\n";
		text += indString;
		// durations
		text += L"call math.copy(durations, [";
		for (unsigned i = 0; i<card.valuesCount(); ++i)
		{
			const unsigned duration((card.getValue(i)>>8) & 0xff);
			text += toWstring(durationTable[duration]);
			if (i+1 != card.valuesCount())
				text += L", ";
		}
		text += L"])\n";
		text += indString;
		text += L"call sound.freq(notes[0], durations[0])\n";
		text += indString;
		text += L"note_index = 1\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionTimer(const Card& card)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"timer.period[0] = "+ toWstring(card.getValue(0)) + L"\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionStateFilter(const Card& card)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		for(unsigned i=0; i<card.valuesCount(); ++i)
		{
			switch (card.getValue(i))
			{
				case 1: text += indString + L"new_state[" + toWstring(i) + L"] = 1\n"; break;
				case 2: text += indString + L"new_state[" + toWstring(i) + L"] = 0\n"; break;
				default: break; // do nothing
			}
		}
		return text;
	}

} } // namespace ThymioVPL / namespace Aseba
