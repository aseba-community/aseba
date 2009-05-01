#ifndef VARIABLE_VIEW_PLUGIN_H
#define VARIABLE_VIEW_PLUGIN_H

#include <QWidget>
#include <QDialog>
#include "../compiler/compiler.h"

class QLabel;
class QComboBox;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class TargetVariablesModel;
	
	class VariablesViewPlugin: public QWidget
	{
	protected:
		TargetVariablesModel *variablesModel;
		
	public:
		VariablesViewPlugin(TargetVariablesModel* variablesModel);
		
		virtual ~VariablesViewPlugin();
		
		void invalidateVariableModel();
		
	protected:
		friend class TargetVariablesModel;
		//! New values are available for a variable this plugin is interested in
		virtual void variableValueUpdated(const QString& name, const VariablesDataVector& values) = 0;
	};
	
	class LinearCameraViewVariablesDialog : public QDialog
	{
		Q_OBJECT
		
	public:
		QComboBox* redVariable;
		QComboBox* greenVariable;
		QComboBox* blueVariable;
		QComboBox* valuesRanges;
		
		LinearCameraViewVariablesDialog(TargetVariablesModel* variablesModel);
		virtual ~LinearCameraViewVariablesDialog() {}
	};
	
	class LinearCameraViewPlugin: public VariablesViewPlugin
	{
		Q_OBJECT
		
	public:
		LinearCameraViewPlugin(TargetVariablesModel* variablesModel);
		
		virtual void variableValueUpdated(const QString& name, const VariablesDataVector& values);
		
	private:
		enum ValuesRange
		{
			VALUES_RANGE_AUTO = 0,
			VALUES_RANGE_8BITS,
			VALUES_RANGE_PERCENT
		} valuesRange;
		QLabel *image;
		QString redName, greenName, blueName;
		VariablesDataVector red, green, blue;
	};
	
	/*@}*/
}; // Aseba

#endif
