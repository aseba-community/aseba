/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

#include "AeslEditor.h"
#include "MainWindow.h"
#include "CustomWidgets.h"
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
						<< "\\bif\\b" << "\\bthen\\b" << "\\belse\\b" << "\\belseif\\b"
						<< "\\bend\\b" << "\\bvar\\b" << "\\bcall\\b"
						<< "\\bonevent\\b" << "\\bontimer\\b" << "\\bwhen\\b"
						<< "\\band\\b" << "\\bor\\b" << "\\bnot\\b" 
						<< "\\bsub\\b" << "\\bcallsub\\b";
		foreach (QString pattern, keywordPatterns)
		{
			rule.pattern = QRegExp(pattern);
			rule.format = keywordFormat;
			highlightingRules.append(rule);
		}
		
		// literals
		QTextCharFormat literalsFormat;
		literalsFormat.setForeground(Qt::darkBlue);
		rule.pattern = QRegExp("\\b(-{0,1}\\d+|0x([0-9]|[a-f]|[A-F])+|0b[0-1]+)\\b");
		rule.format = literalsFormat;
		highlightingRules.append(rule);
		
		// comments #
		QTextCharFormat commentFormat;
		commentFormat.setForeground(Qt::gray);
		rule.pattern = QRegExp("[^\\*]{1}#(?!\\*).*");   // '#' without '*' right after or before
		rule.format = commentFormat;
		highlightingRules.append(rule);

		rule.pattern = QRegExp("^#(?!\\*).*");           // '#' without '*' right after + beginning of line
		rule.format = commentFormat;
		highlightingRules.append(rule);

		// comments #* ... *#
		rule.pattern = QRegExp("#\\*.*\\*#");
		rule.format = commentFormat;
		highlightingRules.append(rule);
		
		// multilines block of comments #* ... [\n]* ... *#
		commentBlockRules.begin = QRegExp("#\\*(?!.*\\*#)");    // '#*' with no corresponding '*#'
		commentBlockRules.end = QRegExp(".*\\*#");
		commentBlockRules.format = commentFormat;

		// todo/fixme
		QTextCharFormat todoFormat;
		todoFormat.setForeground(Qt::black);
		todoFormat.setBackground(QColor(255, 192, 192));
		rule.pattern = QRegExp("#.*(\\bTODO\\b|\\bFIXME\\b).*");
		rule.format = todoFormat;
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
		

		// Prepare the format for multilines comment block
		int index;
		QTextCharFormat format = commentBlockRules.format;

		// Comply with the breakpoint formatting
		if (isBreakpointPending)
			format.setBackground(breakpointPendingColor);

		// Search for a multilines comment block
		setCurrentBlockState(NO_COMMENT);                // No comment
		if (previousBlockState() != COMMENT)
		{
			// Search for a beginning
			if ((index = text.indexOf(commentBlockRules.begin)) != -1)
			{
				// Found one
				setFormat(index, text.length() - index, format);
				setCurrentBlockState(COMMENT);
			}
		}
		else
		{
			// Search for an end
			if ((index = text.indexOf(commentBlockRules.end)) != -1)
			{
				// Found one
				int length = commentBlockRules.end.matchedLength();
				setFormat(0, length, format);
				setCurrentBlockState(NO_COMMENT);
			}
			else
			{
				// Still inside a block
				setFormat(0, text.length(), format);
				setCurrentBlockState(COMMENT);
			}
		}

		// error word in red
		if (uData && uData->properties.contains("errorPos"))
		{
			int pos = uData->properties["errorPos"].toInt();
			int len = 0;

			if (pos + len < text.length())
			{
				// find length of number or word
				while (pos + len < text.length())
					if (
						(!text[pos + len].isDigit()) &&
						(!text[pos + len].isLetter()) &&
						(text[pos + len] != '_') &&
						(text[pos + len] != '.')
					)
						break;
					else
						len++;
			}
			len = len > 0 ? len : 1;
			setFormat(pos, len, Qt::red);
		}
	}
	
	AeslEditor::AeslEditor(const ScriptTab* tab) :
		tab(tab),
		debugging(false),
		dropSourceWidget(0)
	{
		QFont font;
		font.setFamily("");
		font.setStyleHint(QFont::TypeWriter);
		font.setFixedPitch(true);
		//font.setPointSize(10);
		setFont(font);
		setAcceptDrops(true);
		setAcceptRichText(false);
		setTabStopWidth( QFontMetrics(font).width(' ') * 4);
	}
	
	void AeslEditor::dropEvent(QDropEvent *event)
	{
		dropSourceWidget = event->source();
		QTextEdit::dropEvent(event);
		dropSourceWidget = 0;
		setFocus(Qt::MouseFocusReason);
	}
	
	void AeslEditor::insertFromMimeData ( const QMimeData * source )
	{
		QTextCursor cursor(this->textCursor());
		
		// check whether we are at the beginning of a line
		bool startOfLine(cursor.atBlockStart());
		const int posInBlock(cursor.position() - cursor.block().position());
		if (!startOfLine && posInBlock)
		{
			const QString startText(cursor.block().text().left(posInBlock));
			startOfLine = !startText.contains(QRegExp("\\S"));
		}
		
		// if beginning of a line and source widget is known, add some helper text
		if (dropSourceWidget && startOfLine)
		{
			const NodeTab* nodeTab(dynamic_cast<const NodeTab*>(tab));
			QString prefix(""); // before the text
			QString midfix(""); // between the text and the cursor
			QString postfix(""); // after the curser
			if (dropSourceWidget == nodeTab->vmFunctionsView)
			{
				// inserting function
				prefix = "call ";
				midfix = "(";
				// fill call from doc
				const TargetDescription *desc = nodeTab->vmFunctionsModel->descriptionRead;
				const std::wstring funcName = source->text().toStdWString();
				for (size_t i = 0; i < desc->nativeFunctions.size(); i++)
				{
					const TargetDescription::NativeFunction native(desc->nativeFunctions[i]);
					if (native.name == funcName)
					{
						for (size_t j = 0; j < native.parameters.size(); ++j)
						{
							postfix += QString::fromStdWString(native.parameters[j].name);
							if (j + 1 < native.parameters.size())
								postfix += ", ";
						}
						break;
					}
				}
				postfix += ")\n";
			}
			else if (dropSourceWidget == nodeTab->vmMemoryView)
			{
				midfix = " ";
			}
			else if (dropSourceWidget == nodeTab->vmLocalEvents)
			{
				// inserting local event
				prefix = "onevent ";
				midfix = "\n";
			}
			else if (dropSourceWidget == nodeTab->mainWindow->eventsDescriptionsView)
			{
				// inserting global event
				prefix = "onevent ";
				midfix = "\n";
			}
			
			cursor.beginEditBlock();
			cursor.insertText(prefix + source->text() + midfix);
			const int pos = cursor.position();
			cursor.insertText(postfix);
			cursor.setPosition(pos);
			cursor.endEditBlock();

			this->setTextCursor(cursor);
		}
		else
			cursor.insertText(source->text());
		
	}
	
	void AeslEditor::contextMenuEvent ( QContextMenuEvent * e )
	{
		// create menu
		QMenu *menu = createStandardContextMenu();
		if (!isReadOnly())
		{
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
		}
		else
			menu->exec(e->globalPos());
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
					if ((cursor.selectedText() == "\t") ||
						(	(cursor.selectedText() == " ") &&
							(cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 3)) &&
							(cursor.selectedText() == "    ")
						)
					)
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
				if ((cursor.selectedText() == "\t") ||
					(	(cursor.selectedText() == " ") &&
						(cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 3)) &&
						(cursor.selectedText() == "    ")
					)
				)
					cursor.removeSelectedText();
			}
			else
				cursor.insertText("\t");
				
			cursor.endEditBlock();
			return;
		}
		// handle indentation and new line
		else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
		{
			QString headingSpace("\n");
			const QString &line = textCursor().block().text();
			for (size_t i = 0; i < (size_t)line.length(); i++)
			{
				const QChar c(line[(unsigned)i]);
				if (c.isSpace())
					headingSpace += c;
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
