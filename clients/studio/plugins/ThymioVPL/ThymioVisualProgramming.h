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
#include <QToolButton>
#include <QDomDocument>

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
	class CardButton;
	
	/** \addtogroup studio */
	/*@{*/
	class ThymioVisualProgramming : public QWidget, public NodeToolInterface
	{
		Q_OBJECT
		
	public:
		ThymioVisualProgramming(DevelopmentEnvironmentInterface *_de, bool showCloseButton = true);
		~ThymioVisualProgramming();
		
		virtual QWidget* createMenuEntry();
		virtual void closeAsSoonAsPossible();

		virtual void aboutToLoad();
		virtual void loadFromDom(const QDomDocument& content, bool fromFile);
		virtual QDomDocument saveToDom() const;
		virtual void codeChangedInEditor();
		
		bool isModified() const;
	
	signals:
		void modifiedStatusChanged(bool modified);
		void compilationOutcome(bool success);
		
	protected slots:
		void showErrorLine();
		bool closeFile();
		
	private slots:
		void openToUrlFromAction() const;
		void showVPLModal();
		void addButtonsEvent();
		void addProxEvent();
		void addProxGroundEvent();
		void addTapEvent();
		void addClapEvent();
		void addTimeoutEvent();
		
		void addMoveAction();
		void addColorTopAction();
		void addColorBottomAction();
		void addSoundAction();
		void addTimerAction();
		void addMemoryAction();
		
		void newFile();
		void openFile();
		bool save();
		bool saveAs();
		void setColorScheme(int index);
		void run();
		void stop();
		void toggleAdvancedMode();
		void processCompilationResult();
		void processHighlightChange();
		
	private:
		void toggleAdvancedMode(bool advanced, bool force=false);
		void clearSceneWithoutRecompilation();
		void showAtSavedPosition();
		
	protected:
		std::auto_ptr<DevelopmentEnvironmentInterface> de;
		QGraphicsView *view;
		Scene *scene;
		bool loading; //!< true during load, to prevent recursion of changes triggered by VPL itself
		
		// Event & Action buttons
		QList<CardButton *> eventButtons;
		QList<CardButton *> actionButtons;
		QLabel *eventsLabel;
		QLabel *actionsLabel;

		QList<QColor> eventColors;
		QList<QColor> actionColors;
	
		QLabel *compilationResult;
		QLabel *compilationResultImage;
		QPushButton *showCompilationError;

		QToolBar *toolBar;
		QToolButton *newButton;
		QToolButton *openButton;
		QToolButton *saveButton;
		QToolButton *saveAsButton;
		QToolButton *runButton;
		QToolButton *stopButton;
		QToolButton *advancedButton;
		QToolButton *helpButton;
		QComboBox *colorComboButton;
		QToolButton *quitButton;

		QVBoxLayout *mainLayout;
		QHBoxLayout *horizontalLayout;
		QVBoxLayout *eventsLayout;
		QVBoxLayout *sceneLayout;
		QHBoxLayout *compilationResultLayout;
		QVBoxLayout *actionsLayout;
		
	protected:
		friend class Aseba::ThymioVPLStandalone;
		friend struct Aseba::ThymioVPLStandaloneInterface;
		
		QPixmap drawColorScheme(QColor color1, QColor color2);
		void saveGeometryIfVisible();
		bool preDiscardWarningDialog(bool keepCode);
		void clearHighlighting(bool keepCode);
		void setColors(QComboBox *button = 0);
		void closeEvent(QCloseEvent * event);
		
		float computeScale(QResizeEvent *event, int desiredToolbarIconSize);
		virtual void resizeEvent( QResizeEvent *event );
	};
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
