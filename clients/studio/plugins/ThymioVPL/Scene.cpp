#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QDebug>
#include <cmath>

#include "Scene.h"
#include "Block.h"
#include "ThymioVisualProgramming.h"

using namespace std;

namespace Aseba { namespace ThymioVPL
{

	Scene::Scene(ThymioVisualProgramming *vpl) : 
		QGraphicsScene(vpl),
		vpl(vpl),
		compiler(*this),
		sceneModified(false),
		advancedMode(false),
		zoomLevel(1)
	{
		// create initial set
		EventActionsSet *p(createNewEventActionsSet());
		buttonSetHeight = p->boundingRect().height();
		
		connect(this, SIGNAL(selectionChanged()), SIGNAL(highlightChanged()));
	}
	
	Scene::~Scene()
	{
		clear();
	}
	
	//! Add an action block of a given name to the scene, find the right place to put it
	QGraphicsItem *Scene::addAction(const QString& name) 
	{
		Block* item(Block::createBlock(name, advancedMode));
		EventActionsSet* selectedSet(getSelectedSet());
		if (selectedSet)
		{
			// add action to the selected set, if not already present
			if (!selectedSet->hasActionBlock(item->getName()))
			{
				selectedSet->addActionBlock(item);
				ensureOneEmptySetAtEnd();
			}
			return selectedSet;
		}
		else
		{
			// find a set without any action
			for (SetItr itr(setsBegin()); itr != setsEnd(); ++itr)
			{
				EventActionsSet* eventActionsSet(*itr);
				if (!eventActionsSet->hasActionBlock(item->getName()))
				{
					eventActionsSet->addActionBlock(item);
					clearSelection();
					ensureOneEmptySetAtEnd();
					return eventActionsSet;
				}
			}
			// none found, add a new set and acc action there
			EventActionsSet* eventActionsSet(createNewEventActionsSet());
			eventActionsSet->addActionBlock(item);
			clearSelection();
			ensureOneEmptySetAtEnd();
			return eventActionsSet;
		}
	}

	//! Add an event block of a given name to the scene, find the right place to put it
	QGraphicsItem *Scene::addEvent(const QString& name) 
	{
		Block* item(Block::createBlock(name, advancedMode));
		EventActionsSet* selectedSet(getSelectedSet());
		if (selectedSet)
		{
			// replaced action of selected set
			selectedSet->addEventBlock(item);
			ensureOneEmptySetAtEnd();
			return selectedSet;
		}
		else
		{
			// find a set without any action
			for (SetItr itr(setsBegin()); itr != setsEnd(); ++itr)
			{
				EventActionsSet* eventActionsSet(*itr);
				if (!eventActionsSet->hasEventBlock())
				{
					eventActionsSet->addEventBlock(item);
					clearSelection();
					ensureOneEmptySetAtEnd();
					return eventActionsSet;
				}
			}
			// none found, add a new set and add event there
			EventActionsSet* eventActionsSet(createNewEventActionsSet());
			eventActionsSet->addEventBlock(item);
			clearSelection();
			ensureOneEmptySetAtEnd();
			return eventActionsSet;
		}
	}
	
	//! Add a new event-actions set from a DOM Element, used by load
	void Scene::addEventActionsSet(const QDomElement& element)
	{
		EventActionsSet *eventActionsSet(new EventActionsSet(eventActionsSets.size(), advancedMode));
		eventActionsSet->deserialize(element);
		addEventActionsSet(eventActionsSet);
	}
	
	//! Add a new event-actions set from a DOM Element in 1.3 format, used by load
	void Scene::addEventActionsSetOldFormat_1_3(const QDomElement& element)
	{
		EventActionsSet *eventActionsSet(new EventActionsSet(eventActionsSets.size(), advancedMode));
		eventActionsSet->deserializeOldFormat_1_3(element);
		addEventActionsSet(eventActionsSet);
	}
	
	//! Makes sure that there is at least one empty event-actions set at the end of the scene
	void Scene::ensureOneEmptySetAtEnd()
	{
		if (eventActionsSets.empty())
			createNewEventActionsSet();
		else if (!eventActionsSets.last()->isEmpty())
			createNewEventActionsSet();
		relayout();
	}

	//! Create a new event-actions set and adds it
	EventActionsSet *Scene::createNewEventActionsSet()
	{
		EventActionsSet *eventActionsSet(new EventActionsSet(eventActionsSets.size(), advancedMode));
		addEventActionsSet(eventActionsSet);
		return eventActionsSet;
	}
	
