/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
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

#include "MainWindow.h"
#include "ClickableLabel.h"
#include "DashelTarget.h"
#include "TargetModels.h"
#include "NamedValuesVectorModel.h"
#include "AeslEditor.h"
#include "EventViewer.h"
#include "FindDialog.h"
#include "CompilerTranslator.h"
#include "ModelAggregator.h"
#include "../common/consts.h"
#include "../common/productids.h"
#include "../utils/utils.h"
#include <QtGui>
#include <QtXml>
#include <sstream>
#include <iostream>
#include <cassert>
#include <QTabWidget>
#include <QtConcurrentRun>

#include <MainWindow.moc>
#include <version.h>

#include <iostream>

using std::copy;


namespace Aseba
{
// 	/** \addtogroup studio */
	/*@{*/

	CompilationLogDialog::CompilationLogDialog(QWidget *parent) :
		QTextEdit(parent)
	{
		QFont font;
		font.setFamily("");
		font.setStyleHint(QFont::TypeWriter);
		font.setFixedPitch(true);
		font.setPointSize(10);
		
		setFont(font);
		setTabStopWidth( QFontMetrics(font).width(' ') * 4);
		setReadOnly(true);
		
		setWindowTitle(tr("Aseba Studio: Output of last compilation"));
		
		resize(600, 560);
	}
	
	void CompilationLogDialog::hideEvent( QHideEvent * event )
	{
		if (!isVisible())
			emit hidden();
	}
	
	//////

