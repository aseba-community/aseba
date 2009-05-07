#include "VariablesViewPlugin.h"
#include <VariablesViewPlugin.moc>
#include "TargetModels.h"
#include <QtGui>
#include <QtDebug>
#include <limits>
#include <cassert>


//#include <VariablesViewPlugin.moc>

namespace Aseba
{
	VariablesViewPlugin::VariablesViewPlugin(TargetVariablesModel* variablesModel) :
		variablesModel(variablesModel)
	{
		
	}
	
	VariablesViewPlugin::~VariablesViewPlugin()
	{
		if (variablesModel)
			variablesModel->unsubscribeViewPlugin(this);
	}
	
	void VariablesViewPlugin::invalidateVariableModel()
	{
		variablesModel = 0;
	}
	
	
	LinearCameraViewVariablesDialog::LinearCameraViewVariablesDialog(TargetVariablesModel* variablesModel) :
		redVariable(new QComboBox),
		greenVariable(new QComboBox),
		blueVariable(new QComboBox),
		valuesRanges(new QComboBox)
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
	
		layout->addWidget(new QLabel(tr("Please choose your variables")));
		layout->addWidget(new QLabel(tr("red component")));
		layout->addWidget(redVariable);
		layout->addWidget(new QLabel(tr("green component")));
		layout->addWidget(greenVariable);
		layout->addWidget(new QLabel(tr("blue component")));
		layout->addWidget(blueVariable);
		layout->addWidget(new QLabel(tr("range of values")));
		layout->addWidget(valuesRanges);
		
		QPushButton* okButton = new QPushButton(QIcon(":/images/ok.png"), tr("Ok"));
		connect(okButton, SIGNAL(clicked(bool)), SLOT(accept()));
		layout->addWidget(okButton);
		
		valuesRanges->addItem(tr("auto range"));
		valuesRanges->addItem(tr("8 bits range (0–255)"));
		valuesRanges->addItem(tr("percent range (0–100)"));
		
		const QList<TargetVariablesModel::Variable>& variables(variablesModel->getVariables());
		for (int i = 0; i < variables.size(); ++i)
		{
			redVariable->addItem(variables[i].name);
			greenVariable->addItem(variables[i].name);
			blueVariable->addItem(variables[i].name);
		}
		
		redVariable->setEditText("camR");
		greenVariable->setEditText("camG");
		blueVariable->setEditText("camB");
		
		setWindowTitle(tr("Linear Camera View Plugin"));
	}
	
	LinearCameraViewPlugin::LinearCameraViewPlugin(TargetVariablesModel* variablesModel) :
		VariablesViewPlugin(variablesModel)
	{
		{
			LinearCameraViewVariablesDialog linearCameraViewVariablesDialog(variablesModel);
			if (linearCameraViewVariablesDialog.exec() == QDialog::Accepted)
			{
				redName = linearCameraViewVariablesDialog.redVariable->currentText();
				greenName = linearCameraViewVariablesDialog.greenVariable->currentText();
				blueName = linearCameraViewVariablesDialog.blueVariable->currentText();
				valuesRange = (ValuesRange)linearCameraViewVariablesDialog.valuesRanges->currentIndex();
			}
			else
			{
				deleteLater();
				return;
			}
		}
		
		bool ok = true;
		ok &= variablesModel->subscribeToVariableOfInterest(this, redName);
		ok &= variablesModel->subscribeToVariableOfInterest(this, greenName);
		ok &= variablesModel->subscribeToVariableOfInterest(this, blueName);
		
		if (!ok)
		{
			QMessageBox::warning(this, tr("Cannot initialize linear camera view plugin"), tr("One or more variable not found in %1, %2, or %3.").arg(redName).arg(greenName).arg(blueName));
			deleteLater();
		}
		else
		{
			QVBoxLayout *layout = new QVBoxLayout(this);
			image = new QLabel;
			image->resize(200, 40);
			image->setScaledContents(true);
			layout->addWidget(image);
			setAttribute(Qt::WA_DeleteOnClose);
			setWindowTitle(tr("Linear Camera View Plugin"));
			show();
		}
	}
	
	void LinearCameraViewPlugin::variableValueUpdated(const QString& name, const VariablesDataVector& values)
	{
		if (name == redName)
			red = values;
		if (name == greenName)
			green = values;
		if (name == blueName)
			blue = values;
		
		int width = qMin(red.size(), qMin(green.size(), blue.size()));
		
		if (width)
		{
			QPixmap pixmap(width, 1);
			QPainter painter(&pixmap);
			
			int min = std::numeric_limits<int>::max();
			int max = std::numeric_limits<int>::min();
			switch (valuesRange)
			{
				case VALUES_RANGE_AUTO:
				{
					// get min and max values for auto range adjustment
					for (int i = 0; i < width; i++)
					{
						min = std::min(min, (int)red[i]);
						min = std::min(min, (int)green[i]);
						min = std::min(min, (int)blue[i]);
						max = std::max(max, (int)red[i]);
						max = std::max(max, (int)green[i]);
						max = std::max(max, (int)blue[i]);
					}
				}
				break;
				
				case VALUES_RANGE_8BITS:
				{
					min = 0;
					max = 255;
				}
				break;
				
				case VALUES_RANGE_PERCENT:
				{
					min = 0;
					max = 100;
				}
				break;
				
				default:
				assert(false);
				break;
			}
			
			for (int i = 0; i < width; i++)
			{
				int range = max - min;
				if (range == 0)
					range = 1;
				int r = ((red[i] - min) * 255) / range;
				int g = ((green[i] - min) * 255) / range;
				int b = ((blue[i] - min) * 255) / range;
				painter.fillRect(i, 0, 1, 1, QColor(r, g, b));
			}
			
			image->setPixmap(pixmap);
		}
	}
}
