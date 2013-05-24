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

#include <map>
#include <vector>
#include <iterator>
#include <QDomDocument>

#include "ThymioButtons.h"
#include "ThymioScene.h"

#include "../../Plugin.h"
#include "../../DashelTarget.h"

namespace Aseba
{
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

		virtual void loadFromDom(const QDomDocument& content, bool fromFile);
		virtual QDomDocument saveToDom() const;
	
	signals:
		void compilationOutcome(bool success);
		
	public slots:
		bool closeFile();
		
	private slots:
		void openToUrlFromAction() const;
		void showVPLModal();
		void addButtonsEvent();
		void addProxEvent();
		void addProxGroundEvent();
		void addTapEvent();
		void addClapEvent();
		
		void addMoveAction();
		void addColorAction();
		void addCircleAction();
		void addSoundAction();
		void addMemoryAction();
		
		void newFile();
		void openFile();
		bool save();
		bool saveAs();
		void setColorScheme(int index);
		void run();
		void stop();
		void advancedMode();
		void recompileButtonSet();
		
	protected:
		std::auto_ptr<DevelopmentEnvironmentInterface> de;
		QGraphicsView *view;
		ThymioScene *scene;

		// Event & Action buttons
		QList<ThymioPushButton *> eventButtons;
		QList<ThymioPushButton *> actionButtons;
		QLabel *eventsLabel;
		QLabel *actionsLabel;

		QList<QColor> eventColors;
		QList<QColor> actionColors;
	
		QLabel *compilationResult;
		QLabel *compilationResultImage;

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

		QPixmap drawColorScheme(QColor color1, QColor color2);
		bool warningDialog();
		void setColors(QComboBox *button = 0);
		void closeEvent(QCloseEvent * event);
		
		float computeScale(QResizeEvent *event, int desiredToolbarIconSize);
		virtual void resizeEvent( QResizeEvent *event );
	};
	/*@}*/
}; // Aseba

#endif
