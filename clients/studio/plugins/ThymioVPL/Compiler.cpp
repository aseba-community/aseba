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

#include <assert.h>

#include <QObject>
#include "Compiler.h"
#include "Scene.h"

namespace Aseba { namespace ThymioVPL
{
	using namespace std;
	
	//! Create a compilation result object representing a success
	Compiler::CompilationResult::CompilationResult():
		errorType(NO_ERROR),
		errorLine(-1),
		referredLine(-1)
	{}
	
	//! Create a compilation result object with a given error type and lines
	Compiler::CompilationResult::CompilationResult(ErrorType errorType, int errorLine, int referredLine):
		errorType(errorType),
		errorLine(errorLine),
		referredLine(referredLine)
	{}
	
	//! Return whether the compilation was a success or not
	bool Compiler::CompilationResult::isSuccessful() const
	{
		return errorType == NO_ERROR;
	}
	
	//! Return the error message for this result
	QString Compiler::CompilationResult::getMessage(bool advanced) const
	{
		switch (errorType)
		{
			case NO_ERROR:
			{
				return QObject::tr("Compilation success");
			}
			case MISSING_EVENT:
			{
				return QObject::tr("Line %0: Missing event").arg(errorLine+1);
			}
			case MISSING_ACTION:
			{
				return QObject::tr("Line %0: Missing action").arg(errorLine+1);
			}
			case DUPLICATED_EVENT:
			{
				if (!advanced)
					return QObject::tr("The event in line %0 is the same as in line %1").arg(errorLine+1).arg(referredLine+1);
				else
					return QObject::tr("The event and the state condition in line %0 are the same as in line %1").arg(errorLine+1).arg(referredLine+1);
			}
			default:
				return QObject::tr("Unknown VPL error");
		}
	}
	
	//////
	
	//! Compile a scene, return the result
	Compiler::CompilationResult Compiler::compile(const Scene* scene)
	{
		definedEventMap.clear();
		codeGenerator.reset(scene->getAdvanced());
		
		unsigned errorLine = 0;
		for (Scene::SetConstItr it(scene->setsBegin()); it != scene->setsEnd(); ++it, ++errorLine)
		{
			const EventActionsSet& eventActionsSet(*(*it));
			
			// ignore empty sets
			if (eventActionsSet.isEmpty())
				continue;
			
			// we need an event
			if (!eventActionsSet.hasEventBlock())
				return CompilationResult(MISSING_EVENT, errorLine);
			
			// we need at least one action
			if (!eventActionsSet.hasAnyActionBlock())
				return CompilationResult(MISSING_ACTION, errorLine);
			
			// event and state filter configurations must be unique
			const QString hash(eventActionsSet.getEventAndStateFilterHash());
			if (definedEventMap.contains(hash))
				return CompilationResult(DUPLICATED_EVENT, errorLine, definedEventMap[hash]);
			definedEventMap[hash] = errorLine;
			
			// dispatch to code generator
			codeGenerator.visit(eventActionsSet, scene->debugLog());
		}
		
		codeGenerator.addInitialisationCode();
		
		return CompilationResult();
	}
	
	//! Return the mapping between a set identifier (its row) and the corresponding block of code
	int Compiler::getSetToCodeIdMap(int id) const
	{
		if ((id >= 0) && (id < int(codeGenerator.setToCodeIdMap.size())))
			return codeGenerator.setToCodeIdMap[id];
		return -1;
	}
	
	//! Iterator to te start of the code
	Compiler::CodeConstIterator Compiler::beginCode() const
	{ 
		return codeGenerator.generatedCode.begin();
	}
	
	//! Iterator to te end of the code
	Compiler::CodeConstIterator Compiler::endCode() const
	{ 
		return codeGenerator.generatedCode.end(); 
	}
	
} } // namespace ThymioVPL / namespace Aseba
