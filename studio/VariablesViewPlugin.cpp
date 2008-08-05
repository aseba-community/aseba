#include "VariablesViewPlugin.h"
#include "TargetModels.h"

#include <VariablesViewPlugin.moc>


namespace Aseba
{
	VariablesViewPlugin::VariablesViewPlugin(TargetVariablesModel* variables) :
		variables(variables)
	{
		
	}
	
	VariablesViewPlugin::~VariablesViewPlugin()
	{
		if (variables)
			variables->unsubscribeViewPlugin(this);
	}
	
	void VariablesViewPlugin::invalidateVariableModel()
	{
		variables = 0;
	}
}
