#ifndef VPL_SCENE_H
#define VPL_SCENE_H

#include <QGraphicsScene>
#include "EventActionPair.h"

namespace Aseba { namespace ThymioVPL
{
	class Scene : public QGraphicsScene
	{
		Q_OBJECT
		
	public:
		Scene(QObject *parent = 0);
		~Scene();
		
		QGraphicsItem *addAction(Card *item);
		QGraphicsItem *addEvent(Card *item);
		void addButtonSet(Card *event, Card *action);

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
		
		typedef QList<EventActionPair *>::iterator ButtonSetItr;
		typedef QList<EventActionPair *>::const_iterator ButtonSetConstItr;
		
		ButtonSetItr buttonsBegin() { return buttonSets.begin(); }
		ButtonSetItr buttonsEnd() { return buttonSets.end(); }
		
		ButtonSetConstItr buttonsBegin() const { return buttonSets.begin(); }
		ButtonSetConstItr buttonsEnd() const { return buttonSets.end(); }
		
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
		
		EventActionPair *createNewButtonSet();

		bool prevNewEventButton;
		bool prevNewActionButton;
		int lastFocus;
		
		QList<EventActionPair *> buttonSets;
		ThymioCompiler thymioCompiler;
		
		QColor eventButtonColor;
		QColor actionButtonColor;
		// TODO: set this always through a function and emit a signal when it is changed, to update windows title (see issue 154)
		bool sceneModified;
		bool newRow;
		qreal buttonSetHeight;
		bool advancedMode;
	};
} } // namespace ThymioVPL / namespace Aseba

#endif
