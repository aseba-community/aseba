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

#ifdef HAVE_QWT

#include "EventViewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QSettings>
#include <QtDebug>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>

#if QWT_VERSION >= 0x060000
	#include <qwt_series_data.h>
#else
	#include <qwt_data.h>
#endif

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	#if QWT_VERSION >= 0x060000
	class EventDataWrapper : public QwtSeriesData<QPointF>
	{
	private:
		std::deque<double>& _x;
		std::deque<sint16>& _y;
		
	public:
		EventDataWrapper(std::deque<double>& _x, std::deque<sint16>& _y) :
			_x(_x),
			_y(_y)
		{ }
		virtual QRectF boundingRect () const { return qwtBoundingRect(*this); }
		virtual QPointF sample (size_t i) const { return QPointF(_x[i], double(_y[i])); }
		virtual size_t size () const { return _x.size(); }
	};
	#else
	class EventDataWrapper : public QwtData
	{
	private:
		std::deque<double>& _x;
		std::deque<sint16>& _y;
		
	public:
		EventDataWrapper(std::deque<double>& _x, std::deque<sint16>& _y) :
			_x(_x),
			_y(_y)
		{ }
		virtual QwtData *   copy () const { return new EventDataWrapper(*this); }
		virtual size_t   size () const { return _x.size(); }
		virtual double x (size_t i) const { return _x[i]; }
		virtual double y (size_t i) const { return (double)_y[i]; }
	};
	#endif
	
	EventViewer::EventViewer(unsigned eventId, const QString& eventName, unsigned eventVariablesCount, MainWindow::EventViewers* eventsViewers) :
		eventId(eventId),
		eventsViewers(eventsViewers),
		values(eventVariablesCount),
		startingTime(QTime::currentTime())
	{
		QSettings settings;
		
		// create plot
		plot = new QwtPlot;
		plot->setCanvasBackground(Qt::white);
		plot->setAxisTitle(plot->xBottom, tr("Time (seconds)"));
		plot->setAxisTitle(plot->yLeft, tr("Values"));
		
		QwtLegend *legend = new QwtLegend;
		//legend->setItemMode(QwtLegend::CheckableItem);
		plot->insertLegend(legend, QwtPlot::BottomLegend);
		
		for (size_t i = 0; i < values.size(); i++)
		{
			QwtPlotCurve *curve = new QwtPlotCurve(QString("%0").arg(i));
			#if QWT_VERSION >= 0x060000
			curve->setData(new EventDataWrapper(timeStamps, values[i]));
			#else
			curve->setData(EventDataWrapper(timeStamps, values[i]));
			#endif
			curve->attach(plot);
			curve->setPen(QPen(QColor::fromHsv((i * 360) / values.size(), 255, 100), 2));
		}
		
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->addWidget(plot);
		
		// add control
		QHBoxLayout *controlLayout = new QHBoxLayout;
		
		status = new QLabel(tr("Recording..."));
		controlLayout->addWidget(status);
		
		pauseRunButton = new QPushButton(QPixmap(QString(":/images/pause.png")), tr("&Pause"));
		connect(pauseRunButton, SIGNAL(clicked()), SLOT(pauseRunCapture()));
		controlLayout->addWidget(pauseRunButton);
		
		QPushButton *clearButton = new QPushButton(QPixmap(QString(":/images/reset.png")), tr("&Clear"));
		connect(clearButton, SIGNAL(clicked()), SLOT(clearPlot()));
		controlLayout->addWidget(clearButton);
		
		const bool timeWindowEnabled(settings.value("EventViewer/timeWindowEnabled", false).toBool());
		timeWindowCheckBox = new QCheckBox(tr("time &window:"));
		controlLayout->addWidget(timeWindowCheckBox);
		timeWindowCheckBox->setChecked(timeWindowEnabled);
		
		timeWindowLength = new QDoubleSpinBox;
		timeWindowLength->setSuffix("s");
		connect(timeWindowCheckBox, SIGNAL(toggled(bool)), timeWindowLength, SLOT(setEnabled(bool))); 
		timeWindowLength->setValue(settings.value("EventViewer/timeWindowLength", 10.).toDouble());
		timeWindowLength->setEnabled(timeWindowEnabled);
		controlLayout->addWidget(timeWindowLength);
		controlLayout->addStretch();
		
		QPushButton *saveToFileButton = new QPushButton(QPixmap(QString(":/images/filesaveas.png")), tr("Save &As..."));
		connect(saveToFileButton, SIGNAL(clicked()), SLOT(saveToFile()));
		controlLayout->addWidget(saveToFileButton);
		
		layout->addLayout(controlLayout);
		
		// receive events
		eventsViewers->insert(eventId, this);
		isCapturing = true;
	}
	
	EventViewer::~EventViewer()
	{
		QSettings settings;
		settings.setValue("EventViewer/timeWindowEnabled", timeWindowCheckBox->isChecked());
		settings.setValue("EventViewer/timeWindowLength", timeWindowLength->value());
		if (eventsViewers && isCapturing)
			eventsViewers->remove(eventId, this);
	}
	
	void EventViewer::addData(const VariablesDataVector& data)
	{
		const double elapsedTime = (double)startingTime.msecsTo(QTime::currentTime()) / 1000.;
		if (timeWindowCheckBox->isChecked())
		{
			// remove old data
			while (
				(!timeStamps.empty()) &&
				(elapsedTime - timeStamps[0] > timeWindowLength->value())
			)
			{
				timeStamps.pop_front();
				for (size_t i = 0; i < values.size(); i++)
					values[i].pop_front();
			}
		}
			
			
		timeStamps.push_back(elapsedTime);
		for (size_t i = 0; i < values.size(); i++)
		{
			if (i < data.size())
			{
				values[i].push_back(data[i]);
			}
			else
			{
				values[i].push_back(0);
			}
		}
		plot->replot();
	}
	
	void EventViewer::pauseRunCapture()
	{
		if (isCapturing)
		{
			if (eventsViewers)
				eventsViewers->remove(eventId, this);
			isCapturing = false;
			status->setText(tr("Paused..."));
			pauseRunButton->setIcon(QPixmap(QString(":/images/mix_record.png")));
			pauseRunButton->setText(tr("&Record"));
		}
		else
		{
			if (eventsViewers)
				eventsViewers->insert(eventId, this);
			isCapturing = true;
			status->setText(tr("Recording..."));
			pauseRunButton->setIcon(QPixmap(QString(":/images/pause.png")));
			pauseRunButton->setText(tr("&Pause"));
		}
	}
	
	void EventViewer::clearPlot()
	{
		for (size_t i = 0; i < values.size(); i++)
			values[i].clear();
		timeStamps.clear();
		startingTime = QTime::currentTime();
		plot->replot();
	}
	
	void EventViewer::saveToFile()
	{
		QSettings settings;
		QString lastFileName(settings.value("EventViewer/exportFileName", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString());
		QString filter = "Text files (*.txt);;CSV files (*.csv);;All Files (*)";
		QString fileName = QFileDialog::getSaveFileName(this, tr("Save plot data to file"), lastFileName, filter);
		
		QFile file(fileName);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
			return;
		
		settings.setValue("EventViewer/exportFileName", fileName);
		
		QTextStream out(&file);
		for (size_t i = 0; i < timeStamps.size(); ++i)
		{
			out << timeStamps[i] << " ";
			for (size_t j = 0; j < values.size(); ++j)
			{
				out << values[j][i];
				if (j + 1 < values.size())
					out << " ";
			}
			out << "\n";
		}
	}
	
	/*@}*/
} // namespace Aseba

#endif // HAVE_QWT
