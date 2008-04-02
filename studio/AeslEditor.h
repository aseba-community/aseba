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
		AeslEditor *editor;
	};
	
	struct AeslEditorUserData : public QTextBlockUserData
	{
		QMap<QString, QVariant> properties;
		
		AeslEditorUserData(const QString &property, const QVariant &value = QVariant()) { properties.insert(property, value); }
		virtual ~AeslEditorUserData() { }
	};
	
	class AeslEditor : public QTextEdit
	{
		Q_OBJECT
		
	signals:
		void breakpointSet(unsigned line);
		void breakpointCleared(unsigned line);
		void breakpointClearedAll();
		
	public:
		AeslEditor() { debugging = false; }
		virtual ~AeslEditor() { }
		virtual void contextMenuEvent ( QContextMenuEvent * e );
	
	public:
		bool debugging;
	
	protected:
		virtual void keyPressEvent(QKeyEvent * event);
	};
	
	/*@}*/
}; // Aseba

#endif
