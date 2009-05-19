/*
	Challenge - Virtual Robot Challenge System
	Copyright (C) 1999 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2009:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Mobots group (http://mobots.epfl.ch)
		Laboratory of Robotics Systems (http://lsro.epfl.ch)
		EPFL, Lausanne (http://www.epfl.ch)
	
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

#ifndef __CHALLENGE_H
#define __CHALLENGE_H

#include <viewer/Viewer.h>
#include <QDialog>

class QPushButton;
class QCheckBox;
class QComboBox;
class QTextBrowser;

namespace Enki
{
	class ChallengeViewer : public ViewerWidget
	{
		Q_OBJECT
		
	protected:
		bool savingVideo;
		int ePuckCount;
		QPushButton* addRobotButton;
		QPushButton* delRobotButton;
		QCheckBox* autoCameraButtons;
		QCheckBox* hideButtons;
		QPushButton* helpButton;
		QTextBrowser* helpViewer;
		
		QFont titleFont;
		QFont entryFont;
		QFont labelFont;
		
	public:
		ChallengeViewer(World* world, int ePuckCount);
	
	signals:
		void windowClosed();
	
	protected slots:
		void addNewRobot();	
		void removeRobot();
	
	protected:
		virtual void timerEvent(QTimerEvent * event);
		virtual void mouseMoveEvent ( QMouseEvent * event );
		virtual void keyPressEvent ( QKeyEvent * event );
		virtual void keyReleaseEvent ( QKeyEvent * event );
		virtual void closeEvent ( QCloseEvent * event );
		
		void drawQuad2D(double x, double y, double w, double ar);
		
		virtual void initializeGL();
		
		virtual void renderObjectsTypesHook();
		virtual void displayObjectHook(PhysicalObject *object);
		virtual void sceneCompletedHook();
	};
}

class LanguageSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	QComboBox* languageSelectionBox;
	
	LanguageSelectionDialog();
};

#endif

