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

	void ThymioCompiler::AddButtonSet(ThymioIRButtonSet *set)
	{	
		buttonSet.push_back(set);
	}

	void ThymioCompiler::InsertButtonSet(int row, ThymioIRButtonSet *set)
	{
		if(row < buttonSet.size())	
			buttonSet.insert(buttonSet.begin()+row, set);
		else
			buttonSet.push_back(set);
	}
	
	void ThymioCompiler::RemoveButtonSet(int row)
	{
		assert(row < buttonSet.size());
		buttonSet.erase(buttonSet.begin()+row);
	}
	
	void ThymioCompiler::ReplaceButtonSet(int row, ThymioIRButtonSet *set)
	{
		if(row < buttonSet.size())
			buttonSet[row] = set;
		else
			buttonSet.push_back(set);
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
	
	wstring ThymioCompiler::getErrorMessage() const
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
			return QObject::tr("Compilation success.").toStdWString();
			break;
		default:
			break;
		}
		
		return L"";
	}
	
	bool ThymioCompiler::isSuccessful() const
	{
		return ( errorType == THYMIO_NO_TYPE_ERROR ? true : false );
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
		typeChecker.reset();

		vector<ThymioIRButtonSet*>::iterator itr = buttonSet.begin();
		for( itr = buttonSet.begin(); itr != buttonSet.end(); ++itr )
		{
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
