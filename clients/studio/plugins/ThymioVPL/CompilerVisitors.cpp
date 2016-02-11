/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <QtGlobal>
#include <QDebug>
#include "Compiler.h"
#include "Block.h"
#include "EventActionsSet.h"
#include "EventBlocks.h"

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
		useAccAngle(false),
		inWhenBlock(false),
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
		useAccAngle = false;
		inWhenBlock = false;
		inIfBlock = false;
	}
	
	//! Initialize the event to code position map
	void Compiler::CodeGenerator::initEventToCodePosMap() 
	{
		eventToCodePosMap["button"] = qMakePair(-1,0);
		eventToCodePosMap["prox"] = qMakePair(-1,0);
		eventToCodePosMap["tap"] = qMakePair(-1,0);
		eventToCodePosMap["acc"] = qMakePair(-1,0);
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
			text += L"# variables for state\n";
			text += L"var state[4] = [0,0,0,0]\n";
			text += L"var new_state[4] = [0,0,0,0]\n";
			text += L"\n";
		}
		if (useAccAngle)
		{
			text += L"# variable for angle\n";
			text += L"var angle\n";
		}
		if (useSound)
		{
			text += L"# variables for notes\n";
			text += L"var notes[6]\n";
			text += L"var durations[6]\n";
			text += L"var note_index = 6\n";
			text += L"var note_count = 6\n";
			text += L"var wave[142]\n";
			text += L"var i\n";
			text += L"var wave_phase\n";
			text += L"var wave_intensity\n";
			text += L"\n# compute a sinus wave for sound\n";
			text += L"for i in 0:141 do\n";
			text += L"\twave_phase = (i-70)*468\n";
			text += L"\tcall math.cos(wave_intensity, wave_phase)\n";
			text += L"\twave[i] = wave_intensity/256\n";
			text += L"end\n";
			text += L"call sound.wave(wave)\n";
		}
		if (useTimer)
		{
			text += L"# stop timer 0\n";
			text += L"timer.period[0] = 0\n";
		}
		if (useMicrophone)
		{
			text += L"# setup threshold for detecting claps\n";
			text += L"mic.threshold = 250\n";
		}
		text += L"# reset outputs\n";
		text += L"call sound.system(-1)\n";
		text += L"call leds.top(0,0,0)\n";
		text += L"call leds.bottom.left(0,0,0)\n";
		text += L"call leds.bottom.right(0,0,0)\n";
		text += L"call leds.circle(0,0,0,0,0,0,0,0)\n";
		if (advancedMode)
		{
			text += L"\n# subroutine to display the current state\n";
			text += L"sub display_state\n";
			text += L"\tcall leds.circle(0,state[1]*32,0,state[3]*32,0,state[2]*32,0,state[0]*32)\n";
		}
		if (useSound)
		{
			text += L"\n# when a note is finished, play the next note\n";
			text += L"onevent sound.finished\n";
			text += L"\tif note_index != note_count then\n";
			text += L"\t\tcall sound.freq(notes[note_index], durations[note_index])\n";
			text += L"\t\tnote_index += 1\n";
			text += L"\tend\n";
		}
		generatedCode[0] = text + generatedCode[0];
	}
	
	wstring Compiler::CodeGenerator::indentText() const
	{
		wstring text(L"\t");
		if (inWhenBlock)
			text += L"\t";
		if (inIfBlock)
			text += L"\t";
		return text;
	}
	
	//! Generate code for an event-actions set
	void Compiler::CodeGenerator::visit(const EventActionsSet& eventActionsSet, bool debugLog)
	{
		// action and event name
		const QString& eventName(eventActionsSet.getEventBlock()->getName());
		QString lookupEventName;
		if (eventName == "proxground")
			lookupEventName = "prox";
		else
			lookupEventName = eventName;
		
		// the first mode of the acc event is just a tap
		if ((eventName == "acc") && (eventActionsSet.getEventBlock()->getValue(0) == AccEventBlock::MODE_TAP))
			lookupEventName = "tap";
		
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
			else if (lookupEventName == "acc")
			{
				useAccAngle = true;
				generatedCode.push_back(L"\nonevent acc\n");
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
			
			// FIXME: do this in a second pass, once we have written all the code for this event, and only if the state is actually assigned
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
			if (!eventActionsSet.isAnyAdvancedFeature() && !eventActionsSet.getEventBlock()->isAnyValueSet())
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
		
		// possibly add the debug event
		visitExecFeedback(eventActionsSet, currentBlock);
		if (debugLog)
			visitDebugLog(eventActionsSet, currentBlock);
		
		// possibly close condition and add code
		visitEndOfLine(currentBlock);
	}
	
	//! Generate the debug log event for this set
	void Compiler::CodeGenerator::visitExecFeedback(const EventActionsSet& eventActionsSet, unsigned currentBlock)
	{
		wstring text(indentText());
		text += L"emit pair_run " + toWstring(eventActionsSet.getRow()) + L"\n";
		generatedCode[currentBlock].append(text);
	}
	
	//! Generate the debug log event for this set
	void Compiler::CodeGenerator::visitDebugLog(const EventActionsSet& eventActionsSet, unsigned currentBlock)
	{
		wstring text(indentText());
		text += L"_emit debug_log [";
		
		wstringstream ostr;
		ostr << hex << showbase;
		const QVector<quint16> compressedContent(eventActionsSet.getContentCompressed());
        copy(compressedContent.begin(), compressedContent.end(), ostream_iterator<quint16, wchar_t>(ostr, L", "));
		text += ostr.str();
		text.erase(text.size() - 2);
		
		text += L"]\n";
		generatedCode[currentBlock].append(text);
	}

	void Compiler::CodeGenerator::visitEventAndStateFilter(const Block* block, const Block* stateFilterBlock, unsigned currentBlock)
	{
		// generate event code
		wstring text;
		
		// if block generates a test
		if (block->isAnyValueSet())
		{
			// pre-test code
			if (block->getName() == "acc")
			{
				text += visitEventAccPre(block);
			}
			
			// test code
			text += L"\twhen ";
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
			else if (block->getName() == "acc")
			{
				text += visitEventAcc(block);
			}
			else
			{
				// internal compiler error
				abort();
			}
			
			text += L" do\n";
			
			inWhenBlock = true;
		}
		
		// if state filter does generate a test
		if (stateFilterBlock && stateFilterBlock->isAnyValueSet())
		{
			if (inWhenBlock)
				text += L"\t";
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
			
			inIfBlock = true;
		}
		
		generatedCode[currentBlock] = text;
	}
	
	//! End the generated code of a VPL line, close whens and ifs
	void Compiler::CodeGenerator::visitEndOfLine(unsigned currentBlock)
	{
		if (inIfBlock)
		{
			if (inWhenBlock)
				generatedCode[currentBlock].append(L"\t");
			generatedCode[currentBlock].append(L"\tend\n");
			inIfBlock = false;
		}
		if (inWhenBlock)
		{
			generatedCode[currentBlock].append(L"\tend\n");
			inWhenBlock = false;
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
				text += L"] >= " + toWstring(block->getValue(slidersIndex+1));
				first = false;
			} 
			else if(block->getValue(i) == 2)
			{
				text += (first ? L"": L" and ");
				text += L"prox.horizontal[";
				text += toWstring(i);
				text += L"] <= " + toWstring(block->getValue(slidersIndex));
				first = false;
			}
			else if (block->getValue(i) == 3)
			{
				text += (first ? L"": L" and ");
				text += L"prox.horizontal[";
				text += toWstring(i);
				text += L"] >= " + toWstring(block->getValue(slidersIndex));
				text += L" and prox.horizontal[";
				text += toWstring(i);
				text += L"] <= " + toWstring(block->getValue(slidersIndex+1));
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
				text += L"] >= " + toWstring(block->getValue(slidersIndex+1));
				first = false;
			} 
			else if (block->getValue(i) == 2)
			{
				text += (first ? L"": L" and ");
				text += L"prox.ground.delta[";
				text += toWstring(i);
				text += L"] <= " + toWstring(block->getValue(slidersIndex));
				first = false;
			}
			else if (block->getValue(i) == 3)
			{
				text += (first ? L"": L" and ");
				text += L"prox.ground.delta[";
				text += toWstring(i);
				text += L"] >= " + toWstring(block->getValue(slidersIndex));
				text += L" and prox.ground.delta[";
				text += toWstring(i);
				text += L"] <= " + toWstring(block->getValue(slidersIndex+1));
				first = false;
			}
		}
		return text;
	}
	
	//! Pretest, compute angle
	wstring Compiler::CodeGenerator::visitEventAccPre(const Block* block)
	{
		if (block->getValue(0) == AccEventBlock::MODE_PITCH)
			return L"\tcall math.atan2(angle, acc[1], acc[2])\n";
		else if (block->getValue(0) == AccEventBlock::MODE_ROLL)
			return L"\tcall math.atan2(angle, acc[0], acc[2])\n";
		else
			abort();
	}
	
	//! Test, use computed angle
	wstring Compiler::CodeGenerator::visitEventAcc(const Block* block)
	{
		int testValue(0);
		if (block->getValue(0) == AccEventBlock::MODE_PITCH)
			testValue = block->getValue(1);
		else if (block->getValue(0) == AccEventBlock::MODE_ROLL)
			testValue = -block->getValue(1);
		else
			abort();
		
		const int middleValue(testValue * 16384 / AccEventBlock::resolution);
		const int lowBound(middleValue - 8192 / AccEventBlock::resolution);
		const int highBound(middleValue + 8192 / AccEventBlock::resolution);
		
		wstring text;
		if (testValue != -AccEventBlock::resolution)
			text += L"angle > " + toWstring(lowBound);
		if (!text.empty() && (testValue != AccEventBlock::resolution))
			text += L" and ";
		if (testValue != AccEventBlock::resolution)
			text += L"angle < " + toWstring(highBound);
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
		const wstring indString(indentText());
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
		const wstring indString(indentText());
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
		const wstring indString(indentText());
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
		static const int noteTable[6] = { 262, 311, 370, 440, 524, 0 };
		static const int durationTable[3] = {-1, 7, 14};
		
		// find last non-silent note
		unsigned noteCount(block->valuesCount());
		assert(noteCount > 0);
		while ((noteCount > 0) && ((block->getValue(noteCount-1) & 0xff) == 5))
			--noteCount;
		
		// if there is no note, return
		if (noteCount == 0)
			return indentText() + L"# zero notes in sound block\n";
		
		// generate code for notes
		wstring notesCopyText;
		wstring durationsCopyText;
		unsigned accumulatedDuration(0);
		unsigned activeNoteCount(0);
		for (unsigned i = 0; i<noteCount; ++i)
		{
			const unsigned note(block->getValue(i) & 0xff);
			const unsigned duration((block->getValue(i)>>8) & 0xff);
			
			if (note == 5 && i+1 < noteCount && (block->getValue(i+1) & 0xff) == 5)
			{
				// next note is silence, skip
				accumulatedDuration += durationTable[duration];
			}
			else
			{
				notesCopyText += toWstring(noteTable[note]);
				if (i+1 != noteCount)
					notesCopyText += L", ";
				durationsCopyText += toWstring(durationTable[duration]+accumulatedDuration);
				if (i+1 != noteCount)
					durationsCopyText += L", ";
				++activeNoteCount;
				accumulatedDuration = 0;
			}
		}
		
		// enable sound
		useSound = true;
		
		// prepare target string
		wstring text;
		const wstring indString(indentText());
		// notes
		text += indString;
		text += L"call math.copy(notes[0:";
		text += toWstring(activeNoteCount-1);
		text += L"], [";
		text += notesCopyText;
		text += L"])\n";
		// durations
		text += indString;
		text += L"call math.copy(durations[0:";
		text += toWstring(activeNoteCount-1);
		text += L"], [";
		text += durationsCopyText;
		text += L"])\n";
		// write variables
		text += indString;
		text += L"note_index = 1\n";
		text += indString;
		text += L"note_count = " + toWstring(activeNoteCount) + L"\n";
		// start playing
		text += indString;
		text += L"call sound.freq(notes[0], durations[0])\n";
		
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionTimer(const Block* block)
	{
		wstring text;
		const wstring indString(indentText());
		text += indString;
		text += L"timer.period[0] = "+ toWstring(block->getValue(0)) + L"\n";
		return text;
	}
	
	wstring Compiler::CodeGenerator::visitActionSetState(const Block* block)
	{
		wstring text;
		const wstring indString(indentText());
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
