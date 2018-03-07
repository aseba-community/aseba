/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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

	void Compiler::CodeGenerator::EventHandler::generateAdditionalCode()
	{
		if (isStateSet)
			code.append(EventHandler::PairCodeAndIndex(
				L"\tcall math.copy(state, new_state)"
				L"\n\tcallsub display_state\n\n",
			-1));
	}

	void Compiler::CodeGenerator::ButtonEventHandler::generateAdditionalCode()
	{
		EventHandler::generateAdditionalCode();
		code.prepend(EventHandler::PairCodeAndIndex(L"\nonevent buttons\n", -1));
	}

	void Compiler::CodeGenerator::ProxEventHandler::generateAdditionalCode()
	{
		EventHandler::generateAdditionalCode();
		code.prepend(EventHandler::PairCodeAndIndex(L"\nonevent prox\n", -1));
	}

	void Compiler::CodeGenerator::TapEventHandler::generateAdditionalCode()
	{
		EventHandler::generateAdditionalCode();
		code.prepend(EventHandler::PairCodeAndIndex(L"\nonevent tap\n", -1));
	}

	void Compiler::CodeGenerator::AccEventHandler::generateAdditionalCode()
	{
		EventHandler::generateAdditionalCode();
		code.prepend(EventHandler::PairCodeAndIndex(L"\nonevent acc\n", -1));
	}

	void Compiler::CodeGenerator::ClapEventHandler::generateAdditionalCode()
	{
		EventHandler::generateAdditionalCode();
		code.prepend(EventHandler::PairCodeAndIndex(L"\nonevent mic\n", -1));
	}

	void Compiler::CodeGenerator::TimeoutEventHandler::generateAdditionalCode()
	{
		EventHandler::generateAdditionalCode();
		code.prepend(EventHandler::PairCodeAndIndex(
			L"\nonevent timer0\n"
			L"\ttimer.period[0] = 0\n",
		-1));
	}

	void Compiler::CodeGenerator::RemoteControlEventHandler::generateAdditionalCode()
	{
		EventHandler::generateAdditionalCode();
		code.prepend(EventHandler::PairCodeAndIndex(L"\nonevent rc5\n", -1));
	};

	//////

	//! Create a compiler ready to be used
	Compiler::CodeGenerator::CodeGenerator() : 
		advancedMode(false),
		useSound(false),
		inWhenBlock(false),
		inIfBlock(false)
	{
		eventHandlers["button"] = new ButtonEventHandler();
		eventHandlers["prox"] = new ProxEventHandler();
		eventHandlers["tap"] = new TapEventHandler();
		eventHandlers["acc"] = new AccEventHandler();
		eventHandlers["clap"] = new ClapEventHandler();
		eventHandlers["timeout"] = new TimeoutEventHandler();
		eventHandlers["rc5"] = new RemoteControlEventHandler();
	}

	Compiler::CodeGenerator::~CodeGenerator()
	{
		for (EventHandlers::iterator it(eventHandlers.begin()); it != eventHandlers.end(); ++it)
			delete *it;
	}

	//! reset the compiler to its initial state, for advanced or basic mode
	void Compiler::CodeGenerator::reset(bool advanced) 
	{
		clearEventHandlers();

		generatedCode.clear();
		setToCodeIdMap.clear();

		advancedMode = advanced;
		useSound = false;
		inWhenBlock = false;
		inIfBlock = false;
	}

	//! Initialize the map of events and their code
	void Compiler::CodeGenerator::clearEventHandlers() 
	{
		for (EventHandlers::iterator it(eventHandlers.begin()); it != eventHandlers.end(); ++it)
			(*it)->clear();
	}

	//! Link all event handles together into their
	void Compiler::CodeGenerator::link()
	{
		// complete the code of the different event handlers
		bool wasCode(false);
		for (EventHandlers::iterator it(eventHandlers.begin()); it != eventHandlers.end(); ++it)
		{
			EventHandler* eventHandler(*it);
			if (!eventHandler->code.isEmpty())
			{
				wasCode = true;
				eventHandler->generateAdditionalCode();
			}
		}

		// if no code, do not generate any initial code
		if (!wasCode)
			return;

		// generate initialisation code
		generatedCode.push_back(generateInitialisationCode());

		// collect code from all handlers
		map<unsigned, size_t> rowIdToCode;
		int maxRow(0);
		for (EventHandlers::iterator it(eventHandlers.begin()); it != eventHandlers.end(); ++it)
		{
			EventHandler* eventHandler(*it);
			for (QVector<EventHandler::PairCodeAndIndex>::const_iterator jt(eventHandler->code.begin()); jt != eventHandler->code.end(); ++jt)
			{
				const EventHandler::PairCodeAndIndex codeAndIndex(*jt);
				rowIdToCode[codeAndIndex.second] = generatedCode.size();
				generatedCode.push_back(codeAndIndex.first);
				maxRow = std::max(maxRow, codeAndIndex.second);
			}
		}

		// copy the collected set to the vector
		setToCodeIdMap.resize(maxRow+1);
		for (int i = 0; i <= maxRow; ++i)
		{
			map<unsigned, size_t>::const_iterator codeIndex(rowIdToCode.find(i));
			if (codeIndex != rowIdToCode.end())
				setToCodeIdMap[i] = codeIndex->second;
			else
				setToCodeIdMap[i] = -1;
		}
	}

	//! Add initialization code after all pairs have been parsed
	wstring Compiler::CodeGenerator::generateInitialisationCode() const
	{
		wstring text;
		if (advancedMode)
		{
			text += L"# variables for state\n";
			text += L"var state[4] = [0,0,0,0]\n";
			text += L"var new_state[4] = [0,0,0,0]\n";
			text += L"\n";
		}
		if (!eventHandlers["acc"]->code.isEmpty())
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
		if (!eventHandlers["timeout"]->code.isEmpty())
		{
			text += L"# stop timer 0\n";
			text += L"timer.period[0] = 0\n";
		}
		if (!eventHandlers["clap"]->code.isEmpty())
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
		return text;
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

		// the second and third modes of buttons are rc
		if ((eventName == "button") && (eventActionsSet.getEventBlock()->getValue(5) != ArrowButtonsEventBlock::MODE_ARROW))
			lookupEventName = "rc5";

		// the first mode of the acc event is just a tap
		if ((eventName == "acc") && (eventActionsSet.getEventBlock()->getValue(0) == AccEventBlock::MODE_TAP))
			lookupEventName = "tap";

		// get the corresponding event handler
		EventHandlers::iterator eventIt(eventHandlers.find(lookupEventName));
		assert(eventIt != eventHandlers.end());
		EventHandler* eventHandler(*eventIt);

		// add a new entry
		// if non-conditional code, put before any conditional code
		size_t codeEntryIndex(eventHandler->code.size());
		if (!eventActionsSet.isAnyAdvancedFeature() && !eventActionsSet.getEventBlock()->isAnyValueSet())
		{
			// find where "if" block starts for the current event handler
			for (int i = 0; i < eventHandler->code.size(); ++i)
			{
				if ((eventHandler->code[i].first.find(L"if ") != wstring::npos) ||
					(eventHandler->code[i].first.find(L"when ") != wstring::npos))
				{
					codeEntryIndex = i;
					break;
				}
			}
		}
		eventHandler->code.insert(codeEntryIndex, EventHandler::PairCodeAndIndex(L"", eventActionsSet.getRow()));
		wstring& currentCode(eventHandler->code[codeEntryIndex].first);

		// add event and actions details
		visitEventAndStateFilter(eventActionsSet.getEventBlock(), eventActionsSet.getStateFilterBlock(), currentCode);
		for (int i=0; i<eventActionsSet.actionBlocksCount(); ++i)
			visitAction(eventActionsSet.getActionBlock(i), currentCode, eventHandler->isStateSet);

		// possibly add the debug event
		visitExecFeedback(eventActionsSet, currentCode);
		if (debugLog)
			visitDebugLog(eventActionsSet, currentCode);

		// possibly close condition and add code
		visitEndOfLine(currentCode);
		currentCode.append(L"\n");
	}

	//! Generate the debug log event for this set
	void Compiler::CodeGenerator::visitExecFeedback(const EventActionsSet& eventActionsSet, wstring& currentCode)
	{
		wstring text(indentText());
		text += L"emit pair_run " + toWstring(eventActionsSet.getRow()) + L"\n";
		currentCode.append(text);
	}

	//! Generate the debug log event for this set
	void Compiler::CodeGenerator::visitDebugLog(const EventActionsSet& eventActionsSet, wstring& currentCode)
	{
		wstring text(indentText());
		text += L"_emit debug_log [";

		wstringstream ostr;
		ostr << hex << showbase;
		const QVector<uint16_t> compressedContent(eventActionsSet.getContentCompressed());
        copy(compressedContent.begin(), compressedContent.end(), ostream_iterator<uint16_t, wchar_t>(ostr, L", "));
		text += ostr.str();
		text.erase(text.size() - 2);

		text += L"]\n";
		currentCode.append(text);
	}

	void Compiler::CodeGenerator::visitEventAndStateFilter(const Block* block, const Block* stateFilterBlock, wstring& currentCode)
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

			// test code, special case for remote control
			const bool isIf((block->getName() == "button") && (block->getValue(5) != ArrowButtonsEventBlock::MODE_ARROW));
			if (isIf)
				text += L"\tif ";
			else
				text += L"\twhen ";

			// event-specific code
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

			if (isIf)
				text += L" then\n ";
			else
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

		currentCode = text;
	}

	//! End the generated code of a VPL line, close whens and ifs
	void Compiler::CodeGenerator::visitEndOfLine(wstring& currentCode)
	{
		if (inIfBlock)
		{
			if (inWhenBlock)
				currentCode.append(L"\t");
			currentCode.append(L"\tend\n");
			inIfBlock = false;
		}
		if (inWhenBlock)
		{
			currentCode.append(L"\tend\n");
			inWhenBlock = false;
		}
	}

	wstring Compiler::CodeGenerator::visitEventArrowButtons(const Block* block)
	{
		wstring text;

		const int mode(block->getValue(5));
		if (mode == ArrowButtonsEventBlock::MODE_ARROW)
		{
			bool first(true);
			for (unsigned i=0; i<5; ++i)
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
		}
		else if (mode == ArrowButtonsEventBlock::MODE_RC_ARROW)
		{
			const int value(block->getValue(6));
			assert(value >= 0);
			assert(value < 5);
			// top, left, bottom, right, center
			const unsigned mapping[5][3] = {
				{80, 32, unsigned(-1) },
				{85, 17, 77},
				{81, 33, unsigned(-1) },
				{86, 16, 78},
				{87, 13, unsigned(-1)}
			};

			text += L"rc5.command == " + toWstring(mapping[value][0]);
			if (mapping[value][1] != unsigned(-1))
				text += L" or rc5.command == " + toWstring(mapping[value][1]);
			if (mapping[value][2] != unsigned(-1))
				text += L" or rc5.command == " + toWstring(mapping[value][2]);
		}
		else if (mode == ArrowButtonsEventBlock::MODE_RC_KEYPAD)
		{
			const int value(block->getValue(6));
			assert(value >= 0);
			assert(value < 12);
			// row, then column
			const unsigned mapping[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 17, 0, 16 };
			text += L"rc5.command == " + toWstring(mapping[value]);
		}
		else
			assert(false);

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
			text += L"angle >= " + toWstring(lowBound);
		if (!text.empty() && (testValue != AccEventBlock::resolution))
			text += L" and ";
		if (testValue != AccEventBlock::resolution)
			text += L"angle < " + toWstring(highBound);
		return text;
	}

	//! Visit an action block, if 0 return directly
	void Compiler::CodeGenerator::visitAction(const Block* block, wstring& currentCode, bool& isStateSet)
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
			text += visitActionSetState(block, isStateSet);
		}
		else
		{
			// internal compiler error
			abort();
		}

		currentCode.append(text);
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

	wstring Compiler::CodeGenerator::visitActionSetState(const Block* block, bool& isStateSet)
	{
		wstring text;
		const wstring indString(indentText());
		for(unsigned i=0; i<block->valuesCount(); ++i)
		{
			switch (block->getValue(i))
			{
				case 1: text += indString + L"new_state[" + toWstring(i) + L"] = 1\n"; isStateSet = true; break;
				case 2: text += indString + L"new_state[" + toWstring(i) + L"] = 0\n"; isStateSet = true; break;
				default: break; // do nothing
			}
		}
		return text;
	}

} } // namespace ThymioVPL / namespace Aseba
