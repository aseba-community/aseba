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

#include "AeslEditor.h"
#include "CustomWidgets.h"
#include "../../common/utils/utils.h"
#include <QtGui>
#include <QtGlobal>

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
						<< "\\bend\\b" << "\\bvar\\b" << "\\bconst\\b" << "\\bcall\\b"
						<< "\\bonevent\\b" << "\\bontimer\\b" << "\\bwhen\\b"
						<< "\\band\\b" << "\\bor\\b" << "\\bnot\\b" << "\\babs\\b"
						<< "\\bsub\\b" << "\\bcallsub\\b" << "\\breturn\\b";
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
		commentFormat.setForeground(Qt::darkGreen);
		rule.pattern = QRegExp("(?!\\*)#(?!\\*).*");   // '#' without '*' right after or before
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
		AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(currentBlockUserData());
		
		// current line background blue
		bool isActive = uData && uData->properties.contains("active");
		bool isExecutionError = uData && uData->properties.contains("executionError");
		bool isBreakpointPending = uData && uData->properties.contains("breakpointPending");
		bool isBreakpoint = uData && uData->properties.contains("breakpoint");
		
		QColor breakpointPendingColor(255, 240, 178);
		QColor breakpointColor(255, 211, 178);
		QColor activeColor(220, 220, 255);
		QColor errorColor(240, 100, 100);

		/*
		// This code causes trash, should be investigated in details.
		// use the QTextEdit::ExtraSelection class to highlight the background
		// of special lines (breakpoints, active line,...)
		QList<QTextEdit::ExtraSelection> extraSelections;
		if (currentBlock().blockNumber() != 0)
			// past the first line, we recall the previous "selections"
			extraSelections = editor->extraSelections();

		// prepare the current "selection"
		QTextEdit::ExtraSelection selection;
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = QTextCursor(currentBlock());

		if (isBreakpointPending)
			selection.format.setBackground(breakpointPendingColor);
		if (isBreakpoint)
			selection.format.setBackground(breakpointColor);
		if (editor->debugging)
		{
			if (isActive)
				selection.format.setBackground(activeColor);
			if (isExecutionError)
				selection.format.setBackground(errorColor);
		}
		
		// we are done
		extraSelections.append(selection);
		editor->setExtraSelections(extraSelections);
		*/
		
		// This is backup code in case the ExtraSelection creates trashes
		QColor specialBackground("white");
		if (isBreakpointPending)
			specialBackground = breakpointPendingColor;
		if (isBreakpoint)
			specialBackground = breakpointColor;
		if (editor->debugging)
		{
			if (isActive)
				specialBackground = activeColor;
			if (isExecutionError)
				specialBackground = errorColor;
		}
		if (specialBackground != "white")
		{
			QTextCharFormat format;
			format.setBackground(specialBackground);
			setFormat(0, text.length(), format);
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
				
				// This is backup code in case the ExtraSelection creates trashes
				if (specialBackground != "white")
					format.setBackground(specialBackground);
				
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

	AeslEditorSidebar::AeslEditorSidebar(AeslEditor* editor) :
		QWidget(editor),
		editor(editor),
		currentSizeHint(0,0),
		verticalScroll(0)
	{
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
		connect(editor->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(scroll(int)));
		connect(editor, SIGNAL(textChanged()), SLOT(update()));
	}

	void AeslEditorSidebar::scroll(int dy)
	{
		verticalScroll = dy;
		update();		// repaint
	}

	QSize AeslEditorSidebar::sizeHint() const
	{
		return QSize(idealWidth(), 0);
	}

	void AeslEditorSidebar::paintEvent(QPaintEvent *event)
	{
		QSize newSizeHint = sizeHint();

		if (currentSizeHint != newSizeHint)
		{
			// geometry has changed, recompute the layout based on sizeHint()
			updateGeometry();
			currentSizeHint = newSizeHint;
		}
	}

	int AeslEditorSidebar::posToLineNumber(int y)
	{

		// iterate over all text blocks
		QTextBlock block = editor->document()->firstBlock();
		int offset = editor->contentsRect().top();		// top margin

		while (block.isValid())
		{
			QRectF bounds = block.layout()->boundingRect();
			bounds.translate(0, offset + block.layout()->position().y() - verticalScroll);
			if (y > bounds.top() && y < bounds.bottom())
			{
				// this is it
				return block.blockNumber();
			}

			block = block.next();
		}

		// not found
		return -1;
	}

	AeslLineNumberSidebar::AeslLineNumberSidebar(AeslEditor *editor) :
		AeslEditorSidebar(editor)
	{
	}

	void AeslLineNumberSidebar::showLineNumbers(bool state)
	{
		setVisible(state);
	}

	void AeslLineNumberSidebar::paintEvent(QPaintEvent *event)
	{
		AeslEditorSidebar::paintEvent(event);

		// begin painting
		QPainter painter(this);

		// fill the background
		painter.fillRect(event->rect(), QColor(210, 210, 210));

		// get the editor's painting area
		QRect editorRect = editor->contentsRect();

		// enable clipping to match the vertical painting area of the editor
		painter.setClipRect(QRect(0, editorRect.top(), width(), editorRect.bottom()), Qt::ReplaceClip );
		painter.setClipping(true);

		// define the painting area for linenumbers (top / bottom are identic to the editor))
		QRect region = editorRect;
		region.setLeft(0);
		region.setRight(idealWidth());

		// get the first text block
		QTextBlock block = editor->document()->firstBlock();

		painter.setPen(Qt::darkGray);
		// set font from editor
		painter.setFont(editor->font());
		
		// iterate over all text blocks
		// FIXME: do clever painting
		while(block.isValid())
		{
			if (block.isVisible())
			{
				QString number = QString::number(block.blockNumber() + 1);
				// paint the line number
				int y = block.layout()->position().y() + region.top() - verticalScroll;
				painter.drawText(0, y, width(), editor->fontMetrics().height(), Qt::AlignRight, number);
			}

			block = block.next();
		}
	}

	int AeslLineNumberSidebar::idealWidth() const
	{
		// This is based on the Qt code editor example
		// http://doc.qt.nokia.com/latest/widgets-codeeditor.html
		int digits = 1;
		int linenumber = editor->document()->blockCount();
		while (linenumber >= 10) {
			linenumber /= 10;
			digits++;
		}

		int space = 3 + editor->fontMetrics().width(QLatin1Char('9')) * digits;

		return space;
	}

	AeslBreakpointSidebar::AeslBreakpointSidebar(AeslEditor *editor) :
		AeslEditorSidebar(editor),
		borderSize(1)
	{
		
	}

	void AeslBreakpointSidebar::paintEvent(QPaintEvent *event)
	{
		AeslEditorSidebar::paintEvent(event);
		
		// define the breakpoint geometry, according to the font metrics from the editor
		const int lineSpacing = editor->fontMetrics().lineSpacing();
		const QRect breakpoint(borderSize, borderSize, lineSpacing - 2*borderSize, lineSpacing - 2*borderSize);

		// begin painting
		QPainter painter(this);

		// fill the background
		painter.fillRect(event->rect(), QColor(210, 210, 210));

		// get the editor's painting area
		QRect editorRect = editor->contentsRect();

		// enable clipping to match the vertical painting area of the editor
		painter.setClipRect(QRect(0, editorRect.top(), width(), editorRect.bottom()), Qt::ReplaceClip );
		painter.setClipping(true);

		// define the painting area for breakpoints (top / bottom are identic to the editor)
		QRect region = editorRect;
		region.setLeft(0);
		region.setRight(idealWidth());

		// get the first text block
		QTextBlock block = editor->document()->firstBlock();

		painter.setPen(Qt::red);
		painter.setBrush(Qt::red);
		// iterate over all text blocks
		// FIXME: do clever painting
		while(block.isValid())
		{
			if (block.isVisible())
			{
				// get the block data and check for a breakpoint
				if (editor->isBreakpoint(block))
				{
					// paint the breakpoint
					int y = block.layout()->position().y() + region.top() - verticalScroll;
					// FIXME: Hughly breakpoing design...
					painter.drawRect(breakpoint.translated(0, y));
				}

			}

			block = block.next();
		}
	}

	void AeslBreakpointSidebar::mousePressEvent(QMouseEvent *event)
	{
		// click in the breakpoint column
		int line = posToLineNumber(event->pos().y());
		editor->toggleBreakpoint(editor->document()->findBlockByNumber(line));
	}

	int AeslBreakpointSidebar::idealWidth() const
	{
		// get metrics from editor
		const int lineSpacing = editor->fontMetrics().lineSpacing();
		const QRect breakpoint(borderSize, borderSize, lineSpacing - 2*borderSize, lineSpacing - 2*borderSize);
		return breakpoint.width() + 2*borderSize;
	}

	AeslEditor::AeslEditor() :
		debugging(false),
		completer(0),
		vardefRegexp("^var .*"),
		constdefRegexp("^const .*"),
		leftValueRegexp("^\\w+\\s*=.*"),
		previousContext(UnknownContext),
		editingLeftValue(false)
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

#ifdef Q_WS_WIN
		// Fix selection color on Windows when the find dialog is active
		// See issue 93: https://github.com/aseba-community/aseba/issues/93
		// Code from: http://stackoverflow.com/questions/9880441/qt-how-to-display-selected-text-in-an-inactive-window
		QPalette p = palette();
		p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
		p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
		setPalette(p);
#endif // Q_WS_WIN

		// create completer
		completer = new QCompleter(this);
		//completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setWrapAround(false);
		completer->setWidget(this);
		completer->setCompletionMode(QCompleter::PopupCompletion);
		QObject::connect(completer, SIGNAL(activated(QString)), SLOT(insertCompletion(QString)));
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
			//AeslEditorUserData *uData = polymorphic_downcast<AeslEditorUserData *>(block.userData());
			bool breakpointPresent = isBreakpoint(block);
			
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
					clearBreakpoint(block);
				}
				else
				{
					// set breakpoint
					setBreakpoint(block);
				}
			}
			if (selectedAction == breakpointClearAllAction)
			{
				clearAllBreakpoints();
			}
		}
		else
			menu->exec(e->globalPos());
		delete menu;
	}

	bool AeslEditor::isBreakpoint()
	{
		return isBreakpoint(textCursor().block());
	}

	bool AeslEditor::isBreakpoint(QTextBlock block)
	{
		AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
		return (uData && (uData->properties.contains("breakpoint") || uData->properties.contains("breakpointPending") )) ;
	}

	bool AeslEditor::isBreakpoint(int line)
	{
		QTextBlock block = document()->findBlockByNumber(line);
		return isBreakpoint(block);
	}

	void AeslEditor::toggleBreakpoint()
	{
		toggleBreakpoint(textCursor().block());
	}

	void AeslEditor::toggleBreakpoint(QTextBlock block)
	{
		if (isBreakpoint(block))
			clearBreakpoint(block);
		else
			setBreakpoint(block);
	}

	void AeslEditor::setBreakpoint()
	{
		setBreakpoint(textCursor().block());
	}

	void AeslEditor::setBreakpoint(QTextBlock block)
	{
		AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
		if (!uData)
		{
			// create user data
			uData = new AeslEditorUserData("breakpointPending");
			block.setUserData(uData);
		}
		else
			uData->properties.insert("breakpointPending", QVariant());
		emit breakpointSet(block.blockNumber());
	}

	void AeslEditor::clearBreakpoint()
	{
		clearBreakpoint(textCursor().block());
	}

	void AeslEditor::clearBreakpoint(QTextBlock block)
	{
		AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
		uData->properties.remove("breakpointPending");
		uData->properties.remove("breakpoint");
		if (uData->properties.isEmpty())
		{
			// garbage collect UserData
			block.setUserData(0);
		}
		emit breakpointCleared(block.blockNumber());
	}

	void AeslEditor::clearAllBreakpoints()
	{
		for (QTextBlock it = document()->begin(); it != document()->end(); it = it.next())
		{
			AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(it.userData());
			if (uData)
			{
				uData->properties.remove("breakpoint");
				uData->properties.remove("breakpointPending");
			}
		}
		emit breakpointClearedAll();
	}

	void AeslEditor::commentAndUncommentSelection(CommentOperation commentOperation)
	{
		QTextCursor cursor = textCursor();
		bool moveFailed = false;

		// get the last line of the selection
		QTextBlock endBlock = document()->findBlock(cursor.selectionEnd());
		int lineEnd = endBlock.blockNumber();
		int positionInEndBlock = cursor.selectionEnd() - endBlock.position();

		// if the end of the selection is at the begining of a line,
		// this last line should not be taken into account
		if (cursor.hasSelection() && (positionInEndBlock == 0))
			lineEnd--;

		// begin of editing block
		cursor.beginEditBlock();

		// start at the begining of the selection
		cursor.setPosition(cursor.selectionStart());
		cursor.movePosition(QTextCursor::StartOfBlock);
		QTextCursor cursorRestore = cursor;

		while (cursor.block().blockNumber() <= lineEnd)
		{
			// prepare for insertion / deletion
			setTextCursor(cursor);

			if (commentOperation == CommentSelection)
			{
				// insert #
				cursor.insertText("#");
			}
			else if (commentOperation == UncommentSelection)
			{
				// delete #
				if (cursor.block().text().at(0) == QChar('#'))
					cursor.deleteChar();
			}

			// move to the next line
			if (!cursor.movePosition(QTextCursor::NextBlock))
			{
				// end of the document
				moveFailed = true;
				break;
			}
		}

		// select the changed lines
		cursorRestore.movePosition(QTextCursor::StartOfBlock);
		if (!moveFailed)
			cursor.movePosition(QTextCursor::StartOfBlock);
		else
			cursor.movePosition(QTextCursor::EndOfBlock);
		cursorRestore.setPosition(cursor.position(), QTextCursor::KeepAnchor);
		setTextCursor(cursorRestore);

		// end of editing block
		cursor.endEditBlock();
	}
	
	//! Replace current code using a sequence of elements (each can be multiple sloc) and highlight one of these elements
	void AeslEditor::replaceAndHighlightCode(const QList<QString>& code, int elementToHighlight)
	{
		QTextCharFormat format;
		QTextCursor cursor(textCursor());
		cursor.beginEditBlock();
		cursor.select(QTextCursor::Document);
		cursor.removeSelectedText();
		int posStart = 0;
		int posEnd =0;
		
		for (int i=0; i<code.size(); i++)
		{
			if (i == elementToHighlight)
			{
				format.setBackground(QBrush(QColor(255,255,200)));
				posStart = cursor.position();
				cursor.insertText(code[i], format);
				posEnd = cursor.position();
			} 
			else
			{
				format.setBackground(QBrush(Qt::white));
				cursor.insertText(code[i], format);
			}
		}
		
		cursor.endEditBlock();
		cursor.setPosition(posEnd);
		setTextCursor(cursor);
		cursor.setPosition(posStart);
		setTextCursor(cursor);
		ensureCursorVisible();
	}
	
	void AeslEditor::wheelEvent(QWheelEvent * event)
	{
		// handle zooming of text
		if (event->modifiers() & Qt::ControlModifier)
		{
			if (event->delta() > 0)
			{
				zoomIn();
				return;
			}
			else if (event->delta() < 0)
			{
				zoomOut();
				return;
			}
		}
		QTextEdit::wheelEvent(event);
	}

	void AeslEditor::keyPressEvent(QKeyEvent * event)
	{
		// handle special pressed
		if (handleCompleter(event))
			return;
		if (handleTab(event))
			return;
		if (handleNewLine(event))
			return;

		// default handler
		QKeyEvent eventCopy(*event);
		QTextEdit::keyPressEvent(event);
		detectLocalContextChange(&eventCopy);
		doCompletion(&eventCopy);
	}

	bool AeslEditor::handleCompleter(QKeyEvent *event)
	{
		// if the popup is visible, forward special keys to the completer
		if (completer && completer->popup()->isVisible()) {
			switch (event->key()) {
				case Qt::Key_Enter:
				case Qt::Key_Return:
				case Qt::Key_Escape:
				case Qt::Key_Tab:
				case Qt::Key_Backtab:
					event->ignore();
					return true;	// let the completer do default behavior
				default:
					break;
			}
		}

		// not the case, go on with other handlers
		return false;
	}

	bool AeslEditor::handleTab(QKeyEvent *event)
	{
		// handle tab and control tab
		if (!(event->key() == Qt::Key_Tab) || !(textCursor().hasSelection()))
			// I won't handle this event
			return false;

		QTextCursor cursor(document()->findBlock(textCursor().selectionStart()));

		cursor.beginEditBlock();

		while (cursor.position() < textCursor().selectionEnd())
		{
			cursor.movePosition(QTextCursor::StartOfBlock);
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
			cursor.movePosition(QTextCursor::NextBlock);
			cursor.movePosition(QTextCursor::EndOfBlock);
		}

		cursor.movePosition(QTextCursor::StartOfBlock);
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

		// successfully handeled
		return true;
	}

	bool AeslEditor::handleNewLine(QKeyEvent *event)
	{
		// handle indentation and new line
		if (!(event->key() == Qt::Key_Return ) && !(event->key() == Qt::Key_Enter))
			// I won't handle this event
			return false;

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

		// successfully handeled
		return true;
	}

	void AeslEditor::detectLocalContextChange(QKeyEvent *event)
	{
		// this function spy the events to detect the local context
		LocalContext currentContext = UnknownContext;
		QString previous = previousWord();
		QString line = currentLine();

		if (previous == "call")
			currentContext = FunctionContext;
		else if (previous == "callsub")
			currentContext = SubroutineCallContext;
		else if (previous == "onevent" || previous == "emit")
			currentContext = EventContext;
		else if (vardefRegexp.indexIn(line) != -1 || constdefRegexp.indexIn(line) != -1)
			currentContext = VarDefContext;
		else if (leftValueRegexp.indexIn(line) == -1)
			currentContext = LeftValueContext;
		else
			currentContext = GeneralContext;

		if (currentContext != previousContext)
		{
			// local context has changed
			previousContext = currentContext;
			emit refreshModelRequest(currentContext);
		}
	}

	void AeslEditor::doCompletion(QKeyEvent *event)
	{
		static QString eow("~!@#$%^&*()+{}|:\"<>?,/;'[]\\-=");		// end of word
		QString completionPrefix = textUnderCursor();
//		qDebug() << "Completion prefix: " << completionPrefix;

		// don't show the popup if the word is too short, or if we detect an "end of word" character
		if (event->text().isEmpty()|| completionPrefix.length() < 1 || eow.contains(event->text().right(1))) {
			completer->popup()->hide();
			return;
		}

		// update the completion prefix
		if (completionPrefix != completer->completionPrefix()) {
			completer->setCompletionPrefix(completionPrefix);
			completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));
		}

