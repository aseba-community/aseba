/*
	Challenge - Virtual Robot Challenge System
	Copyright (C) 1999--2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
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

#ifndef __CHALLENGE_H
#define __CHALLENGE_H

#include <viewer/Viewer.h>
#include <QDialog>
#include <QWidget>

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

		bool autoCamera;
		
		QFont titleFont;
		QFont entryFont;
		QFont labelFont;
		
	public:
		ChallengeViewer(World* world, int ePuckCount);
	
	public slots:
		void addNewRobot();
		void removeRobot();
		void autoCameraStateChanged(bool state);
	
	protected:
		virtual void timerEvent(QTimerEvent * event);
		//virtual void mouseMoveEvent ( QMouseEvent * event );
		virtual void keyPressEvent ( QKeyEvent * event );
		virtual void keyReleaseEvent ( QKeyEvent * event );
		
		void drawQuad2D(double x, double y, double w, double ar);
		
		virtual void initializeGL();
		
		virtual void renderObjectsTypesHook();
		virtual void displayObjectHook(PhysicalObject *object);
		virtual void sceneCompletedHook();
	};

	class ChallengeApplication : public QWidget
	{
		Q_OBJECT

	protected:
		ChallengeViewer viewer;

		QTextBrowser* helpViewer;

	public:
		ChallengeApplication(World* world, int ePuckCount);
		
	public slots:
		void fullScreenStateChanged(bool fullScreen);

	signals:
		void windowClosed();

	protected:
		virtual void closeEvent ( QCloseEvent * event );
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

