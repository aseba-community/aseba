#include <assert.h>

#include "ThymioIntermediateRepresentation.h"

namespace Aseba
{

	ThymioCompiler::ThymioCompiler() : 
		buttonSet(),
		typeChecker(),
		syntaxChecker(),
		codeGenerator(),
		errorType(THYMIO_NO_TYPE_ERROR)
	{
	}
	
	ThymioCompiler::~ThymioCompiler() 
	{ 
		clear();
	}

	void ThymioCompiler::addButtonSet(ThymioIRButtonSet *set)
	{	
		cout << "Thymio Compiler -- add button set: " << buttonSet.size() << flush;
		
		buttonSet.push_back(set);
		
		cout << "  ..\n" << flush;	
	}

	void ThymioCompiler::insertButtonSet(int row, ThymioIRButtonSet *set)
	{
		cout << "Thymio Compiler -- insert button set: " << row << ", " << buttonSet.size() << flush;
		
		if(row < buttonSet.size())	
			buttonSet.insert(buttonSet.begin()+row, set);
		else
			buttonSet.push_back(set);

		cout << "  ..\n" << flush;	
	}
	
	// crash here!!!
	void ThymioCompiler::removeButtonSet(int row)
	{
		cout << "Thymio Compiler -- remove button set: " << row << ", " << buttonSet.size() << flush;

		assert(row < buttonSet.size());
		buttonSet.erase(buttonSet.begin()+row);

		cout << "  ..\n" << flush;	
	}
	
	void ThymioCompiler::replaceButtonSet(int row, ThymioIRButtonSet *set)
	{
		cout << "Thymio Compiler -- replace button set: " << row << ", " << buttonSet.size() << flush;

		if(row < buttonSet.size())
			buttonSet[row] = set;
		else
			buttonSet.push_back(set);

		cout << "  ..\n" << flush;	
	}
	
	void ThymioCompiler::swap(int row1, int row2)
	{
		cout << "Thymio Compiler -- swap button set: " << row1 << ", " << row2 << ", " << buttonSet.size() << flush;
		
		ThymioIRButtonSet *set = buttonSet[row1];
		buttonSet[row1] = buttonSet[row2];
		buttonSet[row2] = set;

		cout << "  ..\n" << flush;	
	}
	
	void ThymioCompiler::clear()
	{
		buttonSet.clear();
		typeChecker.clear();
		syntaxChecker.clear();
		codeGenerator.clear();
		
		errorType = THYMIO_NO_TYPE_ERROR;
	}
	
	ThymioIRErrorCode ThymioCompiler::getErrorCode()
	{
		switch(errorType)
		{
		case THYMIO_SYNTAX_ERROR:
			return syntaxChecker.getErrorCode();
			break;
		case THYMIO_TYPE_ERROR:
			return typeChecker.getErrorCode();
			break;
		case THYMIO_CODE_ERROR:
			return codeGenerator.getErrorCode();
			break;
		case THYMIO_NO_TYPE_ERROR:
		default:
			break;			
		}
		
		return THYMIO_NO_ERROR;
	}
	
	wstring ThymioCompiler::getErrorMessage()
	{
		switch(errorType)
		{
		case THYMIO_SYNTAX_ERROR:
			return syntaxChecker.getErrorMessage();
			break;
		case THYMIO_TYPE_ERROR:
			return typeChecker.getErrorMessage();
			break;
		case THYMIO_CODE_ERROR:
			return codeGenerator.getErrorMessage();
			break;
		case THYMIO_NO_TYPE_ERROR:
			return L"Compilation success.";
			break;
		default:
			break;			
		}
		
		return L"";		
	}
	
	bool ThymioCompiler::isSuccessful()
	{
		return ( errorType == THYMIO_NO_TYPE_ERROR ? true : false );
	}

	int ThymioCompiler::getErrorLine()
	{
		return ( errorType == THYMIO_NO_TYPE_ERROR ? -1 : errorLine );
	}
	
	vector<wstring>::const_iterator ThymioCompiler::beginCode() 
	{ 
		return codeGenerator.beginCode(); 
	}
	
	vector<wstring>::const_iterator ThymioCompiler::endCode() 
	{ 
		return codeGenerator.endCode(); 
	}	
			
	void ThymioCompiler::compile()
	{
		errorType = THYMIO_NO_TYPE_ERROR;
		errorLine = -1;
		typeChecker.reset();

		vector<ThymioIRButtonSet*>::iterator itr = buttonSet.begin();
		for( itr = buttonSet.begin(); itr != buttonSet.end(); ++itr )
		{
			errorLine++;
			
			(*itr)->accept(&syntaxChecker);			
			if( !syntaxChecker.isSuccessful() ) 
			{
				errorType = THYMIO_SYNTAX_ERROR;
				break;
			}

			(*itr)->accept(&typeChecker);
			if( !typeChecker.isSuccessful() )
			{
				errorType = THYMIO_TYPE_ERROR;
				break;
			}
					
		}
	}

	void ThymioCompiler::generateCode()
	{
		if( errorType == THYMIO_NO_TYPE_ERROR )
		{
			codeGenerator.reset();
			
			vector<ThymioIRButtonSet*>::iterator itr = buttonSet.begin();
			for( itr = buttonSet.begin(); itr != buttonSet.end(); ++itr )
			{
				(*itr)->accept(&codeGenerator);
				if( !codeGenerator.isSuccessful() )
				{
					errorType = THYMIO_CODE_ERROR;
					break;
				}
			}
		}
	}

}; // Aseba
