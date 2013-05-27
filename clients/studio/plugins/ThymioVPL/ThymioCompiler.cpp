#include <assert.h>

#include <QObject>
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
		buttonSet.push_back(set);
	}

	void ThymioCompiler::insertButtonSet(int row, ThymioIRButtonSet *set)
	{
		if(row < (int)buttonSet.size())	
			buttonSet.insert(buttonSet.begin()+row, set);
		else
			buttonSet.push_back(set);
	}
	
	void ThymioCompiler::removeButtonSet(int row)
	{
		assert(row < (int)buttonSet.size());
		buttonSet.erase(buttonSet.begin()+row);
	}
	
	void ThymioCompiler::replaceButtonSet(int row, ThymioIRButtonSet *set)
	{
		if(row < (int)buttonSet.size())
			buttonSet[row] = set;
		else
			buttonSet.push_back(set);	
	}
	
	void ThymioCompiler::swap(int row1, int row2)
	{
		ThymioIRButtonSet *set = buttonSet[row1];
		buttonSet[row1] = buttonSet[row2];
		buttonSet[row2] = set;	
	}
	
	int ThymioCompiler::buttonToCode(int id) const
	{
		return codeGenerator.buttonToCode(id);
	}
	
	void ThymioCompiler::clear()
	{
		buttonSet.clear();
		typeChecker.clear();
		syntaxChecker.clear();
		codeGenerator.clear();
		
		errorType = THYMIO_NO_TYPE_ERROR;
	}
	
	ThymioIRErrorCode ThymioCompiler::getErrorCode() const
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
	
	bool ThymioCompiler::isSuccessful() const
	{
		return ( errorType == THYMIO_NO_TYPE_ERROR ? true : false );
	}

	int ThymioCompiler::getErrorLine() const
	{
		return ( errorType == THYMIO_NO_TYPE_ERROR ? -1 : errorLine );
	}
	
	vector<wstring>::const_iterator ThymioCompiler::beginCode() const
	{ 
		return codeGenerator.beginCode();
	}
	
	vector<wstring>::const_iterator ThymioCompiler::endCode() const
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
			
			codeGenerator.addInitialisation();
		}
	}

}; // Aseba
