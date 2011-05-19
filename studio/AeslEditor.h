/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

#include <QHash>
#include <QTextCharFormat>
#include <QTextBlockUserData>
#include <QTextEdit>

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
	
	class ScriptTab;
	
	class AeslEditor : public QTextEdit
	{
		Q_OBJECT
		
	signals:
		void breakpointSet(unsigned line);
		void breakpointCleared(unsigned line);
		void breakpointClearedAll();
		
	public:
		AeslEditor(const ScriptTab* tab);
		virtual ~AeslEditor() { }
		virtual void contextMenuEvent ( QContextMenuEvent * e );
	
	public:
		const ScriptTab* tab;
		bool debugging;
		QWidget *dropSourceWidget;
	
	protected:
		virtual void dropEvent(QDropEvent *event);
		virtual void insertFromMimeData ( const QMimeData * source );
		virtual void keyPressEvent(QKeyEvent * event);
	};
	
	/*@}*/
}; // Aseba

#endif
