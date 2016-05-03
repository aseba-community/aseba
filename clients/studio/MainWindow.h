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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMessageBox>
#include <QItemSelection>
#include <QTableView>
#include <QSplitter>
#include <QTextEdit>
#include <QMultiMap>
#include <QTabWidget>
#include <QCloseEvent>
#include <QFuture>
#include <QFutureWatcher>
#include <QToolButton>
#include <QToolBar>
#include <QCompleter>
#include <QSortFilterProxyModel>
#include <QSignalMapper>

#include "StudioAeslEditor.h"
#include "CustomDelegate.h"
#include "CustomWidgets.h"
#include "ModelAggregator.h"

#include "../../compiler/compiler.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "Target.h"
#include "TargetModels.h"
#include "Plugin.h"
#include "PluginRegistry.h"
#include "HelpViewer.h"
#include "ConfigDialog.h"

class QLabel;
class QSpinBox;
class QGroupBox;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QTreeView;
class QTranslator;
//class QTextBrowser;
class QToolBox;
class QCheckBox;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class TargetVariablesModel;
	class TargetFunctionsModel;
	class TargetMemoryModel;
	class MaskableNamedValuesVectorModel;
	class NamedValuesVectorModel;
	class ConstantsModel;
	class StudioAeslEditor;
	class AeslLineNumberSidebar;
	class AeslBreakpointSidebar;
	class AeslHighlighter;
	class EventViewer;
	class EditorsPlotsTabWidget;
	class DraggableListWidget;
	class FindDialog;
	class NodeTab;
	class MainWindow;
	
	//! To access private members of MainWindow and its children, a plugin must inherit from this class
	struct StudioInterface: DevelopmentEnvironmentInterface
	{
		NodeTab* nodeTab;
		MainWindow *mainWindow;
		
		StudioInterface(NodeTab* nodeTab);
		
		Target * getTarget();
		unsigned getNodeId() const;
		unsigned getProductId() const;
		void setCommonDefinitions(const CommonDefinitions& commonDefinitions);
		void displayCode(const QList<QString>& code, int elementToHighlight);
		void loadAndRun();
		void stop();
		TargetVariablesModel * getVariablesModel();
		void setVariableValues(unsigned, const VariablesDataVector &);
		bool saveFile(bool as=false);
		void openFile();
		bool newFile();
		void clearOpenedFileName(bool isModified);
		QString openedFileName() const;
	};
	
	class CompilationLogDialog: public QDialog
	{
		Q_OBJECT
		
	public:
		CompilationLogDialog(QWidget *parent = 0);
		void setText(const QString & text) { te->setText(text); }
	signals:
		void hidden();
	protected:
		virtual void hideEvent ( QHideEvent * event );
		QTextEdit* te;
	};

	class EditorsPlotsTabWidget: public QTabWidget
	{
		Q_OBJECT

	public:
		EditorsPlotsTabWidget();
		virtual ~EditorsPlotsTabWidget();
		void addTab(QWidget* widget, const QString& label, bool closable = false);
		void highlightTab(int index, QColor color = Qt::red);
		void setExecutionMode(int index, Target::ExecutionMode state);

	public slots:
		void removeAndDeleteTab(int index = -1);

	protected slots:
		void vmMemoryResized(int, int, int);		// for vmMemoryView QTreeView child widget
		void tabChanged(int index);

	protected:
		virtual void resetHighlight(int index);
		virtual void vmMemoryViewResize(NodeTab* tab);
		virtual void readSettings();
		virtual void writeSettings();

		int vmMemorySize[2];
	};
	
	//! Tab holding code (instead of plot)
	class ScriptTab
	{
	public:
		typedef NodeToolInterface::SavedContent SavedContent;
		typedef QList<SavedContent> SavedPlugins;
		
		ScriptTab(const unsigned id):id(id) {}
		virtual ~ScriptTab() {}
		
		unsigned nodeId() const { return id; }
		virtual SavedPlugins savePlugins() const = 0;
		
	protected:
		void createEditor();
		
		unsigned id; //!< node identifier
		
		friend class MainWindow;
		StudioAeslEditor* editor;
		AeslLineNumberSidebar* linenumbers;
		AeslBreakpointSidebar* breakpoints;
		AeslHighlighter *highlighter;
	};
	
	//! Tab for a target node that is not connected
	class AbsentNodeTab : public QWidget, public ScriptTab
	{
		Q_OBJECT
		
	public:
		AbsentNodeTab(const unsigned id, const QString& name, const QString& sourceCode, const SavedPlugins& savedPlugins);
		
		virtual SavedPlugins savePlugins() const { return savedPlugins; }
	
		const QString name;
		SavedPlugins savedPlugins;
	};
	
	//! Tab for a connected target node
	class NodeTab : public QSplitter, public ScriptTab, public VariableListener
	{
		Q_OBJECT
	
	public:
		struct CompilationResult
		{
			bool dump;
			bool success;
			BytecodeVector bytecode;
			unsigned allocatedVariablesCount;
			VariablesMap variablesMap;
			Compiler::SubroutineTable subroutineTable;
			Error error;
			std::wostringstream compilationMessages;
			
			CompilationResult(bool dump):dump(dump), success(false), allocatedVariablesCount(0) {}
		};
		
		
	public:
		NodeTab(MainWindow* mainWindow, Target *target, const CommonDefinitions *commonDefinitions, const unsigned id, QWidget *parent = 0);
		~NodeTab();
		unsigned productId() const  { return pid; }
		
		void variablesMemoryChanged(unsigned start, const VariablesDataVector &variables);
		
	signals:
		void uploadReadynessChanged(bool);
	
	protected:
		virtual void timerEvent ( QTimerEvent * event );
		virtual void variableValueUpdated(const QString& name, const VariablesDataVector& values);
		void setupWidgets();
		void setupConnections();
		virtual SavedPlugins savePlugins() const;
		void notifyPluginsAboutToLoad();
		void restorePlugins(const SavedPlugins& savedPlugins, bool fromFile);
		void updateToolList();
	
	public slots:
		void clearExecutionErrors();
		void refreshCompleterModel(LocalContext context);
		//void sortCompleterModel();
	
	protected slots:
		void resetClicked();
		void loadClicked();
		void runInterruptClicked();
		void nextClicked();
		void refreshMemoryClicked();
		void autoRefreshMemoryClicked(int state);
		
		void writeBytecode();
		void reboot();
		void saveBytecode();
		
		void setVariableValues(unsigned, const VariablesDataVector &);
		void insertVariableName(const QModelIndex &);
		
		void editorContentChanged();
		void recompile();
		void markTargetUnsynced();
	
		// keywords
		void keywordClicked(QString);
		void showKeywords(bool show);

		void showMemoryUsage(bool show);
		
		void cursorMoved();
		void goToError();
		
		void setBreakpoint(unsigned line);
		void clearBreakpoint(unsigned line);
		void breakpointClearedAll();
		
		void executionPosChanged(unsigned line);
		void executionModeChanged(Target::ExecutionMode mode);
		
		void breakpointSetResult(unsigned line, bool success);
		
		void closePlugins();
		
		void updateHidden();
		
		void compilationCompleted();
	
	protected:
		void processCompilationResult(CompilationResult* result);
		void rehighlight();
		void reSetBreakpoints();
		
		// editor properties code
		bool setEditorProperty(const QString &property, const QVariant &value, unsigned line, bool removeOld = false);
		bool clearEditorProperty(const QString &property, unsigned line);
		bool clearEditorProperty(const QString &property);
		void switchEditorProperty(const QString &oldProperty, const QString &newProperty);
		
	protected:
		friend class MainWindow;
		friend class StudioAeslEditor;
		friend class EditorsPlotsTabWidget;
		
		unsigned pid; //!< node product identifier
		friend struct StudioInterface;
		Target *target; //!< pointer to target
		const CommonDefinitions *commonDefinitions; //!< pointer to common definitions

		MainWindow* mainWindow;
		QLabel *cursorPosText;
		QLabel *compilationResultImage;
		QLabel *compilationResultText;
		QLabel *memoryUsageText;
		
		QLabel *executionModeLabel;
		QPushButton *loadButton;
		QPushButton *resetButton;
		QPushButton *runInterruptButton;
		QPushButton *nextButton;
		QPushButton *refreshMemoryButton;
		QCheckBox *autoRefreshMemoryCheck;
		
		// keywords
		QToolButton *varButton;
		QToolButton *ifButton;
		QToolButton *elseifButton;
		QToolButton *elseButton;
		QToolButton *oneventButton;
		QToolButton *whileButton;
		QToolButton *forButton;
		QToolButton *subroutineButton;
		QToolButton *callsubButton;
		QToolBar *keywordsToolbar;
		QSignalMapper *signalMapper;
		
		TargetVariablesModel *vmMemoryModel;
		TargetSubroutinesModel *vmSubroutinesModel;
		QTreeView *vmMemoryView;
		QLineEdit *vmMemoryFilter;
		
		TargetFunctionsModel *vmFunctionsModel;
		QTreeView *vmFunctionsView;
		
		DraggableListWidget* vmLocalEvents;

		QCompleter *completer;
		QAbstractItemModel* eventAggregator;
		QAbstractItemModel* variableAggregator;
		QSortFilterProxyModel* sortingProxy;
		TreeChainsawFilter* functionsFlatModel;
		
		QToolBox* toolBox;
		QVBoxLayout* toolListLayout;
		int toolListIndex;
		NodeToolInterfaces tools;
		
		int refreshTimer; //!< id of timer for auto refresh of variables, if active
		
		QString lastCompiledSource; //!< content of last source considered for compilation following a textChanged signal
		int errorPos; //!< position of last error, -1 if compilation was success
		int currentPC; //!< current program counter
		Target::ExecutionMode previousMode;
		bool showHidden;
		
		QFuture<CompilationResult*> compilationFuture;
		QFutureWatcher<CompilationResult*> compilationWatcher;
		bool compilationDirty;
		bool isSynchronized;
		
		BytecodeVector bytecode; //!< bytecode resulting of last successfull compilation
		unsigned allocatedVariablesCount; //!< number of allocated variables
	};

	class NewNamedValueDialog : public QDialog
	{
		Q_OBJECT

	public:
		NewNamedValueDialog(QString* name, int* value, int min, int max);
		static bool getNamedValue(QString* name, int* value, int min, int max, QString title, QString valueName, QString valueDescription);

	protected slots:
		void okSlot();
		void cancelSlot();

	protected:
		QLabel* label1;
		QLineEdit* line1;
		QLabel* label2;
		QSpinBox* line2;

		QString* name;
		int* value;
	};

	//! Studio main window
	class MainWindow : public QMainWindow
	{
		Q_OBJECT
	
	public:
		MainWindow(QVector<QTranslator*> translators, const QString& commandLineTarget, bool autoRefresh, QWidget *parent = 0);
		~MainWindow();

	signals:
		void MainWindowClosed();
		
	private slots:
		void about();
		bool newFile();
		void openFile(const QString &path = QString());
		void openRecentFile();
		bool save();
		bool saveFile(const QString &previousFileName = QString());
		void exportMemoriesContent();
		void importMemoriesContent();
		void copyAll();
		void findTriggered();
		void replaceTriggered();
		void commentTriggered();
		void uncommentTriggered();
		void showLineNumbersChanged(bool state);
		void goToLine();
		void zoomIn();
		void zoomOut();
		void showSettings();
		void showHidden(bool show);
		void showKeywords(bool show);

		void toggleBreakpoint();
		void clearAllBreakpoints();
		
		void loadAll();
		void resetAll();
		void runAll();
		void pauseAll();
		void stopAll();
		
		void clearAllExecutionError();
		
		void uploadReadynessChanged();
		void tabChanged(int);
		void sendEvent();
		void sendEventIf(const QModelIndex &);
		void toggleEventVisibleButton(const QModelIndex &);
		void plotEvent();
		void eventContextMenuRequested(const QPoint & pos);
		void plotEvent(const unsigned eventId);
		void logEntryDoubleClicked(QListWidgetItem *);
		void showCompilationMessages(bool doShown);
		void compilationMessagesWasHidden();
		void showMemoryUsage(bool show);
		
		void addEventNameClicked();
		void removeEventNameClicked();
		void eventsUpdated(bool indexChanged = false);
		void eventsUpdatedDirty();
		void eventsDescriptionsSelectionChanged();
		
		void resetStatusText(); // Jiwon
		
		void addConstantClicked();
		void removeConstantClicked();
		void constantsSelectionChanged();
		
		void nodeConnected(unsigned node);
		void nodeDisconnected(unsigned node);
		
		void userEventsDropped(unsigned amount);
		void userEvent(unsigned id, const VariablesDataVector &data);
		void arrayAccessOutOfBounds(unsigned node, unsigned line, unsigned size, unsigned index);
		void divisionByZero(unsigned node, unsigned line);
		void eventExecutionKilled(unsigned node, unsigned line);
		void nodeSpecificError(unsigned node, unsigned line, const QString& message);
		
		void executionPosChanged(unsigned node, unsigned line);
		void executionModeChanged(unsigned node, Target::ExecutionMode mode);
		void variablesMemoryEstimatedDirty(unsigned node);
		
		void variablesMemoryChanged(unsigned node, unsigned start, const VariablesDataVector &variables);
		
		void breakpointSetResult(unsigned node, unsigned line, bool success);
	
		void recompileAll();
		void writeAllBytecodes();
		void rebootAllNodes();
		
		void sourceChanged();
		void updateWindowTitle();

		void showUserManual();
		
		void openToUrlFromAction() const;

	public slots:
		void applySettings();
	
	private:
		// utility functions
		int getIndexFromId(unsigned node) const;
		NodeTab* getTabFromId(unsigned node) const;
		NodeTab* getTabFromName(const QString& name, unsigned preferedId = 0, bool* isPrefered = 0, QSet<int>* filledList = 0) const;
		int getAbsentIndexFromId(unsigned node) const;
		AbsentNodeTab* getAbsentTabFromId(unsigned node) const;
		void addErrorEvent(unsigned node, unsigned line, const QString& message);
		void clearDocumentSpecificTabs();
		bool askUserBeforeDiscarding();
		
		// gui initialisation code
		void regenerateOpenRecentMenu();
		void updateRecentFiles(const QString& fileName);
		void regenerateToolsMenus();
		void generateHelpMenu();
		void regenerateHelpMenu();
		void setupWidgets();
		void setupConnections();
		void setupMenu();
		void closeEvent ( QCloseEvent * event );
		bool readSettings();
		void writeSettings();
		void clearOpenedFileName(bool isModified);
		
		// tabs and nodes
		friend class NodeTab;
		friend class StudioAeslEditor;
		friend struct StudioInterface;
		EditorsPlotsTabWidget* nodes;
		ScriptTab* currentScriptTab;
		
		#ifdef HAVE_QWT
		
		// events viewers. only one event per viewer
		friend class EventViewer;
		typedef QMultiMap<unsigned, EventViewer*> EventViewers;
		EventViewers eventsViewers;
		
		#endif // HAVE_QWT
		
		// events
		QPushButton* addEventNameButton;
		QPushButton* removeEventNameButton;
		QPushButton* sendEventButton;
		#ifdef HAVE_QWT
		QPushButton* plotEventButton;
		#endif // HAVE_QWT
		QListWidget* logger;
		QPushButton* clearLogger;
		QLabel* statusText; // Jiwon
		FixedWidthTableView* eventsDescriptionsView;
		
		// constants
		QPushButton* addConstantButton;
		QPushButton* removeConstantButton;
		FixedWidthTableView* constantsView;
		
		// models
		MaskableNamedValuesVectorModel* eventsDescriptionsModel;
		ConstantsModel* constantsDefinitionsModel;
		
		// global buttons
		QAction* loadAllAct;
		QAction* resetAllAct;
		QAction* runAllAct;
		QAction* pauseAllAct;
		QToolBar* globalToolBar;
		
		// open recent actions
		QMenu *openRecentMenu;
		
		// tools
		QMenu *writeBytecodeMenu;
		QAction *writeAllBytecodesAct;
		QMenu *rebootMenu;
		QMenu *saveBytecodeMenu;
		QMenu *helpMenu;
		typedef QList<QAction*> ActionList;
		QAction* helpMenuTargetSpecificSeparator;
		ActionList targetSpecificHelp;
		
		// Menu action that need dynamic reconnection
		QAction *cutAct;
		QAction *copyAct;
		QAction *pasteAct;
		QAction *undoAct;
		QAction *redoAct;
		QAction *findAct;
		QAction *replaceAct;
		QAction *commentAct;
		QAction *uncommentAct;
		QAction *showLineNumbers;
		QAction *goToLineAct;
		QAction *zoomInAct;
		QAction *zoomOutAct;
		QAction *toggleBreakpointAct;
		QAction *clearAllBreakpointsAct;
		QAction *showHiddenAct;
		QAction *showKeywordsAct; 
		QAction* showCompilationMsg;
		QAction* showMemoryUsageAct;
		
		// gui helper stuff
		CompilationLogDialog *compilationMessageBox; //!< box to show last compilation messages
		FindDialog* findDialog; //!< find dialog
		QString actualFileName; //!< name of opened file, "" if new
		bool sourceModified; //!< true if source code has been modified since last save
		bool autoMemoryRefresh; //! < true if auto memory refresh is on by default
		HelpViewer helpViewer;
		
		// compiler and source code related stuff
		CommonDefinitions commonDefinitions;
		Target *target;
	};
	
	/*@}*/
} // namespace Aseba

#endif

