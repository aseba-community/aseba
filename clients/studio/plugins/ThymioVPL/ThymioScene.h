#ifndef THYMIO_SCENE_H
#define THYMIO_SCENE_H

#include <QGraphicsScene>

#include "ThymioButtons.h"
#include "ThymioButtonSet.h"

namespace Aseba
{	
	class ThymioScene : public QGraphicsScene
	{
		Q_OBJECT
		
	public:
		ThymioScene(QObject *parent = 0);
		~ThymioScene();
		
		QGraphicsItem *addAction(ThymioCard *item);
		QGraphicsItem *addEvent(ThymioCard *item);
		void addButtonSet(ThymioCard *event, ThymioCard *action);

		bool isEmpty() const;
		void reset();
		void clear();
		void haveAtLeastAnEmptyCard();
		void setColorScheme(QColor eventColor, QColor actionColor);
		bool isModified() const { return sceneModified; }
		void setModified(bool mod) { sceneModified=mod; }
		void setScale(qreal scale);
		void setAdvanced(bool advanced);
		bool getAdvanced() const { return advancedMode; }
		bool isAnyStateFilter() const;
		int getNumberOfButtonSets() const { return buttonSets.size(); }
		
		QString getErrorMessage() const;
		QList<QString> getCode() const;
		
		bool isSuccessful() const { return  thymioCompiler.isSuccessful(); }
		int getFocusItemId() const;
		
		typedef QList<ThymioButtonSet *>::iterator ButtonSetItr;
		typedef QList<ThymioButtonSet *>::const_iterator ButtonSetConstItr;
		
		ButtonSetItr buttonsBegin() { return buttonSets.begin(); }
		ButtonSetItr buttonsEnd() { return buttonSets.end(); }
		
	signals:
		void stateChanged();
		
	private slots:
		void buttonUpdateDetected();
		
	protected:
		virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);

		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	private:
		void removeButton(int row);
		void insertButton(int row);
		void rearrangeButtons(int row=0);
		
		ThymioButtonSet *createNewButtonSet();

		bool prevNewEventButton;
		bool prevNewActionButton;
		int lastFocus;
		
		QList<ThymioButtonSet *> buttonSets;
		ThymioCompiler thymioCompiler;
		
		QColor eventButtonColor;
		QColor actionButtonColor;
		// TODO: set this always through a function and emit a signal when it is changed, to update windows title (see issue 154)
		bool sceneModified;
		bool newRow;
		qreal buttonSetHeight;
		bool advancedMode;
	};
};

#endif