	//! Adds an existing event-actions set, *must not* be in the scene already
	void Scene::addEventActionsSet(EventActionsSet *eventActionsSet)
	{
		eventActionsSets.push_back(eventActionsSet);
		
		addItem(eventActionsSet);
		recomputeSceneRect();
		
		connect(eventActionsSet, SIGNAL(contentChanged()), this, SLOT(recompile()));
	}

	//! Return whether the scene is as when started
	bool Scene::isEmpty() const
	{
		if( eventActionsSets.isEmpty() )
			return true;
			
		if( eventActionsSets.size() > 1 ||
			!eventActionsSets[0]->isEmpty() )
			return false;

		return true;
	}

	//! Reset the scene to an empty one
	void Scene::reset()
	{
		clear();
		createNewEventActionsSet();
	}
	
	//! Remove everything from the scene, leaving it with no object (hence not usable directly)
	void Scene::clear()
	{
		disconnect(this, SIGNAL(selectionChanged()), this, SIGNAL(highlightChanged()));
		for(int i=0; i<eventActionsSets.size(); i++)
		{
			EventActionsSet *p(eventActionsSets[i]);
			//disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
			removeItem(p);
			delete(p);
		}
		setSceneRect(QRectF());
		eventActionsSets.clear();
		compiler.clear();
		
		connect(this, SIGNAL(selectionChanged()), SIGNAL(highlightChanged()));
		
		setModified(false);
		
		advancedMode = false;
	}
	
	void Scene::setModified(bool mod)
	{
		sceneModified = mod;
		emit modifiedStatusChanged(sceneModified);
	}

	//! Switch the scene to and from advanced mode
	void Scene::setAdvanced(bool advanced)
	{
		if (advanced != advancedMode)
		{
			advancedMode = advanced;
			for(SetItr itr(setsBegin()); itr != setsEnd(); ++itr)
			{
				EventActionsSet* p(*itr);
				p->disconnect(SIGNAL(contentChanged()), this, SLOT(recompile()));
				p->setAdvanced(advanced);
				connect(p, SIGNAL(contentChanged()), SLOT(recompile()));
			}
			
			recompile();
		}
	}
	
	bool Scene::isAnyAdvancedFeature() const
	{
		for (SetConstItr itr(setsBegin()); itr != setsEnd(); ++itr)
			if ((*itr)->isAnyAdvancedFeature())
				return true;
		return false;
	}

	void Scene::removeSet(int row)
	{
		Q_ASSERT( row < eventActionsSets.size() );
		
		EventActionsSet *p(eventActionsSets[row]);
		disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		eventActionsSets.removeAt(row);
		
		removeItem(p);
		p->deleteLater();
		
		rearrangeSets(row);
		
		ensureOneEmptySetAtEnd();
		
		recomputeSceneRect();
	
		recompile();
	}
	
	void Scene::insertSet(int row) 
	{
		Q_ASSERT( row <= eventActionsSets.size() );

		EventActionsSet *p(new EventActionsSet(row, advancedMode));
		eventActionsSets.insert(row, p);
		
		addItem(p);
		
		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		
		rearrangeSets(row+1);
		relayout();
		
		recompile();
		
		p->setSoleSelection();
		
		QGraphicsView* view;
		foreach (view, views())
			view->ensureVisible(p);

		setModified(true);
	}
	
	void Scene::recomputeSceneRect()
	{
		QRectF r;
		for(int i=0; i<eventActionsSets.size(); i++)
		{
			EventActionsSet *p(eventActionsSets[i]);
			r |= QRectF(p->scenePos(), p->boundingRect().size());
		}
		setSceneRect(r.adjusted(-40,-40,40,40));
		emit sceneSizeChanged();
	}

	void Scene::rearrangeSets(int row)
	{
		for(int i=row; i<eventActionsSets.size(); ++i) 
			eventActionsSets.at(i)->setRow(i);
	}
	
	void Scene::relayout()
	{
		// FIXME: doing this properly would require two pass, but we are not sure that we want to keep the zoom
		for(SetItr itr(setsBegin()); itr != setsEnd(); ++itr)
			(*itr)->repositionElements();
		recomputeSceneRect();
	}
	
