#include <sstream>
#include <QtGlobal>
#include <QDebug>
#include <iostream>
#include "Compiler.h"
#include "Block.h"
#include "EventActionsSet.h"

namespace Aseba { namespace ThymioVPL
{
	using namespace std;
	
	static wstring toWstring(int val)
	{
		wstringstream streamVal;
		streamVal << val;
		
		return streamVal.str();
	}

	//////
	
	//! Create a compiler ready to be used
	Compiler::CodeGenerator::CodeGenerator() : 
		advancedMode(false),
		useSound(false),
		useTimer(false),
		useMicrophone(false),
		inIfBlock(false)
	{
		initEventToCodePosMap();
	}
	
	//! reset the compiler to its initial state, for advanced or basic mode
	void Compiler::CodeGenerator::reset(bool advanced) 
	{
		eventToCodePosMap.clear();
		initEventToCodePosMap();
		
		generatedCode.clear();
		setToCodeIdMap.clear();
		
		advancedMode = advanced;
		useSound = false;
		useTimer= false;
		useMicrophone = false;
		inIfBlock = false;
	}
	
	//! Initialize the event to code position map
	void Compiler::CodeGenerator::initEventToCodePosMap() 
	{
		eventToCodePosMap["button"] = qMakePair(-1,0);
		eventToCodePosMap["prox"] = qMakePair(-1,0);
		eventToCodePosMap["tap"] = qMakePair(-1,0);
		eventToCodePosMap["clap"] = qMakePair(-1,0);
		eventToCodePosMap["timeout"] = qMakePair(-1,0);
	}
	
	//! Add initialization code after all pairs have been parsed
	void Compiler::CodeGenerator::addInitialisationCode()
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
	
	//! Generate code for an event-actions set
	void Compiler::CodeGenerator::visit(const EventActionsSet& eventActionsSet)
	{
		// action and event name
		const QString& eventName(eventActionsSet.getEventBlock()->getName());
		QString lookupEventName;
		if (eventName == "proxground")
			lookupEventName = "prox";
		else
			lookupEventName = eventName;
		
		// lookup corresponding block
		int block = eventToCodePosMap[lookupEventName].first;
		int size = eventToCodePosMap[lookupEventName].second;
		unsigned currentBlock(0);
		
		// if block does not exist, create it for the corresponding event and set the usage flag
		if (block < 0)
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
				// internal compiler error, invalid event found
				abort();
			}
			
			// if in advanced mode, copy setting of state
			if (eventActionsSet.isAdvanced())
			{
				generatedCode.push_back(
					L"\n\tcall math.copy(state, new_state)"
					L"\n\tcallsub display_state\n"
				);
				eventToCodePosMap[lookupEventName] = qMakePair(block, size+1);
			}
			else
				eventToCodePosMap[lookupEventName] = qMakePair(block, size);

			// prepare to add the new code block
			currentBlock = block + size;
		}
		else
		{
			// get current block, insert and update map
			currentBlock = (eventActionsSet.isAdvanced() ? block + size : block + size + 1);
			eventToCodePosMap[lookupEventName].second++;
			for (EventToCodePosMap::iterator itr(eventToCodePosMap.begin()); itr != eventToCodePosMap.end(); ++itr)
				if (itr->first > block)
					itr->first++;
			
			// if non-conditional code
			if (!eventActionsSet.isAdvanced() && !eventActionsSet.getEventBlock()->isAnyValueSet())
			{
				// find where "if" block starts for the current event
				for (unsigned int i=0; i<setToCodeIdMap.size(); i++)
					if (setToCodeIdMap[i] >= block && setToCodeIdMap[i] < (int)currentBlock &&
						generatedCode[setToCodeIdMap[i]].find(L"if ") != wstring::npos)
						currentBlock = setToCodeIdMap[i];
			}
			
			// make "space" for this block
			for (unsigned int i=0; i<setToCodeIdMap.size(); i++)
				if(setToCodeIdMap[i] >= (int)currentBlock)
					setToCodeIdMap[i]++;
		}
		
		// add the code block
		Q_ASSERT(currentBlock <= generatedCode.size());
		generatedCode.insert(generatedCode.begin() + currentBlock, L"");
		setToCodeIdMap.push_back(currentBlock);
		
		// add event and actions details
		visitEventAndStateFilter(eventActionsSet.getEventBlock(), eventActionsSet.getStateFilterBlock(), currentBlock);
		for (int i=0; i<eventActionsSet.actionBlocksCount(); ++i)
			visitAction(eventActionsSet.getActionBlock(i), currentBlock);
		
