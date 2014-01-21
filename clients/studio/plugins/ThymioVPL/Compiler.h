#ifndef THYMIO_INTERMEDIATE_REPRESENTATION_H
#define THYMIO_INTERMEDIATE_REPRESENTATION_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <utility>
#include <QMap>
#include <QPair>

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
			DUPLICATED_EVENT,
			INVALID_CODE
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
			
			void reset(bool advanced);
			void addInitialisationCode();
			void visit(const EventActionsSet& eventActionsSet);
			
		protected:
			void initEventToCodePosMap();
			
			void visitEventAndStateFilter(const Block* block, const Block* stateFilterBlock, unsigned currentBlock);
			std::wstring visitEventArrowButtons(const Block* block);
			std::wstring visitEventProx(const Block* block);
			std::wstring visitEventProxGround(const Block* block);
			
			void visitAction(const Block* block, unsigned currentBlock);
			std::wstring visitActionMove(const Block* block);
			std::wstring visitActionTopColor(const Block* block);
			std::wstring visitActionBottomColor(const Block* block);
			std::wstring visitActionSound(const Block* block);
			std::wstring visitActionTimer(const Block* block);
			std::wstring visitActionSetState(const Block* block);
			
		public:
			std::vector<std::wstring> generatedCode;
			std::vector<int> setToCodeIdMap;
			
		protected:
			typedef QMap<QString, QPair<int, int> > EventToCodePosMap;
			EventToCodePosMap eventToCodePosMap;
			
			bool advancedMode;
			bool useSound;
			bool useTimer;
			bool useMicrophone;
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
