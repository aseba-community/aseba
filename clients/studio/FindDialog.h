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
class QGroupBox;

namespace Aseba
{
	class FindDialog: public QDialog
	{
		 Q_OBJECT
	public:
		FindDialog(QWidget *parent = 0, QTextEdit* editor = 0);
		void setFindText(const QString& text);
		
	protected slots:
		void findNext();
		void findPrevious();
		void findFromTop();
		void replaceFindNext();
		void replaceFindPrevious();
		void replaceAll();
		
	protected:
		bool find(const QTextCursor pos, const QTextDocument::FindFlag dir);
		void replace();
		
	public:
		QTextEdit* editor;
		QGroupBox* replaceGroupBox;
	
	protected:
		QLineEdit *findLineEdit;
		QLineEdit *replaceLineEdit;
		
		// options
		QCheckBox *caseCheckBox;
		QCheckBox *wholeWordsCheckBox;
		QCheckBox *regularExpressionsCheckBox;
		
		// type of search
		QPushButton *findNextButton;
		QPushButton *findPreviousButton;
		QPushButton *findFromTopButton;
		QPushButton *replaceFindNextButton;
		QPushButton *replaceFindPreviousButton;
		QPushButton *replaceAllButton;
		//QPushButton *findFromTopButton;
		
		// warning lineEdit
		QLabel *warningText;
	};
}

#endif // FIND_DIALOG_H
