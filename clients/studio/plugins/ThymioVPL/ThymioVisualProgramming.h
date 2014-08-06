#ifndef THYMIO_VISUAL_PROGRAMMING_H
#define THYMIO_VISUAL_PROGRAMMING_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QGraphicsSvgItem>
#include <QGraphicsItem>
#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include <QComboBox>
#include <QToolBar>
#include <QPushButton>
#include <QDomDocument>
#include <QStack>

#include <map>
#include <vector>
#include <iterator>
#include <memory>

#include "../../Plugin.h"
#include "../../DashelTarget.h"

namespace Aseba
{
	struct ThymioVPLStandaloneInterface;
	class ThymioVPLStandalone;
}

namespace Aseba { namespace ThymioVPL
{
	class Scene;
	class BlockButton;
	class ResizingView;
	
	/** \addtogroup studio */
	/*@{*/
	
	class ThymioVisualProgramming : public QWidget, public NodeToolInterface
	{
		Q_OBJECT
	
	public:
		ThymioVisualProgramming(DevelopmentEnvironmentInterface *_de, bool showCloseButton = true, bool debugLog = false);
		~ThymioVisualProgramming();
		
		virtual QWidget* createMenuEntry();
		virtual void closeAsSoonAsPossible();

		virtual void aboutToLoad();
		virtual void loadFromDom(const QDomDocument& content, bool fromFile);
		virtual QDomDocument saveToDom() const;
		virtual void codeChangedInEditor();
		
		bool isModified() const;
		qreal getViewScale() const;
		
	signals:
		void modifiedStatusChanged(bool modified);
		void compilationOutcome(bool success);
		
	protected slots:
		void showErrorLine();
		bool closeFile();
		
	private slots:
		void openHelp() const;
		void showVPLModal();
		
		void addEvent();
		void addAction();
		
		void newFile();
		void openFile();
		bool save();
		bool saveAs();
		void setColorScheme(int index);
		void run();
		void stop();
		void toggleAdvancedMode();
		void pushUndo();
		void undo();
		void redo();
		void processCompilationResult();
		void processHighlightChange();
		
	private:
		void clearUndo();
		void toggleAdvancedMode(bool advanced, bool force=false, bool ignoreSceneCheck=false);
		void clearSceneWithoutRecompilation();
		void showAtSavedPosition();
		
	public:
		const bool debugLog;
		
	protected:
		friend class Aseba::ThymioVPL::BlockButton;
		friend class Aseba::ThymioVPL::Scene;
		
		std::auto_ptr<DevelopmentEnvironmentInterface> de;
		ResizingView *view;
		Scene *scene;
		bool loading; //!< true during load, to prevent recursion of changes triggered by VPL itself
		
		QStack<QString> undoStack; //!< keep string version of QDomDocument
		int undoPos; //!< current position of undo in the stack, -1 if invalid
		
		// Event & Action buttons
		QList<BlockButton *> eventButtons;
		QList<BlockButton *> actionButtons;
		QLabel *eventsLabel;
		QLabel *actionsLabel;
	
		QLabel *compilationResult;
		QLabel *compilationResultImage;
		QPushButton *showCompilationError;
		
		//QSlider *zoomSlider;

		QToolBar *toolBar;
		QGridLayout *toolLayout;
		QPushButton *newButton;
		QPushButton *openButton;
		QPushButton *saveButton;
		QPushButton *saveAsButton;
		QPushButton *undoButton;
		QPushButton *redoButton;
		QPushButton *runButton;
		QPushButton *stopButton;
		QPushButton *advancedButton;
		QPushButton *helpButton;
		QComboBox *colorComboButton;
		QPushButton *quitButton;
		QSpacerItem *quitSpotSpacer;
		
		// run button animation
		QVector<QPixmap> runAnimFrames;
		int runAnimFrame;
		int runAnimTimer;

		QVBoxLayout *mainLayout;
		QHBoxLayout *horizontalLayout;
		QVBoxLayout *eventsLayout;
		QVBoxLayout *sceneLayout;
		QHBoxLayout *compilationResultLayout;
		QVBoxLayout *actionsLayout;
		
	protected:
		friend class Aseba::ThymioVPLStandalone;
		friend struct Aseba::ThymioVPLStandaloneInterface;
		
		QPixmap drawColorScheme(const QColor& eventColor, const QColor& stateColor, const QColor& actionColor);
		void saveGeometryIfVisible();
		bool preDiscardWarningDialog(bool keepCode);
		void clearHighlighting(bool keepCode);
		void setColors(QComboBox *comboBox);
		void updateBlockButtonImages();
		void closeEvent(QCloseEvent * event);
		
		void regenerateRunButtonAnimation(const QSize& iconSize);
		float computeScale(QResizeEvent *event, int desiredToolbarIconSize);
		virtual void resizeEvent( QResizeEvent *event );
		virtual void timerEvent ( QTimerEvent * event );
	};
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
