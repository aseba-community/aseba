#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMessageBox>
#include <QItemSelection>
#include <QTableView>

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
class QTextEdit;
class QListWidget;
class QListWidgetItem;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class TargetVariablesModel;
	class TargetFunctionsModel;
	class TargetMemoryModel;
	class AeslEditor;
	class AeslHighlighter;
	class CompilationLogDialog;
	
	class MemoryTableView : public QTableView
	{
	public:
		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		void resizeColumnsToContents ();
	};
	
	class NodeTab : public QWidget
	{
		Q_OBJECT
		
	public:
		NodeTab(Target *target, const EventsNamesVector *eventsNames, int id, QWidget *parent = 0);
		~NodeTab();
		unsigned nodeId() const { return id; }
		
		void variablesMemoryChanged(unsigned start, const VariablesDataVector &variables);
	
	signals:
		void uploadReadynessChanged();
	
	protected:
		void setupWidgets();
		void setupConnections();
		
	protected slots:
		void resetClicked();
		void debugClicked();
		void runInterruptClicked();
		void nextClicked();
		void refreshMemoryClicked();
		
		void editorContentChanged();
		void recompile();
		
		void cursorMoved();
		void goToError();
		
		void clearExecutionErrors();
		void setBreakpoint(unsigned line);
		void clearBreakpoint(unsigned line);
		
		void executionPosChanged(unsigned line);
		void executionModeChanged(Target::ExecutionMode mode);
		
		void breakpointSetResult(unsigned line, bool success);
	
	private:
		void rehighlight();
		void reSetBreakpoints();
		
		// editor properties code
		bool setEditorProperty(const QString &property, const QVariant &value, unsigned line, bool removeOld = false);
		bool clearEditorProperty(const QString &property, unsigned line);
		bool clearEditorProperty(const QString &property);
		
	protected:
		unsigned id;
		Target *target;
	
	private:
		friend class MainWindow;
		AeslEditor *editor;
		AeslHighlighter *highlighter;
		QLabel *cursorPosText;
		QLabel *compilationResultImage;
		QLabel *compilationResultText;
		
		QLabel *executionModeLabel;
		QPushButton *debugButton;
		QPushButton *resetButton;
		QPushButton *runInterruptButton;
		QPushButton *nextButton;
		QPushButton *refreshMemoryButton;
		
		TargetMemoryModel *vmMemoryModel;
		MemoryTableView *vmMemoryView;
		
		TargetFunctionsModel *vmFunctionsModel;
		QTableView *vmFunctionsView;
		
		bool rehighlighting; //!< is the next contentChanged due to rehighlight() call ?
		int errorPos; //!< position of last error, -1 if compilation was success
		
		Compiler compiler; //!< Aesl compiler
		BytecodeVector bytecode; //!< bytecode resulting of last successfull compilation
		VariablesNamesVector variablesNames; //!< names of variables resulting of last successfull compilation
		unsigned allocatedVariablesCount; //!< number of allocated variables
	};
	
	class MainWindow : public QMainWindow
	{
		Q_OBJECT
	
	public:
		MainWindow(QWidget *parent = 0);
		~MainWindow();
		void loadDefaultFile();
	
	private slots:
		void about();
		void newFile();
		void openFile(const QString &path = QString());
		void save();
		void saveFile(const QString &previousFileName = QString());
		
		void debugAll();
		void resetAll();
		void runAll();
		void pauseAll();
		void stopAll();
		
		void uploadReadynessChanged();
		void tabChanged(int);
		void sendEvent();
		void logEntryDoubleClicked(QListWidgetItem *);
		void showCompilationMessages();
		
		void addEventNameClicked();
		void removeEventNameClicked();
		void eventsNamesSelectionChanged();
		
		void nodeConnected(unsigned node);
		void nodeDisconnected(unsigned node);
		void networkDisconnected();
		
		void userEvent(unsigned id, const VariablesDataVector &data);
		void arrayAccessOutOfBounds(unsigned node, unsigned line, unsigned index);
		void divisionByZero(unsigned node, unsigned line);
		
		void executionPosChanged(unsigned node, unsigned line);
		void executionModeChanged(unsigned node, Target::ExecutionMode mode);
		void variablesMemoryEstimatedDirty(unsigned node);
		
		void variablesMemoryChanged(unsigned node, unsigned start, const VariablesDataVector &variables);
		
		void breakpointSetResult(unsigned node, unsigned line, bool success);
	
	private:
		// utility functions
		int getIndexFromId(unsigned node);
		NodeTab* getTabFromId(unsigned node);
		NodeTab* getTabFromName(const QString& name);
		void rebuildEventsNames();
		void recompileAll();
		
		// initialisation code
		void setupWidgets();
		void setupConnections();
		void setupMenu();
		void hideEvent(QHideEvent * event);
		
		// data members
		QTabWidget* nodes;
		NodeTab* previousActiveTab;
		
		// events
		QPushButton* addEventNameButton;
		QPushButton* removeEventNameButton;
		QListWidget* logger;
		QListWidget* eventsNamesList;
		EventsNamesVector eventsNames;
		
		// global buttons
		QAction* debugAllAct;
		QAction* resetAllAct;
		QAction* runAllAct;
		QAction* pauseAllAct;
		QAction* sendEventAct;
		
		// Menu action that need dynamic reconnection
		QAction *cutAct;
		QAction *copyAct;
		QAction *pasteAct;
		QAction *undoAct;
		QAction *redoAct;
		
		CompilationLogDialog *compilationMessageBox; //!< box to show last compilation messages
		QString actualFileName; //!< name of opened file, "" if new
		Compiler compiler; //!< Aesl compiler
		Target *target;
	};
	
	/*@}*/
}; // Aseba

#endif

