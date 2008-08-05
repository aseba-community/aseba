#ifndef VARIABLE_VIEW_PLUGIN_H
#define VARIABLE_VIEW_PLUGIN_H

#include <QObject>
#include "../compiler/compiler.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class TargetVariablesModel;
	
	class VariablesViewPlugin:public QObject
	{
		Q_OBJECT
		
	private:
		TargetVariablesModel *variables;
		
	public:
		VariablesViewPlugin(TargetVariablesModel* variables);
		
		virtual ~VariablesViewPlugin();
		
		void invalidateVariableModel();
		
	protected:
		friend class TargetVariablesModel;
		//! New values are available for a variable this plugin is interested in
		virtual void variableValueUpdated(const QString& name, const VariablesDataVector& values) = 0;
	};
	
	/*@}*/
}; // Aseba

#endif
