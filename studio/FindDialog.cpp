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

#include "FindDialog.h"

#include <cassert>

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QTextDocument>
#include <QGridLayout>
#include <QButtonGroup>

#include <FindDialog.moc>

namespace Aseba
{
	FindDialog::FindDialog(QWidget *parent, QTextEdit* editor):
		QDialog(parent),
		editor(editor)
	{
		QLabel* label = new QLabel(tr("&Search for:"));
		lineEdit = new QLineEdit;
		label->setBuddy(lineEdit);
		QHBoxLayout *entryLayout = new QHBoxLayout;
		entryLayout->addWidget(label);
		entryLayout->addWidget(lineEdit);
		
		caseCheckBox = new QCheckBox(tr("&Case sensitive"));
		wholeWordsCheckBox = new QCheckBox(tr("&Whole words"));
		regularExpressionsCheckBox = new QCheckBox(tr("&Regular expressions"));
		QHBoxLayout *optionsLayout = new QHBoxLayout;
		optionsLayout->addWidget(caseCheckBox);
		optionsLayout->addWidget(wholeWordsCheckBox);
		optionsLayout->addWidget(regularExpressionsCheckBox);
		
		findNextButton = new QPushButton(tr("&Find Next"));
		findNextButton->setDefault(true);
		findPreviousButton = new QPushButton(tr("Find &Previous"));
		findFromTopButton = new QPushButton(tr("Find from &Top"));
		QHBoxLayout *buttonsLayout = new QHBoxLayout;
		buttonsLayout->addWidget(findNextButton);
		buttonsLayout->addWidget(findPreviousButton);
		buttonsLayout->addWidget(findFromTopButton);
		
		warningText = new QLabel;
		warningText->setStyleSheet("color: darkred; font-weight: bold");
		
		QVBoxLayout* layout = new QVBoxLayout;
		layout->addLayout(entryLayout);
		layout->addLayout(optionsLayout);
		layout->addLayout(buttonsLayout);
		layout->addWidget(warningText);
		
		setLayout(layout);
		
		connect(findNextButton, SIGNAL(clicked()), SLOT(findNext()));
		connect(findPreviousButton, SIGNAL(clicked()), SLOT(findPrevious()));
		connect(findFromTopButton, SIGNAL(clicked()), SLOT(findFromTop()));
	}
	
	void FindDialog::findNext()
	{
		find(editor->textCursor(), QTextDocument::FindFlag(0));
	}
	
	void FindDialog::findPrevious()
	{
		find(editor->textCursor(), QTextDocument::FindBackward);
	}
	
	void FindDialog::findFromTop()
	{
		find(QTextCursor(editor->document()), QTextDocument::FindFlag(0));
	}
	
	void FindDialog::find(const QTextCursor cc, const QTextDocument::FindFlag dir)
	{
		assert(editor);
		
		QTextDocument::FindFlags flags(dir);
		QTextCursor nc;
		
		if (wholeWordsCheckBox->isChecked())
			flags |= QTextDocument::FindWholeWords;
		
		if (regularExpressionsCheckBox->isChecked())
		{
			Qt::CaseSensitivity cs;
			if (caseCheckBox->isChecked())
				cs = Qt::CaseSensitive;
			else
				cs = Qt::CaseInsensitive;
			nc = editor->document()->find(QRegExp(lineEdit->text(), cs), cc ,flags);
		}
		else
		{
			if (caseCheckBox->isChecked())
				flags |= QTextDocument::FindCaseSensitively;
			nc = editor->document()->find(lineEdit->text(), cc ,flags);
		}
		
		if (nc.isNull())
		{
			nc = QTextCursor(editor->document());
			if (dir == QTextDocument::FindBackward)
				nc.movePosition(QTextCursor::End);
			warningText->setText(tr("End of document reached!"));
		}
		else
			warningText->setText("");
		editor->setTextCursor(nc);
	}
}