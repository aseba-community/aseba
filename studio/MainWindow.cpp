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
#include "DashelTarget.h"
#include "TargetModels.h"
#include "NamedValuesVectorModel.h"
#include "CustomDelegate.h"
#include "AeslEditor.h"
#include "../common/consts.h"
#include <QtGui>
#include <QtXml>
#include <sstream>
#include <iostream>
#include <cassert>
#include <QTabWidget>

#include <MainWindow.moc>

using std::copy;

// Asserts a dynamic cast.	Similar to the one in boost/cast.hpp
template<typename Derived, typename Base>
static inline Derived polymorphic_downcast(Base base)
{
	Derived derived = dynamic_cast<Derived>(base);
	if (!derived)
		abort();
	return derived;
}

namespace Aseba
{
	#define ASEBA_SVN_REV "$Revision$"
	
	/** \addtogroup studio */
	/*@{*/
	
	 CompilationLogDialog::CompilationLogDialog(QWidget *parent) :
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


	FixedWidthTableView::FixedWidthTableView()
	{
		col1Width = 50;
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}
	
	void FixedWidthTableView::setSecondColumnLongestContent(const QString& content)
	{
		Q_ASSERT(model ()->columnCount() == 2);
		QFontMetrics fm(font());
		col1Width = fm.width(content);
	}
	
	void FixedWidthTableView::resizeEvent ( QResizeEvent * event )
	{
		Q_ASSERT(model ()->columnCount() == 2);
		int col0Width = event->size().width() - col1Width;
		setColumnWidth(0, col0Width);
		setColumnWidth(1, col1Width);
	}
	
	//////
	
	NodeTab::NodeTab(Target *target, const CommonDefinitions *commonDefinitions, int id, QWidget *parent) :
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
		compiler.setCommonDefinitions(commonDefinitions);
		vmFunctionsModel = new TargetFunctionsModel(target->getConstDescription(id), 0);
		vmMemoryModel = new TargetMemoryModel();
		
		// create gui
		setupWidgets();
		setupConnections();
		
		editor->setFocus();
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
		
		// get the value of the variables
		recompile();
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
		
		loadButton = new QPushButton(QIcon(":/images/upload.png"), tr("Load"));
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
		
		vmMemoryView = new FixedWidthTableView;
		vmMemoryView->setShowGrid(false);
		vmMemoryView->verticalHeader()->hide();
		vmMemoryView->horizontalHeader()->hide();
		vmMemoryView->setModel(vmMemoryModel);
		vmMemoryView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		vmMemoryView->setSelectionMode(QAbstractItemView::NoSelection);
		vmMemoryView->setItemDelegateForColumn(1, new SpinBoxDelegate(-32768, 32767, this));
		vmMemoryView->setSecondColumnLongestContent("-888888##");
		vmMemoryView->resizeRowsToContents();
		
		
		// functions
		vmFunctionsView = new QTableView;
		vmFunctionsView->setMinimumHeight(40);
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
		
		// local events
		vmLocalEvents = new QListWidget;
		vmLocalEvents->setMinimumHeight(40);
		vmLocalEvents->setSelectionMode(QAbstractItemView::NoSelection);	
		for (size_t i = 0; i < compiler.getTargetDescription()->localEvents.size(); i++)
		{
			QListWidgetItem* item = new QListWidgetItem(QString::fromUtf8(compiler.getTargetDescription()->localEvents[i].name.c_str()));
			item->setToolTip(QString::fromUtf8(compiler.getTargetDescription()->localEvents[i].description.c_str()));
			vmLocalEvents->addItem(item);
		}
		
		// toolbox
		QToolBox* toolBox = new QToolBox;
		toolBox->addItem(vmFunctionsView, tr("Native Functions"));
		toolBox->addItem(vmLocalEvents, tr("Local Events"));
		
		// panel
		QVBoxLayout *panelLayout = new QVBoxLayout;
		panelLayout->addLayout(executionTitleLayout);
		panelLayout->addWidget(loadButton);
		panelLayout->addWidget(resetButton);
		panelLayout->addWidget(runInterruptButton);
		panelLayout->addWidget(nextButton);
		panelLayout->addLayout(memoryTitleLayout);
		panelLayout->addWidget(vmMemoryView, 3);
		panelLayout->addWidget(toolBox, 1);
		
