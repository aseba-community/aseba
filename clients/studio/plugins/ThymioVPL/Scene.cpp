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

	QGraphicsItem *Scene::addAction(Block *item) 
	{
		EventActionsSet* selectedPair(getSelectedPair());
		if (selectedPair)
		{
			// add action to the selected pair, if not already present
			if (!selectedPair->hasActionBlock(item->getName()))
			{
				selectedPair->addActionBlock(item);
				ensureOneEmptyPairAtEnd();
			}
			return selectedPair;
		}
		else
		{
			qDebug() << item->getName();
			// find a pair without any action
			for (PairItr pItr(pairsBegin()); pItr != pairsEnd(); ++pItr)
			{
				EventActionsSet* p(*pItr);
				if (!p->hasActionBlock(item->getName()))
				{
					p->addActionBlock(item);
					clearSelection();
					ensureOneEmptyPairAtEnd();
					return p;
				}
			}
			// none found, add a new pair and acc action there
			EventActionsSet* p(createNewEventActionsSet());
			p->addActionBlock(item);
			clearSelection();
			ensureOneEmptyPairAtEnd();
			return p;
		}
	}

	QGraphicsItem *Scene::addEvent(Block *item) 
	{
		EventActionsSet* selectedPair(getSelectedPair());
		if (selectedPair)
		{
			// replaced action of selected pair
			selectedPair->addEventBlock(item);
			ensureOneEmptyPairAtEnd();
			return selectedPair;
		}
		else
		{
			// find a pair without any action
			for (PairItr pItr(pairsBegin()); pItr != pairsEnd(); ++pItr)
			{
				EventActionsSet* p(*pItr);
				if (!p->hasEventBlock())
				{
					p->addEventBlock(item);
					clearSelection();
					ensureOneEmptyPairAtEnd();
					return p;
				}
			}
			// none found, add a new pair and add event there
			EventActionsSet* p(createNewEventActionsSet());
			p->addEventBlock(item);
			clearSelection();
			ensureOneEmptyPairAtEnd();
			return p;
		}
	}
	
	// used by load, avoid recompiling
	void Scene::addEventActionsSet(Block *event, Block *action)
	{
		EventActionsSet *p(createNewEventActionsSet());
		disconnect(this, SIGNAL(selectionChanged()), this, SIGNAL(highlightChanged()));
		disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		if(event)
			p->addEventBlock(event);
		if(action)
			p->addActionBlock(action);
		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		connect(this, SIGNAL(selectionChanged()), SIGNAL(highlightChanged()));
	}
	
	void Scene::ensureOneEmptyPairAtEnd()
	{
		if (eventActionPairs.empty())
			createNewEventActionsSet();
		else if (!eventActionPairs.last()->isEmpty())
			createNewEventActionsSet();
		relayout();
	}

	EventActionsSet *Scene::createNewEventActionsSet()
	{
		EventActionsSet *p(new EventActionsSet(eventActionPairs.size(), advancedMode));
		eventActionPairs.push_back(p);
		
		addItem(p);
		recomputeSceneRect();
		
		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		
		return p;
	}

	bool Scene::isEmpty() const
	{
		if( eventActionPairs.isEmpty() )
			return true;
			
		if( eventActionPairs.size() > 1 ||
			!eventActionPairs[0]->isEmpty() )
			return false;

		return true;
	}

	void Scene::reset()
	{
		clear();
		createNewEventActionsSet();
	}
	
	void Scene::clear()
	{
		disconnect(this, SIGNAL(selectionChanged()), this, SIGNAL(highlightChanged()));
		for(int i=0; i<eventActionPairs.size(); i++)
		{
			EventActionsSet *p(eventActionPairs[i]);
			//disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
			removeItem(p);
			delete(p);
		}
		setSceneRect(QRectF());
		eventActionPairs.clear();
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

	void Scene::setAdvanced(bool advanced)
	{
		if (advanced != advancedMode)
		{
			advancedMode = advanced;
			for(PairItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
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
		for (PairConstItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
			if ((*itr)->isAnyAdvancedFeature())
				return true;
		return false;
	}

	void Scene::removePair(int row)
	{
		Q_ASSERT( row < eventActionPairs.size() );
		
		EventActionsSet *p(eventActionPairs[row]);
		disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		eventActionPairs.removeAt(row);
		
		removeItem(p);
		p->deleteLater();
		
		rearrangeSets(row);
		
		ensureOneEmptyPairAtEnd();
		
		recomputeSceneRect();
	
		recompile();
	}
	
	void Scene::insertPair(int row) 
	{
		Q_ASSERT( row <= eventActionPairs.size() );

		EventActionsSet *p(new EventActionsSet(row, advancedMode));
		eventActionPairs.insert(row, p);
		
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
		for(int i=0; i<eventActionPairs.size(); i++)
		{
			EventActionsSet *p(eventActionPairs[i]);
			r |= QRectF(p->scenePos(), p->boundingRect().size());
		}
		setSceneRect(r.adjusted(-40,-40,40,40));
		emit sceneSizeChanged();
	}

	void Scene::rearrangeSets(int row)
	{
		for(int i=row; i<eventActionPairs.size(); ++i) 
			eventActionPairs.at(i)->setRow(i);
	}
	
	void Scene::relayout()
	{
		// FIXME: doing this properly would require two pass, but we are not sure that we want to keep the zoom
		for(PairItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
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
			return tr("The event-action pair in line %0 is the same as in line %1").arg(compiler.getErrorLine()).arg(compiler.getSecondErrorLine());
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

	int Scene::getSelectedPairId() const 
	{ 
		EventActionsSet* selectedPair(getSelectedPair());
		if (selectedPair)
			return compiler.buttonToCode(selectedPair->data(1).toInt());
		else
			return -1;
	}
	
	EventActionsSet *Scene::getSelectedPair() const
	{
		if (selectedItems().empty())
			return 0;
		QGraphicsItem *item;
		foreach (item, selectedItems())
		{
			if (item->data(0).toString() == "eventactionpair")
			{
				return dynamic_cast<EventActionsSet*>(item);
			}
		}
		return 0;
	}
	
	EventActionsSet *Scene::getPairRow(int row) const
	{
		return eventActionPairs.at(row);
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
		
		for(int i=0; i<eventActionPairs.size(); i++)
			eventActionPairs[i]->setErrorStatus(false);
		
		if (!compiler.isSuccessful())
			eventActionPairs[compiler.getErrorLine()]->setErrorStatus(true);
		
		emit contentRecompiled();
	}
	
	void Scene::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("EventActionsSet") )
		{
			QByteArray buttonData = event->mimeData()->data("EventActionsSet");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);

			int prevRow, currentRow;
			dataStream >> prevRow;
			
			const unsigned count(pairsCount());
			EventActionsSet *p(eventActionPairs.at(prevRow));
			eventActionPairs.removeAt(prevRow);
			
			#define CLAMP(a, l, h) ((a)<(l)?(l):((a)>(h)?(h):(a)))

			const qreal currentXPos = event->scenePos().x();
			const qreal currentYPos = event->scenePos().y();
			const unsigned rowPerCol(ceilf(float(count)/float(zoomLevel)));
			const unsigned xIndex(CLAMP(currentXPos/1088, 0, zoomLevel-1));
			const unsigned yIndex(CLAMP(currentYPos/420, 0, rowPerCol));
			currentRow = xIndex*rowPerCol + yIndex;
			
			eventActionPairs.insert(currentRow, p);

			rearrangeSets( prevRow < currentRow ? prevRow : currentRow );
			
			ensureOneEmptyPairAtEnd();
			
			event->setDropAction(Qt::MoveAction);
			event->accept();
			
			recompile();
		}
		else
			QGraphicsScene::dropEvent(event);
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
