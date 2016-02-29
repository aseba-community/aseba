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

#include "StudioAeslEditor.h"
#include "MainWindow.h"
#include <QtGui>
#include <QtGlobal>

#include <cassert>

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/

	StudioAeslEditor::StudioAeslEditor(const ScriptTab* tab) :
		tab(tab),
		dropSourceWidget(0)
	{
	}
	
	void StudioAeslEditor::dropEvent(QDropEvent *event)
	{
		dropSourceWidget = event->source();
		QTextEdit::dropEvent(event);
		dropSourceWidget = 0;
		setFocus(Qt::MouseFocusReason);
	}
	
	void StudioAeslEditor::insertFromMimeData ( const QMimeData * source )
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
			assert(nodeTab);
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
				const std::wstring varName = source->text().toStdWString();
				if (nodeTab->vmMemoryModel->getVariableSize(QString::fromStdWString(varName)) > 1)
				{
					midfix = "[";
					postfix = "] ";
				}
				else
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
	
	/*@}*/
} // namespace Aseba