//		qDebug() << completer->completionCount();
		// if we match the only completion available, close the popup.
		if (completer->completionCount() == 1)
			if (completer->currentCompletion() == completionPrefix)
			{
				completer->popup()->hide();
				return;
			}

		// show the popup
		QRect cr = cursorRect();
		cr.setWidth(completer->popup()->sizeHintForColumn(0) + completer->popup()->verticalScrollBar()->sizeHint().width());
		completer->complete(cr);
	}

	void AeslEditor::insertCompletion(const QString& completion)
	{
		QTextCursor tc = textCursor();
		// move at the end of the word (make sure to be on it)
		tc.movePosition(QTextCursor::Left);
		tc.movePosition(QTextCursor::EndOfWord);
		// move the cursor left, by the number of completer's prefix characters
		tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, completer->completionPrefix().length());
		tc.removeSelectedText();
		// insert completion
		tc.insertText(completion);
		setTextCursor(tc);
	}

	QString AeslEditor::textUnderCursor() const
	{
		QTextCursor tc = textCursor();
		// found the end
		tc.movePosition(QTextCursor::EndOfWord);
		int endPosition = tc.position();
		// found the begining, including possible "." in the word
		tc.movePosition(QTextCursor::StartOfWord);
		while( (document()->characterAt(tc.position() - 1)) == QChar('.') )
		{
			tc.movePosition(QTextCursor::Left);
			tc.movePosition(QTextCursor::WordLeft);
		}
		// select
		tc.setPosition(endPosition, QTextCursor::KeepAnchor);
		return tc.selectedText();
	}

	QString AeslEditor::previousWord() const
	{
		QTextCursor tc = textCursor();
		// if we are right after a '.', go left
		if ((document()->characterAt(tc.position() - 1)) == QChar('.'))
			tc.movePosition(QTextCursor::Left);
		// go at the begining of the current word (including possible '.' in the word)
		tc.movePosition(QTextCursor::WordLeft);
		while((document()->characterAt(tc.position() - 1)) == QChar('.') ) {
			tc.movePosition(QTextCursor::Left);
			tc.movePosition(QTextCursor::WordLeft);
		}
		// go to the previous word
		tc.movePosition(QTextCursor::PreviousWord);
		// select it
		tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
		return tc.selectedText();
	}

	QString AeslEditor::currentLine() const
	{
		// return everything between the start and the cursor
		QTextCursor tc = textCursor();
		tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
		return tc.selectedText();
	}

	void AeslEditor::setCompleterModel(QAbstractItemModel *model)
	{
		completer->setModel(model);
		completer->setCompletionRole(Qt::DisplayRole);
	}

	
	/*@}*/
} // namespace Aseba