		// possibly close condition and add code
		generatedCode[currentBlock].append(inIfBlock ? L"\tend\n" : L"");
		inIfBlock = false;
	}

	void Compiler::CodeGenerator::visitEventAndStateFilter(const Block* block, const Block* stateFilterBlock, unsigned currentBlock)
	{
		// generate event code
		wstring text;
		if (block->isAnyValueSet())
		{
			text = L"\tif ";
			if (block->getName() == "button")
			{
				text += visitEventArrowButtons(block);
			}
			else if (block->getName() == "prox")
			{
				text += visitEventProx(block);
			}
			else if (block->getName() == "proxground")
			{
				text += visitEventProxGround(block);
			}
			else
			{
				// internal compiler error
				abort();
			}
			
			// if set, add state filter tests
			if (stateFilterBlock)
			{
				for (unsigned i=0; i<stateFilterBlock->valuesCount(); ++i)
				{
					switch (stateFilterBlock->getValue(i))
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
			// if block does not generate a test, see if state filter does
			if (stateFilterBlock && stateFilterBlock->isAnyValueSet())
			{
				text += L"\tif ";
				bool first(true);
				for (unsigned i=0; i<stateFilterBlock->valuesCount(); ++i)
				{
					switch (stateFilterBlock->getValue(i))
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
	
	wstring Compiler::CodeGenerator::visitEventArrowButtons(const Block* block)
	{
		wstring text;
		bool first(true);
		for (unsigned i=0; i<block->valuesCount(); ++i)
		{
			if (block->getValue(i) > 0)
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
	
	wstring Compiler::CodeGenerator::visitEventProx(const Block* block)
	{
		wstring text;
		bool first(true);
		const unsigned slidersIndex(block->valuesCount()-2);
		for(unsigned i=0; i<block->valuesCount(); ++i)
		{
			if (block->getValue(i) == 1)
			{
				text += (first ? L"": L" and ");
				text += L"prox.horizontal[";
				text += toWstring(i);
				text += L"] < " + toWstring(block->getValue(slidersIndex));
				first = false;
			} 
			else if(block->getValue(i) == 2)
			{
				text += (first ? L"": L" and ");
				text += L"prox.horizontal[";
				text += toWstring(i);
				text += L"] > " + toWstring(block->getValue(slidersIndex+1));
				first = false;
			} 
		}
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitEventProxGround(const Block* block)
	{
		wstring text;
		bool first(true);
		const unsigned slidersIndex(block->valuesCount()-2);
		for(unsigned i=0; i<slidersIndex; ++i)
		{
			if (block->getValue(i) == 1)
			{
				text += (first ? L"": L" and ");
				text += L"prox.ground.delta[";
				text += toWstring(i);
				text += L"] < " + toWstring(block->getValue(slidersIndex));
				first = false;
			} 
			else if (block->getValue(i) == 2)
			{
				text += (first ? L"": L" and ");
				text += L"prox.ground.delta[";
				text += toWstring(i);
				text += L"] > " + toWstring(block->getValue(slidersIndex+1));
				first = false;
			} 
		}
		return text;
	}
	
	//! Visit an action block, if 0 return directly
	void Compiler::CodeGenerator::visitAction(const Block* block, unsigned currentBlock)
	{
		if (!block)
			return;
		
		wstring text;
		
		if (block->getName() == "move")
		{
			text += visitActionMove(block);
		}
		else if (block->getName() == "colortop")
		{
			text += visitActionTopColor(block);
		}
		else if (block->getName() == "colorbottom")
		{
			text += visitActionBottomColor(block);
		}
		else if (block->getName() == "sound")
		{
			text += visitActionSound(block);
		}
		else if (block->getName() == "timer")
		{
			text += visitActionTimer(block);
		}
		else if (block->getName() == "setstate")
		{
			text += visitActionSetState(block);
		}
		else
		{
			// internal compiler error
			abort();
		}
		
		generatedCode[currentBlock].append(text);
	}
	
	wstring Compiler::CodeGenerator::visitActionMove(const Block* block)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"motor.left.target = ";
		text += toWstring(block->getValue(0));
		text += L"\n";
		text += indString;
		text += L"motor.right.target = ";
		text += toWstring(block->getValue(1));
		text += L"\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionTopColor(const Block* block)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"call leds.top(";
		text += toWstring(block->getValue(0));
		text += L",";
		text += toWstring(block->getValue(1));
		text += L",";
		text += toWstring(block->getValue(2));
		text += L")\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionBottomColor(const Block* block)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"call leds.bottom.left(";
		text += toWstring(block->getValue(0));
		text += L",";
		text += toWstring(block->getValue(1));
		text += L",";
		text += toWstring(block->getValue(2));
		text += L")\n";
		text += indString;
		text += L"call leds.bottom.right(";
		text += toWstring(block->getValue(0));
		text += L",";
		text += toWstring(block->getValue(1));
		text += L",";
		text += toWstring(block->getValue(2));
		text += L")\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionSound(const Block* block)
	{
		static const int noteTable[5] = { 262, 311, 370, 440, 524 };
		static const int durationTable[3] = {-1, 8, 15};
		
		useSound = true;
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"call math.copy(notes, [";
		for (unsigned i = 0; i<block->valuesCount(); ++i)
		{
			const unsigned note(block->getValue(i) & 0xff);
			text += toWstring(noteTable[note]);
			if (i+1 != block->valuesCount())
				text += L", ";
		}
		text += L"])\n";
		text += indString;
		// durations
		text += L"call math.copy(durations, [";
		for (unsigned i = 0; i<block->valuesCount(); ++i)
		{
			const unsigned duration((block->getValue(i)>>8) & 0xff);
			text += toWstring(durationTable[duration]);
			if (i+1 != block->valuesCount())
				text += L", ";
		}
		text += L"])\n";
		text += indString;
		text += L"call sound.freq(notes[0], durations[0])\n";
		text += indString;
		text += L"note_index = 1\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionTimer(const Block* block)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		text += indString;
		text += L"timer.period[0] = "+ toWstring(block->getValue(0)) + L"\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionSetState(const Block* block)
	{
		wstring text;
		const wstring indString(inIfBlock ? L"\t\t" : L"\t");
		for(unsigned i=0; i<block->valuesCount(); ++i)
		{
			switch (block->getValue(i))
			{
				case 1: text += indString + L"new_state[" + toWstring(i) + L"] = 1\n"; break;
				case 2: text += indString + L"new_state[" + toWstring(i) + L"] = 0\n"; break;
				default: break; // do nothing
			}
		}
		return text;
	}

} } // namespace ThymioVPL / namespace Aseba
