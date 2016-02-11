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

#ifndef AESL_EDITOR_H
#define AESL_EDITOR_H

#include <QSyntaxHighlighter>

#include <QWidget>
#include <QHash>
#include <QTextCharFormat>
#include <QTextBlockUserData>
#include <QTextEdit>
#include <QCompleter>
#include <QRegExp>

class QTextDocument;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/

	class AeslEditor;
	
	class AeslHighlighter : public QSyntaxHighlighter
	{
		Q_OBJECT
	
	public:
		AeslHighlighter(AeslEditor *editor, QTextDocument *parent = 0);
	
	protected:
		void highlightBlock(const QString &text);
	
	private:
		struct HighlightingRule
		{
			QRegExp pattern;
			QTextCharFormat format;
		};
		QVector<HighlightingRule> highlightingRules;
		struct CommentBlockRule
		{
			QRegExp begin;
			QRegExp end;
			QTextCharFormat format;
		};
		// For multi-lines comments
		CommentBlockRule commentBlockRules;
		enum BlockState
		{
			STATE_DEFAULT=-1,       // Qt default
			NO_COMMENT=0,           // Normal block
			COMMENT,                // Block with multilines comments
		};

		AeslEditor *editor;
	};
	
	struct AeslEditorUserData : public QTextBlockUserData
	{
		QMap<QString, QVariant> properties;
		
		AeslEditorUserData(const QString &property, const QVariant &value = QVariant()) { properties.insert(property, value); }
		virtual ~AeslEditorUserData() { }
	};

	class AeslEditorSidebar : public QWidget
	{
		Q_OBJECT

	public:
		AeslEditorSidebar(AeslEditor* editor);
		virtual QSize sizeHint() const;

	public slots:
		virtual void scroll(int verticalScroll);

	protected:
		virtual void paintEvent(QPaintEvent *event);
		virtual void mousePressEvent(QMouseEvent *event) {QWidget::mousePressEvent(event);}

		virtual int idealWidth() const = 0;
		int posToLineNumber(int y);

	protected:
		AeslEditor* editor;
		QSize currentSizeHint;
		int verticalScroll;
	};

	class AeslLineNumberSidebar : public AeslEditorSidebar
	{
		Q_OBJECT

	public:
		AeslLineNumberSidebar(AeslEditor* editor);

	public slots:
		void showLineNumbers(bool state);

	protected:
		virtual void paintEvent(QPaintEvent *event);
		virtual int idealWidth() const;
	};

	class AeslBreakpointSidebar : public AeslEditorSidebar
	{
		Q_OBJECT

	public:
		AeslBreakpointSidebar(AeslEditor* editor);

	protected:
		virtual void paintEvent(QPaintEvent *event);
		virtual void mousePressEvent(QMouseEvent *event);
		virtual int idealWidth() const;

	protected:
		const int borderSize;
	};
	
	enum LocalContext {
		UnknownContext,
		VarDefContext,
		LeftValueContext,
		FunctionContext,
		SubroutineCallContext,
		EventContext,
		GeneralContext
	};

	class AeslEditor : public QTextEdit
	{
		Q_OBJECT
		
	signals:
		void breakpointSet(unsigned line);
		void breakpointCleared(unsigned line);
		void breakpointClearedAll();

		void refreshModelRequest(LocalContext context);
		
	public:
		AeslEditor();
		virtual ~AeslEditor() { }
		virtual void contextMenuEvent ( QContextMenuEvent * e );

		bool isBreakpoint();			// apply to the current line
		bool isBreakpoint(QTextBlock block);
		bool isBreakpoint(int line);
		void toggleBreakpoint();		// apply to the current line
		void toggleBreakpoint(QTextBlock block);
		void setBreakpoint();			// apply to the current line
		void setBreakpoint(QTextBlock block);
		void clearBreakpoint();			// apply to the current line
		void clearBreakpoint(QTextBlock block);
		void clearAllBreakpoints();

		void setCompleterModel(QAbstractItemModel* model);

		enum CommentOperation {
			CommentSelection,
			UncommentSelection
		};
		void commentAndUncommentSelection(CommentOperation commentOperation);
		
		void replaceAndHighlightCode(const QList<QString>& code, int elementToHighlight);
	
	public:
		bool debugging;
	
	protected slots:
		void insertCompletion(const QString &completion);

	protected:
		virtual void wheelEvent(QWheelEvent * event);
		virtual void keyPressEvent(QKeyEvent * event);

		virtual bool handleCompleter(QKeyEvent * event);
		virtual bool handleTab(QKeyEvent * event);
		virtual bool handleNewLine(QKeyEvent * event);
		virtual void detectLocalContextChange(QKeyEvent * event);
		virtual void doCompletion(QKeyEvent * event);

		QString textUnderCursor() const;
		QString previousWord() const;
		QString currentLine() const;

	protected:
		QCompleter *completer;
		const QRegExp vardefRegexp;
		const QRegExp constdefRegexp;
		const QRegExp leftValueRegexp;
		LocalContext previousContext;
		bool editingLeftValue;
	};
	
	/*@}*/
} // namespace Aseba

#endif