	EditorsPlotsTabWidget::EditorsPlotsTabWidget()
	{
		vmMemorySize[0] = -1;	// not yet initialized
		vmMemorySize[1] = -1;	// not yet initialized
		readSettings();		// read user's preferences
		connect(this, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
	}

	EditorsPlotsTabWidget::~EditorsPlotsTabWidget()
	{
		// store user's preferences
		writeSettings();
	}
	
	void EditorsPlotsTabWidget::addTab(QWidget* widget, const QString& label, bool closable)
	{
		const int index = QTabWidget::addTab(widget, label);
		#if QT_VERSION >= 0x040500
		if (closable)
		{
			QPushButton* button = new QPushButton(QIcon(":/images/remove.png"), "");
			button->setFlat(true);
			connect(button, SIGNAL(clicked(bool)), this, SLOT(removeAndDeleteTab()));
			tabBar()->setTabButton(index, QTabBar::RightSide, button);
		}
		#endif // QT_VERSION >= 0x040500

		// manage the sections size for the vmMemoryView child widget
		NodeTab* tab = dynamic_cast<NodeTab*>(widget);
		if (tab)
		{
			vmMemoryViewResize(tab);
			connect(tab->vmMemoryView->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(vmMemoryResized(int,int,int)));
		}
	}

	void EditorsPlotsTabWidget::highlightTab(int index, QColor color)
	{
		tabBar()->setTabTextColor(index, color);
	}

	void EditorsPlotsTabWidget::setExecutionMode(int index, Target::ExecutionMode state)
	{
		if (state == Target::EXECUTION_UNKNOWN)
		{
			setTabIcon(index, QIcon());
		}
		else if (state == Target::EXECUTION_RUN)
		{
			setTabIcon(index, QIcon(":/images/play.png"));
		}
		else if (state == Target::EXECUTION_STEP_BY_STEP)
		{
			setTabIcon(index, QIcon(":/images/pause.png"));
		}
		else if (state == Target::EXECUTION_STOP)
		{
			setTabIcon(index, QIcon(":/images/stop.png"));
		}
	}

	void EditorsPlotsTabWidget::removeAndDeleteTab(int index)
	{
		#if QT_VERSION >= 0x040500
		if (index < 0)
		{
			QWidget* button(polymorphic_downcast<QWidget*>(sender()));
			for (int i = 0; i < count(); ++i)
			{
				if (tabBar()->tabButton(i, QTabBar::RightSide) == button)
				{
					index = i;
					break;
				}
			}
		}
		#endif // QT_VERSION >= 0x040500

		if (index >= 0)
		{
			QWidget* w(widget(index));
			QTabWidget::removeTab(index);
			w->deleteLater();
		}
	}

	void EditorsPlotsTabWidget::vmMemoryResized(int col, int oldSize, int newSize)
	{
		// keep track of the current value, to apply it on the other tabs
		vmMemorySize[col] = newSize;
	}

	void EditorsPlotsTabWidget::tabChanged(int index)
	{
		// resize the vmMemoryView, to match the user choice
		NodeTab* tab = dynamic_cast<NodeTab*>(currentWidget());
		if (!tab)
			return;

		vmMemoryViewResize(tab);
		// reset the tab highlight
		resetHighlight(index);
	}

	void EditorsPlotsTabWidget::resetHighlight(int index)
	{
		tabBar()->setTabTextColor(index, Qt::black);
	}

	void EditorsPlotsTabWidget::vmMemoryViewResize(NodeTab *tab)
	{
		if (!tab)
			return;

		// resize the vmMemoryView QTreeView, according to the global value
		for (int i = 0; i < 2; i++)
			if (vmMemorySize[i] != -1)
				tab->vmMemoryView->header()->resizeSection(i, vmMemorySize[i]);
	}

	void EditorsPlotsTabWidget::readSettings()
	{
		QSettings settings;
		vmMemorySize[0] = settings.value("vmMemoryView/col0", -1).toInt();
		vmMemorySize[1] = settings.value("vmMemoryView/col1", -1).toInt();
	}

	void EditorsPlotsTabWidget::writeSettings()
	{
		QSettings settings;
		settings.setValue("vmMemoryView/col0", vmMemorySize[0]);
		settings.setValue("vmMemoryView/col1", vmMemorySize[1]);
	}

	//////
	
	void ScriptTab::createEditor()
	{
		// editor widget
		editor = new AeslEditor(this);
		breakpoints = new AeslBreakpointSidebar(editor);
		linenumbers = new AeslLineNumberSidebar(editor);
		highlighter = new AeslHighlighter(editor, editor->document());
	}
	
	//////
	
	AbsentNodeTab::AbsentNodeTab(const unsigned id, const QString& name, const QString& sourceCode) :
		ScriptTab(id),
		name(name)
	{
		createEditor();
		editor->setReadOnly(true);
		editor->setPlainText(sourceCode);
		QVBoxLayout *layout = new QVBoxLayout;
		layout->addWidget(editor);
		setLayout(layout);
	}
	
	//////
	
	NodeTab::CompilationResult* compilationThread(const TargetDescription targetDescription, const CommonDefinitions commonDefinitions, QString source, bool dump);
	
	NodeTab::NodeTab(MainWindow* mainWindow, Target *target, const CommonDefinitions *commonDefinitions, int id, QWidget *parent) :
		QSplitter(parent),
		ScriptTab(id),
		VariableListener(0),
		pid(ASEBA_PID_UNDEFINED),
		target(target),
		commonDefinitions(commonDefinitions),
		mainWindow(mainWindow),
		currentPC(0),
		previousMode(Target::EXECUTION_UNKNOWN),
		firstCompilation(true),
		showHidden(mainWindow->showHiddenAct->isChecked()),
		compilationDirty(false),
		isSynchronized(true)
	{
		// setup some variables
		rehighlighting = false;
		errorPos = -1;
		allocatedVariablesCount = 0;
		
		// create models
		vmFunctionsModel = new TargetFunctionsModel(target->getDescription(id));
		vmMemoryModel = new TargetVariablesModel();
		variablesModel = vmMemoryModel;
		subscribeToVariableOfInterest(ASEBA_PID_VAR_NAME);

		// create gui
		setupWidgets();
		setupConnections();

		// create aggregated models
		// local and global events
		ModelAggregator* aggregator = new ModelAggregator(this);
		aggregator->addModel(vmLocalEvents->model());
		aggregator->addModel(mainWindow->eventsDescriptionsModel);
		eventAggregator = aggregator;
		// variables and constants
		aggregator = new ModelAggregator(this);
		aggregator->addModel(vmMemoryModel);
		aggregator->addModel(mainWindow->constantsDefinitionsModel);
		variableAggregator = aggregator;

		// create the sorting proxy
		sortingProxy = new QSortFilterProxyModel(this);
		sortingProxy->setDynamicSortFilter(true);
		sortingProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
		sortingProxy->setSortRole(Qt::DisplayRole);

		// create the chainsaw filter for native functions
		functionsFlatModel = new TreeChainsawFilter(this);
		functionsFlatModel->setSourceModel(vmFunctionsModel);

		editor->setFocus();
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
		
		// get the value of the variables
		// compile in this thread the first time
		NodeTab::CompilationResult* result = compilationThread(*target->getDescription(id), *commonDefinitions, editor->toPlainText(), false);
		processCompilationResult(result);
	}

	NodeTab::~NodeTab()
	{
		compilationFuture.waitForFinished();
		for (NodeToolInterfaces::const_iterator it(tools.begin()); it != tools.end(); ++it)
		{
			NodeToolInterface* tool(*it);
			if (!tool->surviveTabDestruction())
				delete (*it);
		}
		delete vmFunctionsModel;
		delete vmMemoryModel;
	}
	
	void NodeTab::timerEvent ( QTimerEvent * event )
	{
		// only fetch what is visible
		const QList<TargetVariablesModel::Variable> variables(variablesModel->getVariables());
		assert(variables.size() == variablesModel->rowCount());
		
		unsigned currentReqCount(0);
		unsigned currentReqPos(0);
		for (int i = 0; i < variables.size(); ++i)
		{
			const TargetVariablesModel::Variable& var(variables[i]);
			if  (((var.value.size() == 1) ||
				 (vmMemoryView->isExpanded(variablesModel->index(i, 0)))
				 ) &&
				 (!vmMemoryView->isRowHidden(i, QModelIndex()))
				)
			{
				if (currentReqCount == 0)
				{
					// new request
					currentReqPos = var.pos;
					currentReqCount = var.value.size();
				}
				else if (currentReqPos + currentReqCount == var.pos)
				{
					// continuous, append
					currentReqCount += var.value.size();
				}
				else
				{
					// flush and reset
					target->getVariables(id, currentReqPos, currentReqCount);
					// new request
					currentReqPos = var.pos;
					currentReqCount = var.value.size();
				}
			}
		}
		if (currentReqCount != 0)
		{
			// flush request
			target->getVariables(id, currentReqPos, currentReqCount);
		}
	}
	
	void NodeTab::variableValueUpdated(const QString& name, const VariablesDataVector& values)
	{
		if ((name == ASEBA_PID_VAR_NAME) && (values.size() >= 1) && (tools.empty()))
		{
			pid = values[0];
			//qDebug() << "received product id " << pid;
			
			tools = nodeToolRegistrer.create(pid, this);
			if (tools.empty())
				return;
			
			QWidget* widget(new QWidget);
			QVBoxLayout* layout(new QVBoxLayout);
			for (NodeToolInterfaces::const_iterator it(tools.begin()); it != tools.end(); ++it)
				layout->addWidget((*it)->createMenuEntry());
			widget->setLayout(layout);
			int index = toolBox->addItem(widget, tr("Local Tools"));
			toolBox->setCurrentIndex(index);
			
			mainWindow->regenerateHelpMenu();
			
			connect(mainWindow, SIGNAL(MainWindowClosed()), SLOT(closePlugins()));
		}
	}
	
	void NodeTab::setupWidgets()
	{
		createEditor();
		
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

		// memory usage notification area
		memoryUsageText = new QLabel();
		memoryUsageText->setAlignment(Qt::AlignLeft);
		
		// editor area
		QHBoxLayout *editorAreaLayout = new QHBoxLayout;
		editorAreaLayout->setSpacing(0);
		editorAreaLayout->addWidget(breakpoints);
		editorAreaLayout->addWidget(linenumbers);
		editorAreaLayout->addWidget(editor);
		
		// keywords
		keywordsToolbar = new QToolBar();
		varButton = new QToolButton(); varButton->setText("var");
		ifButton = new QToolButton(); ifButton->setText("if");
		elseifButton = new QToolButton(); elseifButton->setText("elseif");
		elseButton = new QToolButton(); elseButton->setText("else");
		oneventButton = new QToolButton(); oneventButton->setText("onevent");
		whileButton = new QToolButton(); whileButton->setText("while");
		forButton = new QToolButton(); forButton->setText("for");
		subroutineButton = new QToolButton(); subroutineButton->setText("sub");
		callsubButton = new QToolButton(); callsubButton->setText("callsub");
		keywordsToolbar->addWidget(new QLabel(tr("<b>Keywords</b>")));
		keywordsToolbar->addSeparator();
		keywordsToolbar->addWidget(varButton);
		keywordsToolbar->addWidget(ifButton);
		keywordsToolbar->addWidget(elseifButton);
		keywordsToolbar->addWidget(elseButton);
		keywordsToolbar->addWidget(oneventButton);
		keywordsToolbar->addWidget(whileButton);
		keywordsToolbar->addWidget(forButton);
		keywordsToolbar->addWidget(subroutineButton);
		keywordsToolbar->addWidget(callsubButton);
		
		QVBoxLayout *editorLayout = new QVBoxLayout;
		editorLayout->addWidget(keywordsToolbar); // Jiwon
		editorLayout->addLayout(editorAreaLayout);
		editorLayout->addLayout(compilationResultLayout);
		editorLayout->addWidget(memoryUsageText);
		
		// panel
		
		// buttons
		executionModeLabel = new QLabel(tr("unknown"));
		
		loadButton = new QPushButton(QIcon(":/images/upload.png"), tr("Load"));
		resetButton = new QPushButton(QIcon(":/images/reset.png"), tr("Reset"));
		resetButton->setEnabled(false);
		runInterruptButton = new QPushButton(QIcon(":/images/play.png"), tr("Run"));
		runInterruptButton->setEnabled(false);
		nextButton = new QPushButton(QIcon(":/images/step.png"), tr("Next"));
		nextButton->setEnabled(false);
		refreshMemoryButton = new QPushButton(QIcon(":/images/rescan.png"), tr("refresh"));
		autoRefreshMemoryCheck = new QCheckBox(tr("auto"));
		
		QGridLayout* buttonsLayout = new QGridLayout;
		buttonsLayout->addWidget(new QLabel(tr("<b>Execution</b>")), 0, 0);
		buttonsLayout->addWidget(executionModeLabel, 0, 1);
		buttonsLayout->addWidget(loadButton, 1, 0);
		buttonsLayout->addWidget(runInterruptButton, 1, 1);
		buttonsLayout->addWidget(resetButton, 2, 0);
		buttonsLayout->addWidget(nextButton, 2, 1);
		
		// memory
		vmMemoryView = new QTreeView;
		vmMemoryView->setModel(vmMemoryModel);
		vmMemoryView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		vmMemoryView->setItemDelegate(new SpinBoxDelegate(-32768, 32767, this));
		vmMemoryView->setColumnWidth(0, 200-QFontMetrics(QFont()).width("-8888888##"));
		vmMemoryView->setColumnWidth(1, QFontMetrics(QFont()).width("-8888888##"));
		vmMemoryView->setSelectionMode(QAbstractItemView::SingleSelection);
		vmMemoryView->setSelectionBehavior(QAbstractItemView::SelectItems);
		vmMemoryView->setDragDropMode(QAbstractItemView::DragOnly);
		vmMemoryView->setDragEnabled(true);
		//vmMemoryView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		//vmMemoryView->setHeaderHidden(true);
		
		QVBoxLayout *memoryLayout = new QVBoxLayout;
		QHBoxLayout *memorySubLayout = new QHBoxLayout;
		memorySubLayout->addWidget(new QLabel(tr("<b>Variables</b>")));
		memorySubLayout->addStretch();
		memorySubLayout->addWidget(autoRefreshMemoryCheck);
		memorySubLayout->addWidget(refreshMemoryButton);
		memoryLayout->addLayout(memorySubLayout);
		memoryLayout->addWidget(vmMemoryView);
		memorySubLayout = new QHBoxLayout;
		QLabel* filterLabel(new QLabel(tr("F&ilter:")));
		memorySubLayout->addWidget(filterLabel);
		vmMemoryFilter= new QLineEdit;
		filterLabel->setBuddy(vmMemoryFilter);
		memorySubLayout->addWidget(vmMemoryFilter);
		memoryLayout->addLayout(memorySubLayout);
		
		// functions
		vmFunctionsView = new QTreeView;
		vmFunctionsView->setMinimumHeight(40);
		vmFunctionsView->setMinimumSize(QSize(50,40));
		vmFunctionsView->setModel(vmFunctionsModel);
		vmFunctionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		vmFunctionsView->setSelectionMode(QAbstractItemView::SingleSelection);
		vmFunctionsView->setSelectionBehavior(QAbstractItemView::SelectItems);
		vmFunctionsView->setDragDropMode(QAbstractItemView::DragOnly);
		vmFunctionsView->setDragEnabled(true);
		vmFunctionsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
		vmFunctionsView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
		#if QT_VERSION >= 0x040400
		vmFunctionsView->setHeaderHidden(true);
		#elif (!defined(_MSC_VER))
		#warning "Some feature have been disabled because you are using Qt < 4.4.0"
		#endif
		
		// local events
		vmLocalEvents = new DraggableListWidget;
		vmLocalEvents->setMinimumHeight(40);
		vmLocalEvents->setSelectionMode(QAbstractItemView::SingleSelection);
		vmLocalEvents->setDragDropMode(QAbstractItemView::DragOnly);
		vmLocalEvents->setDragEnabled(true);
		for (size_t i = 0; i < target->getDescription(id)->localEvents.size(); i++)
		{
			QListWidgetItem* item = new QListWidgetItem(QString::fromStdWString(target->getDescription(id)->localEvents[i].name));
			item->setToolTip(QString::fromStdWString(target->getDescription(id)->localEvents[i].description));
			vmLocalEvents->addItem(item);
		}
		
		// toolbox
		toolBox = new QToolBox;
		toolBox->addItem(vmFunctionsView, tr("Native Functions"));
		toolBox->addItem(vmLocalEvents, tr("Local Events"));
		QVBoxLayout* toolBoxLayout = new QVBoxLayout;
		toolBoxLayout->addWidget(toolBox);
		QWidget* toolBoxWidget = new QWidget;
		toolBoxWidget->setLayout(toolBoxLayout);
		
		// panel
		QSplitter *panelSplitter = new QSplitter(Qt::Vertical);
		
		QWidget* buttonsWidget = new QWidget;
		buttonsWidget->setLayout(buttonsLayout);
		panelSplitter->addWidget(buttonsWidget);
		panelSplitter->setCollapsible(0, false);
		
		QWidget* memoryWidget = new QWidget;
		memoryWidget->setLayout(memoryLayout);
		panelSplitter->addWidget(memoryWidget);
		panelSplitter->setStretchFactor(1, 3);
		
		panelSplitter->addWidget(toolBoxWidget);
		panelSplitter->setStretchFactor(2, 1);
		
		addWidget(panelSplitter);
		QWidget *editorWidget = new QWidget;
		editorWidget->setLayout(editorLayout);
		addWidget(editorWidget);
		setSizes(QList<int>() << 250 << 550);
	}
	
	void NodeTab::setupConnections()
	{
		// compiler
		connect(&compilationWatcher, SIGNAL(finished()), SLOT(compilationCompleted()));
		
		// execution
		connect(loadButton, SIGNAL(clicked()), SLOT(loadClicked()));
		connect(resetButton, SIGNAL(clicked()), SLOT(resetClicked()));
		connect(runInterruptButton, SIGNAL(clicked()), SLOT(runInterruptClicked()));
		connect(nextButton, SIGNAL(clicked()), SLOT(nextClicked()));
		connect(refreshMemoryButton, SIGNAL(clicked()), SLOT(refreshMemoryClicked()));
		connect(autoRefreshMemoryCheck, SIGNAL(stateChanged(int)), SLOT(autoRefreshMemoryClicked(int)));
		
		// memory
		connect(vmMemoryModel, SIGNAL(variableValuesChanged(unsigned, const VariablesDataVector &)), SLOT(setVariableValues(unsigned, const VariablesDataVector &)));
		connect(vmMemoryFilter, SIGNAL(textChanged(const QString &)), SLOT(updateHidden()));
		
		// editor
		connect(editor, SIGNAL(textChanged()), SLOT(editorContentChanged()));
		connect(editor, SIGNAL(cursorPositionChanged() ), SLOT(cursorMoved()));
		connect(editor, SIGNAL(breakpointSet(unsigned)), SLOT(setBreakpoint(unsigned)));
		connect(editor, SIGNAL(breakpointCleared(unsigned)), SLOT(clearBreakpoint(unsigned)));
		connect(editor, SIGNAL(breakpointClearedAll()), SLOT(breakpointClearedAll()));
		connect(editor, SIGNAL(refreshModelRequest(LocalContext)), SLOT(refreshCompleterModel(LocalContext)));
		
		connect(compilationResultImage, SIGNAL(clicked()), SLOT(goToError()));
		connect(compilationResultText, SIGNAL(clicked()), SLOT(goToError()));
						
		// keywords
		connect(varButton, SIGNAL(clicked()), SLOT(varButtonClicked()));
		connect(ifButton, SIGNAL(clicked()), SLOT(ifButtonClicked()));
		connect(elseifButton, SIGNAL(clicked()), SLOT(elseifButtonClicked()));
		connect(elseButton, SIGNAL(clicked()), SLOT(elseButtonClicked()));
		connect(oneventButton, SIGNAL(clicked()), SLOT(oneventButtonClicked()));
		connect(whileButton, SIGNAL(clicked()), SLOT(whileButtonClicked()));
		connect(forButton, SIGNAL(clicked()), SLOT(forButtonClicked()));
		connect(subroutineButton, SIGNAL(clicked()), SLOT(subroutineButtonClicked()));
		connect(callsubButton, SIGNAL(clicked()), SLOT(callsubButtonClicked()));		
		
		// following default settings
		if (mainWindow->autoMemoryRefresh)
			autoRefreshMemoryCheck->setChecked(true);
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
		
		isSynchronized = true;
		mainWindow->resetStatusText();
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
		// as we explicitely clicked, refresh all variables
		target->getVariables(id, 0, allocatedVariablesCount);
	}
	
	void NodeTab::autoRefreshMemoryClicked(int state)
	{
		if (state == Qt::Checked)
		{
			refreshMemoryButton->setEnabled(false);
			refreshTimer = startTimer(200); // 5 Hz
		}
		else
		{
			refreshMemoryButton->setEnabled(true);
			killTimer(refreshTimer);
			refreshMemoryClicked();
		}
	}
	
	void NodeTab::writeBytecode()
	{
		if (errorPos == -1)
		{
			loadClicked();
			target->writeBytecode(id);
		}
	}
	
	void NodeTab::reboot()
	{
		markTargetUnsynced();
		target->reboot(id);
	}
	
	static void write16(QIODevice& dev, const uint16 v)
	{
		dev.write((const char*)&v, 2);
	}
	
	static void write16(QIODevice& dev, const VariablesDataVector& data, const char *varName)
	{
		if (data.empty())
		{
			std::cerr << "Warning, cannot find " << varName << " required to save bytecode, using 0" << std::endl;
			write16(dev, 0);
		}
		else
			dev.write((const char *)&data[0], 2);
	}
	
	static uint16 crcXModem(const uint16 oldCrc, const QString& s)
	{
		return crcXModem(oldCrc, s.toStdWString());
	}

	void NodeTab::saveBytecode()
	{
		const QString& nodeName(target->getName(id));
		QString bytecodeFileName = QFileDialog::getSaveFileName(this, tr("Save the binary code of %0").arg(nodeName), "", "Aseba Binary Object (*.abo);;All Files (*)");
		
		QFile file(bytecodeFileName);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
			return;
		
		// See AS001 at https://aseba.wikidot.com/asebaspecifications
		
		// header
		const char* magic = "ABO";
		file.write(magic, 4);
		write16(file, 0); // binary format version
		write16(file, ASEBA_PROTOCOL_VERSION);
		write16(file, vmMemoryModel->getVariableValue("_productId"), "product identifier (_productId)");
		write16(file, vmMemoryModel->getVariableValue("_fwversion"), "firmware version (_fwversion)");
		write16(file, id);
		write16(file, crcXModem(0, nodeName));
		write16(file, target->getDescription(id)->crc());
		
		// bytecode
		write16(file, bytecode.size());
		uint16 crc(0);
		for (size_t i = 0; i < bytecode.size(); ++i)
		{
			const uint16 bc(bytecode[i]);
			write16(file, bc);
			crc = crcXModem(crc, bc);
		}
		write16(file, crc);
	}
	
	void NodeTab::setVariableValues(unsigned index, const VariablesDataVector &values)
	{
		target->setVariables(id, index, values);
	}
	
	void NodeTab::insertVariableName(const QModelIndex &index)
	{
		// only top level names have to be inserted
		if (!index.parent().isValid() && (index.column() == 0))
			editor->insertPlainText(index.data().toString());
	}
	
	void NodeTab::editorContentChanged()
	{
		if (rehighlighting)
		{
			rehighlighting = false;
		}
		else
		{
			QTextCursor cursor(editor->textCursor());
			if (cursor.atBlockEnd() && ConfigDialog::getAutoCompletion())// && !cursor.block().next().isValid())
			{
				// language completion
				const QString& line(cursor.block().text());
				const QString keyword(line.trimmed());
				
				QString prefix;
				QString postfix;
				if (keyword == "if")
				{
					const QString headSpace = line.left(line.indexOf("if"));
					prefix = " ";
					postfix = " then\n" + headSpace + "\t\n" + headSpace + "end";
				}
				else if (keyword == "when")
				{
					const QString headSpace = line.left(line.indexOf("when"));
					prefix = " ";
					postfix = " do\n" + headSpace + "\t\n" + headSpace + "end";
				}
				else if (keyword == "for")
				{
					const QString headSpace = line.left(line.indexOf("for"));
					prefix = " ";
					postfix = "i in 0:0 do\n" + headSpace + "\t\n" + headSpace + "end";
				}
				else if (keyword == "while")
				{
					const QString headSpace = line.left(line.indexOf("while"));
					prefix = " ";
					postfix = " do\n" + headSpace + "\t\n" + headSpace + "end";
				}
				
				if (!prefix.isNull() || !postfix.isNull())
				{
					cursor.beginEditBlock();
					cursor.insertText(prefix);
					const int pos = cursor.position();
					cursor.insertText(postfix);
					cursor.setPosition(pos);
					cursor.endEditBlock();
					editor->setTextCursor(cursor);
				}
				
				if (keyword == "elseif") // Jiwon
				{
					const QString tab = QString("\t");
					QString headSpace = line.left(line.indexOf("elseif"));

					if( headSpace.size() - tab.size() > 0 ) 
						headSpace = headSpace.left(headSpace.size() - tab.size());
					else
						headSpace.clear();
					
					postfix = " then\n" + headSpace + "\t";

					cursor.select(QTextCursor::BlockUnderCursor);
					cursor.removeSelectedText();
					
					cursor.beginEditBlock();
					cursor.insertText("\n" + headSpace + "elseif ");
					const int pos = cursor.position();
					cursor.insertText(postfix);
					cursor.setPosition(pos);
					cursor.endEditBlock();
					editor->setTextCursor(cursor);
				}
				//TODO
			}
			recompile();
			if (!firstCompilation)
				mainWindow->sourceChanged();
			else
				firstCompilation = false;
		}
	}

	
	void NodeTab::varButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());
		
