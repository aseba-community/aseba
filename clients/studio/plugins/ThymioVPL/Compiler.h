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

#ifndef VPL_COMPILER_H
#define VPL_COMPILER_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <utility>
#include <QMap>
#include <QPair>
#include <QVector>

namespace Aseba { namespace ThymioVPL
{
	class Scene;
	class Block;
	class EventActionsSet;
	
	//! The VPL compiler
	class Compiler
	{
	public:
		//! Possible errors that can happen during compilation
		enum ErrorType
		{
			NO_ERROR = 0,
			MISSING_EVENT,
			MISSING_ACTION,
			DUPLICATED_EVENT
		};
		
		//! Result of a compilation
		struct CompilationResult
		{
			CompilationResult();
			CompilationResult(ErrorType errorType, int errorLine=-1, int referredLine=-1);
			
			ErrorType errorType;
			int errorLine; //!< error line number, counting from 0
			int referredLine; //!< referred line number, counting from 0
			
			bool isSuccessful() const;
			QString getMessage(bool advanced) const;
		};
		
		//! An iterator on the generated code
		typedef std::vector<std::wstring>::const_iterator CodeConstIterator;
		
	protected:
		//! Everything needed to generate code
		class CodeGenerator
		{
		public:
			CodeGenerator();
			~CodeGenerator();
			
			void reset(bool advanced);
			void visit(const EventActionsSet& eventActionsSet, bool debugLog);
			void link();
			
		protected:
			void clearEventHandlers();
			std::wstring generateInitialisationCode() const;
			
			std::wstring indentText() const;
			
			void visitEndOfLine(std::wstring& currentCode);
			
			void visitExecFeedback(const EventActionsSet& eventActionsSet, std::wstring& currentCode);
			void visitDebugLog(const EventActionsSet& eventActionsSet, std::wstring& currentCode);
			
			void visitEventAndStateFilter(const Block* block, const Block* stateFilterBlock, std::wstring& currentCode);
			std::wstring visitEventArrowButtons(const Block* block);
			std::wstring visitEventProx(const Block* block);
			std::wstring visitEventProxGround(const Block* block);
			std::wstring visitEventAccPre(const Block* block);
			std::wstring visitEventAcc(const Block* block);
			
			void visitAction(const Block* block, std::wstring& currentCode, bool& isStateSet);
			std::wstring visitActionMove(const Block* block);
			std::wstring visitActionTopColor(const Block* block);
			std::wstring visitActionBottomColor(const Block* block);
			std::wstring visitActionSound(const Block* block);
			std::wstring visitActionTimer(const Block* block);
			std::wstring visitActionSetState(const Block* block, bool& isStateSet);
			
		public:
			std::vector<std::wstring> generatedCode;
			std::vector<int> setToCodeIdMap;
			
		protected:
			struct EventHandler
			{
				typedef QPair<std::wstring, int> PairCodeAndIndex;
				QVector<PairCodeAndIndex> code;
				bool isStateSet;
				
				virtual ~EventHandler() {}
				
				void clear() { code.clear(); isStateSet = false; }
				
				virtual void generateAdditionalCode();
			};
			
			struct ButtonEventHandler: EventHandler
			{
				virtual void generateAdditionalCode();
			};
			
			struct ProxEventHandler: EventHandler
			{
				virtual void generateAdditionalCode();
			};
			
			struct TapEventHandler: EventHandler
			{
				virtual void generateAdditionalCode();
			};
			
			struct AccEventHandler: EventHandler
			{
				virtual void generateAdditionalCode();
			};
			
			struct ClapEventHandler: EventHandler
			{
				virtual void generateAdditionalCode();
			};
			
			struct TimeoutEventHandler: EventHandler
			{
				virtual void generateAdditionalCode();
			};
			
			struct RemoteControlEventHandler: EventHandler
			{
				virtual void generateAdditionalCode();
			};
			
			typedef QMap<QString, EventHandler* > EventHandlers;
			EventHandlers eventHandlers;
			std::wstring initCode;
			
			bool advancedMode;
			bool useSound;
			bool inWhenBlock;
			bool inIfBlock;
		};
		
	public:
		CompilationResult compile(const Scene* scene);
		int getSetToCodeIdMap(int id) const;
		CodeConstIterator beginCode() const;
		CodeConstIterator endCode() const; 
		
	protected:
		//! Map of defined events, for duplication checking
		typedef QMap<QString, unsigned> DefinedEventMap;
	
	protected:
		//! The code generator holds the result of compilation
		CodeGenerator codeGenerator;
		//! The map of defined events, for duplication checking
		DefinedEventMap definedEventMap;
	};
	
} } // namespace ThymioVPL / namespace Aseba

#endif
