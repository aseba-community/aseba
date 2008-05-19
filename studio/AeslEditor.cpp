/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
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

#include "AeslEditor.h"
#include <QtGui>

#include <AeslEditor.moc>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	AeslHighlighter::AeslHighlighter(AeslEditor *editor, QTextDocument *parent) :
		QSyntaxHighlighter(parent),
		editor(editor)
	{
		HighlightingRule rule;
		
		// keywords
		QTextCharFormat keywordFormat;
		keywordFormat.setForeground(Qt::darkRed);
		QStringList keywordPatterns;
		keywordPatterns << "\\bemit\\b" << "\\bwhile\\b" << "\\bdo\\b"
						<< "\\bfor\\b" << "\\bin\\b" << "\\bstep\\b"
						<< "\\bif\\b" << "\\bthen\\b" << "\\belse\\b"
						<< "\\bend\\b" << "\\bvar\\b" << "\\bcall\\b"
						<< "\\bonevent\\b" << "\\bontimer\\b" << "\\bwhen\\b";
		foreach (QString pattern, keywordPatterns)
		{
			rule.pattern = QRegExp(pattern);
			rule.format = keywordFormat;
			highlightingRules.append(rule);
		}
		
		// literals
		QTextCharFormat literalsFormat;
		literalsFormat.setForeground(Qt::darkBlue);
		rule.pattern = QRegExp("\\b-{0,1}[0-9]+\\b");
		rule.format = literalsFormat;
		highlightingRules.append(rule);
		
		// comments
		QTextCharFormat commentFormat;
		commentFormat.setForeground(Qt::gray);
		rule.pattern = QRegExp("#[^\n]*");
		rule.format = commentFormat;
		highlightingRules.append(rule);
	}
	
	void AeslHighlighter::highlightBlock(const QString &text)
	{
		AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(currentBlockUserData());
		
		// current line background blue
		bool isActive = uData && uData->properties.contains("active");
		bool isExecutionError = uData && uData->properties.contains("executionError");
		bool isBreakpointPending = uData && uData->properties.contains("breakpointPending");
		bool isBreakpoint = uData && uData->properties.contains("breakpoint");
		
		QColor breakpointPendingColor(255, 240, 178);
		QColor breakpointColor(255, 211, 178);
		QColor activeColor(220, 220, 255);
		QColor errorColor(240, 100, 100);
		
		if (isBreakpointPending)
		{
			QTextCharFormat format;
			format.setBackground(breakpointPendingColor);
			setFormat(0, text.length(), format);
		}
		if (isBreakpoint)
		{
			QTextCharFormat format;
			//format.setBackground(QColor(255, 231, 185));
			format.setBackground(breakpointColor);
			setFormat(0, text.length(), format);
		}
		if (editor->debugging)
		{
			if (isActive)
			{
				QTextCharFormat format;
				format.setBackground(activeColor);
				setFormat(0, text.length(), format);
			}
			if (isExecutionError)
			{
				QTextCharFormat format;
				format.setBackground(errorColor);
				setFormat(0, text.length(), format);
			}
		}
		
		// syntax highlight
		foreach (HighlightingRule rule, highlightingRules)
		{
			QRegExp expression(rule.pattern);
			int index = text.indexOf(expression);
			while (index >= 0)
			{
				int length = expression.matchedLength();
				QTextCharFormat format = rule.format;
				
				if (isBreakpointPending)
					format.setBackground(breakpointPendingColor);
				if (isBreakpoint)
					format.setBackground(breakpointColor);
				if (editor->debugging)
				{
					if (isActive)
						format.setBackground(activeColor);
					if (isExecutionError)
						format.setBackground(errorColor);
				}
				
				setFormat(index, length, format);
				index = text.indexOf(expression, index + length);
			}
		}
		
		// error word in red
		if (uData && uData->properties.contains("errorPos"))
		{
			int pos = uData->properties["errorPos"].toInt();
			int len = 0;
			
			if (pos + len < text.length())
			{
				if (text[pos + len].isDigit())
				{
					// find length of number
					while (pos + len < text.length())
						if (!text[pos + len].isDigit())
							break;
						else
							len++;
				}
				else
				{
					// find length of word
					while (pos + len < text.length())
						if (!text[pos + len].isLetter() && (text[pos + len] != '_'))
							break;
						else
							len++;
				}
			}
			len = len > 0 ? len : 1;
			setFormat(pos, len, Qt::red);
		}
	}
	
	void AeslEditor::contextMenuEvent ( QContextMenuEvent * e )
	{
		// create menu
		QMenu *menu = createStandardContextMenu();
		QAction *breakpointAction;
		menu->addSeparator();
		
		// check for breakpoint
		QTextBlock block = cursorForPosition(e->pos()).block();
		AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(block.userData());
		bool breakpointPresent = (uData && (uData->properties.contains("breakpoint") || uData->properties.contains("breakpointPending") )) ;
		
		// add action
		if (breakpointPresent)
			breakpointAction = menu->addAction(tr("Clear breakpoint"));
		else
			breakpointAction = menu->addAction(tr("Set breakpoint"));
		QAction *breakpointClearAllAction = menu->addAction(tr("Clear all breakpoints"));
		
		// execute menu
		QAction* selectedAction = menu->exec(e->globalPos());
		
		// do actions
		if (selectedAction == breakpointAction)
		{
			// modify editor state
			if (breakpointPresent)
			{
				// clear breakpoint
				uData->properties.remove("breakpointPending");
				uData->properties.remove("breakpoint");
				if (uData->properties.isEmpty())
				{
					// garbage collect UserData
					delete uData;
					block.setUserData(0);
				}
				emit breakpointCleared(cursorForPosition(e->pos()).blockNumber());
			}
			else
			{
				// set breakpoint
				if (!uData)
				{
					// create user data
					uData = new AeslEditorUserData("breakpointPending");
					block.setUserData(uData);
				}
				else
					uData->properties.insert("breakpointPending", QVariant());
				emit breakpointSet(cursorForPosition(e->pos()).blockNumber());
			}
		}
		if (selectedAction == breakpointClearAllAction)
		{
			for (QTextBlock it = document()->begin(); it != document()->end(); it = it.next())
			{
				AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(it.userData());
				if (uData)
				{
					uData->properties.remove("breakpoint");
					uData->properties.remove("breakpointPending");
				}
			}
			emit breakpointClearedAll();
		}
		delete menu;
	}
	
	void AeslEditor::keyPressEvent(QKeyEvent * event)
	{
		// handle tab and control tab
		if ((event->key() == Qt::Key_Tab) && textCursor().hasSelection())
		{
			QTextCursor cursor(document()->findBlock(textCursor().selectionStart()));
			
			cursor.beginEditBlock();
			
			while (cursor.position() < textCursor().selectionEnd())
			{
				cursor.movePosition(QTextCursor::StartOfLine);
				if (event->modifiers() & Qt::ControlModifier)
				{
					cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
					if (cursor.selectedText() == "\t")
						cursor.removeSelectedText();
				}
				else
					cursor.insertText("\t");
				cursor.movePosition(QTextCursor::Down);
				cursor.movePosition(QTextCursor::EndOfLine);
			}
			
			cursor.movePosition(QTextCursor::StartOfLine);
			if (event->modifiers() & Qt::ControlModifier)
			{
				cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
				if (cursor.selectedText() == "\t")
					cursor.removeSelectedText();
			}
			else
				cursor.insertText("\t");
				
			cursor.endEditBlock();
			return;
		}
		
		// handle indentation and new line
		if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
		{
			QString headingSpace("\n");
			const QString &line = textCursor().block().text();
			for (size_t i = 0; i < line.length(); i++)
			{
				if (line[i].isSpace())
					headingSpace += line[i];
				else
					break;
					
			}
			insertPlainText(headingSpace);
		}
		else
			QTextEdit::keyPressEvent(event);
		
	}
	
	/*@}*/
}; // Aseba
