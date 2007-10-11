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

#include "MainWindow.h"
#include "ClickableLabel.h"
#include "TcpTarget.h"
#include "TargetModels.h"
#include "CustomDelegate.h"
#include "FunctionParametersDialog.h"
#include "AeslEditor.h"
#include "../common/consts.h"
#include <QtGui>
#include <QtXml>
#include <sstream>
#include <iostream>
#include <QTabWidget>
#include <boost/cast.hpp>

using namespace boost;
using std::copy;

namespace Aseba
{
	class CompilationLogDialog: public QDialog
	{
	public:
		 CompilationLogDialog(QWidget *parent) :
		 	QDialog(parent)
		{
			QFont font;
			font.setFamily("");
			font.setStyleHint(QFont::TypeWriter);
			font.setFixedPitch(true);
			font.setPointSize(10);
			
			text = new QTextEdit;
			text->setFont(font);
			text->setTabStopWidth( QFontMetrics(font).width(' ') * 4);
			text->setReadOnly(true);
			
			QVBoxLayout* layout = new QVBoxLayout(this);
			layout->addWidget(text);
			
			//setStandardButtons(QMessageBox::Ok);
			setWindowTitle(tr("Aseba Studio: Output of last compilation"));
			setModal(false);
			setSizeGripEnabled(true);
		}
	
		QTextEdit *text;
	};

