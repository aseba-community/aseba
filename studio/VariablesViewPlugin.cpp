#include "VariablesViewPlugin.h"
#include "TargetModels.h"
#include <QtGui>
#include <QtDebug>
#include <limits>


//#include <VariablesViewPlugin.moc>

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
	
	
	LinearCameraViewPlugin::LinearCameraViewPlugin(TargetVariablesModel* variables) :
		VariablesViewPlugin(variables)
	{
		bool ok = true;
		ok &= variables->subscribeToVariableOfInterest(this, "camR");
		ok &= variables->subscribeToVariableOfInterest(this, "camG");
		ok &= variables->subscribeToVariableOfInterest(this, "camB");
		if (!ok)
		{
			QMessageBox::warning(this, tr("Cannot initialize linear camera view plugin"), tr("One or more variable not found in camR, camG, or camB"));
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
			show();
		}
	}
	
	void LinearCameraViewPlugin::variableValueUpdated(const QString& name, const VariablesDataVector& values)
	{
		if (name == "camR")
			red = values;
		if (name == "camG")
			green = values;
		if (name == "camB")
			blue = values;
		
		int width = qMin(red.size(), qMin(green.size(), blue.size()));
		
		if (width)
		{
			QPixmap pixmap(width, 1);
			QPainter painter(&pixmap);
			
			// get min and max values for autoset
			int min = std::numeric_limits<int>::max();
			int max = std::numeric_limits<int>::min();
			for (int i = 0; i < width; i++)
			{
				min = std::min(min, (int)red[i]);
				min = std::min(min, (int)green[i]);
				min = std::min(min, (int)blue[i]);
				max = std::max(max, (int)red[i]);
				max = std::max(max, (int)green[i]);
				max = std::max(max, (int)blue[i]);
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
