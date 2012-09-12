#ifndef THYMIO_VISUAL_PROGRAMMING_H
#define THYMIO_VISUAL_PROGRAMMING_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <QGraphicsItem>
#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include <QComboBox>
#include <QToolBar>
#include <QToolButton>

#include <dashel/dashel.h>

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
	class ThymioVisualProgramming : public QDialog, public InvasivePlugin, public NodeToolInterface
	{
		Q_OBJECT
		
	public:
		ThymioVisualProgramming(NodeTab* nodeTab);
		~ThymioVisualProgramming();
		
		virtual QWidget* createMenuEntry();
		virtual void closeAsSoonAsPossible();

		virtual void loadFromDom(const QDomDocument& content);
		virtual QDomDocument saveToDom() const;
		//virtual bool surviveTabDestruction() const;
		
	private slots:
		void showFlashDialog();
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
		void closeFile();
		
		void recompileButtonSet();
		
	protected:
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
		QComboBox *colorComboButton;
		QToolButton *quitButton;

		QVBoxLayout *mainLayout;
		QHBoxLayout *horizontalLayout;
		QVBoxLayout *eventsLayout;
		QVBoxLayout *sceneLayout;
		QHBoxLayout *compilationResultLayout;
		QVBoxLayout *actionsLayout;


		QSvgRenderer *tapSvg;
		QSvgRenderer *clapSvg;
		
		QString thymioFilename;

		int windowWidth;
		int windowHeight;
		
		// we must cache this because during the flashing process, the tab is not there any more
		unsigned nodeId;
//		Target * target;
//		Dashel::Stream 	*stream;
		
		QPixmap drawColorScheme(QColor color1, QColor color2);
		bool warningDialog();
		void setColors(QComboBox *button = 0);
		void closeEvent(QCloseEvent * event);
		bool saveFile(QString filename);		
		bool loadFile(QString filename);
		
		virtual void resizeEvent( QResizeEvent *event );
	};
	/*@}*/
}; // Aseba

#endif
