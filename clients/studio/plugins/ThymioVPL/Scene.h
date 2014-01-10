#ifndef VPL_SCENE_H
#define VPL_SCENE_H

#include <QGraphicsScene>
#include "EventActionsSet.h"

namespace Aseba { namespace ThymioVPL
{
	class ThymioVisualProgramming;
	
	class Scene : public QGraphicsScene
	{
		Q_OBJECT
		
	public:
		Scene(ThymioVisualProgramming *vpl);
		~Scene();
		
		QGraphicsItem *addAction(Block *item);
		QGraphicsItem *addEvent(Block *item);
		void addEventActionsSet(Block *event, Block *action);
		void ensureOneEmptyPairAtEnd();

		bool isEmpty() const;
		void reset();
		void clear();
		void setColorScheme(QColor eventColor, QColor actionColor);
		bool isModified() const { return sceneModified; }
		void setModified(bool mod);
		void setScale(qreal scale);
		void setAdvanced(bool advanced);
		bool getAdvanced() const { return advancedMode; }
		bool isAnyAdvancedFeature() const;
		
		QString getErrorMessage() const;
		QList<QString> getCode() const;
		
		bool isSuccessful() const { return compiler.isSuccessful(); }
		int getErrorLine() const { return compiler.getErrorLine(); }
		int getSelectedPairId() const;
		EventActionsSet *getSelectedPair() const;
		EventActionsSet *getPairRow(int row) const;
		
		void removePair(int row);
		void insertPair(int row);
		void recomputeSceneRect();
		
		typedef QList<EventActionsSet *>::iterator PairItr;
		typedef QList<EventActionsSet *>::const_iterator PairConstItr;
		
		PairItr pairsBegin() { return eventActionPairs.begin(); }
		PairItr pairsEnd() { return eventActionPairs.end(); }
		PairConstItr pairsBegin() const { return eventActionPairs.begin(); }
		PairConstItr pairsEnd() const { return eventActionPairs.end(); }
		unsigned pairsCount() const { return eventActionPairs.size(); }
		
		unsigned getZoomLevel() const { return zoomLevel; }
		
	signals:
		void highlightChanged();
		void contentRecompiled();
		void modifiedStatusChanged(bool modified);
		void zoomChanged();
		
	public slots:
		void recompile();
		void recompileWithoutSetModified();
		void updateZoomLevel();
		
	protected:
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
		virtual void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);

	protected:
		void rearrangeButtons(int row=0);
		void relayout();
		
		EventActionsSet *createNewEventActionsSet();

		ThymioVisualProgramming* vpl;
		
		QList<EventActionsSet *> eventActionPairs;
		Compiler compiler;
		
		QColor eventBlockColor;
		QColor actionBlockColor;
		// TODO: set this always through a function and emit a signal when it is changed, to update windows title (see issue 154)
		bool sceneModified;
		qreal buttonSetHeight;
		bool advancedMode;
		unsigned zoomLevel;
	};
} } // namespace ThymioVPL / namespace Aseba

#endif
