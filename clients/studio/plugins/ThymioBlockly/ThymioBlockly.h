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

#ifndef THYMIO_BLOCKLY_H
#define THYMIO_BLOCKLY_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QToolBar>
#include <QDomDocument>
#include <QWebView>

#include <map>
#include <vector>
#include <iterator>
#include <memory>

#include "../../Plugin.h"
#include "../../DashelTarget.h"

#include "AsebaJavascriptInterface.h"

namespace Aseba { namespace ThymioBlockly
{
	/** \addtogroup studio */
	/*@{*/
	
	class ThymioBlockly : public QWidget, public NodeToolInterface
	{
		Q_OBJECT
	
	public:
		ThymioBlockly(DevelopmentEnvironmentInterface *_de, bool showCloseButton = true, bool debugLog = false, bool execFeedback = false);
		~ThymioBlockly();
		
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
		bool closeFile();
		
	private slots:
		void openHelp() const;
		void saveSnapshot() const;
		
		void newFile();
		void openFile();
		bool save();
		bool saveAs();
		void run();
		void stop();
		
		void initAsebaJavascriptInterface();
		void codeUpdated(const QString& code);
		void workspaceSaved(const QString& xml);
		void blocklyLoaded();

	private:
		void clearSceneWithoutRecompilation();
		void showAtSavedPosition();
		
	protected:
		std::auto_ptr<DevelopmentEnvironmentInterface> de;
		QWebView *webview;
		mutable AsebaJavascriptInterface asebaJavascriptInterface;
		QString currentSavedXml;
		bool skipNextUpdate;

		QToolBar *toolBar;
		QHBoxLayout *toolLayout;
		QPushButton *newButton;
		QPushButton *openButton;
		QPushButton *saveButton;
		QPushButton *saveAsButton;
		QPushButton *runButton;
		QPushButton *stopButton;
		QPushButton *helpButton;
		QPushButton *snapshotButton;
		QFrame *firstSeparator;
		QFrame *secondSeparator;
		QSpacerItem *spacer1;
		QSpacerItem *spacer2;
		QSpacerItem *spacer3;
		QSpacerItem *spacerRunStop;
		QSpacerItem *spacer4;
		QSpacerItem *spacer5;
		QSpacerItem *spacer6;
		
		QVBoxLayout *mainLayout;
		QHBoxLayout *horizontalLayout;
		QVBoxLayout *sceneLayout;
		
	protected:
		void saveGeometryIfVisible();
		bool preDiscardWarningDialog(bool keepCode);
		void closeEvent(QCloseEvent * event);
		
		virtual void resizeEvent( QResizeEvent *event );
	};

	/*@}*/
} } // namespace ThymioBlockly / namespace Aseba

#endif
