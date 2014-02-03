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
		
		QGraphicsItem *addAction(const QString& name);
		QGraphicsItem *addEvent(const QString& name);
		void addEventActionsSetOldFormat_1_3(const QDomElement& element);
		void addEventActionsSet(const QDomElement& element);
		void ensureOneEmptySetAtEnd();

		bool isEmpty() const;
		void reset();
		void clear();
		bool isModified() const { return sceneModified; }
		void setModified(bool mod);
		void setScale(qreal scale);
		void setAdvanced(bool advanced);
		bool getAdvanced() const { return advancedMode; }
		bool isAnyAdvancedFeature() const;
		
		QString getErrorMessage() const;
		QList<QString> getCode() const;
		
		const Compiler::CompilationResult& compilationResult() { return lastCompilationResult; }
		int getSelectedSetCodeId() const;
		EventActionsSet *getSelectedSet() const;
		EventActionsSet *getSetRow(int row) const;
		
		void removeSet(int row);
		void insertSet(int row);
		void recomputeSceneRect();
		
		typedef QList<EventActionsSet *>::iterator SetItr;
		typedef QList<EventActionsSet *>::const_iterator SetConstItr;
		
		SetItr setsBegin() { return eventActionsSets.begin(); }
		SetItr setsEnd() { return eventActionsSets.end(); }
		SetConstItr setsBegin() const { return eventActionsSets.begin(); }
		SetConstItr setsEnd() const { return eventActionsSets.end(); }
		unsigned setsCount() const { return eventActionsSets.size(); }
		
		unsigned getZoomLevel() const { return zoomLevel; }
		
	signals:
		void highlightChanged();
		void contentRecompiled();
		void modifiedStatusChanged(bool modified);
		void sceneSizeChanged();
		
	public slots:
		void recompile();
		void recompileWithoutSetModified();
		//void updateZoomLevel();
		
	protected:
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
		//virtual void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);

	protected:
		void rearrangeSets(int row=0);
		void relayout();
		void addEventActionsSet(EventActionsSet *eventActionsSet);
		EventActionsSet *createNewEventActionsSet();
		
	protected:
		ThymioVisualProgramming* vpl;
		
		QList<EventActionsSet *> eventActionsSets;
		Compiler compiler;
		Compiler::CompilationResult lastCompilationResult;
		
		// TODO: set this always through a function and emit a signal when it is changed, to update windows title (see issue 154)
		bool sceneModified;
		qreal buttonSetHeight;
		bool advancedMode;
		unsigned zoomLevel;
	};
} } // namespace ThymioVPL / namespace Aseba

#endif
