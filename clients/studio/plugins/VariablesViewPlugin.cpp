#include "VariablesViewPlugin.h"
#include "../Target.h"
#include "../../../common/productids.h"
#include <QtGui>
#include <QtDebug>
#include <limits>
#include <cassert>

namespace Aseba
{
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
		
		const int redIndex(redVariable->findText("cam.red"));
		if (redIndex != -1)
			redVariable->setCurrentIndex(redIndex);
		const int greenIndex(redVariable->findText("cam.green"));
		if (greenIndex != -1)
			greenVariable->setCurrentIndex(greenIndex);
		const int blueIndex(redVariable->findText("cam.blue"));
		if (blueIndex != -1)
			blueVariable->setCurrentIndex(blueIndex);
		
		setWindowTitle(tr("Linear Camera View Plugin"));
	}
	
	//! Construct the plugin, taking ownership of an interface to a development environment
	LinearCameraViewPlugin::LinearCameraViewPlugin(DevelopmentEnvironmentInterface *_de) :
		VariableListener(_de->getVariablesModel()),
		de(_de),
		componentsReceived(0),
		timerId(0)
	{
		QVBoxLayout *layout = new QVBoxLayout(this);
		image = new QLabel;
		image->setScaledContents(true);
		layout->addWidget(image);
		setWindowTitle(tr("Linear Camera View Plugin"));
		resize(200, 40);
	}


	QWidget* LinearCameraViewPlugin::createMenuEntry()
	{
		QCheckBox *checkbox = new QCheckBox(tr("linear camera viewer"));
		connect(checkbox, SIGNAL(clicked(bool)), SLOT(setEnabled(bool)));
		connect(this, SIGNAL(dialogBoxResult(bool)), checkbox, SLOT(setChecked(bool)));
		return checkbox;
	}
	
	void LinearCameraViewPlugin::closeAsSoonAsPossible()
	{
		close();
	}
	
	void LinearCameraViewPlugin::setEnabled(bool enabled)
	{
		if (enabled)
			enablePlugin();
		else
			close();
	}
	
	void LinearCameraViewPlugin::enablePlugin()
	{
		// unsubscribe to existing variables, if any
		disablePlugin();
		
		{
			LinearCameraViewVariablesDialog linearCameraViewVariablesDialog(variablesModel);
			if (linearCameraViewVariablesDialog.exec() != QDialog::Accepted)
			{
				emit dialogBoxResult(false);
				return;
			}
			
			redName = linearCameraViewVariablesDialog.redVariable->currentText();
			greenName = linearCameraViewVariablesDialog.greenVariable->currentText();
			blueName = linearCameraViewVariablesDialog.blueVariable->currentText();
			valuesRange = (ValuesRange)linearCameraViewVariablesDialog.valuesRanges->currentIndex();
		}
		
		bool ok = true;
		ok &= subscribeToVariableOfInterest(redName);
		ok &= subscribeToVariableOfInterest(greenName);
		ok &= subscribeToVariableOfInterest(blueName);
		
		if (!ok)
		{
			QMessageBox::warning(this, tr("Cannot initialize linear camera view plugin"), tr("One or more variable not found in %1, %2, or %3.").arg(redName).arg(greenName).arg(blueName));
			unsubscribeToVariablesOfInterest();
			emit dialogBoxResult(false);
			return;
		}
		
		redPos = de->getVariablesModel()->getVariablePos(redName);
		greenPos = de->getVariablesModel()->getVariablePos(greenName);
		bluePos = de->getVariablesModel()->getVariablePos(blueName);
		redSize = de->getVariablesModel()->getVariableSize(redName);
		greenSize = de->getVariablesModel()->getVariableSize(greenName);
		blueSize = de->getVariablesModel()->getVariableSize(blueName);
		
		de->getTarget()->getVariables(de->getNodeId(), redPos, redSize);
		de->getTarget()->getVariables(de->getNodeId(), greenPos, greenSize);
		de->getTarget()->getVariables(de->getNodeId(), bluePos, blueSize);
		
		if (!timerId)
		{
			if (de->getProductId() == ASEBA_PID_EPUCK)
				timerId = startTimer(200);
			else
				timerId = startTimer(100);
		}
		
		show();
	}
	
	void LinearCameraViewPlugin::disablePlugin()
	{
		// unsubscribe to existing variables, if any
		unsubscribeToVariablesOfInterest();
		if (timerId)
		{
			killTimer(timerId);
			timerId = 0;
		}
	}
	
	void LinearCameraViewPlugin::timerEvent ( QTimerEvent * event )
	{
		de->getTarget()->getVariables(de->getNodeId(), redPos, redSize);
		de->getTarget()->getVariables(de->getNodeId(), greenPos, greenSize);
		de->getTarget()->getVariables(de->getNodeId(), bluePos, blueSize);
	}
	
	void LinearCameraViewPlugin::closeEvent ( QCloseEvent * event )
	{
		disablePlugin();
		emit dialogBoxResult(false);
	}
	
	void LinearCameraViewPlugin::variableValueUpdated(const QString& name, const VariablesDataVector& values)
	{
		if (name == redName)
		{
			red = values;
			componentsReceived |= 1 << 0;
		}
		if (name == greenName)
		{
			green = values;
			componentsReceived |= 1 << 1;
		}
		if (name == blueName)
		{
			blue = values;
			componentsReceived |= 1 << 2;
		}
		
		const int width = qMin(red.size(), qMin(green.size(), blue.size()));
		if ((componentsReceived == 0x7) && width)
		{
			componentsReceived = 0;
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
