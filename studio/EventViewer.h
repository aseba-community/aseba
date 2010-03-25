/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EVENT_VIEWER_H
#define EVENT_VIEWER_H

#ifdef HAVE_QWT

#ifdef _MSC_VER
#define QWT_DLL
#endif // _MSC_VER

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>

#include <QTime>

#include "MainWindow.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
		
	class EventViewer:  public QwtPlot
	{
	protected:
		unsigned eventId;
		MainWindow::EventViewers* eventsViewers;
		
		std::vector<VariablesDataVector> values;
		std::vector<double> timeStamps;
		QTime startingTime;
	
	public:
		EventViewer(unsigned eventId, const QString& eventName, unsigned eventVariablesCount, MainWindow::EventViewers* eventsViewers);
		virtual ~EventViewer();
		
		void detachFromMain() { eventsViewers=0; }
		void addData(const VariablesDataVector& data);
	};
	
	/*@}*/
}; // Aseba

#endif // HAVE_QWT

#endif // EVENT_VIEWER_H
