#ifndef THYMIO_INTERMEDIATE_REPRESENTATION_H
#define THYMIO_INTERMEDIATE_REPRESENTATION_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <utility>

namespace Aseba { namespace ThymioVPL
{
	class Scene;
	class Block;
	class EventActionsSet;
	
	class Compiler 
	{
	public:
		enum ErrorCode
		{
			NO_ERROR = 0,
			MISSING_EVENT,
			MISSING_ACTION,
			EVENT_REPEATED,
			INVALID_CODE
		};
		
	protected:
		enum ErrorType
		{
			COMPILATION_SUCCESS = 0,
			SYNTAX_ERROR,
			TYPE_ERROR,
			CODE_ERROR
		};
		
	public:
		class Visitor
		{
		public:
			Visitor() : errorCode(NO_ERROR) {}
			virtual ~Visitor() {}
			
			virtual void visit(const EventActionsSet& p);

			ErrorCode getErrorCode() const;
			bool isSuccessful() const;

		protected:
			ErrorCode errorCode;
		};

		class TypeChecker : public Visitor
		{
		public:
			virtual void visit(const EventActionsSet& p, int& secondErrorLine);
			
			void reset();
			void clear();
			
		private:
			std::multimap<std::wstring, const Block*> moveHash;
			std::multimap<std::wstring, const Block*> colorTopHash;
			std::multimap<std::wstring, const Block*> colorBottomHash;
			std::multimap<std::wstring, const Block*> soundHash;
			std::multimap<std::wstring, const Block*> timerHash;
			std::multimap<std::wstring, const Block*> memoryHash;
		};

		class SyntaxChecker : public Visitor
		{
		public:
			virtual void visit(const EventActionsSet& p); 
		};

		class CodeGenerator : public Visitor
		{
		public:
			CodeGenerator();
			~CodeGenerator();
			
			virtual void visit(const EventActionsSet& p);

			std::vector<std::wstring>::const_iterator beginCode() const { return generatedCode.begin(); }
			std::vector<std::wstring>::const_iterator endCode() const { return generatedCode.end(); }
			void reset();
			void clear();
			void addInitialisation();
			
			int buttonToCode(int id) const;
			
		protected:
			void visitEvent(const Block& card, unsigned currentBlock);
			std::wstring visitEventArrowButtons(const Block& card);
			std::wstring visitEventProx(const Block& card);
			std::wstring visitEventProxGround(const Block& card);
			
			void visitAction(const Block& card, unsigned currentBlock);
			std::wstring visitActionMove(const Block& card);
			std::wstring visitActionTopColor(const Block& card);
			std::wstring visitActionBottomColor(const Block& card);
			std::wstring visitActionSound(const Block& card);
			std::wstring visitActionTimer(const Block& card);
			std::wstring visitActionStateFilter(const Block& card);
			
		protected:
			typedef std::map<QString, std::pair<int, int> > EventToCodePosMap;
			EventToCodePosMap editor;

			std::vector<std::wstring> generatedCode;
			bool advancedMode;
			bool useSound;
			bool useTimer;
			bool useMicrophone;
			bool inIfBlock;
			std::vector<int> buttonToCodeMap;
		};
		
	public:
		Compiler(Scene& scene);
		~Compiler();
		
		void compile();
		void generateCode();
		
 		int buttonToCode(int id) const;

		ErrorCode getErrorCode() const;
		bool isSuccessful() const;
		int getErrorLine() const;
		int getSecondErrorLine() const;
		
		typedef std::vector<std::wstring>::const_iterator CodeConstIterator;

		CodeConstIterator beginCode() const;
		CodeConstIterator endCode() const; 

		void clear();

	private:
		Scene& scene;
		
		TypeChecker   typeChecker;
		SyntaxChecker syntaxChecker;
		CodeGenerator codeGenerator;
		
		ErrorType errorType;

		int errorLine;
		int secondErrorLine;
	};

} } // namespace ThymioVPL / namespace Aseba

#endif
