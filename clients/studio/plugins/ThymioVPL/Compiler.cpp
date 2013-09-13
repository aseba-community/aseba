#include <assert.h>

#include <QObject>
#include "Compiler.h"
#include "Scene.h"

namespace Aseba { namespace ThymioVPL
{
	using namespace std;
	
	Compiler::Compiler(Scene& scene) :
		scene(scene),
		typeChecker(),
		syntaxChecker(),
		codeGenerator(),
		errorType(COMPILATION_SUCCESS)
	{
	}
	
	Compiler::~Compiler() 
	{ 
		clear();
	}
	
	int Compiler::buttonToCode(int id) const
	{
		return codeGenerator.buttonToCode(id);
	}
	
	void Compiler::clear()
	{
		typeChecker.clear();
		codeGenerator.clear();
		
		errorType = COMPILATION_SUCCESS;
	}
	
	Compiler::ErrorCode Compiler::getErrorCode() const
	{
		switch(errorType)
		{
		case SYNTAX_ERROR:
			return syntaxChecker.getErrorCode();
			break;
		case TYPE_ERROR:
			return typeChecker.getErrorCode();
			break;
		case CODE_ERROR:
			return codeGenerator.getErrorCode();
			break;
		case COMPILATION_SUCCESS:
		default:
			break;
		}
		
		return NO_ERROR;
	}
	
	bool Compiler::isSuccessful() const
	{
		return ( errorType == COMPILATION_SUCCESS ? true : false );
	}

	int Compiler::getErrorLine() const
	{
		return ( errorType == COMPILATION_SUCCESS ? -1 : errorLine );
	}
	
	int Compiler::getSecondErrorLine() const
	{
		return ( errorType == COMPILATION_SUCCESS ? -1 : secondErrorLine );
	}
	
	vector<wstring>::const_iterator Compiler::beginCode() const
	{ 
		return codeGenerator.beginCode();
	}
	
	vector<wstring>::const_iterator Compiler::endCode() const
	{ 
		return codeGenerator.endCode(); 
	}
	
	void Compiler::compile()
	{
		errorType = COMPILATION_SUCCESS;
		errorLine = -1;
		secondErrorLine = -1;
		typeChecker.reset();

		for (Scene::PairConstItr it(scene.pairsBegin()); it != scene.pairsEnd(); ++it)
		{
			const EventActionPair& p(*(*it));
			errorLine++;
			
			syntaxChecker.visit(p);
			if( !syntaxChecker.isSuccessful() ) 
			{
				errorType = SYNTAX_ERROR;
				break;
			}

			typeChecker.visit(p, secondErrorLine);
			if( !typeChecker.isSuccessful() )
			{
				errorType = TYPE_ERROR;
				break;
			}
		}
	}

	void Compiler::generateCode()
	{
		if( errorType == COMPILATION_SUCCESS )
		{
			codeGenerator.reset();
			
			for (Scene::PairConstItr it(scene.pairsBegin()); it != scene.pairsEnd(); ++it)
			{
				const EventActionPair& p(*(*it));
				codeGenerator.visit(p);
				if( !codeGenerator.isSuccessful() )
				{
					errorType = CODE_ERROR;
					break;
				}
			}
			
			codeGenerator.addInitialisation();
		}
	}

} } // namespace ThymioVPL / namespace Aseba
