#ifndef VARIABLE_VIEW_PLUGIN_H
#define VARIABLE_VIEW_PLUGIN_H

#include <QWidget>
#include "../compiler/compiler.h"

class QLabel;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class TargetVariablesModel;
	
	class VariablesViewPlugin: public QWidget
	{
	protected:
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
	
	class LinearCameraViewPlugin: public VariablesViewPlugin
	{
	public:
		LinearCameraViewPlugin(TargetVariablesModel* variables);
		
		virtual void variableValueUpdated(const QString& name, const VariablesDataVector& values);
		
	private:
		QLabel *image;
		VariablesDataVector red, green, blue;
	};
	
	/*@}*/
}; // Aseba

#endif