		cursor.beginEditBlock();
		cursor.insertText("var ");
		cursor.endEditBlock();
	}
	
	void NodeTab::ifButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());

		cursor.beginEditBlock();
		cursor.insertText("if");
		cursor.endEditBlock();
		editorContentChanged();
	}
	
	void NodeTab::elseifButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());

		cursor.beginEditBlock();
		cursor.insertText("elseif");
		cursor.endEditBlock();
		editorContentChanged();
	}
	
	void NodeTab::elseButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());

		cursor.beginEditBlock();
		cursor.insertText("else\n\t");
		const int pos = cursor.position();
		cursor.insertText("\n");
		cursor.setPosition(pos);
		cursor.endEditBlock();
		editor->setTextCursor(cursor);
	}

	void NodeTab::oneventButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());

		cursor.beginEditBlock();
		cursor.insertText("onevent ");
		cursor.endEditBlock();
	}
	
	void NodeTab::whileButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());
		
		cursor.beginEditBlock();
		cursor.insertText("while");
		cursor.endEditBlock();
		editorContentChanged();
	}
	
	void NodeTab::forButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());
		
		cursor.beginEditBlock();
		cursor.insertText("for");
		cursor.endEditBlock();
		editorContentChanged();
	}
	
	void NodeTab::subroutineButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());

		cursor.beginEditBlock();
		cursor.insertText("sub ");
		cursor.endEditBlock();
	}	

	void NodeTab::callsubButtonClicked()
	{
		QTextCursor cursor(editor->textCursor());

		cursor.beginEditBlock();
		cursor.insertText("callsub ");
		cursor.endEditBlock();
	}	

	void NodeTab::displayCode(QList<QString> code)
	{
		editor->clear();
		
		editor->textCursor();
		for(int i=0; i<code.size(); i++)
			editor->insertPlainText(code[i]);
	}

	void NodeTab::showKeywords(bool show)
	{
		if(show)
			keywordsToolbar->show();
		else
			keywordsToolbar->hide();
	}

	void NodeTab::showMemoryUsage(bool show)
	{
		memoryUsageText->setVisible(show);
	}

	void NodeTab::updateHidden() 
	{
		const QString& filterString(vmMemoryFilter->text());
		// Quick hack to hide hidden variable in the treeview and not in vmMemoryModel
		for(int i = 0; i < vmMemoryModel->rowCount(QModelIndex()); i++) 
		{
			QString name(vmMemoryModel->data(vmMemoryModel->index(i,0), Qt::DisplayRole).toString());
			bool hidden(false);
			if (
				(!showHidden && (name.at(0) == '_' || name.contains(QString("._")))) ||
				(!filterString.isEmpty() && !name.contains(filterString))
			)
				hidden = true;
			vmMemoryView->setRowHidden(i,QModelIndex(), hidden);
		}

	}
	
	NodeTab::CompilationResult* compilationThread(const TargetDescription targetDescription, const CommonDefinitions commonDefinitions, QString source, bool dump)
	{
		NodeTab::CompilationResult* result(new NodeTab::CompilationResult(dump));
		
		Compiler compiler;
		compiler.setTargetDescription(&targetDescription);
		compiler.setCommonDefinitions(&commonDefinitions);
		compiler.setTranslateCallback(CompilerTranslator::translate);
		
		std::wistringstream is(source.toStdWString());
		
		if (dump)
			result->success = compiler.compile(is, result->bytecode, result->allocatedVariablesCount, result->error, &result->compilationMessages);
		else
			result->success = compiler.compile(is, result->bytecode, result->allocatedVariablesCount, result->error);
		
		if (result->success)	
			result->variablesMap = *compiler.getVariablesMap();
		
		return result;
	}
	
	void NodeTab::recompile()
	{
		// compile
		if (compilationFuture.isRunning())
			compilationDirty = true;
		else
		{
			bool dump(mainWindow->nodes->currentWidget() == this);
			compilationFuture = QtConcurrent::run(compilationThread, *target->getDescription(id), *commonDefinitions, editor->toPlainText(), dump);
			compilationWatcher.setFuture(compilationFuture);
			compilationDirty = false;
			
			// show progress icon
			compilationResultImage->setPixmap(QPixmap(QString(":/images/busy.png")));
		}
	}
	
	void NodeTab::compilationCompleted()
	{
		CompilationResult* result(compilationFuture.result());
		assert(result);
		
		// as long as result is dirty, continue compilation
		if (compilationDirty)
		{
			delete result;
			recompile();
			return;
		}
		
		// process results
		processCompilationResult(result);
	}
	
	void NodeTab::processCompilationResult(CompilationResult* result)
	{
		// clear old user data
		// doRehighlight is required to prevent infinite recursion because there are no slot
		// to differentiate user changes from highlight changes in documents
		bool doRehighlight = clearEditorProperty("errorPos");
		
		if (result->dump)
		{
			mainWindow->compilationMessageBox->setWindowTitle(
				tr("Aseba Studio: Output of last compilation for %0").arg(target->getName(id))
			);
			
			if (result->success)
				mainWindow->compilationMessageBox->setText(
					tr("Compilation success.") + QString("\n\n") + 
					QString::fromStdWString(result->compilationMessages.str())
				);
			else 	
				mainWindow->compilationMessageBox->setText(
					QString::fromStdWString(result->error.toWString()) + ".\n\n" +
					QString::fromStdWString(result->compilationMessages.str())
				);
				
		}

		// show memory usage
		if (result->success)
		{
			const unsigned variableCount = result->allocatedVariablesCount;
			const unsigned variableTotal = (*target->getDescription(id)).variablesSize;
			const unsigned bytecodeCount = result->bytecode.size();
			const unsigned bytecodeTotal = (*target->getDescription(id)).bytecodeSize;
			assert(variableCount);
			assert(bytecodeCount);
			const QString variableText = tr("variables: %1 on %2 (%3\%)").arg(variableCount).arg(variableTotal).arg((double)variableCount*100./variableTotal, 0, 'f', 1);
			const QString bytecodeText = tr("bytecode: %1 on %2 (%3\%)").arg(bytecodeCount).arg(bytecodeTotal).arg((double)bytecodeCount*100./bytecodeTotal, 0, 'f', 1);
			memoryUsageText->setText(trUtf8("<b>Memory usage</b> : %1, %2").arg(variableText).arg(bytecodeText));
		}
		
		// update state following result
		if (result->success)
		{
			bytecode = result->bytecode;
			allocatedVariablesCount = result->allocatedVariablesCount;
			vmMemoryModel->updateVariablesStructure(&result->variablesMap);
			
			updateHidden();
			compilationResultText->setText(tr("Compilation success."));
			compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
			loadButton->setEnabled(true);
			emit uploadReadynessChanged(true);
			
			errorPos = -1;
		}
		else
		{
			compilationResultText->setText(QString::fromStdWString(result->error.toWString()));
			compilationResultImage->setPixmap(QPixmap(QString(":/images/warning.png")));
			loadButton->setEnabled(false);
			emit uploadReadynessChanged(false);
			
			// we have an error, set the correct user data
			if (result->error.pos.valid)
			{
				errorPos = result->error.pos.character;
				QTextBlock textBlock = editor->document()->findBlock(errorPos);
				int posInBlock = errorPos - textBlock.position();
				if (textBlock.userData())
					polymorphic_downcast<AeslEditorUserData *>(textBlock.userData())->properties["errorPos"] = posInBlock;
				else
					textBlock.setUserData(new AeslEditorUserData("errorPos", posInBlock));
				doRehighlight = true;
			}
		}
		
		// we have finished with the results
		delete result;
		
		// clear bearkpoints of target if currently in debugging mode
		if (editor->debugging)
		{
			//target->stop(id);
			markTargetUnsynced();
			doRehighlight = true;
			
		}
		
		if (doRehighlight)
			rehighlight();
	}
	
	//! When code is changed or target is rebooted, remove breakpoints from target but keep them locally as pending for next code load
	void NodeTab::markTargetUnsynced()
	{
		editor->debugging = false;
		resetButton->setEnabled(false);
		runInterruptButton->setEnabled(false);
		nextButton->setEnabled(false);
		target->clearBreakpoints(id);
		switchEditorProperty("breakpoint", "breakpointPending");
		executionModeLabel->setText(tr("unknown"));
		mainWindow->nodes->setExecutionMode(mainWindow->getIndexFromId(id), Target::EXECUTION_UNKNOWN);
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
	
	void NodeTab::refreshCompleterModel(LocalContext context)
	{
//		qDebug() << "New context: " << context;
		disconnect(mainWindow->eventsDescriptionsModel, 0, sortingProxy, 0);

		if ((context == GeneralContext) || (context == UnknownContext))
		{
			sortingProxy->setSourceModel(variableAggregator);
			sortingProxy->sort(0);
			editor->setCompleterModel(sortingProxy);	// both variables and constants
		}
		else if (context == LeftValueContext)
		{
			sortingProxy->setSourceModel(vmMemoryModel);
			sortingProxy->sort(0);
			editor->setCompleterModel(sortingProxy);	// only variables
		}
		else if (context == VarDefContext)
			editor->setCompleterModel(0);		// disable auto-completion in this case
		else if (context == FunctionContext)
		{
			sortingProxy->setSourceModel(functionsFlatModel);
			sortingProxy->sort(0);
			editor->setCompleterModel(sortingProxy);	// native functions
		}
		else if (context == EventContext)
		{
			sortingProxy->setSourceModel(eventAggregator);
			sortingProxy->sort(0);
			//connect(mainWindow->eventsDescriptionsModel, SIGNAL(publicRowsInserted()), SLOT(sortCompleterModel()));
			//connect(mainWindow->eventsDescriptionsModel, SIGNAL(publicRowsRemoved()), SLOT(sortCompleterModel()));
			editor->setCompleterModel(sortingProxy);	// both local and global events
		}
	}
/*
	void NodeTab::sortCompleterModel()
	{
		sortingProxy->sort(0);
		editor->setCompleterModel(0);
		editor->setCompleterModel(sortingProxy);
	}
*/
	void NodeTab::variablesMemoryChanged(unsigned start, const VariablesDataVector &variables)
	{
		// update memory view
		qDebug() << "received variables 2";
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
		currentPC = line;
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

		// Filter spurious messages, to detect a stop at a breakpoint
		if (previousMode != mode)
		{
			previousMode = mode;
			if ((mode == Target::EXECUTION_STEP_BY_STEP) && editor->isBreakpoint(currentPC))
			{
				// we are at a breakpoint
				if (mainWindow->currentScriptTab != this)
				{
					// not the current tab -> hidden tab -> highlight me in red
					mainWindow->nodes->highlightTab(mainWindow->getIndexFromId(id), Qt::red);
				}
				// go to this line
				editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC)));
				editor->ensureCursorVisible();
			}
		}
		
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

			// go to this line and next line is visible
			editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC+1)));
			editor->ensureCursorVisible();
			editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC)));
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

		// set the tab icon to show the current execution mode
		mainWindow->nodes->setExecutionMode(mainWindow->getIndexFromId(id), mode);
	}
	
	void NodeTab::breakpointSetResult(unsigned line, bool success)
	{
		clearEditorProperty("breakpointPending", line);
		if (success)
			setEditorProperty("breakpoint", QVariant(), line);
		rehighlight();
	}
	
	void NodeTab::closePlugins()
	{
		for (NodeToolInterfaces::const_iterator it(tools.begin()); it != tools.end(); ++it)
			(*it)->closeAsSoonAsPossible();
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
			AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
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
			AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
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
				AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
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
			AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
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
			AeslEditorUserData *uData = polymorphic_downcast_or_null<AeslEditorUserData *>(block.userData());
			if (uData && uData->properties.contains(oldProperty))
			{
				uData->properties.remove(oldProperty);
				uData->properties[newProperty] = QVariant();
			}
			block = block.next();
		}
	}

	NewNamedValueDialog::NewNamedValueDialog(QString *name, int *value, int min, int max) :
		name(name),
		value(value)
	{
		// create the widgets
		label1 = new QLabel(tr("Name", "Name of the named value (can be a constant, event,...)"));
		line1 = new QLineEdit(*name);
		label2 = new QLabel(tr("Default description", "When no description is given for the named value"));
		line2 = new QSpinBox();
		line2->setRange(min, max);
		line2->setValue(*value);
		QLabel* lineHelp = new QLabel(QString("(%1 ... %2)").arg(min).arg(max));
		QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

		// create the layout
		QVBoxLayout* mainLayout = new QVBoxLayout();
		mainLayout->addWidget(label1);
		mainLayout->addWidget(line1);
		mainLayout->addWidget(label2);
		mainLayout->addWidget(line2);
		mainLayout->addWidget(lineHelp);
		mainLayout->addWidget(buttonBox);
		setLayout(mainLayout);

		// set modal
		setModal(true);

		// connections
		connect(buttonBox, SIGNAL(accepted()), this, SLOT(okSlot()));
		connect(buttonBox, SIGNAL(rejected()), this, SLOT(cancelSlot()));
	}

	bool NewNamedValueDialog::getNamedValue(QString *name, int *value, int min, int max, QString title, QString valueName, QString valueDescription)
	{
		NewNamedValueDialog dialog(name, value, min, max);
		dialog.setWindowTitle(title);
		dialog.label1->setText(valueName);
		dialog.label2->setText(valueDescription);
		dialog.resize(500, 0);		// make it wide enough

		int ret = dialog.exec();

		if (ret)
			return true;
		else
			return false;
	}

	void NewNamedValueDialog::okSlot()
	{
		*name = line1->text();
		*value = line2->value();
		accept();
	}

	void NewNamedValueDialog::cancelSlot()
	{
		*name = "";
		*value = -1;
		reject();
	}

	MainWindow::MainWindow(QVector<QTranslator*> translators, const QString& commandLineTarget, bool autoRefresh, QWidget *parent) :
		QMainWindow(parent),
		getDescriptionTimer(0),
		sourceModified(false),
		autoMemoryRefresh(autoRefresh)
	{
		// create target
		target = new DashelTarget(translators, commandLineTarget);

		// create models
		eventsDescriptionsModel = new MaskableNamedValuesVectorModel(&commonDefinitions.events, tr("Event number %0"), this);
		eventsDescriptionsModel->setExtraMimeType("application/aseba-events");
		constantsDefinitionsModel = new NamedValuesVectorModel(&commonDefinitions.constants, this);
		constantsDefinitionsModel->setExtraMimeType("application/aseba-constants");
		constantsDefinitionsModel->setEditable(true);
		
		// create config dialog + read settings on-disk
		ConfigDialog::init(this);

		// create gui
		setupWidgets();
		setupMenu();
		setupConnections();
		
		// cosmetic fix-up
		updateWindowTitle();
		if (readSettings() == false)
			resize(1000,700);
	}
	
	MainWindow::~MainWindow()
	{
		#ifdef HAVE_QWT
		for (EventViewers::iterator it = eventsViewers.begin(); it != eventsViewers.end(); ++it)
		{
			it.value()->detachFromMain();
		}
		#endif // HAVE_QWT
		
		delete target;
	}
	
	void MainWindow::about()
	{
		QString text = tr("<p>Aseba version informations:</p>" \
						"<ul><li>Aseba ver. %0"\
						"<br/>(build ver. %1/protocol ver. %2)" \
						"</li><li>Dashel ver. %3"\
						"</li></ul>" \
						"<p>(c) 2006-2012 <a href=\"http://stephane.magnenat.net\">Stéphane Magnenat</a> and other contributors.</p>" \
						"<p><a href=\"%5\">%5</a></p>" \
						"<p>Aseba is open-source licensed under the LGPL version 3.</p>");
		
		text = text.
			arg(ASEBA_VERSION).
			arg(ASEBA_BUILD_VERSION).
			arg(ASEBA_PROTOCOL_VERSION).
			arg(DASHEL_VERSION).
			arg(tr("http://aseba.wikidot.com/en:start"));
		
		QMessageBox::about(this, tr("About Aseba Studio"), text);
	}
	
	void MainWindow::newFile()
	{
		if (askUserBeforeDiscarding())
		{
			clearDocumentSpecificTabs();
			// we must only have NodeTab* left.
			for (int i = 0; i < nodes->count(); i++)
			{
				NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
				Q_ASSERT(tab);
				tab->editor->clear();
			}
			constantsDefinitionsModel->clear();
			eventsDescriptionsModel->clear();
			actualFileName.clear();
		}
	}
	
	void MainWindow::openFile(const QString &path)
	{
		// make sure we do not loose changes
		if (askUserBeforeDiscarding() == false)
			return;
		
		QString fileName = path;
	
		if (fileName.isEmpty())
			fileName = QFileDialog::getOpenFileName(this,
				tr("Open Script"), "", "Aseba scripts (*.aesl)");
		
		QFile file(fileName);
		if (!file.open(QFile::ReadOnly))
			return;
		
		QDomDocument document("aesl-source");
		QString errorMsg;
		int errorLine;
		int errorColumn;
		if (document.setContent(&file, false, &errorMsg, &errorLine, &errorColumn))
		{
			eventsDescriptionsModel->clear();
			constantsDefinitionsModel->clear();
			
			int noNodeCount = 0;
			clearDocumentSpecificTabs();
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
						{
							nodes->addTab(new AbsentNodeTab(0, element.attribute("name"), element.firstChild().toText().data()), element.attribute("name") + tr(" (not available)"));
							noNodeCount++;
						}
					}
					else if (element.tagName() == "event")
					{
						const QString eventName(element.attribute("name"));
						const unsigned eventSize(element.attribute("size").toUInt());
						eventsDescriptionsModel->addNamedValue(NamedValue(eventName.toStdWString(), std::min(unsigned(ASEBA_MAX_EVENT_ARG_SIZE), eventSize)));
					}
					else if (element.tagName() == "constant")
					{
						constantsDefinitionsModel->addNamedValue(NamedValue(element.attribute("name").toStdWString(), element.attribute("value").toInt()));
					}
					else if (element.tagName() == "keywords") // Jiwon
					{
						if( element.attribute("flag") == "true" )
							showKeywordsAct->setChecked(true);
						else
							showKeywordsAct->setChecked(false);
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
			
			recompileAll();
		
			sourceModified = false;
			constantsDefinitionsModel->clearWasModified();
			eventsDescriptionsModel->clearWasModified();
			updateWindowTitle();
		}
		else
		{
			QMessageBox::warning(this,
				tr("Loading"),
				tr("Error in XML source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn)
			);
		}
		
		file.close();
	}
	
	void MainWindow::openRecentFile()
	{
		QAction* entry = polymorphic_downcast<QAction*>(sender());
		openFile(entry->text());
	}
	
	bool MainWindow::save()
	{
		return saveFile(actualFileName);
	}
	
	bool MainWindow::saveFile(const QString &previousFileName)
	{
		QString fileName = previousFileName;
		
		if (fileName.isEmpty())
			fileName = QFileDialog::getSaveFileName(this,
				tr("Save Script"), actualFileName, "Aseba scripts (*.aesl)");
		
		if (fileName.isEmpty())
			return false;
		
		if (fileName.lastIndexOf(".") < 0)
			fileName += ".aesl";
		
		QFile file(fileName);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
			return false;
		
		actualFileName = fileName;
		updateRecentFiles(fileName);
		
		// initiate DOM tree
		QDomDocument document("aesl-source");
		QDomElement root = document.createElement("network");
		document.appendChild(root);
		
		root.appendChild(document.createTextNode("\n\n\n"));
		root.appendChild(document.createComment("list of global events"));		
		
		// events
		for (size_t i = 0; i < commonDefinitions.events.size(); i++)
		{
			QDomElement element = document.createElement("event");
			element.setAttribute("name", QString::fromStdWString(commonDefinitions.events[i].name));
			element.setAttribute("size", QString::number(commonDefinitions.events[i].value));
			root.appendChild(element);
		}
		
		root.appendChild(document.createTextNode("\n\n\n"));
		root.appendChild(document.createComment("list of constants"));
		
		// constants
		for (size_t i = 0; i < commonDefinitions.constants.size(); i++)
		{
			QDomElement element = document.createElement("constant");
			element.setAttribute("name", QString::fromStdWString(commonDefinitions.constants[i].name));
			element.setAttribute("value", QString::number(commonDefinitions.constants[i].value));
			root.appendChild(element);
		}
		
		// keywords Jiwon
		root.appendChild(document.createTextNode("\n\n\n"));
		root.appendChild(document.createComment("show keywords state"));
		
		QDomElement keywords = document.createElement("keywords"); 
		if( showKeywordsAct->isChecked() ) 
			keywords.setAttribute("flag", "true");
		else
			keywords.setAttribute("flag", "false");
		root.appendChild(keywords);
		
		// source code
		for (int i = 0; i < nodes->count(); i++)
		{
			const ScriptTab* tab = dynamic_cast<const ScriptTab*>(nodes->widget(i));
			if (tab)
			{
				QString nodeName;
				
				const NodeTab* nodeTab = dynamic_cast<const NodeTab*>(tab);
				if (nodeTab)
					nodeName = target->getName(nodeTab->nodeId());
				
				const AbsentNodeTab* absentNodeTab = dynamic_cast<const AbsentNodeTab*>(tab);
				if (absentNodeTab)
					nodeName = absentNodeTab->name;
				
				const QString& nodeContent = tab->editor->toPlainText();
				
				root.appendChild(document.createTextNode("\n\n\n"));
				root.appendChild(document.createComment(QString("source code of node %0").arg(nodeName)));
				
				QDomElement element = document.createElement("node");
				element.setAttribute("name", nodeName);
				QDomText text = document.createTextNode(nodeContent);
				element.appendChild(text);
				root.appendChild(element);
			}
		}
		root.appendChild(document.createTextNode("\n\n\n"));
		
		QTextStream out(&file);
		document.save(out, 0);
		
		sourceModified = false;
		constantsDefinitionsModel->clearWasModified();
		eventsDescriptionsModel->clearWasModified();
		updateWindowTitle();

		return true;
	}
	
	void MainWindow::exportMemoriesContent()
	{
		QString exportFileName = QFileDialog::getSaveFileName(this, tr("Export memory content"), "", "All Files (*);;CSV files (*.csv);;Text files (*.txt)");
		
		QFile file(exportFileName);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
			return;
		
		QTextStream out(&file);
		
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
			{
				const QString nodeName(target->getName(tab->nodeId()));
				const QList<TargetVariablesModel::Variable>& variables(tab->vmMemoryModel->getVariables());
				
				for (int j = 0; j < variables.size(); ++j)
				{
					const TargetVariablesModel::Variable& variable(variables[j]);
					out << nodeName << "." << variable.name << " ";
					for (size_t k = 0; k < variable.value.size(); ++k)
					{
						out << variable.value[k] << " ";
					}
					out << "\n";
				}
			}
		}
	}
	
	void MainWindow::importMemoriesContent()
	{
		QString importFileName = QFileDialog::getOpenFileName(this, tr("Import memory content"), "", "All Files (*);;CSV files (*.csv);;Text files (*.txt)");
		
		QFile file(importFileName);
		if (!file.open(QFile::ReadOnly))
			return;
		
		QTextStream in(&file);
		
		QSet<QString> nodesNotFound;
		QStringList variablesNotFound;
		
		while (!in.atEnd())
		{
			QString line(in.readLine());
			int pointPos(line.indexOf('.'));
			QString nodeName(line.left(pointPos));
			NodeTab* tab(getTabFromName(nodeName));
			if (tab)
			{
				int endVarNamePos(line.indexOf(' ', pointPos+1));
				if (endVarNamePos != -1)
				{
					QString variableName(line.mid(pointPos+1, endVarNamePos-pointPos-1));
					VariablesDataVector values;
					int index(endVarNamePos);
					while (index != -1)
					{
						int nextIndex(line.indexOf(' ', index+1));
						QString value(line.mid(index+1, nextIndex - index - 1));
						if (value.isEmpty())
							break;
						values.push_back(value.toShort());
						index = nextIndex;
					}
					if (!tab->vmMemoryModel->setVariableValues(variableName, values))
					{
						variablesNotFound << tr("%0 on node %1").arg(variableName).arg(nodeName);
					}
				}
			}
			else
				nodesNotFound.insert(nodeName);
		}
		
		if (!nodesNotFound.isEmpty() || !variablesNotFound.isEmpty())
		{
			QString msg;
			if (!nodesNotFound.isEmpty())
			{
				msg += tr("The following nodes are not present in the current network and their associated content was not imported:\n");
				foreach (QString value, nodesNotFound)
					msg += "• " + value + "\n";
			}
			if (!variablesNotFound.isEmpty())
			{
				msg += tr("The following variables are not present in the current network and their associated content was not imported:\n");
				foreach (QString value, variablesNotFound)
					msg += "• " + value + "\n";
			}
			QMessageBox::warning(this,
				tr("Some content was not imported"),
				msg
			);
		}
	}
	
	void MainWindow::copyAll()
	{
		QString toCopy;
		for (int i = 0; i < nodes->count(); i++)
		{
			const NodeTab* nodeTab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (nodeTab)
			{
				toCopy += QString("# node %0\n").arg(target->getName(nodeTab->nodeId()));
				toCopy += nodeTab->editor->toPlainText();
				toCopy += "\n\n";
			}
			const AbsentNodeTab* absentNodeTab = dynamic_cast<AbsentNodeTab*>(nodes->widget(i));
			if (absentNodeTab)
			{
				toCopy += QString("# abscent node named %0\n").arg(absentNodeTab->name);
				toCopy += absentNodeTab->editor->toPlainText();
				toCopy += "\n\n";
			}
		}
		 QApplication::clipboard()->setText(toCopy);
	}
	
	void MainWindow::findTriggered()
	{
		ScriptTab* tab = dynamic_cast<ScriptTab*>(nodes->currentWidget());
		if (tab && tab->editor->textCursor().hasSelection())
			findDialog->setFindText(tab->editor->textCursor().selectedText());
		findDialog->replaceGroupBox->setChecked(false);
		findDialog->show();
	}
	
	void MainWindow::replaceTriggered()
	{
		findDialog->replaceGroupBox->setChecked(true);
		findDialog->show();
	}

	void MainWindow::commentTriggered()
	{
		assert(currentScriptTab);
		currentScriptTab->editor->commentAndUncommentSelection(AeslEditor::CommentSelection);
	}

	void MainWindow::uncommentTriggered()
	{
		assert(currentScriptTab);
		currentScriptTab->editor->commentAndUncommentSelection(AeslEditor::UncommentSelection);
	}

	void MainWindow::showLineNumbersChanged(bool state)
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = polymorphic_downcast<NodeTab*>(nodes->widget(i));
			Q_ASSERT(tab);
			tab->linenumbers->showLineNumbers(state);
		}
		ConfigDialog::setShowLineNumbers(state);
	}
	
	void MainWindow::goToLine()
	{
		assert(currentScriptTab);
		QTextEdit* editor(currentScriptTab->editor);
		const QTextDocument* document(editor->document());
		QTextCursor cursor(editor->textCursor());
		bool ok;
		const int curLine = cursor.blockNumber() + 1;
		const int minLine = 1;
		const int maxLine = document->lineCount();
		const int line = QInputDialog::getInt(
			this, tr("Go To Line"), tr("Line:"), curLine, minLine, maxLine, 1, &ok);
		if (ok)
			editor->setTextCursor(QTextCursor(document->findBlockByLineNumber(line-1)));
	}

	void MainWindow::showSettings()
	{
		ConfigDialog::showConfig();
	}

	void MainWindow::toggleBreakpoint()
	{
		assert(currentScriptTab);
		currentScriptTab->editor->toggleBreakpoint();
	}

	void MainWindow::clearAllBreakpoints()
	{
		assert(currentScriptTab);
		currentScriptTab->editor->clearAllBreakpoints();
	}
	
	void MainWindow::resetAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->resetClicked();
		}
	}
	
	void MainWindow::loadAll()
	{	
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->loadClicked();
		}
	}
	
	void MainWindow::runAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				target->run(tab->nodeId());
		}
	}
	
	void MainWindow::pauseAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				target->pause(tab->nodeId());
		}
	}
	
	void MainWindow::stopAll()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				target->stop(tab->nodeId());
		}
	}

	void MainWindow::showHidden(bool show) 
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
			{
				tab->vmFunctionsModel->recreateTreeFromDescription(show);
				tab->showHidden = show;
				tab->updateHidden();
			}
		}
		ConfigDialog::setShowHidden(show);
	}

	void MainWindow::showKeywords(bool show) // Jiwon
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->showKeywords(show);
		}
		ConfigDialog::setShowKeywordToolbar(show);
	}

	void MainWindow::clearAllExecutionError()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->clearExecutionErrors();
		}
		logger->setStyleSheet("");
	}
	
	void MainWindow::uploadReadynessChanged()
	{
		bool ready = true;
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
			{
				if (!tab->loadButton->isEnabled())
				{
					ready = false;
					break;
				}
			}
		}
		
		loadAllAct->setEnabled(ready);
		writeAllBytecodesAct->setEnabled(ready);
	}
	
	void MainWindow::sendEvent()
	{
		QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
		Q_ASSERT(currentRow.isValid());
		
		const unsigned eventId = currentRow.row();
		const QString eventName = QString::fromStdWString(commonDefinitions.events[eventId].name);
		const int argsCount = commonDefinitions.events[eventId].value;
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
		userEvent(eventId, data);
	}
	
	void MainWindow::sendEventIf(const QModelIndex &index)
	{
		if (index.column() == 0)
			sendEvent();
	}
	
	void MainWindow::toggleEventVisibleButton(const QModelIndex &index)
	{
		if (index.column() == 2)
			eventsDescriptionsModel->toggle(index);
	}
		
	void MainWindow::plotEvent()
	{
		#ifdef HAVE_QWT
		QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
		Q_ASSERT(currentRow.isValid());
		const unsigned eventId = currentRow.row();
		plotEvent(eventId);
		#endif // HAVE_QWT
	}
	
	void MainWindow::eventContextMenuRequested(const QPoint & pos)
	{
		#ifdef HAVE_QWT
		const QModelIndex index(eventsDescriptionsView->indexAt(pos));
		if (index.isValid() && (index.column() == 0))
		{
			const QString eventName(eventsDescriptionsModel->data(index).toString());
			QMenu menu;
			menu.addAction(tr("Plot event %1").arg(eventName));
			const QAction* ret = menu.exec(eventsDescriptionsView->mapToGlobal(pos));
			if (ret)
			{
				const unsigned eventId = index.row();
				plotEvent(eventId);
			}
		}
		#endif // HAVE_QWT
	}
	
	void MainWindow::plotEvent(const unsigned eventId)
	{
		#ifdef HAVE_QWT
		const unsigned eventVariablesCount(eventsDescriptionsModel->data(eventsDescriptionsModel->index(eventId, 1)).toUInt());
		const QString eventName(eventsDescriptionsModel->data(eventsDescriptionsModel->index(eventId, 0)).toString());
		const QString tabTitle(tr("plot of %1").arg(eventName));
		nodes->addTab(new EventViewer(eventId, eventName, eventVariablesCount, &eventsViewers), tabTitle, true);
		#endif // HAVE_QWT
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
			tab->editor->setTextCursor(QTextCursor(tab->editor->document()->findBlockByLineNumber(line)));
			tab->editor->setFocus();
		}
	}
	
	void MainWindow::tabChanged(int index)
	{
		// remove old connections, if any
		if (currentScriptTab)
		{
			disconnect(cutAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(cut()));
			disconnect(copyAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(copy()));
			disconnect(pasteAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(paste()));
			disconnect(undoAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(undo()));
			disconnect(redoAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(redo()));
			
			disconnect(currentScriptTab->editor, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
			disconnect(currentScriptTab->editor, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
			disconnect(currentScriptTab->editor, SIGNAL(undoAvailable(bool)), undoAct, SLOT(setEnabled(bool)));
			disconnect(currentScriptTab->editor, SIGNAL(redoAvailable(bool)), redoAct, SLOT(setEnabled(bool)));
			
			pasteAct->setEnabled(false);
			findDialog->hide();
			findDialog->editor = 0;
			findAct->setEnabled(false);
			replaceAct->setEnabled(false);
			goToLineAct->setEnabled(false);
		}
		
		// reconnect to new
		if (index >= 0)
		{
			ScriptTab *tab = dynamic_cast<ScriptTab*>(nodes->widget(index));
			if (tab)
			{
				connect(copyAct, SIGNAL(triggered()), tab->editor, SLOT(copy()));
				connect(tab->editor, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
				
				findDialog->editor = tab->editor;
				findAct->setEnabled(true);
				goToLineAct->setEnabled(true);
				
				NodeTab *nodeTab = dynamic_cast<NodeTab*>(tab);
				if (nodeTab)
				{
					connect(cutAct, SIGNAL(triggered()), tab->editor, SLOT(cut()));
					connect(pasteAct, SIGNAL(triggered()), tab->editor, SLOT(paste()));
					connect(undoAct, SIGNAL(triggered()), tab->editor, SLOT(undo()));
					connect(redoAct, SIGNAL(triggered()), tab->editor, SLOT(redo()));
					
					connect(tab->editor, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
					connect(tab->editor, SIGNAL(undoAvailable(bool)), undoAct, SLOT(setEnabled(bool)));
					connect(tab->editor, SIGNAL(redoAvailable(bool)), redoAct, SLOT(setEnabled(bool)));
					
					if (compilationMessageBox->isVisible())
						nodeTab->recompile();
					
					// because this is a new tab, get content of variables
					target->getVariables(nodeTab->id, 0, nodeTab->allocatedVariablesCount);
					
					showCompilationMsg->setEnabled(true);
					findDialog->replaceGroupBox->setEnabled(true);
					// paste and replace are only available when the editor is in read/write mode
					pasteAct->setEnabled(true);
					replaceAct->setEnabled(true);
				}
				else
				{
					showCompilationMsg->setEnabled(false);
					findDialog->replaceGroupBox->setEnabled(false);
				}
				
				// TODO: it would be nice to find a way to setup this correctly
				cutAct->setEnabled(false);
				copyAct->setEnabled(false);
				undoAct->setEnabled(false);
				redoAct->setEnabled(false);
				
				currentScriptTab = tab;
			}
			else
				currentScriptTab = 0;
		}
		else
			currentScriptTab = 0;
	}
	
	void MainWindow::showCompilationMessages(bool doShow)
	{
		// this slot shouldn't be callable when an unactive tab is show
		compilationMessageBox->setVisible(doShow);
		if (nodes->currentWidget())
			polymorphic_downcast<NodeTab *>(nodes->currentWidget())->recompile();
	}
	
	void MainWindow::compilationMessagesWasHidden()
	{
		showCompilationMsg->setChecked(false);
	}

	void MainWindow::showMemoryUsage(bool show)
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->showMemoryUsage(show);
		}
		ConfigDialog::setShowMemoryUsage(show);
	}
	
	void MainWindow::addEventNameClicked()
	{
		QString eventName;
		int eventNbArgs = 0;

		// prompt the user for the named value
		const bool ok = NewNamedValueDialog::getNamedValue(&eventName, &eventNbArgs, 0, 32767, tr("Add a new event"), tr("Name:"), tr("Number of arguments", "For the newly created event"));

		eventName = eventName.trimmed();
		if (ok && !eventName.isEmpty())
		{
			if (commonDefinitions.events.contains(eventName.toStdWString()))
			{
				QMessageBox::warning(this, tr("Event already exists"), tr("Event %0 already exists.").arg(eventName));
			}
			else if (!QRegExp("\\w(\\w|\\.)*").exactMatch(eventName) || eventName[0].isDigit())
			{
				QMessageBox::warning(this, tr("Invalid event name"), tr("Event %0 has an invalid name. Valid names start with an alphabetical character or an \"_\", and continue with any number of alphanumeric characters, \"_\" and \".\"").arg(eventName));
			}
			else
			{
				eventsDescriptionsModel->addNamedValue(NamedValue(eventName.toStdWString(), eventNbArgs));
			}
		}
	}
	
	void MainWindow::removeEventNameClicked()
	{
		QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
		Q_ASSERT(currentRow.isValid());
		eventsDescriptionsModel->delNamedValue(currentRow.row());

		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->isSynchronized = false; // Jiwon
		}
	}

	void MainWindow::eventsUpdated(bool indexChanged)
	{
		if (indexChanged)
		{
			statusText->setText(tr("Desynchronised! Please reload."));
			statusText->show();
		}
		recompileAll();
		updateWindowTitle();
	}

	void MainWindow::eventsUpdatedDirty()
	{
		eventsUpdated(true);
	}
	
	void MainWindow::eventsDescriptionsSelectionChanged()
	{
		bool isSelected = eventsDescriptionsView->selectionModel()->currentIndex().isValid();
		removeEventNameButton->setEnabled(isSelected);
		sendEventButton->setEnabled(isSelected);
		#ifdef HAVE_QWT
		plotEventButton->setEnabled(isSelected);
		#endif // HAVE_QWT
	}
	
	void MainWindow::resetStatusText()
	{
		bool flag;
		int i;
		
		for (i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab) 
			{
				flag = tab->isSynchronized;
				break;
			}
		}

		for (; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab) 
				flag = (flag && tab->isSynchronized);
		}
		
		if (flag) 
		{
			statusText->clear();
			statusText->hide();
		}
	}
	
	void MainWindow::addConstantClicked()
	{
		bool ok;
		QString constantName;
		int constantValue = 0;

		// prompt the user for the named value
		ok = NewNamedValueDialog::getNamedValue(&constantName, &constantValue, -32768, 32767, tr("Add a new constant"), tr("Name:"), tr("Value", "Value assigned to the constant"));

		if (ok && !constantName.isEmpty())
		{
			if (commonDefinitions.constants.contains(constantName.toStdWString()))
			{
				QMessageBox::warning(this, tr("Constant already defined"), tr("Constant %0 is already defined.").arg(constantName));
			}
			else
			{
				constantsDefinitionsModel->addNamedValue(NamedValue(constantName.toStdWString(), constantValue));
				recompileAll();
				updateWindowTitle();
			}
		}
	}
	
	void MainWindow::removeConstantClicked()
	{
		QModelIndex currentRow = constantsView->selectionModel()->currentIndex();
		Q_ASSERT(currentRow.isValid());
		constantsDefinitionsModel->delNamedValue(currentRow.row());
		
		recompileAll();
		updateWindowTitle();
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
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->recompile();
		}
	}
	
	void MainWindow::writeAllBytecodes()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->writeBytecode();
		}
	}
	
	void MainWindow::rebootAllNodes()
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				tab->reboot();
		}
	}
	
	void MainWindow::sourceChanged()
	{
		sourceModified = true;
		updateWindowTitle();
	}

	void MainWindow::showUserManual()
	{
		helpViewer.showHelp(HelpViewer::USERMANUAL);
	}
	
	//! A new node has connected to the network.
	void MainWindow::nodeConnected(unsigned node)
	{
		// create a new tab for the node
		NodeTab* tab = new NodeTab(this, target, &commonDefinitions, node);
		tab->showKeywords(showKeywordsAct->isChecked());
		tab->linenumbers->showLineNumbers(showLineNumbers->isChecked());
		tab->showMemoryUsage(showMemoryUsageAct->isChecked());
		
		// check if there is an absent node tab with this id and name, and copy data
		const int absentIndex(getAbsentIndexFromId(node));
		const AbsentNodeTab* absentTab(getAbsentTabFromId(node));
		if (absentTab && nodes->tabText(absentIndex) == target->getName(node))
		{
			tab->editor->document()->setPlainText(absentTab->editor->document()->toPlainText());
			nodes->removeAndDeleteTab(absentIndex);
		}
		
		// connect and show new tab
		connect(tab, SIGNAL(uploadReadynessChanged(bool)), SLOT(uploadReadynessChanged()));
		nodes->addTab(tab, target->getName(node));
		
		regenerateToolsMenus();
	}
	
	//! A node has disconnected from the network.
	void MainWindow::nodeDisconnected(unsigned node)
	{
		const int index = getIndexFromId(node);
		Q_ASSERT(index >= 0);
		const NodeTab* tab = getTabFromId(node);
		const QString& tabName = nodes->tabText(index);
		
		nodes->addTab(
			new AbsentNodeTab(
				node,
				tabName,
				tab->editor->document()->toPlainText()
			),
			tabName
		);
		
		nodes->removeAndDeleteTab(index);
		
		regenerateToolsMenus();
		regenerateHelpMenu();
		
		if (!getDescriptionTimer)
			getDescriptionTimer = startTimer(2000);
	}
	
	//! The network connection has been cut: all nodes have disconnected.
	void MainWindow::networkDisconnected()
	{
		// collect all node ids to disconnect
		std::vector<unsigned> toDisconnect;
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				toDisconnect.push_back(tab->nodeId());
		}
		
		// disconnect all node ids
		for (size_t i = 0; i < toDisconnect.size(); i++)
			nodeDisconnected(toDisconnect[i]);
	}
	
	//! A user event has arrived from the network.
	void MainWindow::userEvent(unsigned id, const VariablesDataVector &data)
	{	
		if (eventsDescriptionsModel->isVisible(id)) 
		{	
			QString text = QTime::currentTime().toString("hh:mm:ss.zzz");

			if (id < commonDefinitions.events.size())
					text += QString("\n%0 : ").arg(QString::fromStdWString(commonDefinitions.events[id].name));
			else
				text += tr("\nevent %0 : ").arg(id);

			for (size_t i = 0; i < data.size(); i++)
				text += QString("%0 ").arg(data[i]);
			
			if (logger->count() > 50)
				delete logger->takeItem(0);
			QListWidgetItem * item = new QListWidgetItem(QIcon(":/images/info.png"), text, logger);
			logger->scrollToBottom();
			Q_UNUSED(item);
		}
		
		#ifdef HAVE_QWT
		
		// iterate over all viewer for this event
		QList<EventViewer*> viewers = eventsViewers.values(id);
		for (int i = 0; i < viewers.size(); ++i)
			viewers.at(i)->addData(data);
		
		#endif // HAVE_QWT
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
		Q_UNUSED(item);
		logger->setStyleSheet(" QListView::item { background: rgb(255,128,128); }");
	}
	
	//! A node did an access out of array bounds exception.
	void MainWindow::arrayAccessOutOfBounds(unsigned node, unsigned line, unsigned size, unsigned index)
	{
		addErrorEvent(node, line, tr("array access at %0 out of bounds [0..%1]").arg(index).arg(size-1));
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
	
	//! If any node was disconnected, send get description
	void MainWindow::timerEvent ( QTimerEvent * event )
	{
		bool doSend(false);
		
		for (int i = 0; i < nodes->count(); i++)
		{
			AbsentNodeTab* tab = dynamic_cast<AbsentNodeTab*>(nodes->widget(i));
			if (tab)
				doSend = doSend || (tab->id != 0);
		}
		
		if (doSend)
		{
			target->broadcastGetDescription();
		}
		else
		{
			killTimer(getDescriptionTimer);
			getDescriptionTimer = 0;
		}
	}
	
	//! Get the tab widget index of a corresponding node id
	int MainWindow::getIndexFromId(unsigned node) const
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
			{
				if (tab->nodeId() == node)
					return i;
			}
		}
		return -1;
	}
	
	//! Get the tab widget pointer of a corresponding node id
	NodeTab* MainWindow::getTabFromId(unsigned node) const
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
			{
				if (tab->nodeId() == node)
					return tab;
			}
		}
		return 0;
	}
	
	//! Get the tab widget pointer of a corresponding node name
	NodeTab* MainWindow::getTabFromName(const QString& name) const
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
			{
				if (target->getName(tab->nodeId()) == name)
					return tab;
			}
		}
		return 0;
	}
	
	//! Get the absent tab widget index of a corresponding node id
	int MainWindow::getAbsentIndexFromId(unsigned node) const
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			AbsentNodeTab* tab = dynamic_cast<AbsentNodeTab*>(nodes->widget(i));
			if (tab)
			{
				if (tab->nodeId() == node)
					return i;
			}
		}
		return -1;
	}
	
	//! Get the absent tab widget pointer of a corresponding node id
	AbsentNodeTab* MainWindow::getAbsentTabFromId(unsigned node) const
	{
		for (int i = 0; i < nodes->count(); i++)
		{
			AbsentNodeTab* tab = dynamic_cast<AbsentNodeTab*>(nodes->widget(i));
			if (tab)
			{
				if (tab->nodeId() == node)
					return tab;
			}
		}
		return 0;
	}
	
	void MainWindow::clearDocumentSpecificTabs()
	{
		bool changed = false;
		do
		{
			changed = false;
			for (int i = 0; i < nodes->count(); i++)
			{
				QWidget* tab = nodes->widget(i);
				
				#ifdef HAVE_QWT
				if (dynamic_cast<AbsentNodeTab*>(tab) || dynamic_cast<EventViewer*>(tab))
				#else // HAVE_QWT
				if (dynamic_cast<AbsentNodeTab*>(tab))
				#endif // HAVE_QWT
				{
					nodes->removeAndDeleteTab(i);
					changed = true;
					break;
				}
			}
		}
		while (changed);
	}
	
	void MainWindow::setupWidgets()
	{
		currentScriptTab = 0;
		nodes = new EditorsPlotsTabWidget;
		nodes->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
		
		QSplitter *splitter = new QSplitter();
		splitter->addWidget(nodes);
		setCentralWidget(splitter);

		addConstantButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
		removeConstantButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
		addConstantButton->setToolTip(tr("Add a new constant"));
		removeConstantButton->setToolTip(tr("Remove this constant"));
		removeConstantButton->setEnabled(false);
		
		constantsView = new FixedWidthTableView;
		constantsView->setShowGrid(false);
		constantsView->verticalHeader()->hide();
		constantsView->horizontalHeader()->hide();
		constantsView->setModel(constantsDefinitionsModel);
		constantsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		constantsView->setSelectionMode(QAbstractItemView::SingleSelection);
		constantsView->setSelectionBehavior(QAbstractItemView::SelectRows);
		constantsView->setDragDropMode(QAbstractItemView::InternalMove);
		constantsView->setDragEnabled(true);
		constantsView->setDropIndicatorShown(true);
		constantsView->setItemDelegateForColumn(1, new SpinBoxDelegate(-32768, 32767, this));
		constantsView->setMinimumHeight(100);
		constantsView->setSecondColumnLongestContent("-888888##");
		constantsView->resizeRowsToContents();
		
		QGridLayout* constantsLayout = new QGridLayout;
		constantsLayout->addWidget(new QLabel(tr("<b>Constants</b>")),0,0);
		constantsLayout->setColumnStretch(0, 1);
		constantsLayout->addWidget(addConstantButton,0,1);
		constantsLayout->setColumnStretch(1, 0);
		constantsLayout->addWidget(removeConstantButton,0,2);
		constantsLayout->setColumnStretch(2, 0);
		constantsLayout->addWidget(constantsView, 1, 0, 1, 3);
		//setColumnStretch
		
		/*QHBoxLayout* constantsAddRemoveLayout = new QHBoxLayout;;
		constantsAddRemoveLayout->addStretch();
		addConstantButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
		constantsAddRemoveLayout->addWidget(addConstantButton);
		removeConstantButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
		removeConstantButton->setEnabled(false);
		constantsAddRemoveLayout->addWidget(removeConstantButton);
		
		eventsDockLayout->addLayout(constantsAddRemoveLayout);
		eventsDockLayout->addWidget(constantsView, 1);*/
		
		
		/*eventsDockLayout->addWidget(new QLabel(tr("<b>Events</b>")));
		
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
				
		eventsDockLayout->addWidget(eventsDescriptionsView, 1);*/
		
		
		addEventNameButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
		removeEventNameButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
		removeEventNameButton->setEnabled(false);
		sendEventButton = new QPushButton(QPixmap(QString(":/images/newmsg.png")), "");
		sendEventButton->setEnabled(false);

		addEventNameButton->setToolTip(tr("Add a new event"));
		removeEventNameButton->setToolTip(tr("Remove this event"));
		sendEventButton->setToolTip(tr("Send this event"));

		#ifdef HAVE_QWT
		plotEventButton = new QPushButton(QPixmap(QString(":/images/plot.png")), "");
		plotEventButton->setEnabled(false);
		plotEventButton->setToolTip(tr("Plot this event"));
		#endif // HAVE_QWT

		eventsDescriptionsView = new FixedWidthTableView;
		eventsDescriptionsView->setShowGrid(false);
		eventsDescriptionsView->verticalHeader()->hide();
		eventsDescriptionsView->horizontalHeader()->hide();
		eventsDescriptionsView->setModel(eventsDescriptionsModel);
		eventsDescriptionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		eventsDescriptionsView->setSelectionMode(QAbstractItemView::SingleSelection);
		eventsDescriptionsView->setSelectionBehavior(QAbstractItemView::SelectRows);
		eventsDescriptionsView->setDragDropMode(QAbstractItemView::InternalMove);
		eventsDescriptionsView->setDragEnabled(true);
		eventsDescriptionsView->setDropIndicatorShown(true);
		eventsDescriptionsView->setItemDelegateForColumn(1, new SpinBoxDelegate(0, (ASEBA_MAX_PACKET_SIZE-6)/2, this));
		eventsDescriptionsView->setMinimumHeight(100);
		eventsDescriptionsView->setSecondColumnLongestContent("255###");
		eventsDescriptionsView->resizeRowsToContents();
		eventsDescriptionsView->setContextMenuPolicy(Qt::CustomContextMenu);
		
		QGridLayout* eventsLayout = new QGridLayout;
		eventsLayout->addWidget(new QLabel(tr("<b>Global Events</b>")),0,0,1,4);
		eventsLayout->addWidget(addEventNameButton,1,0);
		//eventsLayout->setColumnStretch(2, 0);
		eventsLayout->addWidget(removeEventNameButton,1,1);
		//eventsLayout->setColumnStretch(3, 0);
		//eventsLayout->setColumnStretch(0, 1);
		eventsLayout->addWidget(sendEventButton,1,2);
		//eventsLayout->setColumnStretch(1, 0);
		#ifdef HAVE_QWT
		eventsLayout->addWidget(plotEventButton,1,3);
		#endif // HAVE_QWT
		eventsLayout->addWidget(eventsDescriptionsView, 2, 0, 1, 4);
		
		/*logger = new QListWidget;
		logger->setMinimumSize(80,100);
		logger->setSelectionMode(QAbstractItemView::NoSelection);
		eventsDockLayout->addWidget(logger, 3);
		clearLogger = new QPushButton(tr("Clear"));
		eventsDockLayout->addWidget(clearLogger);*/
		
		logger = new QListWidget;
		logger->setMinimumSize(80,100);
		logger->setSelectionMode(QAbstractItemView::NoSelection);
		clearLogger = new QPushButton(tr("Clear"));
		statusText = new QLabel(""); // Jiwon
		statusText->hide();
		
		QVBoxLayout* loggerLayout = new QVBoxLayout;
		loggerLayout->addWidget(statusText); // Jiwon
		loggerLayout->addWidget(logger);
		loggerLayout->addWidget(clearLogger);
		
		// panel
		QSplitter* rightPanelSplitter = new QSplitter(Qt::Vertical);
		
		QWidget* constantsWidget = new QWidget;
		constantsWidget->setLayout(constantsLayout);
		rightPanelSplitter->addWidget(constantsWidget);
		
		QWidget* eventsWidget = new QWidget;
		eventsWidget->setLayout(eventsLayout);
		rightPanelSplitter->addWidget(eventsWidget);
		
		QWidget* loggerWidget = new QWidget;
		loggerWidget->setLayout(loggerLayout);
		rightPanelSplitter->addWidget(loggerWidget);
		
		// main window
		splitter->addWidget(rightPanelSplitter);
		splitter->setSizes(QList<int>() << 800 << 200);
		
		// dialog box
		compilationMessageBox = new CompilationLogDialog();
		connect(this, SIGNAL(MainWindowClosed()), compilationMessageBox, SLOT(close()));
		connect(this, SIGNAL(MainWindowClosed()), compilationMessageBox, SLOT(deleteLater()));
		findDialog = new FindDialog(this);
		connect(this, SIGNAL(MainWindowClosed()), findDialog, SLOT(close()));
		
		// help viewer
		QString lang = target->getLanguage().left(2);
		if (lang != "")
			helpViewer.setLanguage(lang);
		else
			helpViewer.setLanguage(QLocale::system().name());
		connect(this, SIGNAL(MainWindowClosed()), &helpViewer, SLOT(close()));
	}
	
	void MainWindow::setupConnections()
	{
		// general connections
		connect(nodes, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
		connect(logger, SIGNAL(itemDoubleClicked(QListWidgetItem *)), SLOT(logEntryDoubleClicked(QListWidgetItem *)));
		connect(ConfigDialog::getInstance(), SIGNAL(settingsChanged()), SLOT(applySettings()));
		
		// global actions
		connect(loadAllAct, SIGNAL(triggered()), SLOT(loadAll()));
		connect(resetAllAct, SIGNAL(triggered()), SLOT(resetAll()));
		connect(runAllAct, SIGNAL(triggered()), SLOT(runAll()));
		connect(pauseAllAct, SIGNAL(triggered()), SLOT(pauseAll()));

		// events
		connect(addEventNameButton, SIGNAL(clicked()), SLOT(addEventNameClicked()));
		connect(removeEventNameButton, SIGNAL(clicked()), SLOT(removeEventNameClicked()));
		connect(sendEventButton, SIGNAL(clicked()), SLOT(sendEvent()));
		#ifdef HAVE_QWT
		connect(plotEventButton, SIGNAL(clicked()), SLOT(plotEvent()));
		#endif // HAVE_QWT
		connect(eventsDescriptionsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), SLOT(eventsDescriptionsSelectionChanged()));
		connect(eventsDescriptionsView, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(sendEventIf(const QModelIndex &)));
		connect(eventsDescriptionsView, SIGNAL(clicked(const QModelIndex &)), SLOT(toggleEventVisibleButton(const QModelIndex &)) );
		connect(eventsDescriptionsModel, SIGNAL(dataChanged ( const QModelIndex &, const QModelIndex & ) ), SLOT(eventsUpdated()));
		connect(eventsDescriptionsModel, SIGNAL(publicRowsInserted()), SLOT(eventsUpdated()));
		connect(eventsDescriptionsModel, SIGNAL(publicRowsRemoved()), SLOT(eventsUpdatedDirty()));
		connect(eventsDescriptionsView, SIGNAL(customContextMenuRequested ( const QPoint & )), SLOT(eventContextMenuRequested(const QPoint & )));

		// logger
		connect(clearLogger, SIGNAL(clicked()), logger, SLOT(clear()));
		connect(clearLogger, SIGNAL(clicked()), SLOT(clearAllExecutionError()));
		
		// constants
		connect(addConstantButton, SIGNAL(clicked()), SLOT(addConstantClicked()));
		connect(removeConstantButton, SIGNAL(clicked()), SLOT(removeConstantClicked()));
		connect(constantsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), SLOT(constantsSelectionChanged()));
		connect(constantsDefinitionsModel, SIGNAL(dataChanged ( const QModelIndex &, const QModelIndex & ) ), SLOT(recompileAll()));
		connect(constantsDefinitionsModel, SIGNAL(dataChanged ( const QModelIndex &, const QModelIndex & ) ), SLOT(updateWindowTitle()));

		// target events
		connect(target, SIGNAL(nodeConnected(unsigned)), SLOT(nodeConnected(unsigned)));
		connect(target, SIGNAL(nodeDisconnected(unsigned)), SLOT(nodeDisconnected(unsigned)));
		connect(target, SIGNAL(networkDisconnected()),  SLOT(networkDisconnected()));
		
		connect(target, SIGNAL(userEvent(unsigned, const VariablesDataVector &)), SLOT(userEvent(unsigned, const VariablesDataVector &)));
		connect(target, SIGNAL(userEventsDropped(unsigned)), SLOT(userEventsDropped(unsigned)));
		connect(target, SIGNAL(arrayAccessOutOfBounds(unsigned, unsigned, unsigned, unsigned)), SLOT(arrayAccessOutOfBounds(unsigned, unsigned, unsigned, unsigned)));
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
		
		// Add all other actions excepted the one we are processing
		QSettings settings;
		QStringList recentFiles = settings.value("recent files").toStringList();
		for (int i = 0; i < recentFiles.size(); i++)
		{
			const QString& fileName(recentFiles.at(i));
			openRecentMenu->addAction(fileName, this, SLOT(openRecentFile()));
		}
	}
	
	void MainWindow::updateRecentFiles(const QString& fileName)
	{
		QSettings settings;
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
		rebootMenu->clear();
		saveBytecodeMenu->clear();
		
		unsigned activeVMCount(0);
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
			{
				QAction *act = writeBytecodeMenu->addAction(tr("...inside %0").arg(target->getName(tab->nodeId())),tab, SLOT(writeBytecode()));
				connect(tab, SIGNAL(uploadReadynessChanged(bool)), act, SLOT(setEnabled(bool)));
				
				rebootMenu->addAction(tr("...%0").arg(target->getName(tab->nodeId())),tab, SLOT(reboot()));
				
				act = saveBytecodeMenu->addAction(tr("...of %0").arg(target->getName(tab->nodeId())),tab, SLOT(saveBytecode()));
				connect(tab, SIGNAL(uploadReadynessChanged(bool)), act, SLOT(setEnabled(bool)));
				
				++activeVMCount;
			}
		}
		
		writeBytecodeMenu->addSeparator();
		writeAllBytecodesAct = writeBytecodeMenu->addAction(tr("...inside all nodes"), this, SLOT(writeAllBytecodes()));
		
		rebootMenu->addSeparator();
		rebootMenu->addAction(tr("...all nodes"), this, SLOT(rebootAllNodes()));
		
		globalToolBar->setVisible(activeVMCount > 1);
	}
	
	void MainWindow::generateHelpMenu()
	{
		helpMenu->addAction(tr("&User Manual..."), this, SLOT(showUserManual()), QKeySequence(tr("F1", "Help|User Manual")));
		helpMenu->addSeparator();
		
		helpMenuTargetSpecificSeparator = helpMenu->addSeparator();
		helpMenu->addAction(tr("Web site Aseba..."), this, SLOT(openToUrlFromAction()))->setData(QUrl(tr("http://aseba.wikidot.com/en:start")));
		helpMenu->addAction(tr("Report bug..."), this, SLOT(openToUrlFromAction()))->setData(QUrl(tr("http://github.com/aseba-community/aseba/issues/new")));
		
		#ifdef Q_WS_MAC
		helpMenu->addAction("about", this, SLOT(about()));
		helpMenu->addAction("About &Qt...", qApp, SLOT(aboutQt()));
		#else // Q_WS_MAC
		helpMenu->addSeparator();
		helpMenu->addAction(tr("&About..."), this, SLOT(about()));
		helpMenu->addAction(tr("About &Qt..."), qApp, SLOT(aboutQt()));
		#endif // Q_WS_MAC
	}
	
	void MainWindow::regenerateHelpMenu()
	{
		// remove old target-specific actions
		while (!targetSpecificHelp.isEmpty())
		{
			QAction *action(targetSpecificHelp.takeFirst());
			helpMenu->removeAction(action);
			delete action;
		}
		
		// add back target-specific actions
		typedef std::set<int> ProductIds;
		ProductIds productIds;
		for (int i = 0; i < nodes->count(); i++)
		{
			NodeTab* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
			if (tab)
				productIds.insert(tab->productId());
		}
		for (ProductIds::const_iterator it(productIds.begin()); it != productIds.end(); ++it)
		{
			QAction *action;
			switch (*it)
			{
				case ASEBA_PID_THYMIO2:
				action = new QAction(tr("Thymio programming tutorial..."), helpMenu);
				connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
				action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymiotutoriel")));
				targetSpecificHelp.append(action);
				helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
				action = new QAction(tr("Thymio programming interface..."), helpMenu);
				connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
				action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymioapi")));
				targetSpecificHelp.append(action);
				helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
				break;
				
				case ASEBA_PID_CHALLENGE:
				action = new QAction(tr("Challenge tutorial..."), helpMenu);
				connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
				action->setData(QUrl(tr("http://aseba.wikidot.com/en:gettingstarted")));
				targetSpecificHelp.append(action);
				helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
				break;
				
				case ASEBA_PID_MARXBOT:
				action = new QAction(tr("MarXbot user manual..."), helpMenu);
				connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
				action->setData(QUrl(tr("http://mobots.epfl.ch/data/robots/marxbot-user-manual.pdf")));
				targetSpecificHelp.append(action);
				helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
				break;
				
				default:
				break;
			}
		}
	}
	
	void MainWindow::openToUrlFromAction() const
	{
		const QAction *action(reinterpret_cast<QAction *>(sender()));
		QDesktopServices::openUrl(action->data().toUrl());
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
		openRecentMenu = new QMenu(tr("Open &Recent"), fileMenu);
		regenerateOpenRecentMenu();
		fileMenu->addMenu(openRecentMenu)->setIcon(QIcon(":/images/fileopen.png"));
		
		fileMenu->addAction(QIcon(":/images/filesave.png"), tr("&Save..."),
							this, SLOT(save()),
							QKeySequence(tr("Ctrl+S", "File|Save")));
		fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Save &As..."),
							this, SLOT(saveFile()));
		
		fileMenu->addSeparator();
		/*fileMenu->addAction(QIcon(":/images/network.png"), tr("Connect to &target"),
							target, SLOT(connect()),
							QKeySequence(tr("Ctrl+T", "File|Connect to target")));
		fileMenu->addSeparator();*/
		fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Export &memories content..."),
							this, SLOT(exportMemoriesContent()));
		fileMenu->addAction(QIcon(":/images/fileopen.png"), tr("&Import memories content..."),
							this, SLOT(importMemoriesContent()));
		fileMenu->addSeparator();
		#ifdef Q_WS_MAC
		fileMenu->addAction(QIcon(":/images/exit.png"), "quit",
							this, SLOT(close()),
							QKeySequence(tr("Ctrl+Q", "File|Quit")));
		#else // Q_WS_MAC
		fileMenu->addAction(QIcon(":/images/exit.png"), tr("&Quit"),
							this, SLOT(close()),
							QKeySequence(tr("Ctrl+Q", "File|Quit")));
		#endif // Q_WS_MAC
		
		// Edit menu
		cutAct = new QAction(QIcon(":/images/editcut.png"), tr("Cu&t"), this);
		cutAct->setShortcut(tr("Ctrl+X", "Edit|Cut"));
		cutAct->setEnabled(false);
		
		copyAct = new QAction(QIcon(":/images/editcopy.png"), tr("&Copy"), this);
		copyAct->setShortcut(tr("Ctrl+C", "Edit|Copy"));
		copyAct->setEnabled(false);
		
		pasteAct = new QAction(QIcon(":/images/editpaste.png"), tr("&Paste"), this);
		pasteAct->setShortcut(tr("Ctrl+V", "Edit|Paste"));
		pasteAct->setEnabled(false);
		
		undoAct = new QAction(QIcon(":/images/undo.png"), tr("&Undo"), this);
		undoAct->setShortcut(tr("Ctrl+Z", "Edit|Undo"));
		undoAct->setEnabled(false);
		
		redoAct = new QAction(QIcon(":/images/redo.png"), tr("Re&do"), this);
		redoAct->setShortcut(tr("Ctrl+Shift+Z", "Edit|Redo"));
		redoAct->setEnabled(false);
		
		findAct = new QAction(QIcon(":/images/find.png"), tr("&Find..."), this);
		findAct->setShortcut(tr("Ctrl+F", "Edit|Find"));
		connect(findAct, SIGNAL(triggered()), SLOT(findTriggered()));
		findAct->setEnabled(false);
		
		replaceAct = new QAction(QIcon(":/images/edit.png"), tr("&Replace..."), this);
		replaceAct->setShortcut(tr("Ctrl+R", "Edit|Replace"));
		connect(replaceAct, SIGNAL(triggered()), SLOT(replaceTriggered()));
		replaceAct->setEnabled(false);

		commentAct = new QAction(tr("Comment the selection"), this);
		commentAct->setShortcut(tr("Ctrl+D", "Edit|Comment the selection"));
		connect(commentAct, SIGNAL(triggered()), SLOT(commentTriggered()));

		uncommentAct = new QAction(tr("Uncomment the selection"), this);
		uncommentAct->setShortcut(tr("Shift+Ctrl+D", "Edit|Uncomment the selection"));
		connect(uncommentAct, SIGNAL(triggered()), SLOT(uncommentTriggered()));

		QMenu *editMenu = new QMenu(tr("&Edit"), this);
		menuBar()->addMenu(editMenu);
		editMenu->addAction(cutAct);
		editMenu->addAction(copyAct);
		editMenu->addAction(pasteAct);
		editMenu->addSeparator();
		editMenu->addAction(QIcon(":/images/editcopy.png"), tr("Copy &all"), this, SLOT(copyAll()));
		editMenu->addSeparator();
		editMenu->addAction(undoAct);
		editMenu->addAction(redoAct);
		editMenu->addSeparator();
		editMenu->addAction(findAct);
		editMenu->addAction(replaceAct);
		editMenu->addSeparator();
		editMenu->addAction(commentAct);
		editMenu->addAction(uncommentAct);
		
		// View menu
		showKeywordsAct = new QAction(tr("Show &keywords"), this);
		showKeywordsAct->setCheckable(true);
		connect(showKeywordsAct, SIGNAL(toggled(bool)), SLOT(showKeywords(bool)));

		showMemoryUsageAct = new QAction(tr("Show memory usage"), this);
		showMemoryUsageAct->setCheckable(true);
		connect(showMemoryUsageAct, SIGNAL(toggled(bool)), SLOT(showMemoryUsage(bool)));

		showHiddenAct = new QAction(tr("S&how hidden variables and functions"), this);
		showHiddenAct->setCheckable(true);
		connect(showHiddenAct, SIGNAL(toggled(bool)), SLOT(showHidden(bool)));

		showLineNumbers = new QAction(tr("Show Line Numbers"), this);
		showLineNumbers->setShortcut(tr("F11", "Edit|Show Line Numbers"));
		showLineNumbers->setCheckable(true);
		connect(showLineNumbers, SIGNAL(toggled(bool)), SLOT(showLineNumbersChanged(bool)));

		goToLineAct = new QAction(QIcon(":/images/goto.png"), tr("&Go To Line..."), this);
		goToLineAct->setShortcut(tr("Ctrl+G", "Edit|Go To Line"));
		goToLineAct->setEnabled(false);
		connect(goToLineAct, SIGNAL(triggered()), SLOT(goToLine()));

		QMenu *viewMenu = new QMenu(tr("&View"), this);
		viewMenu->addAction(showKeywordsAct);
		viewMenu->addAction(showMemoryUsageAct);
		viewMenu->addAction(showHiddenAct);
		viewMenu->addSeparator();
		viewMenu->addAction(showLineNumbers);
		viewMenu->addAction(goToLineAct);
		viewMenu->addSeparator();
		viewMenu->addAction(tr("&Settings"), this, SLOT(showSettings()));
		menuBar()->addMenu(viewMenu);

		// Debug actions
		loadAllAct = new QAction(QIcon(":/images/upload.png"), tr("&Load all"), this);
		loadAllAct->setShortcut(tr("F7", "Load|Load all"));
		
		resetAllAct = new QAction(QIcon(":/images/reset.png"), tr("&Reset all"), this);
		resetAllAct->setShortcut(tr("F8", "Debug|Reset all"));
		
		runAllAct = new QAction(QIcon(":/images/play.png"), tr("Ru&n all"), this);
		runAllAct->setShortcut(tr("F9", "Debug|Run all"));
		
		pauseAllAct = new QAction(QIcon(":/images/pause.png"), tr("&Pause all"), this);
		pauseAllAct->setShortcut(tr("F10", "Debug|Pause all"));

		// Debug toolbar
		globalToolBar = addToolBar(tr("Debug"));
		globalToolBar->setObjectName("debug toolbar");
		globalToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		globalToolBar->addAction(loadAllAct);
		globalToolBar->addAction(resetAllAct);
		globalToolBar->addAction(runAllAct);
		globalToolBar->addAction(pauseAllAct);
		
		// Debug menu
		toggleBreakpointAct = new QAction(tr("Toggle breakpoint"), this);
		toggleBreakpointAct->setShortcut(tr("Ctrl+B", "Debug|Toggle breakpoint"));
		connect(toggleBreakpointAct, SIGNAL(triggered()), SLOT(toggleBreakpoint()));

		clearAllBreakpointsAct = new QAction(tr("Clear all breakpoints"), this);
		//clearAllBreakpointsAct->setShortcut();
		connect(clearAllBreakpointsAct, SIGNAL(triggered()), SLOT(clearAllBreakpoints()));

		QMenu *debugMenu = new QMenu(tr("&Debug"), this);
		menuBar()->addMenu(debugMenu);
		debugMenu->addAction(toggleBreakpointAct);
		debugMenu->addAction(clearAllBreakpointsAct);
		debugMenu->addSeparator();
		debugMenu->addAction(loadAllAct);
		debugMenu->addAction(resetAllAct);
		debugMenu->addAction(runAllAct);
		debugMenu->addAction(pauseAllAct);
		
		// Tool menu
		QMenu *toolMenu = new QMenu(tr("&Tools"), this);
		menuBar()->addMenu(toolMenu);
		/*toolMenu->addAction(QIcon(":/images/view_text.png"), tr("&Show last compilation messages"),
							this, SLOT(showCompilationMessages()),
							QKeySequence(tr("Ctrl+M", "Tools|Show last compilation messages")));*/
		showCompilationMsg = new QAction(QIcon(":/images/view_text.png"), tr("&Show last compilation messages"), this);
		showCompilationMsg->setCheckable(true);
		toolMenu->addAction(showCompilationMsg);
		connect(showCompilationMsg, SIGNAL(toggled(bool)), SLOT(showCompilationMessages(bool)));
		connect(compilationMessageBox, SIGNAL(hidden()), SLOT(compilationMessagesWasHidden()));
		toolMenu->addSeparator();
		writeBytecodeMenu = new QMenu(tr("Write the program(s)..."), toolMenu);
		toolMenu->addMenu(writeBytecodeMenu);
		rebootMenu = new QMenu(tr("Reboot..."), toolMenu);
		toolMenu->addMenu(rebootMenu);
		saveBytecodeMenu = new QMenu(tr("Save the binary code..."), toolMenu);
		toolMenu->addMenu(saveBytecodeMenu);
		
		// Help menu
		helpMenu = new QMenu(tr("&Help"), this);
		menuBar()->addMenu(helpMenu);
		generateHelpMenu();
		regenerateHelpMenu();
		
		// add dynamic stuff
		regenerateToolsMenus();

		// Load the state from the settings (thus from hard drive)
		applySettings();
	}
	//! Ask the user to save or discard or ignore the operation that would destroy the unmodified data.
	/*!
		\return true if it is ok to discard, false if operation must abort
	*/
	bool MainWindow::askUserBeforeDiscarding()
	{
		const bool anythingModified = sourceModified || constantsDefinitionsModel->checkIfModified() || eventsDescriptionsModel->checkIfModified();
		if (anythingModified == false)
			return true;

		QString docName(tr("Untitled"));
		if (!actualFileName.isEmpty())
			docName = actualFileName.mid(actualFileName.lastIndexOf("/") + 1);
		
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Aseba Studio - Confirmation Dialog"));
		msgBox.setText(tr("The document \"%0\" has been modified.").arg(docName));
		msgBox.setInformativeText(tr("Do you want to save your changes or discard them?"));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Save);
		
		int ret = msgBox.exec();
		switch (ret)
		{
			case QMessageBox::Save:
				// Save was clicked
				if (save())
					return true;
				else
					return false;
			case QMessageBox::Discard:
				// Don't Save was clicked
				return true;
			case QMessageBox::Cancel:
				// Cancel was clicked
				return false;
			default:
				// should never be reached
				assert(false);
				break;
		}

		return false;
	}

	void MainWindow::closeEvent ( QCloseEvent * event )
	{
		if (askUserBeforeDiscarding())
		{
			writeSettings();
			event->accept();
			emit MainWindowClosed();
		}
		else
		{
			event->ignore();
		}
	}

	bool MainWindow::readSettings()
	{
		bool result;

		QSettings settings;
		result = restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
		restoreState(settings.value("MainWindow/windowState").toByteArray());
		return result;
	}

	void MainWindow::writeSettings()
	{
		QSettings settings;
		settings.setValue("MainWindow/geometry", saveGeometry());
		settings.setValue("MainWindow/windowState", saveState());
	}
	
	void MainWindow::updateWindowTitle()
	{
		const bool anythingModified = sourceModified || constantsDefinitionsModel->checkIfModified() || eventsDescriptionsModel->checkIfModified();
		
		QString modifiedText;
		if (anythingModified)
			modifiedText = tr("[modified] ");
			
		QString docName(tr("Untitled"));
		if (!actualFileName.isEmpty())
			docName = actualFileName.mid(actualFileName.lastIndexOf("/") + 1);
		
		setWindowTitle(tr("%0 %1- Aseba Studio").arg(docName).arg(modifiedText));
	}

	void MainWindow::applySettings()
	{
		showKeywordsAct->setChecked(ConfigDialog::getShowKeywordToolbar());
		showMemoryUsageAct->setChecked(ConfigDialog::getShowMemoryUsage());
		showHiddenAct->setChecked(ConfigDialog::getShowHidden());
		showLineNumbers->setChecked(ConfigDialog::getShowLineNumbers());
	}
	
	/*@}*/
}; // Aseba

