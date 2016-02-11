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
#include <QGroupBox>

namespace Aseba
{
	FindDialog::FindDialog(QWidget *parent, QTextEdit* editor):
		QDialog(parent),
		editor(editor)
	{
		setWindowTitle(tr("Aseba Studio - Search and Replace"));
		
		QLabel* label = new QLabel(tr("&Search for:"));
		findLineEdit = new QLineEdit;
		label->setBuddy(findLineEdit);
		QHBoxLayout *entryLayout = new QHBoxLayout;
		entryLayout->addWidget(label);
		entryLayout->addWidget(findLineEdit);
		
		caseCheckBox = new QCheckBox(tr("&Case sensitive"));
		wholeWordsCheckBox = new QCheckBox(tr("&Whole words"));
		regularExpressionsCheckBox = new QCheckBox(tr("Re&gular expressions"));
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
		
		replaceGroupBox = new QGroupBox(tr("&Replace"));
		replaceGroupBox->setCheckable(true);
		replaceGroupBox->setChecked(false);
		label = new QLabel(tr("w&ith:"));
		QHBoxLayout *entryLayout2 = new QHBoxLayout;
		replaceLineEdit = new QLineEdit;
		label->setBuddy(replaceLineEdit);
		entryLayout2->addWidget(label);
		entryLayout2->addWidget(replaceLineEdit);
		replaceFindNextButton = new QPushButton(tr("Replace and\nFind &Next"));
		replaceFindPreviousButton = new QPushButton(tr("Replace and\nFind Previo&us"));
		replaceAllButton = new QPushButton(tr("Replace &All\nOccurrences"));
		QHBoxLayout *buttonsLayout2 = new QHBoxLayout;
		buttonsLayout2->addWidget(replaceFindNextButton);
		buttonsLayout2->addWidget(replaceFindPreviousButton);
		buttonsLayout2->addWidget(replaceAllButton);
		QVBoxLayout *replaceLayout = new QVBoxLayout;
		replaceLayout->addLayout(entryLayout2);
		replaceLayout->addLayout(buttonsLayout2);
		replaceGroupBox->setLayout(replaceLayout);
		
		QVBoxLayout* layout = new QVBoxLayout;
		layout->addLayout(entryLayout);
		layout->addLayout(optionsLayout);
		layout->addLayout(buttonsLayout);
		layout->addWidget(warningText);
		layout->addWidget(replaceGroupBox);
		
		setLayout(layout);
		
		connect(findNextButton, SIGNAL(clicked()), SLOT(findNext()));
		connect(findPreviousButton, SIGNAL(clicked()), SLOT(findPrevious()));
		connect(findFromTopButton, SIGNAL(clicked()), SLOT(findFromTop()));
		connect(replaceFindNextButton, SIGNAL(clicked()), SLOT(replaceFindNext()));
		connect(replaceFindPreviousButton, SIGNAL(clicked()), SLOT(replaceFindPrevious()));
		connect(replaceAllButton, SIGNAL(clicked()), SLOT(replaceAll()));
	}
	
	void FindDialog::setFindText(const QString& text)
	{
		findLineEdit->setText(text);
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
	
	void FindDialog::replaceFindNext()
	{
		replace();
		find(editor->textCursor(), QTextDocument::FindFlag(0));
	}
	
	void FindDialog::replaceFindPrevious()
	{
		replace();
		find(editor->textCursor(), QTextDocument::FindBackward);
	}
	
	void FindDialog::replaceAll()
	{
		QTextCursor c(editor->document());
		while (find(c, QTextDocument::FindFlag(0)))
		{
			replace();
			c = editor->textCursor();
		}
	}
	
	bool FindDialog::find(const QTextCursor cc, const QTextDocument::FindFlag dir)
	{
		assert(editor);
		
		//if (cc.hasSelection())
		
		QTextDocument::FindFlags flags(dir);
		QTextCursor nc;
		
		// search text
		if (wholeWordsCheckBox->isChecked())
			flags |= QTextDocument::FindWholeWords;
		if (regularExpressionsCheckBox->isChecked())
		{
			Qt::CaseSensitivity cs;
			if (caseCheckBox->isChecked())
				cs = Qt::CaseSensitive;
			else
				cs = Qt::CaseInsensitive;
			nc = editor->document()->find(QRegExp(findLineEdit->text(), cs), cc ,flags);
		}
		else
		{
			if (caseCheckBox->isChecked())
				flags |= QTextDocument::FindCaseSensitively;
			nc = editor->document()->find(findLineEdit->text(), cc ,flags);
		}
		
		// do something with found text
		if (nc.isNull())
		{
			nc = QTextCursor(editor->document());
			if (dir == QTextDocument::FindBackward)
				nc.movePosition(QTextCursor::End);
			warningText->setText(tr("End of document reached!"));
			editor->setTextCursor(nc);
			return false;
		}
		else
		{
			warningText->setText("");
			editor->setTextCursor(nc);
			return true;
		}
	}
	
	void FindDialog::replace()
	{
		QTextCursor cc(editor->textCursor());
		if (cc.isNull() || !cc.hasSelection())
			return;
		if (replaceGroupBox->isChecked())
		{
			const int p = std::min(cc.anchor(), cc.position());
			const int l = replaceLineEdit->text().length();
			cc.insertText(replaceLineEdit->text());
			cc.setPosition(p);
			cc.setPosition(p+l, QTextCursor::KeepAnchor);
			editor->setTextCursor(cc);
		}
	}
}