	QSize MemoryTableView::minimumSizeHint() const
	{
		QSize size( QTableView::sizeHint() );
		int width = 0;
		
		for (int c = 0; c < model ()->columnCount(); ++c)
			width += columnWidth( c );
		
		
		size.setWidth( width + style()->pixelMetric(QStyle::PM_ScrollBarExtent) + 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth));
		return size;
	}
	
	QSize MemoryTableView::sizeHint() const
	{
		return minimumSizeHint();
	}
	
	void MemoryTableView::resizeColumnsToContents ()
	{
		QFontMetrics fm(font());
		setColumnWidth(0, fm.width("it is long text [9]"));
		setColumnWidth(1, fm.width("-888888"));
	}
	
	NodeTab::NodeTab(Target *target, const EventsNamesVector *eventsNames, int id, QWidget *parent) :
		QWidget(parent),
		id(id),
		target(target)
	{
		// setup some variables
		rehighlighting = false;
		errorPos = -1;
		allocatedVariablesCount = 0;
		
		// create models
		compiler.setTargetDescription(target->getConstDescription(id));
		compiler.setEventsNames(eventsNames);
		vmFunctionsModel = new TargetFunctionsModel(target->getConstDescription(id), 0);
		vmMemoryModel = new TargetMemoryModel();
		
		// create gui
		setupWidgets();
		setupConnections();
		
		editor->setFocus();
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
		
		// get the value of the variables
		refreshMemoryClicked();
	}
	
	NodeTab::~NodeTab()
	{
		compiler.setTargetDescription(0);
		delete vmFunctionsModel;
		delete vmMemoryModel;
	}
	
	void NodeTab::setupWidgets()
	{
		// editor widget
		QFont font;
		font.setFamily("");
		font.setStyleHint(QFont::TypeWriter);
		font.setFixedPitch(true);
		font.setPointSize(10);

		editor = new AeslEditor;
		editor->setFont(font);
		editor->setTabStopWidth( QFontMetrics(font).width(' ') * 4);
		highlighter = new AeslHighlighter(editor, editor->document());
		
		// editor related notification widgets
		cursorPosText = new QLabel;
		compilationResultImage = new ClickableLabel;
		compilationResultText = new ClickableLabel;
		compilationResultText->setWordWrap(true);
		compilationResultText->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
		QHBoxLayout *compilationResultLayout = new QHBoxLayout;
		compilationResultLayout->addWidget(cursorPosText);
		compilationResultLayout->addWidget(compilationResultText, 1000);
		compilationResultLayout->addWidget(compilationResultImage);
		
		// editor area
		QVBoxLayout *editorLayout = new QVBoxLayout;
		editorLayout->addWidget(editor);
		editorLayout->addLayout(compilationResultLayout);
		
		// panel
		
		// buttons
		QHBoxLayout *executionTitleLayout = new QHBoxLayout;
		executionTitleLayout->addWidget(new QLabel(tr("<b>Execution</b>")));
		executionModeLabel = new QLabel(tr("unknown"));
		executionTitleLayout->addWidget(executionModeLabel);
		
		debugButton = new QPushButton(QIcon(":/images/upload.png"), tr("Debug"));
		resetButton = new QPushButton(QIcon(":/images/reset.png"), tr("Reset"));
		resetButton->setEnabled(false);
		runInterruptButton = new QPushButton(QIcon(":/images/play.png"), tr("Run"));
		runInterruptButton->setEnabled(false);
		nextButton = new QPushButton(QIcon(":/images/step.png"), tr("Next"));
		nextButton->setEnabled(false);
		refreshMemoryButton = new QPushButton(QIcon(":/images/rescan.png"), tr("refresh"));
		
		// memory
		QHBoxLayout *memoryTitleLayout = new QHBoxLayout;
		memoryTitleLayout->addWidget(new QLabel(tr("<b>Memory</b>")));
		memoryTitleLayout->addWidget(refreshMemoryButton);
		
		vmMemoryView = new MemoryTableView;
		vmMemoryView->setShowGrid(false);
		vmMemoryView->verticalHeader()->hide();
		vmMemoryView->horizontalHeader()->hide();
		vmMemoryView->setModel(vmMemoryModel);
		vmMemoryView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		vmMemoryView->setSelectionMode(QAbstractItemView::NoSelection);
		vmMemoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
		vmMemoryView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
		vmMemoryView->resizeColumnsToContents();
		vmMemoryView->resizeRowsToContents();
		
		
		// functions
		vmFunctionsView = new QTableView;
		vmFunctionsView->setShowGrid(false);
		vmFunctionsView->verticalHeader()->hide();
		vmFunctionsView->horizontalHeader()->hide();
		vmFunctionsView->setMinimumSize(QSize(50,40));
		vmFunctionsView->setModel(vmFunctionsModel);
		vmFunctionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		vmFunctionsView->setSelectionMode(QAbstractItemView::NoSelection);	
		vmFunctionsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
		vmFunctionsView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
		vmFunctionsView->resizeColumnsToContents();
		vmFunctionsView->resizeRowsToContents();
		
		// panel
		QVBoxLayout *panelLayout = new QVBoxLayout;
		panelLayout->addLayout(executionTitleLayout);
		panelLayout->addWidget(debugButton);
		panelLayout->addWidget(resetButton);
		panelLayout->addWidget(runInterruptButton);
		panelLayout->addWidget(nextButton);
		panelLayout->addLayout(memoryTitleLayout);
		panelLayout->addWidget(vmMemoryView, 3);
		panelLayout->addWidget(new QLabel(tr("<b>Native Functions</b>")));
		panelLayout->addWidget(vmFunctionsView, 1);
		
		QHBoxLayout *layout = new QHBoxLayout;
		layout->addLayout(panelLayout, 1);
		layout->addLayout(editorLayout, 8);
		setLayout(layout);
	}
	
	void NodeTab::setupConnections()
	{
		// execution
		connect(debugButton, SIGNAL(clicked()), SLOT(debugClicked()));
		connect(resetButton, SIGNAL(clicked()), SLOT(resetClicked()));
		connect(runInterruptButton, SIGNAL(clicked()), SLOT(runInterruptClicked()));
		connect(nextButton, SIGNAL(clicked()), SLOT(nextClicked()));
		connect(refreshMemoryButton, SIGNAL(clicked()), SLOT(refreshMemoryClicked()));
		
		// editor
		connect(editor, SIGNAL(textChanged()), SLOT(editorContentChanged()));
		connect(editor, SIGNAL(cursorPositionChanged() ), SLOT(cursorMoved()));
		connect(editor, SIGNAL(breakpointSet(unsigned)), SLOT(setBreakpoint(unsigned)));
		connect(editor, SIGNAL(breakpointCleared(unsigned)), SLOT(clearBreakpoint(unsigned)));
		
		connect(compilationResultImage, SIGNAL(clicked()), SLOT(goToError()));
		connect(compilationResultText, SIGNAL(clicked()), SLOT(goToError()));
	}
	
	void NodeTab::resetClicked()
	{
		clearEditorProperty("executionError");
		target->reset(id);
	}
	
	void NodeTab::debugClicked()
	{
		if (errorPos == -1)
		{
			clearEditorProperty("executionError");
			target->uploadBytecode(id, bytecode);
			//target->getVariables(id, 0, allocatedVariablesCount);
			editor->debugging = true;
			reSetBreakpoints();
			rehighlight();
		}
	}
	
	void NodeTab::runInterruptClicked()
	{
		if (runInterruptButton->text() == tr("Run"))
			target->run(id);
		else
			target->pause(id);
	}
	
	void NodeTab::nextClicked()
	{
		target->next(id);
	}
	
	void NodeTab::refreshMemoryClicked()
	{
		target->getVariables(id, 0, allocatedVariablesCount);
	}
	
	void NodeTab::editorContentChanged()
	{
		if (rehighlighting)
			rehighlighting = false;
		else
			recompile();
	}
	
	void NodeTab::recompile()
	{
		// output bytecode
		Error error;
		
		// clear old user data
		// doRehighlight is required to prevent infinite recursion because there is not slot
		// to differentiate user changes from highlight changes in documents
		bool doRehighlight = clearEditorProperty("errorPos");
		
		// compile
		std::istringstream is(std::string(editor->toPlainText().toUtf8()));
		bool result = compiler.compile(is, bytecode, variablesNames, allocatedVariablesCount, error);
		if (result)
		{
			vmMemoryModel->setVariablesNames(variablesNames);
			compilationResultText->setText(tr("Compilation success"));
			compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
			debugButton->setEnabled(true);
			emit uploadReadynessChanged();
			
			errorPos = -1;
		}
		else
		{
			compilationResultText->setText(QString::fromUtf8(error.toString().c_str()));
			compilationResultImage->setPixmap(QPixmap(QString(":/images/no.png")));
			debugButton->setEnabled(false);
			emit uploadReadynessChanged();
			
			// we have an error, set the correct user data
			if (error.pos.valid)
			{
				errorPos = error.pos.character;
				QTextBlock textBlock = editor->document()->findBlock(errorPos);
				int posInBlock = errorPos - textBlock.position();
				if (textBlock.userData())
					static_cast<AeslEditorUserData *>(textBlock.userData())->properties["errorPos"] = posInBlock;
				else
					textBlock.setUserData(new AeslEditorUserData("errorPos", posInBlock));
				doRehighlight = true;
			}
		}
		
		// stop target if currently in debugging mode
		if (editor->debugging)
		{
			target->stop(id);
			doRehighlight = true;
			editor->debugging = false;
			resetButton->setEnabled(false);
			runInterruptButton->setEnabled(false);
			nextButton->setEnabled(false);
			target->clearBreakpoints(id);
		}
		
		if (doRehighlight)
			rehighlight();
	}
	
	void NodeTab::cursorMoved()
	{
		// fix tab
		cursorPosText->setText(QString("Line: %0 Col: %1").arg(editor->textCursor().blockNumber() + 1).arg(editor->textCursor().columnNumber() + 1));
	}
	
	void NodeTab::goToError()
	{
		if (errorPos >= 0)
		{
			QTextCursor cursor = editor->textCursor();
			cursor.setPosition(errorPos);
			editor->setTextCursor(cursor);
			editor->ensureCursorVisible();
		}
	}
	
	void NodeTab::clearExecutionErrors()
	{
		// remove execution error
		if (clearEditorProperty("executionError"))
			rehighlight();
	}
	
	void NodeTab::variablesMemoryChanged(unsigned start, const VariablesDataVector &variables)
	{
		// update memory view
		vmMemoryModel->setVariablesData(start, variables);
	}
	
	void NodeTab::setBreakpoint(unsigned line)
	{
		rehighlight();
		target->setBreakpoint(id, line);
	}
	
	void NodeTab::clearBreakpoint(unsigned line)
	{
		rehighlight();
		target->clearBreakpoint(id, line);
	}
	
	void NodeTab::executionPosChanged(unsigned line)
	{
		// change active state
		if (setEditorProperty("active", QVariant(), line, true))
			rehighlight();
	}
	
	void NodeTab::executionModeChanged(Target::ExecutionMode mode)
	{
		// ignore those messages if we are not in debugging mode
		if (!editor->debugging)
			return;
		
		resetButton->setEnabled(true);
		runInterruptButton->setEnabled(true);
		compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
		
		if (mode == Target::EXECUTION_RUN)
		{
			executionModeLabel->setText(tr("running"));
			
			runInterruptButton->setText(tr("Pause"));
			runInterruptButton->setIcon(QIcon(":/images/pause.png"));
			
			nextButton->setEnabled(false);
		}
		else if (mode == Target::EXECUTION_STEP_BY_STEP)
		{
			executionModeLabel->setText(tr("step by step"));
			
			runInterruptButton->setText(tr("Run"));
			runInterruptButton->setIcon(QIcon(":/images/play.png"));
			
			nextButton->setEnabled(true);
		}
		else if (mode == Target::EXECUTION_STOP)
		{
			executionModeLabel->setText(tr("stopped"));
			
			runInterruptButton->setText(tr("Run"));
			runInterruptButton->setIcon(QIcon(":/images/play.png"));
			
			nextButton->setEnabled(false);
			
			if (clearEditorProperty("active"))
				rehighlight();
		}
	}
	
	void NodeTab::breakpointSetResult(unsigned line, bool success)
	{
		clearEditorProperty("breakpointPending", line);
		if (success)
			setEditorProperty("breakpoint", QVariant(), line);
		rehighlight();
	}
	
	void NodeTab::rehighlight()
	{
		rehighlighting = true;
		highlighter->rehighlight();
	}
	
	void NodeTab::reSetBreakpoints()
	{
		QTextBlock block = editor->document()->begin();
		unsigned lineCounter = 0;
		while (block != editor->document()->end())
		{
			AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(block.userData());
			if (uData && uData->properties.contains("breakpoint"))
				target->setBreakpoint(id, lineCounter);
			block = block.next();
			lineCounter++;
		}
	}
	
	bool NodeTab::setEditorProperty(const QString &property, const QVariant &value, unsigned line, bool removeOld)
	{
		bool changed = false;
		
		QTextBlock block = editor->document()->begin();
		unsigned lineCounter = 0;
		while (block != editor->document()->end())
		{
			AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(block.userData());
			if (lineCounter == line)
			{
				// set propety
				if (uData)
				{
					if (!uData->properties.contains(property) || (uData->properties[property] != value))
					{
						uData->properties[property] = value;
						changed = true;
					}
				}
				else
				{
					block.setUserData(new AeslEditorUserData(property, value));
					changed = true;
				}
			}
			else
			{
				// remove if exists
				if (uData && removeOld && uData->properties.contains(property))
				{
					uData->properties.remove(property);
					if (uData->properties.isEmpty())
					{
						// garbage collect UserData
						delete uData;
						block.setUserData(0);
					}
					changed = true;
				}
			}
			
			block = block.next();
			lineCounter++;
		}
		
		return changed;
	}
	
	bool NodeTab::clearEditorProperty(const QString &property, unsigned line)
	{
		bool changed = false;
		
		// find line, remove property
		QTextBlock block = editor->document()->begin();
		unsigned lineCounter = 0;
		while (block != editor->document()->end())
		{
			if (lineCounter == line)
			{
				AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(block.userData());
				if (uData && uData->properties.contains(property))
				{
					uData->properties.remove(property);
					if (uData->properties.isEmpty())
					{
						// garbage collect UserData
						delete uData;
						block.setUserData(0);
					}
					changed = true;
				}
			}
			block = block.next();
			lineCounter++;
		}
		
		return changed;
	}
	
	bool NodeTab::clearEditorProperty(const QString &property)
	{
		bool changed = false;
		
		// go through all blocks, remove property if found
		QTextBlock block = editor->document()->begin();
		while (block != editor->document()->end())
		{
			AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(block.userData());
			if (uData && uData->properties.contains(property))
			{
				uData->properties.remove(property);
				if (uData->properties.isEmpty())
				{
					// garbage collect UserData
					delete uData;
					block.setUserData(0);
				}
				changed = true;
			}
			block = block.next();
		}
		
		return changed;
	}
	
	
	
	
	MainWindow::MainWindow(QWidget *parent) :
		QMainWindow(parent)
	{
		// create target
		target = new TcpTarget;
		
		// create gui
		setupWidgets();
		setupMenu();
		setupConnections();
		
		// cosmetic dix-up
		setWindowTitle(tr("Aseba Studio"));
		resize(1000,700);
		
		// connect to target
		target->connect();
	}
	
	MainWindow::~MainWindow()
	{
		delete target;
	}
	
	void MainWindow::loadDefaultFile()
	{
		/*QFile file("testscripts/default.aesl");
		if (file.open(QFile::ReadOnly | QFile::Text))
			editor->setPlainText(file.readAll());*/
	}
	
	void MainWindow::about()
	{
		QMessageBox::about(this, tr("About Aseba Studio"),
					tr(	"<p>Aseba pre-release</p>" \
						"<p>(c) 2006-2007 Stephane Magnenat</p>" \
						"<p>Mobots group - LSRO1 - EPFL</p>"));
	}
	
	void MainWindow::newFile()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			tab->editor->clear();
		}
	}
	
	void MainWindow::openFile(const QString &path)
	{
		QString fileName = path;
	
		if (fileName.isEmpty())
			fileName = QFileDialog::getOpenFileName(this,
				tr("Open Script"), "", "AESL scripts (*.aesl)");
		
		QFile file(fileName);
		if (!file.open(QFile::ReadOnly))
			return;
		
		eventsNames.clear();
		eventsNamesList->clear();
		
		QDomDocument document("aesl-source");
		QString errorMsg;
		int errorLine;
		int errorColumn;
		if (document.setContent(&file, false, &errorMsg, &errorLine, &errorColumn))
		{
			int noNodeCount = 0;
			actualFileName = fileName;
			QDomNode domNode = document.documentElement().firstChild();
			while (!domNode.isNull())
			{
				if (domNode.isElement())
				{
					QDomElement element = domNode.toElement();
					if (element.tagName() == "node")
					{
						NodeTab* tab = getTabFromName(element.attribute("name"));
						if (tab)
							tab->editor->setPlainText(element.firstChild().toText().data());
						else
							noNodeCount++;
					}
					else if (element.tagName() == "event")
					{
						eventsNamesList->addItem(element.attribute("name"));
						eventsNames.push_back(element.attribute("name").toStdString());
					}
				}
				domNode = domNode.nextSibling();
			}
			if (noNodeCount)
				QMessageBox::warning(this,
					tr("Loading"),
					tr("%0 scripts have no corresponding nodes in the current network and have not been loaded.").arg(noNodeCount)
				);
		}
		else
			QMessageBox::warning(this,
				tr("Loading"),
				tr("Error in source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn)
			);
		file.close();
		
		recompileAll();
	}
	
	void MainWindow::save()
	{
		saveFile(actualFileName);
	}
	
	void MainWindow::saveFile(const QString &previousFileName)
	{
		QString fileName = previousFileName;
		
		if (fileName.isEmpty())
			fileName = QFileDialog::getSaveFileName(this,
				tr("Save Script"), actualFileName, "AESL scripts (*.aesl)");
		
		if (fileName.isEmpty())
			return;
		
		if (fileName.lastIndexOf(".") < 0)
			fileName += ".aesl";
		
		QFile file(fileName);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
			return;
		
		actualFileName = fileName;
		
		// initiate DOM tree
		QDomDocument document("aesl-source");
		QDomElement root = document.createElement("network");
		document.appendChild(root);
		
		// events
		for (size_t i = 0; i < eventsNames.size(); i++)
		{
			QDomElement element = document.createElement("event");
			element.setAttribute("name", QString::fromStdString(eventsNames[i]));
			root.appendChild(element);
		}
		
		// source code
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			QDomElement element = document.createElement("node");
			element.setAttribute("name", target->getName(tab->nodeId()));
			QDomText text = document.createTextNode(tab->editor->toPlainText());
			element.appendChild(text);
			root.appendChild(element);
		}
		
		QTextStream out(&file);
		document.save(out, 4);
	}
	
	void MainWindow::resetAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			tab->resetClicked();
		}
	}
	
	void MainWindow::debugAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			tab->debugClicked();
		}
	}
	
	void MainWindow::runAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			target->run(tab->nodeId());
		}
	}
	
	void MainWindow::pauseAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			target->pause(tab->nodeId());
		}
	}
	
	void MainWindow::stopAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			target->stop(tab->nodeId());
		}
	}
	
	void MainWindow::uploadReadynessChanged()
	{
		bool ready = true;
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			if (!tab->debugButton->isEnabled())
			{
				ready = false;
				break;
			}
		}
		
		debugAllAct->setEnabled(ready);
	}
	
	void MainWindow::sendEvent()
	{
		bool ok;
		int id = QInputDialog::getInteger(this, tr("Initiate event code"),
											tr("Event identifier:"), 0, 0, 16383, 1, &ok);
		if (ok)
		{
			VariablesDataVector data;
			target->sendEvent((unsigned)id, data);
		}
	}
	
	void MainWindow::logEntryDoubleClicked(QListWidgetItem * item)
	{
		if (item->data(Qt::UserRole).type() == QVariant::Point)
		{
			int node = item->data(Qt::UserRole).toPoint().x();
			int line = item->data(Qt::UserRole).toPoint().y();
			
			NodeTab* tab = getTabFromId(node);
			Q_ASSERT(tab);
			nodes->setCurrentWidget(tab);
			QTextBlock block = tab->editor->document()->begin();
			for (int i = 0; i < line; i++)
				block = block.next();
			tab->editor->textCursor().setPosition(block.position(), QTextCursor::MoveAnchor);
		}
	}
	
	void MainWindow::tabChanged(int index)
	{
		// remove old connections, if any
		if (previousActiveTab)
		{
			disconnect(cutAct, SIGNAL(triggered()), previousActiveTab->editor, SLOT(cut()));
			disconnect(copyAct, SIGNAL(triggered()), previousActiveTab->editor, SLOT(copy()));
			disconnect(pasteAct, SIGNAL(triggered()), previousActiveTab->editor, SLOT(paste()));
			disconnect(undoAct, SIGNAL(triggered()), previousActiveTab->editor, SLOT(undo()));
			disconnect(redoAct, SIGNAL(triggered()), previousActiveTab->editor, SLOT(redo()));
			
			disconnect(previousActiveTab->editor, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
			disconnect(previousActiveTab->editor, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
			disconnect(previousActiveTab->editor, SIGNAL(undoAvailable(bool)), undoAct, SLOT(setEnabled(bool)));
			disconnect(previousActiveTab->editor, SIGNAL(redoAvailable(bool)), redoAct, SLOT(setEnabled(bool)));
		}
		
		// reconnect to new
		NodeTab *tab = polymorphic_downcast<NodeTab *>(nodes->widget(index));
		
		connect(cutAct, SIGNAL(triggered()), tab->editor, SLOT(cut()));
		connect(copyAct, SIGNAL(triggered()), tab->editor, SLOT(copy()));
		connect(pasteAct, SIGNAL(triggered()), tab->editor, SLOT(paste()));
		connect(undoAct, SIGNAL(triggered()), tab->editor, SLOT(undo()));
		connect(redoAct, SIGNAL(triggered()), tab->editor, SLOT(redo()));
		
		connect(tab->editor, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
		connect(tab->editor, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
		connect(tab->editor, SIGNAL(undoAvailable(bool)), undoAct, SLOT(setEnabled(bool)));
		connect(tab->editor, SIGNAL(redoAvailable(bool)), redoAct, SLOT(setEnabled(bool)));
		
		// TODO: it would be nice to find a way to setup this correctly
		cutAct->setEnabled(false);
		copyAct->setEnabled(false);
		undoAct->setEnabled(false);
		redoAct->setEnabled(false);
		
		previousActiveTab = tab;
		
		target->getVariables(tab->id, 0, tab->allocatedVariablesCount);
	}
	
	void MainWindow::showCompilationMessages()
	{
		if (nodes->currentWidget())
		{
			compilationMessageBox->hide();
			NodeTab *tab = polymorphic_downcast<NodeTab *>(nodes->currentWidget());
			
			
			// recompile with verbose
			std::istringstream is(std::string(tab->editor->toPlainText().toUtf8()));
			std::ostringstream compilationMessages;
			BytecodeVector bytecode;
			Error error;
			bool result = tab->compiler.compile(is, bytecode, tab->variablesNames, tab->allocatedVariablesCount, error, &compilationMessages);
			
			if (result)
				compilationMessageBox->text->setText(
					tr("Compilation success.\n\n") +
					QString::fromUtf8(compilationMessages.str().c_str())
				);
			else
				compilationMessageBox->text->setText(
					QString::fromUtf8(error.toString().c_str()) + ".\n\n" +
					QString::fromUtf8(compilationMessages.str().c_str())
				);
			
			compilationMessageBox->show();
		}
	}
	
	void MainWindow::addEventNameClicked()
	{
		bool ok;
		QString eventName = QInputDialog::getText(this, tr("Add a new event"), tr("Name:"), QLineEdit::Normal, "", &ok);
		if (ok && !eventName.isEmpty())
		{
			eventsNamesList->addItem(eventName);
			rebuildEventsNames();
			
			recompileAll();
		}
	}
	
	void MainWindow::removeEventNameClicked()
	{
		delete eventsNamesList->takeItem(eventsNamesList->currentRow());
		rebuildEventsNames();
		
		recompileAll();
	}
	
	void MainWindow::eventsNamesSelectionChanged()
	{
		removeEventNameButton->setEnabled(!eventsNamesList->selectedItems ().isEmpty());
	}
	
	void MainWindow::rebuildEventsNames()
	{
		eventsNames.clear();
		for (int i = 0; i < eventsNamesList->count (); ++i)
		{
			QListWidgetItem *item = eventsNamesList->item(i);
			if (item)
				eventsNames.push_back(item->text().toStdString ());
		}
	}
	
	void MainWindow::recompileAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			tab->recompile();
		}
	}
	
	//! A new node has connected to the network.
	void MainWindow::nodeConnected(unsigned node)
	{
		NodeTab* tab = new NodeTab(target, &eventsNames, node);
		connect(tab, SIGNAL(uploadReadynessChanged()), SLOT(uploadReadynessChanged()));
		nodes->addTab(tab, target->getName(node));
	}
	
	//! A node has disconnected from the network.
	void MainWindow::nodeDisconnected(unsigned node)
	{
		int index = getIndexFromId(node);
		Q_ASSERT(index >= 0);
		NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(index));
		
		nodes->removeTab(index);
		delete tab;
	}
	
	//! A user event has arrived from the network.
	void MainWindow::userEvent(unsigned id, const VariablesDataVector &data)
	{
		QString text = QTime::currentTime().toString("hh:mm:ss.zzz");
		if (id < eventsNames.size())
			text += tr("\n%0 : ").arg(QString::fromStdString(eventsNames[id]));
		else
			text += tr("\nevent %0 : ").arg(id);
		for (size_t i = 0; i < data.size(); i++)
			text += QString("%0 ").arg(data[i]);
		
		if (logger->count() > 50)
			delete logger->takeItem(0);
		QListWidgetItem * item = new QListWidgetItem(QIcon(":/images/info.png"), text, logger);
		logger->scrollToBottom();
	}
	
	//! A node did an access out of array bounds exception.
	void MainWindow::arrayAccessOutOfBounds(unsigned node, unsigned line, unsigned index)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		if (tab->setEditorProperty("executionError", QVariant(), line, true))
		{
			tab->rehighlighting = true;
			tab->highlighter->rehighlight();
		}
		
		QString text = QTime::currentTime().toString("hh:mm:ss.zzz");
		text += tr("\n%0:%1: access out of memory (%2/%3)").arg(target->getName(node)).arg(line + 1).arg(index).arg(target->getConstDescription(node)->variablesSize);
		
		if (logger->count() > 50)
			delete logger->takeItem(0);
		QListWidgetItem *item = new QListWidgetItem(QIcon(":/images/warning.png"), text, logger);
		item->setData(Qt::UserRole, QPoint(node, line));
		logger->scrollToBottom();
		
		tab->executionModeChanged(Target::EXECUTION_STOP);
	}
	
	//! A node did a division by zero exception.
	void MainWindow::divisionByZero(unsigned node, unsigned line)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		if (tab->setEditorProperty("executionError", QVariant(), line, true))
		{
			tab->rehighlighting = true;
			tab->highlighter->rehighlight();
		}
		
		QString text = QTime::currentTime().toString("hh:mm:ss.zzz");
		text += tr("\n%0:%1: division by zero").arg(target->getName(node)).arg(line + 1);
		
		if (logger->count() > 50)
			delete logger->takeItem(0);
		QListWidgetItem *item = new QListWidgetItem(QIcon(":/images/warning.png"), text, logger);
		item->setData(Qt::UserRole, QPoint(node, line));
		logger->scrollToBottom();
		
		tab->executionModeChanged(Target::EXECUTION_STOP);
	}
	
	
	//! The program counter of a node has changed, causing a change of position in source code.
	void MainWindow::executionPosChanged(unsigned node, unsigned line)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		tab->executionPosChanged(line);
	}
	
	//! The mode of execution of a node (stop, run, step by step) has changed.
	void MainWindow::executionModeChanged(unsigned node, Target::ExecutionMode mode)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		tab->executionModeChanged(mode);
	}
	
	//! The execution state logic thinks variables might need a refresh
	void MainWindow::variablesMemoryEstimatedDirty(unsigned node)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		tab->refreshMemoryClicked();
	}
	
	//! The content of the variables memory of a node has changed.
	void MainWindow::variablesMemoryChanged(unsigned node, unsigned start, const VariablesDataVector &variables)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		tab->vmMemoryModel->setVariablesData(start, variables);
	}
	
	//! The result of a set breakpoint is known
	void MainWindow::breakpointSetResult(unsigned node, unsigned line, bool success)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		tab->breakpointSetResult(line, success);
	}
	
	int MainWindow::getIndexFromId(unsigned node)
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			if (tab->nodeId() == node)
				return i;
		}
		return -1;
	}
	
	NodeTab* MainWindow::getTabFromId(unsigned node)
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			if (tab->nodeId() == node)
				return tab;
		}
		return 0;
	}
	
	NodeTab* MainWindow::getTabFromName(const QString& name)
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			if (target->getName(tab->nodeId()) == name)
				return tab;
		}
		return 0;
	}
	
	void MainWindow::setupWidgets()
	{
		previousActiveTab = 0;
		nodes = new QTabWidget;
		nodes->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
		
		QSplitter *splitter = new QSplitter();
		splitter->addWidget(nodes);
		setCentralWidget(splitter);
		
		QWidget* eventsDockWidget = new QWidget;
		QVBoxLayout* eventsDockLayout = new QVBoxLayout(eventsDockWidget);
		
		QHBoxLayout* eventsAddRemoveLayout = new QHBoxLayout;
		eventsAddRemoveLayout->addWidget(new QLabel(tr("<b>Events</b>")));
		eventsAddRemoveLayout->addStretch();
		addEventNameButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
		eventsAddRemoveLayout->addWidget(addEventNameButton);
		removeEventNameButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
		removeEventNameButton->setEnabled(false);
		eventsAddRemoveLayout->addWidget(removeEventNameButton);
		
		eventsDockLayout->addLayout(eventsAddRemoveLayout);
		
		eventsNamesList = new QListWidget;
		eventsNamesList->setMinimumSize(80,80);
		eventsNamesList->setSelectionMode(QAbstractItemView::SingleSelection);
		eventsDockLayout->addWidget(eventsNamesList, 1);
		
		logger = new QListWidget;
		logger->setMinimumSize(80,80);
		logger->setSelectionMode(QAbstractItemView::NoSelection);
		eventsDockLayout->addWidget(logger, 3);
		
		splitter->addWidget(eventsDockWidget);
		
		splitter->setSizes(QList<int>() << 800 << 200);
		
		// dialog box
		compilationMessageBox = new CompilationLogDialog(this);
	}
	
	void MainWindow::setupConnections()
	{
		// general connections
		connect(nodes, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
		connect(logger, SIGNAL(itemDoubleClicked(QListWidgetItem *)), SLOT(logEntryDoubleClicked(QListWidgetItem *)));
		
		// global actions
		connect(debugAllAct, SIGNAL(triggered()), SLOT(debugAll()));
		connect(resetAllAct, SIGNAL(triggered()), SLOT(resetAll()));
		connect(runAllAct, SIGNAL(triggered()), SLOT(runAll()));
		connect(pauseAllAct, SIGNAL(triggered()), SLOT(pauseAll()));
		connect(sendEventAct, SIGNAL(triggered()), SLOT(sendEvent()));
		
		// events
		connect(addEventNameButton, SIGNAL(clicked()), SLOT(addEventNameClicked()));
		connect(removeEventNameButton, SIGNAL(clicked()), SLOT(removeEventNameClicked()));
		connect(eventsNamesList, SIGNAL(itemSelectionChanged()), SLOT(eventsNamesSelectionChanged()));
		
		// target events
		connect(target, SIGNAL(nodeConnected(unsigned)), SLOT(nodeConnected(unsigned)));
		connect(target, SIGNAL(nodeDisconnected(unsigned)), SLOT(nodeDisconnected(unsigned)));
		
		connect(target, SIGNAL(userEvent(unsigned, const VariablesDataVector &)), SLOT(userEvent(unsigned, const VariablesDataVector &)));
		connect(target, SIGNAL(arrayAccessOutOfBounds(unsigned, unsigned, unsigned)), SLOT(arrayAccessOutOfBounds(unsigned, unsigned, unsigned)));
		connect(target, SIGNAL(divisionByZero(unsigned, unsigned)), SLOT(divisionByZero(unsigned, unsigned)));
		
		connect(target, SIGNAL(executionPosChanged(unsigned, unsigned)), SLOT(executionPosChanged(unsigned, unsigned)));
		connect(target, SIGNAL(executionModeChanged(unsigned, Target::ExecutionMode)), SLOT(executionModeChanged(unsigned, Target::ExecutionMode)));
		connect(target, SIGNAL(variablesMemoryEstimatedDirty(unsigned)), SLOT(variablesMemoryEstimatedDirty(unsigned)));
		
		connect(target, SIGNAL(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector &)), SLOT(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector &)));
		
		connect(target, SIGNAL(breakpointSetResult(unsigned, unsigned, bool)), SLOT(breakpointSetResult(unsigned, unsigned, bool)));
	}
	
	void MainWindow::setupMenu()
	{
		// File menu
		QMenu *fileMenu = new QMenu(tr("&File"), this);
		menuBar()->addMenu(fileMenu);
	
		fileMenu->addAction(QIcon(":/images/filenew.png"), tr("&New"),
							this, SLOT(newFile()),
							QKeySequence(tr("Ctrl+N", "File|New")));
		fileMenu->addAction(QIcon(":/images/fileopen.png"), tr("&Open..."), 
							this, SLOT(openFile()),
							QKeySequence(tr("Ctrl+O", "File|Open")));
		fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Save &As..."),
							this, SLOT(saveFile()));
		fileMenu->addAction(QIcon(":/images/filesave.png"), tr("&Save..."),
							this, SLOT(save()),
							QKeySequence(tr("Ctrl+S", "File|Save")));
		fileMenu->addSeparator();
		fileMenu->addAction(QIcon(":/images/network.png"), tr("Connect to &target"),
							target, SLOT(connect()),
							QKeySequence(tr("Ctrl+T", "File|Connect to target")));
		fileMenu->addSeparator();
		fileMenu->addAction(QIcon(":/images/exit.png"), tr("E&xit"),
							qApp, SLOT(quit()),
							QKeySequence(tr("Ctrl+Q", "File|Exit")));
		
		// Edit menu
		cutAct = new QAction(QIcon(":/images/editcut.png"), tr("Cu&t"), this);
		cutAct->setShortcut(tr("Ctrl+X", "Edit|Cut"));
		
		copyAct = new QAction(QIcon(":/images/editcopy.png"), tr("&Copy"), this);
		copyAct->setShortcut(tr("Ctrl+C", "Edit|Copy"));
		
		pasteAct = new QAction(QIcon(":/images/editpaste.png"), tr("&Paste"), this);
		pasteAct->setShortcut(tr("Ctrl+V", "Edit|Paste"));
		
		undoAct = new QAction(QIcon(":/images/undo.png"), tr("&Undo"), this);
		undoAct->setShortcut(tr("Ctrl+Z", "Edit|Undo"));
		
		redoAct = new QAction(QIcon(":/images/redo.png"), tr("Re&do"), this);
		redoAct->setShortcut(tr("Ctrl+Shift+Z", "Edit|Redo"));
		
		cutAct->setEnabled(false);
		copyAct->setEnabled(false);
		undoAct->setEnabled(false);
		redoAct->setEnabled(false);
		
		QMenu *editMenu = new QMenu(tr("&Edit"), this);
		menuBar()->addMenu(editMenu);
		editMenu->addAction(cutAct);
		editMenu->addAction(copyAct);
		editMenu->addAction(pasteAct);
		editMenu->addSeparator();
		editMenu->addAction(undoAct);
		editMenu->addAction(redoAct);
		editMenu->addSeparator();
		
		debugAllAct = new QAction(QIcon(":/images/upload.png"), tr("&Debug all"), this);
		debugAllAct->setShortcut(tr("F7", "Debug|Debug all"));
		
		resetAllAct = new QAction(QIcon(":/images/reset.png"), tr("&Reset all"), this);
		resetAllAct->setShortcut(tr("F8", "Debug|Reset all"));
		
		runAllAct = new QAction(QIcon(":/images/play.png"), tr("Ru&n all"), this);
		runAllAct->setShortcut(tr("F9", "Debug|Run all"));
		
		pauseAllAct = new QAction(QIcon(":/images/pause.png"), tr("&Pause all"), this);
		pauseAllAct->setShortcut(tr("F10", "Debug|Pause all"));
		
		sendEventAct = new QAction(QIcon(":/images/newmsg.png"), tr("&Send event"), this);
		sendEventAct->setShortcut(tr("F11", "Debug|Send event"));
		
		// Debug toolbar
		QToolBar* globalToolBar = addToolBar(tr("Debug"));
		globalToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		globalToolBar->addAction(debugAllAct);
		globalToolBar->addAction(resetAllAct);
		globalToolBar->addAction(runAllAct);
		globalToolBar->addAction(pauseAllAct);
		globalToolBar->addAction(sendEventAct);
		
		// Debug menu
		QMenu *debugMenu = new QMenu(tr("&Debug"), this);
		menuBar()->addMenu(debugMenu);
		debugMenu->addAction(debugAllAct);
		debugMenu->addAction(resetAllAct);
		debugMenu->addAction(runAllAct);
		debugMenu->addAction(pauseAllAct);
		debugMenu->addSeparator();
		debugMenu->addAction(sendEventAct);
		
		// Compilation menu
		QMenu *compilationMenu = new QMenu(tr("&Compilation"), this);
		menuBar()->addMenu(compilationMenu);
		compilationMenu->addAction(QIcon(":/images/view_text.png"), tr("&Show messages"),
							this, SLOT(showCompilationMessages()),
							QKeySequence(tr("Ctrl+M", "Compilation|Show messages")));
		
		// Help menu
		QMenu *helpMenu = new QMenu(tr("&Help"), this);
		menuBar()->addMenu(helpMenu);
		helpMenu->addAction(tr("&About"), this, SLOT(about()));
		helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
	}
	
	void MainWindow::hideEvent(QHideEvent * event)
	{
		compilationMessageBox->hide();
	}
}; // Aseba