		QHBoxLayout *layout = new QHBoxLayout;
		layout->addLayout(panelLayout, 1);
		layout->addLayout(editorLayout, 4);
		setLayout(layout);
	}
	
	void NodeTab::setupConnections()
	{
		// execution
		connect(loadButton, SIGNAL(clicked()), SLOT(loadClicked()));
		connect(resetButton, SIGNAL(clicked()), SLOT(resetClicked()));
		connect(runInterruptButton, SIGNAL(clicked()), SLOT(runInterruptClicked()));
		connect(nextButton, SIGNAL(clicked()), SLOT(nextClicked()));
		connect(refreshMemoryButton, SIGNAL(clicked()), SLOT(refreshMemoryClicked()));
		
		// memory
		connect(vmMemoryModel, SIGNAL(variableValueChanged(unsigned, int)), SLOT(setVariableValue(unsigned, int)));
		connect(vmMemoryView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(insertVariableName(const QModelIndex &)));
		
		// editor
		connect(editor, SIGNAL(textChanged()), SLOT(editorContentChanged()));
		connect(editor, SIGNAL(cursorPositionChanged() ), SLOT(cursorMoved()));
		connect(editor, SIGNAL(breakpointSet(unsigned)), SLOT(setBreakpoint(unsigned)));
		connect(editor, SIGNAL(breakpointCleared(unsigned)), SLOT(clearBreakpoint(unsigned)));
		connect(editor, SIGNAL(breakpointClearedAll()), SLOT(breakpointClearedAll()));
		
		connect(compilationResultImage, SIGNAL(clicked()), SLOT(goToError()));
		connect(compilationResultText, SIGNAL(clicked()), SLOT(goToError()));
	}
	
	void NodeTab::resetClicked()
	{
		clearExecutionErrors();
		target->reset(id);
	}
	
	void NodeTab::loadClicked()
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
	
	void NodeTab::writeBytecode()
	{
		if (errorPos == -1)
		{
			loadClicked();
			target->writeBytecode(id);
		}
	}
	
	void NodeTab::setVariableValue(unsigned index, int value)
	{
		VariablesDataVector data(1, value);
		target->setVariables(id, index, data);
	}
	
	void NodeTab::insertVariableName(const QModelIndex &index)
	{
		if ((index.column() == 0) && (index.row() < variablesNames.size()))
		{
			editor->insertPlainText(QString::fromUtf8(variablesNames[index.row()].c_str()));
		}
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
		std::istringstream is(editor->toPlainText().toStdString());
		bool result = compiler.compile(is, bytecode, variablesNames, allocatedVariablesCount, error);
		if (result)
		{
			vmMemoryModel->setVariablesNames(variablesNames);
			compilationResultText->setText(tr("Compilation success."));
			compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
			loadButton->setEnabled(true);
			emit uploadReadynessChanged(true);
			
			errorPos = -1;
		}
		else
		{
			compilationResultText->setText(QString::fromStdString(error.toString()));
			compilationResultImage->setPixmap(QPixmap(QString(":/images/no.png")));
			loadButton->setEnabled(false);
			emit uploadReadynessChanged(false);
			
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
		
		// clear bearkpoints of target if currently in debugging mode
		if (editor->debugging)
		{
			//target->stop(id);
			// when code is changed, remove breakpoints from target but keep them locally as pending for next code load
			doRehighlight = true;
			editor->debugging = false;
			resetButton->setEnabled(false);
			runInterruptButton->setEnabled(false);
			nextButton->setEnabled(false);
			target->clearBreakpoints(id);
			switchEditorProperty("breakpoint", "breakpointPending");
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
	
	void NodeTab::breakpointClearedAll()
	{
		rehighlight();
		target->clearBreakpoints(id);
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
			
			if (clearEditorProperty("active"))
				rehighlight();
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
		target->clearBreakpoints(id);
		QTextBlock block = editor->document()->begin();
		unsigned lineCounter = 0;
		while (block != editor->document()->end())
		{
			AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(block.userData());
			if (uData && (uData->properties.contains("breakpoint") || uData->properties.contains("breakpointPending")))
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
					block.setUserData(0);
				}
				changed = true;
			}
			block = block.next();
		}
		
		return changed;
	}
	
	void NodeTab::switchEditorProperty(const QString &oldProperty, const QString &newProperty)
	{
		QTextBlock block = editor->document()->begin();
		while (block != editor->document()->end())
		{
			AeslEditorUserData *uData = static_cast<AeslEditorUserData *>(block.userData());
			if (uData && uData->properties.contains(oldProperty))
			{
				uData->properties.remove(oldProperty);
				uData->properties[newProperty] = QVariant();
			}
			block = block.next();
		}
	}
	
	
	MainWindow::MainWindow(QWidget *parent) :
		QMainWindow(parent)
	{
		// create target
		target = new DashelTarget();
		
		// create models
		eventsDescriptionsModel = new NamedValuesVectorModel(&commonDefinitions.events, this);
		constantsDefinitionsModel = new NamedValuesVectorModel(&commonDefinitions.constants, this);
		
		// create gui
		setupWidgets();
		setupMenu();
		setupConnections();
		
		// cosmetic dix-up
		setWindowTitle(tr("Aseba Studio"));
		resize(1000,700);
	}
	
	MainWindow::~MainWindow()
	{
		delete target;
	}
	
	#ifndef SVN_REV
	#define SVN_REV "unknown, please configure your build system to set SVN_REV correctly"
	#endif
	
	void MainWindow::about()
	{
		QMessageBox::about(this, tr("About Aseba Studio"),
					tr(	"<p>Aseba pre-release</p>" \
						"<ul><li>Aseba " \
						ASEBA_SVN_REV \
						"</li><li>Dashel "\
						DASHEL_SVN_REV \
						"</li></ul>" \
						"<p>(c) 2006-2008 <a href=\"http://stephane.magnenat.net\">Stephane Magnenat</a></p>" \
						"<p><a href=\"http://mobots.epfl.ch\">Mobots group</a> - <a href=\"http://lsro.epfl.ch\">LSRO1</a> - <a href=\"http://www.epfl.ch\">EPFL</a></p>"));
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
		
		eventsDescriptionsModel->clear();
		
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
						eventsDescriptionsModel->addNamedValue(NamedValue(element.attribute("name").toStdString(), element.attribute("size").toUInt()));
					}
					else if (element.tagName() == "constant")
					{
						constantsDefinitionsModel->addNamedValue(NamedValue(element.attribute("name").toStdString(), element.attribute("value").toInt()));
					}
				}
				domNode = domNode.nextSibling();
			}
			
			// check if there was some matching problem
			if (noNodeCount)
				QMessageBox::warning(this,
					tr("Loading"),
					tr("%0 scripts have no corresponding nodes in the current network and have not been loaded.").arg(noNodeCount)
				);
			
			// update recent files
			updateRecentFiles(fileName);
			regenerateOpenRecentMenu();
		}
		else
			QMessageBox::warning(this,
				tr("Loading"),
				tr("Error in XML source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn)
			);
		file.close();
		
		recompileAll();
		
		
	}
	
	void MainWindow::openRecentFile()
	{
		QAction* entry = polymorphic_downcast<QAction*>(sender());
		openFile(entry->text());
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
		updateRecentFiles(fileName);
		
		// initiate DOM tree
		QDomDocument document("aesl-source");
		QDomElement root = document.createElement("network");
		document.appendChild(root);
		
		// events
		for (size_t i = 0; i < commonDefinitions.events.size(); i++)
		{
			QDomElement element = document.createElement("event");
			element.setAttribute("name", QString::fromStdString(commonDefinitions.events[i].name));
			element.setAttribute("size", QString::number(commonDefinitions.events[i].value));
			root.appendChild(element);
		}
		
		// constants
		for (size_t i = 0; i < commonDefinitions.constants.size(); i++)
		{
			QDomElement element = document.createElement("constant");
			element.setAttribute("name", QString::fromStdString(commonDefinitions.constants[i].name));
			element.setAttribute("value", QString::number(commonDefinitions.constants[i].value));
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
	
	void MainWindow::exportMemoriesContent()
	{
		QString exportFileName = QFileDialog::getSaveFileName(this, tr("Export memory content"), "", "All Files (*.*);;CSV files (*.csv);;Text files (*.txt)");
		
		QFile file(exportFileName);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
			return;
		
		QTextStream out(&file);
		
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			std::string oldName;
			bool first = true;
			for (int j = 0; j < tab->variablesNames.size(); j++)
			{
				const std::string& name = tab->variablesNames[j];
				if (!name.empty())
				{
					if (name != oldName)
					{
						if (first)
							first = false;
						else
							out << "\n";
						out << target->getName(tab->nodeId()) << "." << QString::fromUtf8(name.c_str());
						oldName = name;
					}
					out << ", " << tab->vmMemoryModel->variablesData[j];
				}
			}
		}
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
	
	void MainWindow::loadAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			tab->loadClicked();
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
	
	void MainWindow::clearAllExecutionError()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			tab->clearExecutionErrors();
		}
	}
	
	void MainWindow::uploadReadynessChanged()
	{
		bool ready = true;
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			if (!tab->loadButton->isEnabled())
			{
				ready = false;
				break;
			}
		}
		
		loadAllAct->setEnabled(ready);
		writeAllBytecodesAct->setEnabled(ready);
	}
	
	void MainWindow::sendEvent()
	{
		QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
		Q_ASSERT(currentRow.isValid());
		
		unsigned eventId = currentRow.row();
		QString eventName = QString::fromStdString(commonDefinitions.events[eventId].name);
		int argsCount = commonDefinitions.events[eventId].value;
		VariablesDataVector data(argsCount);
		
		if (argsCount > 0)
		{
			QString argList;
			while (true)
			{
				bool ok;
				argList = QInputDialog::getText(this, tr("Specify event arguments"), tr("Please specify the %0 arguments of event %1").arg(argsCount).arg(eventName), QLineEdit::Normal, argList, &ok);
				if (ok)
				{
					QStringList args = argList.split(QRegExp("[\\s,]+"), QString::SkipEmptyParts);
					if (args.size() != argsCount)
					{
						QMessageBox::warning(this,
							tr("Wrong number of arguments"),
							tr("You gave %0 arguments where event %1 requires %2").arg(args.size()).arg(eventName).arg(argsCount)
						);
						continue;
					}
					for (int i = 0; i < args.size(); i++)
					{
						data[i] = args.at(i).toShort(&ok);
						if (!ok)
						{
							QMessageBox::warning(this,
								tr("Invalid value"),
								tr("Invalid value for argument %0 of event %1").arg(i).arg(eventName)
							);
							break;
						}
					}
					if (ok)
						break;
				}
				else
					return;
			}
		}
		
		target->sendEvent(eventId, data);
	}
	
	void MainWindow::sendEventIf(const QModelIndex &index)
	{
		if (index.column() == 0)
			sendEvent();
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
			std::istringstream is(tab->editor->toPlainText().toStdString());
			std::ostringstream compilationMessages;
			BytecodeVector bytecode;
			Error error;
			bool result = tab->compiler.compile(is, bytecode, tab->variablesNames, tab->allocatedVariablesCount, error, &compilationMessages);
			
			if (result)
				compilationMessageBox->text->setText(
					tr("Compilation success.") + QString("\n\n") + 
					QString::fromStdString(compilationMessages.str())
				);
			else
				compilationMessageBox->text->setText(
					QString::fromStdString(error.toString()) + ".\n\n" +
					QString::fromStdString(compilationMessages.str())
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
			if (commonDefinitions.events.contains(eventName.toStdString()))
			{
				QMessageBox::warning(this, tr("Event already exists"), tr("Event %0 already exists.").arg(eventName));
			}
			else
			{
				eventsDescriptionsModel->addNamedValue(NamedValue(eventName.toStdString(), 0));
				recompileAll();
			}
		}
	}
	
	void MainWindow::removeEventNameClicked()
	{
		QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
		Q_ASSERT(currentRow.isValid());
		eventsDescriptionsModel->delNamedValue(currentRow.row());
		
		recompileAll();
	}
	
	void MainWindow::eventsDescriptionsSelectionChanged()
	{
		bool isSelected = eventsDescriptionsView->selectionModel()->currentIndex().isValid();
		removeEventNameButton->setEnabled(isSelected);
		sendEventButton->setEnabled(isSelected);
	}
	
	void MainWindow::addConstantClicked()
	{
		bool ok;
		QString constantName = QInputDialog::getText(this, tr("Add a new constant"), tr("Name:"), QLineEdit::Normal, "", &ok);
		if (ok && !constantName.isEmpty())
		{
			if (commonDefinitions.constants.contains(constantName.toStdString()))
			{
				QMessageBox::warning(this, tr("Constant already defined"), tr("Constant %0 is already defined.").arg(constantName));
			}
			else
			{
				constantsDefinitionsModel->addNamedValue(NamedValue(constantName.toStdString(), 0));
				recompileAll();
			}
		}
	}
	
	void MainWindow::removeConstantClicked()
	{
		QModelIndex currentRow = constantsView->selectionModel()->currentIndex();
		Q_ASSERT(currentRow.isValid());
		constantsDefinitionsModel->delNamedValue(currentRow.row());
		
		recompileAll();
	}
	
	void MainWindow::constantsSelectionChanged()
	{
		bool isSelected = constantsView->selectionModel()->currentIndex().isValid();
		removeConstantButton->setEnabled(isSelected);
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
	
	void MainWindow::writeAllBytecodes()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			tab->writeBytecode();
		}
	}
	
	//! A new node has connected to the network.
	void MainWindow::nodeConnected(unsigned node)
	{
		NodeTab* tab = new NodeTab(target, &commonDefinitions, node);
		connect(tab, SIGNAL(uploadReadynessChanged(bool)), SLOT(uploadReadynessChanged()));
		nodes->addTab(tab, target->getName(node));
		
		regenerateToolsMenus();
	}
	
	//! A node has disconnected from the network.
	void MainWindow::nodeDisconnected(unsigned node)
	{
		int index = getIndexFromId(node);
		Q_ASSERT(index >= 0);
		NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(index));
		
		nodes->removeTab(index);
		delete tab;
		
		regenerateToolsMenus();
	}
	
	//! The network connection has been cut: all nodes have disconnected.
	void MainWindow::networkDisconnected()
	{
		disconnect(nodes, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
		std::vector<QWidget *> widgets(nodes->count());
		for (int i = 0; i < nodes->count(); i++)
			widgets[i] = nodes->widget(i);
		nodes->clear();
		for (size_t i = 0; i < widgets.size(); i++)
			delete widgets[i];
		connect(nodes, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
	}
	
	//! A user event has arrived from the network.
	void MainWindow::userEvent(unsigned id, const VariablesDataVector &data)
	{
		QString text = QTime::currentTime().toString("hh:mm:ss.zzz");
		if (id < commonDefinitions.events.size())
			text += QString("\n%0 : ").arg(QString::fromStdString(commonDefinitions.events[id].name));
		else
			text += tr("\nevent %0 : ").arg(id);
		for (size_t i = 0; i < data.size(); i++)
			text += QString("%0 ").arg(data[i]);
		
		if (logger->count() > 50)
			delete logger->takeItem(0);
		QListWidgetItem * item = new QListWidgetItem(QIcon(":/images/info.png"), text, logger);
		logger->scrollToBottom();
	}
	
	//! Some user events have been dropped, i.e. not sent to the gui
	void MainWindow::userEventsDropped(unsigned amount)
	{
		QString text = QTime::currentTime().toString("hh:mm:ss.zzz");
		text += QString("\n%0 user events not shown").arg(amount);
		
		if (logger->count() > 50)
			delete logger->takeItem(0);
		QListWidgetItem * item = new QListWidgetItem(QIcon(":/images/info.png"), text, logger);
		logger->scrollToBottom();
	}
	
	//! A node did an access out of array bounds exception.
	void MainWindow::arrayAccessOutOfBounds(unsigned node, unsigned line, unsigned index)
	{
		addErrorEvent(node, line, tr("access out of memory (%2/%3)").arg(index).arg(target->getConstDescription(node)->variablesSize));
	}
	
	//! A node did a division by zero exception.
	void MainWindow::divisionByZero(unsigned node, unsigned line)
	{
		addErrorEvent(node, line, tr("division by zero"));
	}
	
	//! A new event was run and the current killed on a node
	void MainWindow::eventExecutionKilled(unsigned node, unsigned line)
	{
		addErrorEvent(node, line, tr("event execution killed"));
	}
	
	//! A node has produced an error specific to it
	void MainWindow::nodeSpecificError(unsigned node, unsigned line, const QString& message)
	{
		addErrorEvent(node, line, message);
	}
	
	//! Generic part of error events reporting
	void MainWindow::addErrorEvent(unsigned node, unsigned line, const QString& message)
	{
		NodeTab* tab = getTabFromId(node);
		Q_ASSERT(tab);
		
		if (tab->setEditorProperty("executionError", QVariant(), line, true))
		{
			tab->rehighlighting = true;
			tab->highlighter->rehighlight();
		}
		
		QString text = QTime::currentTime().toString("hh:mm:ss.zzz");
		text += "\n" + tr("%0:%1: %2").arg(target->getName(node)).arg(line + 1).arg(message);
		
		if (logger->count() > 50)
			delete logger->takeItem(0);
		QListWidgetItem *item = new QListWidgetItem(QIcon(":/images/warning.png"), text, logger);
		item->setData(Qt::UserRole, QPoint(node, line));
		logger->scrollToBottom();
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
		
		eventsDockLayout->addWidget(new QLabel(tr("<b>Constants</b>")));
		
		QHBoxLayout* constantsAddRemoveLayout = new QHBoxLayout;;
		constantsAddRemoveLayout->addStretch();
		addConstantButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
		constantsAddRemoveLayout->addWidget(addConstantButton);
		removeConstantButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
		removeConstantButton->setEnabled(false);
		constantsAddRemoveLayout->addWidget(removeConstantButton);
		
		eventsDockLayout->addLayout(constantsAddRemoveLayout);
		
		constantsView = new FixedWidthTableView;
		constantsView->setShowGrid(false);
		constantsView->verticalHeader()->hide();
		constantsView->horizontalHeader()->hide();
		constantsView->setModel(constantsDefinitionsModel);
		constantsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		constantsView->setSelectionMode(QAbstractItemView::SingleSelection);
		constantsView->setSelectionBehavior(QAbstractItemView::SelectRows);
		constantsView->setItemDelegateForColumn(1, new SpinBoxDelegate(-32768, 32767, this));
		constantsView->setMinimumHeight(100);
		constantsView->setSecondColumnLongestContent("-888888##");
		constantsView->resizeRowsToContents();
		
		eventsDockLayout->addWidget(constantsView, 1);
		
		eventsDockLayout->addWidget(new QLabel(tr("<b>Events</b>")));
		
		QHBoxLayout* eventsAddRemoveLayout = new QHBoxLayout;;
		eventsAddRemoveLayout->addStretch();
		addEventNameButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
		eventsAddRemoveLayout->addWidget(addEventNameButton);
		removeEventNameButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
		removeEventNameButton->setEnabled(false);
		eventsAddRemoveLayout->addWidget(removeEventNameButton);
		sendEventButton = new QPushButton(QPixmap(QString(":/images/newmsg.png")), "");
		sendEventButton->setEnabled(false);
		eventsAddRemoveLayout->addWidget(sendEventButton);
		
		eventsDockLayout->addLayout(eventsAddRemoveLayout);
		
		eventsDescriptionsView = new FixedWidthTableView;
		eventsDescriptionsView->setShowGrid(false);
		eventsDescriptionsView->verticalHeader()->hide();
		eventsDescriptionsView->horizontalHeader()->hide();
		eventsDescriptionsView->setModel(eventsDescriptionsModel);
		eventsDescriptionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		eventsDescriptionsView->setSelectionMode(QAbstractItemView::SingleSelection);
		eventsDescriptionsView->setSelectionBehavior(QAbstractItemView::SelectRows);
		eventsDescriptionsView->setItemDelegateForColumn(1, new SpinBoxDelegate(0, (ASEBA_MAX_PACKET_SIZE-6)/2, this));
		eventsDescriptionsView->setMinimumHeight(100);
		eventsDescriptionsView->setSecondColumnLongestContent("255###");
		eventsDescriptionsView->resizeRowsToContents();
		
		eventsDockLayout->addWidget(eventsDescriptionsView, 1);
		
		logger = new QListWidget;
		logger->setMinimumSize(80,100);
		logger->setSelectionMode(QAbstractItemView::NoSelection);
		eventsDockLayout->addWidget(logger, 3);
		clearLogger = new QPushButton(tr("Clear"));
		eventsDockLayout->addWidget(clearLogger);
		
		splitter->addWidget(eventsDockWidget);
		
		splitter->setSizes(QList<int>() << 800 << 150);
		
		// dialog box
		compilationMessageBox = new CompilationLogDialog(this);
	}
	
	void MainWindow::setupConnections()
	{
		// general connections
		connect(nodes, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
		connect(logger, SIGNAL(itemDoubleClicked(QListWidgetItem *)), SLOT(logEntryDoubleClicked(QListWidgetItem *)));
		
		// global actions
		connect(loadAllAct, SIGNAL(triggered()), SLOT(loadAll()));
		connect(resetAllAct, SIGNAL(triggered()), SLOT(resetAll()));
		connect(runAllAct, SIGNAL(triggered()), SLOT(runAll()));
		connect(pauseAllAct, SIGNAL(triggered()), SLOT(pauseAll()));
		
		// events
		connect(addEventNameButton, SIGNAL(clicked()), SLOT(addEventNameClicked()));
		connect(removeEventNameButton, SIGNAL(clicked()), SLOT(removeEventNameClicked()));
		connect(sendEventButton, SIGNAL(clicked()), SLOT(sendEvent()));
		connect(eventsDescriptionsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), SLOT(eventsDescriptionsSelectionChanged()));
		connect(eventsDescriptionsView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(sendEventIf(const QModelIndex &)));
		connect(eventsDescriptionsModel, SIGNAL(dataChanged ( const QModelIndex &, const QModelIndex & ) ), SLOT(recompileAll()));
		
		// logger
		connect(clearLogger, SIGNAL(clicked()), logger, SLOT(clear()));
		connect(clearLogger, SIGNAL(clicked()), SLOT(clearAllExecutionError()));
		
		// constants
		connect(addConstantButton, SIGNAL(clicked()), SLOT(addConstantClicked()));
		connect(removeConstantButton, SIGNAL(clicked()), SLOT(removeConstantClicked()));
		connect(constantsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), SLOT(constantsSelectionChanged()));
		connect(constantsDefinitionsModel, SIGNAL(dataChanged ( const QModelIndex &, const QModelIndex & ) ), SLOT(recompileAll()));
		
		// target events
		connect(target, SIGNAL(nodeConnected(unsigned)), SLOT(nodeConnected(unsigned)));
		connect(target, SIGNAL(nodeDisconnected(unsigned)), SLOT(nodeDisconnected(unsigned)));
		connect(target, SIGNAL(networkDisconnected()),  SLOT(networkDisconnected()));
		
		connect(target, SIGNAL(userEvent(unsigned, const VariablesDataVector &)), SLOT(userEvent(unsigned, const VariablesDataVector &)));
		connect(target, SIGNAL(userEventsDropped(unsigned)), SLOT(userEventsDropped(unsigned)));
		connect(target, SIGNAL(arrayAccessOutOfBounds(unsigned, unsigned, unsigned)), SLOT(arrayAccessOutOfBounds(unsigned, unsigned, unsigned)));
		connect(target, SIGNAL(divisionByZero(unsigned, unsigned)), SLOT(divisionByZero(unsigned, unsigned)));
		connect(target, SIGNAL(eventExecutionKilled(unsigned, unsigned)), SLOT(eventExecutionKilled(unsigned, unsigned)));
		connect(target, SIGNAL(nodeSpecificError(unsigned, unsigned, QString)), SLOT(nodeSpecificError(unsigned, unsigned, QString)));
		
		connect(target, SIGNAL(executionPosChanged(unsigned, unsigned)), SLOT(executionPosChanged(unsigned, unsigned)));
		connect(target, SIGNAL(executionModeChanged(unsigned, Target::ExecutionMode)), SLOT(executionModeChanged(unsigned, Target::ExecutionMode)));
		connect(target, SIGNAL(variablesMemoryEstimatedDirty(unsigned)), SLOT(variablesMemoryEstimatedDirty(unsigned)));
		
		connect(target, SIGNAL(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector &)), SLOT(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector &)));
		
		connect(target, SIGNAL(breakpointSetResult(unsigned, unsigned, bool)), SLOT(breakpointSetResult(unsigned, unsigned, bool)));
	}
	
	void MainWindow::regenerateOpenRecentMenu()
	{
		openRecentMenu->clear();
		
		QSettings settings("EPFL-LSRO-Mobots", "Aseba Studio");
		QStringList recentFiles = settings.value("recent files").toStringList();
		
		for (int i = 0; i < recentFiles.size(); i++)
			openRecentMenu->addAction(recentFiles.at(i), this, SLOT(openRecentFile()));
	}
	
	void MainWindow::updateRecentFiles(const QString& fileName)
	{
		QSettings settings("EPFL-LSRO-Mobots", "Aseba Studio");
		QStringList recentFiles = settings.value("recent files").toStringList();
		if (recentFiles.contains(fileName))
			recentFiles.removeAt(recentFiles.indexOf(fileName));
		recentFiles.push_front(fileName);
		const int maxRecentFiles = 8;
		if (recentFiles.size() > maxRecentFiles)
			recentFiles.pop_back();
		settings.setValue("recent files", recentFiles);
	}
	
	void MainWindow::regenerateToolsMenus()
	{
		writeBytecodeMenu->clear();
		
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			
			QAction *act = writeBytecodeMenu->addAction(tr("...inside %0").arg(target->getName(tab->nodeId())),tab, SLOT(writeBytecode()));
			
			connect(tab, SIGNAL(uploadReadynessChanged(bool)), act, SLOT(setEnabled(bool)));
		}
		
		writeBytecodeMenu->addSeparator();
		
		writeAllBytecodesAct = writeBytecodeMenu->addAction(tr("...inside all nodes"), this, SLOT(writeAllBytecodes()));
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
		openRecentMenu = new QMenu(tr("Open &Recent"));
		regenerateOpenRecentMenu();
		fileMenu->addMenu(openRecentMenu)->setIcon(QIcon(":/images/fileopen.png"));
		
		fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Save &As..."),
							this, SLOT(saveFile()));
		fileMenu->addAction(QIcon(":/images/filesave.png"), tr("&Save..."),
							this, SLOT(save()),
							QKeySequence(tr("Ctrl+S", "File|Save")));
		fileMenu->addSeparator();
		/*fileMenu->addAction(QIcon(":/images/network.png"), tr("Connect to &target"),
							target, SLOT(connect()),
							QKeySequence(tr("Ctrl+T", "File|Connect to target")));
		fileMenu->addSeparator();*/
		fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Export &memories content..."),
							this, SLOT(exportMemoriesContent()));
		fileMenu->addSeparator();
		fileMenu->addAction(QIcon(":/images/exit.png"), tr("&Quit"),
							qApp, SLOT(quit()),
							QKeySequence(tr("Ctrl+Q", "File|Quit")));
		
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
		
		loadAllAct = new QAction(QIcon(":/images/upload.png"), tr("&Load all"), this);
		loadAllAct->setShortcut(tr("F7", "Load|Load all"));
		
		resetAllAct = new QAction(QIcon(":/images/reset.png"), tr("&Reset all"), this);
		resetAllAct->setShortcut(tr("F8", "Debug|Reset all"));
		
		runAllAct = new QAction(QIcon(":/images/play.png"), tr("Ru&n all"), this);
		runAllAct->setShortcut(tr("F9", "Debug|Run all"));
		
		pauseAllAct = new QAction(QIcon(":/images/pause.png"), tr("&Pause all"), this);
		pauseAllAct->setShortcut(tr("F10", "Debug|Pause all"));
		
		// Debug toolbar
		QToolBar* globalToolBar = addToolBar(tr("Debug"));
		globalToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		globalToolBar->addAction(loadAllAct);
		globalToolBar->addAction(resetAllAct);
		globalToolBar->addAction(runAllAct);
		globalToolBar->addAction(pauseAllAct);
		
		// Debug menu
		QMenu *debugMenu = new QMenu(tr("&Debug"), this);
		menuBar()->addMenu(debugMenu);
		debugMenu->addAction(loadAllAct);
		debugMenu->addAction(resetAllAct);
		debugMenu->addAction(runAllAct);
		debugMenu->addAction(pauseAllAct);
		
		// Compilation menu
		QMenu *compilationMenu = new QMenu(tr("&Compilation"), this);
		menuBar()->addMenu(compilationMenu);
		compilationMenu->addAction(QIcon(":/images/view_text.png"), tr("&Show messages"),
							this, SLOT(showCompilationMessages()),
							QKeySequence(tr("Ctrl+M", "Compilation|Show messages")));
		
		// Tool menu
		QMenu *toolMenu = new QMenu(tr("&Tools"), this);
		menuBar()->addMenu(toolMenu);
		writeBytecodeMenu = new QMenu(tr("Write the program(s)..."));
		toolMenu->addMenu(writeBytecodeMenu);
		regenerateToolsMenus();
		
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
	
	/*@}*/
}; // Aseba

