/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
		ASL, Autonomous Systems Lab, ETHZ, Zurich
	
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

#ifndef FIND_DIALOG_H
#define FIND_DIALOG_H

#include <QDialog>
#include <QTextDocument>
#include <QTextCursor>

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QTextEdit;

namespace Aseba
{
	class FindDialog: public QDialog
	{
		 Q_OBJECT
	public:
		FindDialog(QWidget *parent = 0, QTextEdit* editor = 0);
	
	protected slots:
		void findNext();
		void findPrevious();
		void findFromTop();
		
	protected:
		void find(const QTextCursor pos, const QTextDocument::FindFlag dir);
		
	public:
		QTextEdit* editor;
	
	protected:
		QLineEdit *lineEdit;
		
		// options
		QCheckBox *caseCheckBox;
		QCheckBox *wholeWordsCheckBox;
		QCheckBox *regularExpressionsCheckBox;
		
		// type of search
		QPushButton *findNextButton;
		QPushButton *findPreviousButton;
		QPushButton *findFromTopButton;
		
		// warning lineEdit
		QLabel *warningText;
	};
}

#endif // FIND_DIALOG_H
