#ifndef FUNCTION_PARAMETERS_DIALOG_H
#define FUNCTION_PARAMETERS_DIALOG_H

#include <QDialog>
#include <QString>
#include <vector>
#include "CustomDelegate.h"

class QTableWidget;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	//! Dialog that let the user choose setup parameters for functions
	class FunctionParametersDialog : public QDialog
	{
		Q_OBJECT
	
	public:
		//! Create a dialog for function name with initial parameters vector
		FunctionParametersDialog(const QString &name, const std::vector<unsigned> &parameters);
		//! Write the result of this dialog in parameters 
		std::vector<unsigned> getParameters() const;
	
	private slots:
		void addParameter();
		void deleteParameter();
		void argumentSelectionChanged();
		void cellClicked(int row, int column);
		
	private:
		//! Add a new entry in the parameter table with a specific value
		void addEntry(int value);
		
		SpinBoxDelegate spinBoxDelegate;
		QTableWidget *parametersTable;
		QPushButton *addParameterButton;
		QPushButton *delParameterButton;
	};
	
	/*@}*/
}

#endif
