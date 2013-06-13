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
		void addEventActionPair(Card *event, Card *action);

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
		
		QString getErrorMessage() const;
		QList<QString> getCode() const;
		
		bool isSuccessful() const { return  compiler.isSuccessful(); }
		int getSelectedPairId() const;
		EventActionPair *getSelectedPair() const;
		
		typedef QList<EventActionPair *>::iterator PairItr;
		typedef QList<EventActionPair *>::const_iterator PairConstItr;
		
		PairItr pairsBegin() { return eventActionPairs.begin(); }
		PairItr pairsEnd() { return eventActionPairs.end(); }
		
		PairConstItr pairsBegin() const { return eventActionPairs.begin(); }
		PairConstItr pairsEnd() const { return eventActionPairs.end(); }
		
	signals:
		void contentRecompiled();
		
	protected slots:
		void recompile();
		
	protected:
		virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);

		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	protected:
		void removePair(int row);
		void insertPair(int row);
		void rearrangeButtons(int row=0);
		
		EventActionPair *createNewEventActionPair();

		QList<EventActionPair *> eventActionPairs;
		Compiler compiler;
		
		QColor eventCardColor;
		QColor actionCardColor;
		// TODO: set this always through a function and emit a signal when it is changed, to update windows title (see issue 154)
		bool sceneModified;
		qreal buttonSetHeight;
		bool advancedMode;
	};
} } // namespace ThymioVPL / namespace Aseba

#endif
