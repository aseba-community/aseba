#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMessageBox>
#include <QItemSelection>
#include <QTableView>
#include <QSplitter>
#include <QTextEdit>

#include "CustomDelegate.h"

#include "../compiler/compiler.h"
#include <fstream>
#include <sstream>
#include "Target.h"

class QLabel;
class QSpinBox;
class QGroupBox;
class QPushButton;
class QTabWidget;
class QListWidget;
class QListWidgetItem;
class QTreeView;
class QTranslator;
class QTextBrowser;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class TargetVariablesModel;
	class TargetFunctionsModel;
	class TargetMemoryModel;
	class NamedValuesVectorModel;
	class AeslEditor;
	class AeslHighlighter;
	
	class CompilationLogDialog: public QTextEdit
	{
		Q_OBJECT
		
	public:
		CompilationLogDialog(QWidget *parent = 0);
	};
	
	class FixedWidthTableView : public QTableView
	{
	protected:
		int col1Width;
		
	public:
		FixedWidthTableView();
		void setSecondColumnLongestContent(const QString& content);
	
	protected:
		virtual void resizeEvent ( QResizeEvent * event );
	};
	
	class MainWindow;
	
	class ScriptTab
	{
	public:
		virtual ~ScriptTab() {}
		
	protected:
		void createEditor();
		
		friend class MainWindow;
		AeslEditor* editor;
		AeslHighlighter *highlighter;
	};
	
	class AbsentNodeTab : public QWidget, public ScriptTab
	{
		Q_OBJECT
		
	public:
		AbsentNodeTab(const QString& name, const QString& sourceCode);
	
		const QString name;
	};
	
	class NodeTab : public QSplitter, public ScriptTab
	{
		Q_OBJECT
		
	public:
		NodeTab(MainWindow* mainWindow, Target *target, const CommonDefinitions *commonDefinitions, int id, QWidget *parent = 0);
		~NodeTab();
		unsigned nodeId() const { return id; }
		
		void variablesMemoryChanged(unsigned start, const VariablesDataVector &variables);
	
	signals:
		void uploadReadynessChanged(bool);
	
	protected:
		void setupWidgets();
		void setupConnections();
	
	public slots:
		void clearExecutionErrors();
	
	protected slots:
		void resetClicked();
		void loadClicked();
		void runInterruptClicked();
		void nextClicked();
		void refreshMemoryClicked();
		
		void writeBytecode();
		void reboot();
		
		void setVariableValue(unsigned, int);
		void insertVariableName(const QModelIndex &);
		
		void editorContentChanged();
		void recompile();
		void markTargetUnsynced();
		
		void cursorMoved();
		void goToError();
		
		void setBreakpoint(unsigned line);
		void clearBreakpoint(unsigned line);
		void breakpointClearedAll();
		
		void executionPosChanged(unsigned line);
		void executionModeChanged(Target::ExecutionMode mode);
		
		void breakpointSetResult(unsigned line, bool success);
	
	private:
		void rehighlight();
		void reSetBreakpoints();
		void updateHidden();

		// editor properties code
		bool setEditorProperty(const QString &property, const QVariant &value, unsigned line, bool removeOld = false);
		bool clearEditorProperty(const QString &property, unsigned line);
		bool clearEditorProperty(const QString &property);
		void switchEditorProperty(const QString &oldProperty, const QString &newProperty);
		
	protected:
		unsigned id;
		Target *target;
	
	private:
		friend class MainWindow;
		MainWindow* mainWindow;
		QLabel *cursorPosText;
		QLabel *compilationResultImage;
		QLabel *compilationResultText;
		
		QLabel *executionModeLabel;
		QPushButton *loadButton;
		QPushButton *resetButton;
		QPushButton *runInterruptButton;
		QPushButton *nextButton;
		QPushButton *refreshMemoryButton;
		
		TargetVariablesModel *vmMemoryModel;
		QTreeView *vmMemoryView;
		
		TargetFunctionsModel *vmFunctionsModel;
		QTreeView *vmFunctionsView;
		
		QListWidget* vmLocalEvents;
		
		bool rehighlighting; //!< is the next contentChanged due to rehighlight() call ?
		int errorPos; //!< position of last error, -1 if compilation was success
		bool firstCompilation; //!< true if first compilation after creation
		bool showHidden;
		
		Compiler compiler; //!< Aesl compiler
		BytecodeVector bytecode; //!< bytecode resulting of last successfull compilation
		VariablesNamesVector variablesNames; //!< names of variables resulting of last successfull compilation
		unsigned allocatedVariablesCount; //!< number of allocated variables
	};
	
	class MainWindow : public QMainWindow
	{
		Q_OBJECT
	
	public:
		MainWindow(QVector<QTranslator*> translators, QWidget *parent = 0);
		~MainWindow();

	signals:
		void MainWindowClosed();
		
	private slots:
		void about();
		void newFile();
		void openFile(const QString &path = QString());
		void openRecentFile();
		bool save();
		bool saveFile(const QString &previousFileName = QString());
		void exportMemoriesContent();
		void copyAll();
		void showHidden(bool show);
		
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
		void logEntryDoubleClicked(QListWidgetItem *);
		void showCompilationMessages(bool doShown);
		
		void addEventNameClicked();
		void removeEventNameClicked();
		void eventsDescriptionsSelectionChanged();
		
		void addConstantClicked();
		void removeConstantClicked();
		void constantsSelectionChanged();
		
		void nodeConnected(unsigned node);
		void nodeDisconnected(unsigned node);
		void networkDisconnected();
		
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
		
		void addPluginLinearCameraView();
		
		void showHelpLanguage();
		void showHelpStudio();
	
	private:
		// utility functions
		int getIndexFromId(unsigned node);
		NodeTab* getTabFromId(unsigned node);
		NodeTab* getTabFromName(const QString& name);
		void addErrorEvent(unsigned node, unsigned line, const QString& message);
		void clearAbsentNodesTabs();
		
		// gui initialisation code
		void regenerateOpenRecentMenu();
		void updateRecentFiles(const QString& fileName);
		void regenerateToolsMenus();
		void setupWidgets();
		void setupConnections();
		void setupMenu();
		void hideEvent(QHideEvent * event);
		void closeEvent ( QCloseEvent * event );
		void updateWindowTitle();
		
		// tabs and nodes
		friend class NodeTab;
		QTabWidget* nodes;
		ScriptTab* previousActiveTab;
		
		// events
		QPushButton* addEventNameButton;
		QPushButton* removeEventNameButton;
		QPushButton* sendEventButton;
		QListWidget* logger;
		QPushButton* clearLogger;
		FixedWidthTableView* eventsDescriptionsView;
		
		// constants
		QPushButton* addConstantButton;
		QPushButton* removeConstantButton;
		FixedWidthTableView* constantsView;
		
		// models
		NamedValuesVectorModel* eventsDescriptionsModel;
		NamedValuesVectorModel* constantsDefinitionsModel;
		
		// global buttons
		QAction* loadAllAct;
		QAction* resetAllAct;
		QAction* runAllAct;
		QAction* pauseAllAct;
		
		// open recent actions
		QMenu *openRecentMenu;
		
		// tools
		QMenu *writeBytecodeMenu;
		QAction *writeAllBytecodesAct;
		QMenu *rebootMenu;
		// TODO: continue there
		
		// Menu action that need dynamic reconnection
		QAction *cutAct;
		QAction *copyAct;
		QAction *pasteAct;
		QAction *undoAct;
		QAction *redoAct;
		QAction *showHiddenAct;
		QAction* showCompilationMsg;
		
		// gui helper stuff
		CompilationLogDialog *compilationMessageBox; //!< box to show last compilation messages
		QString actualFileName; //!< name of opened file, "" if new
		bool sourceModified; //!< true if source code has been modified since last save
		QTextBrowser* helpViewer;
		
		// compiler and source code related stuff
		CommonDefinitions commonDefinitions;
		Compiler compiler; //!< Aesl compiler
		Target *target;
	};
	
	/*@}*/
}; // Aseba

#endif