	QString Scene::getErrorMessage() const
	{ 
		switch(compiler.getErrorCode())
		{
		case Compiler::MISSING_EVENT:
			return tr("Line %0: Missing event").arg(compiler.getErrorLine());
			break;
		case Compiler::MISSING_ACTION:
			return tr("Line %0: Missing action").arg(compiler.getErrorLine());
			break;
		case Compiler::EVENT_REPEATED:
			return tr("The event-action set in line %0 is the same as in line %1").arg(compiler.getErrorLine()).arg(compiler.getSecondErrorLine());
			break;
			// 
		case Compiler::INVALID_CODE:
			return tr("Line %0: Unknown event/action type").arg(compiler.getErrorLine());
			break;
		case Compiler::NO_ERROR:
			return tr("Compilation success");
		default:
			return tr("Unknown VPL error");
		}
	}
	
	QList<QString> Scene::getCode() const
	{
		QList<QString> out;
		
		for(std::vector<std::wstring>::const_iterator itr = compiler.beginCode();
			itr != compiler.endCode(); ++itr)
			out.push_back(QString::fromStdWString(*itr));
	
		return out;
	}

	int Scene::getSelectedSetId() const 
	{ 
		EventActionsSet* selectedSet(getSelectedSet());
		if (selectedSet)
			return compiler.buttonToCode(selectedSet->getRow());
		else
			return -1;
	}
	
	EventActionsSet *Scene::getSelectedSet() const
	{
		if (selectedItems().empty())
			return 0;
		QGraphicsItem *item;
		foreach (item, selectedItems())
		{
			EventActionsSet* eventActionsSet(dynamic_cast<EventActionsSet*>(item));
			if (eventActionsSet)
				return eventActionsSet;
		}
		return 0;
	}
	
	EventActionsSet *Scene::getSetRow(int row) const
	{
		return eventActionsSets.at(row);
	}
	
	void Scene::recompile()
	{
		setModified(true);
		recompileWithoutSetModified();
	}
	
	void Scene::recompileWithoutSetModified()
	{
		compiler.compile();
		compiler.generateCode();
		
		for(int i=0; i<eventActionsSets.size(); i++)
			eventActionsSets[i]->setErrorStatus(false);
		
		if (!compiler.isSuccessful())
			eventActionsSets[compiler.getErrorLine()]->setErrorStatus(true);
		
		emit contentRecompiled();
	}
	
	void Scene::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		// FIXME: do we still need that?
		qDebug() << "Scene::dropEvent";
		qDebug() << event->isAccepted();
		QGraphicsScene::dropEvent(event);
		qDebug() << event->isAccepted();
		if (!event->isAccepted() && event->mimeData()->hasFormat("EventActionsSet") )
		{
			qDebug() << "dropped on scene";
			/*QByteArray buttonData = event->mimeData()->data("EventActionsSet");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);

			int prevRow, currentRow;
			dataStream >> prevRow;
			
			const unsigned count(setsCount());
			EventActionsSet *p(eventActionsSets.at(prevRow));
			eventActionsSets.removeAt(prevRow);
			
			#define CLAMP(a, l, h) ((a)<(l)?(l):((a)>(h)?(h):(a)))

			const qreal currentXPos = event->scenePos().x();
			const qreal currentYPos = event->scenePos().y();
			const unsigned rowPerCol(ceilf(float(count)/float(zoomLevel)));
			const unsigned xIndex(CLAMP(currentXPos/1088, 0, zoomLevel-1));
			const unsigned yIndex(CLAMP(currentYPos/420, 0, rowPerCol));
			currentRow = xIndex*rowPerCol + yIndex;
			
			eventActionsSets.insert(currentRow, p);

			rearrangeSets( prevRow < currentRow ? prevRow : currentRow );
			
			ensureOneEmptySetAtEnd();
			
			event->setDropAction(Qt::MoveAction);
			event->accept();
			
			recompile();
			*/
		}
	}
	
	void Scene::wheelEvent(QGraphicsSceneWheelEvent * wheelEvent)
	{
		if (wheelEvent->modifiers() & Qt::ControlModifier)
		{
			if (wheelEvent->delta() > 0)
				vpl->zoomSlider->setValue(vpl->zoomSlider->value()-1);
			else if (wheelEvent->delta() < 0)
				vpl->zoomSlider->setValue(vpl->zoomSlider->value()+1);
			wheelEvent->accept();
		}
	}
	
	void Scene::updateZoomLevel()
	{
		// TODO: if we keep slider, use its value instead of doing copies
		zoomLevel = vpl->zoomSlider->value();
		relayout();
	}
} } // namespace ThymioVPL / namespace Aseba
