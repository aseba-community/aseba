#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QDebug>
#include <cmath>

#include "Scene.h"
#include "Card.h"

using namespace std;

namespace Aseba { namespace ThymioVPL
{

	Scene::Scene(QObject *parent) : 
		QGraphicsScene(parent),
		compiler(*this),
		eventCardColor(QColor(0,191,255)),
		actionCardColor(QColor(218,112,214)),
		sceneModified(false),
		advancedMode(false),
		zoomLevel(1)
	{
		// create initial button
		EventActionPair *p(createNewEventActionPair());
		buttonSetHeight = p->boundingRect().height();
		
		connect(this, SIGNAL(selectionChanged()), SIGNAL(highlightChanged()));
	}
	
	Scene::~Scene()
	{
		clear();
	}

	QGraphicsItem *Scene::addAction(Card *item) 
	{
		EventActionPair* selectedPair(getSelectedPair());
		if (selectedPair)
		{
			// replaced action of selected pair
			selectedPair->addActionCard(item);
			ensureOneEmptyPairAtEnd();
			return selectedPair;
		}
		else
		{
			// find a pair without any action
			for (PairItr pItr(pairsBegin()); pItr != pairsEnd(); ++pItr)
			{
				EventActionPair* p(*pItr);
				if (!p->hasActionCard())
				{
					p->addActionCard(item);
					clearSelection();
					ensureOneEmptyPairAtEnd();
					return p;
				}
			}
			// none found, add a new pair and acc action there
			EventActionPair* p(createNewEventActionPair());
			p->addActionCard(item);
			clearSelection();
			ensureOneEmptyPairAtEnd();
			return p;
		}
	}

	QGraphicsItem *Scene::addEvent(Card *item) 
	{
		EventActionPair* selectedPair(getSelectedPair());
		if (selectedPair)
		{
			// replaced action of selected pair
			selectedPair->addEventCard(item);
			ensureOneEmptyPairAtEnd();
			return selectedPair;
		}
		else
		{
			// find a pair without any action
			for (PairItr pItr(pairsBegin()); pItr != pairsEnd(); ++pItr)
			{
				EventActionPair* p(*pItr);
				if (!p->hasEventCard())
				{
					p->addEventCard(item);
					clearSelection();
					ensureOneEmptyPairAtEnd();
					return p;
				}
			}
			// none found, add a new pair and add event there
			EventActionPair* p(createNewEventActionPair());
			p->addEventCard(item);
			clearSelection();
			ensureOneEmptyPairAtEnd();
			return p;
		}
	}
	
	// used by load, avoid recompiling
	void Scene::addEventActionPair(Card *event, Card *action)
	{
		EventActionPair *p(createNewEventActionPair());
		disconnect(this, SIGNAL(selectionChanged()), this, SIGNAL(highlightChanged()));
		disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		if(event)
			p->addEventCard(event);
		if(action)
			p->addActionCard(action);
		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		connect(this, SIGNAL(selectionChanged()), SIGNAL(highlightChanged()));
	}
	
	void Scene::ensureOneEmptyPairAtEnd()
	{
		if (eventActionPairs.empty())
			createNewEventActionPair();
		else if (!eventActionPairs.last()->isEmpty())
			createNewEventActionPair();
		relayout();
	}

	EventActionPair *Scene::createNewEventActionPair()
	{
		EventActionPair *p(new EventActionPair(eventActionPairs.size(), advancedMode));
		p->setColorScheme(eventCardColor, actionCardColor);
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
			eventActionPairs[0]->hasActionCard() ||
			eventActionPairs[0]->hasEventCard() )
			return false;

		return true;
	}

	void Scene::reset()
	{
		clear();
		createNewEventActionPair();
	}
	
	void Scene::clear()
	{
		disconnect(this, SIGNAL(selectionChanged()), this, SIGNAL(highlightChanged()));
		for(int i=0; i<eventActionPairs.size(); i++)
		{
			EventActionPair *p(eventActionPairs[i]);
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
	
	void Scene::setColorScheme(QColor eventColor, QColor actionColor)
	{
		eventCardColor = eventColor;
		actionCardColor = actionColor;

		for(PairItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
			(*itr)->setColorScheme(eventColor, actionColor);

		update();
	
		setModified(true);
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
				EventActionPair* p(*itr);
				p->disconnect(SIGNAL(contentChanged()), this, SLOT(recompile()));
				p->setAdvanced(advanced);
				connect(p, SIGNAL(contentChanged()), SLOT(recompile()));
			}
			
			recompile();
		}
	}
	
	bool Scene::isAnyStateFilter() const
	{
		for (PairConstItr itr(pairsBegin()); itr != pairsEnd(); ++itr)
			if ((*itr)->isAnyStateFilter())
				return true;
		return false;
	}

	void Scene::removePair(int row)
	{
		Q_ASSERT( row < eventActionPairs.size() );
		
		EventActionPair *p(eventActionPairs[row]);
		disconnect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		eventActionPairs.removeAt(row);
		
		removeItem(p);
		p->deleteLater();
		
		rearrangeButtons(row);
		
		ensureOneEmptyPairAtEnd();
		
		recomputeSceneRect();
	
		recompile();
	}
	
	void Scene::insertPair(int row) 
	{
		Q_ASSERT( row <= eventActionPairs.size() );

		EventActionPair *p(new EventActionPair(row, advancedMode));
		p->setColorScheme(eventCardColor, actionCardColor);
		eventActionPairs.insert(row, p);
		
		addItem(p);
		
		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		
		rearrangeButtons(row+1);
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
			EventActionPair *p(eventActionPairs[i]);
			r |= QRectF(p->scenePos(), p->boundingRect().size());
		}
		setSceneRect(r.adjusted(0,-20,0,20));
	}

	void Scene::rearrangeButtons(int row)
	{
		for(int i=row; i<eventActionPairs.size(); ++i) 
			eventActionPairs.at(i)->setRow(i);
	}
	
	void Scene::relayout()
	{
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
		EventActionPair* selectedPair(getSelectedPair());
		if (selectedPair)
			return compiler.buttonToCode(selectedPair->data(1).toInt());
		else
			return -1;
	}
	
	EventActionPair *Scene::getSelectedPair() const
	{
		if (selectedItems().empty())
			return 0;
		QGraphicsItem *item;
		foreach (item, selectedItems())
		{
			if (item->data(0).toString() == "eventactionpair")
			{
				return dynamic_cast<EventActionPair*>(item);
			}
		}
		return 0;
	}
	
	EventActionPair *Scene::getPairRow(int row) const
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
		if ( event->mimeData()->hasFormat("EventActionPair") )
		{
			QByteArray buttonData = event->mimeData()->data("EventActionPair");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);

			int prevRow, currentRow;
			dataStream >> prevRow;
			
			const unsigned count(pairsCount());
			EventActionPair *p(eventActionPairs.at(prevRow));
			eventActionPairs.removeAt(prevRow);
			
			#define CLAMP(a, l, h) ((a)<(l)?(l):((a)>(h)?(h):(a)))

			const qreal currentXPos = event->scenePos().x();
			const qreal currentYPos = event->scenePos().y();
			const unsigned rowPerCol(ceilf(float(count)/float(zoomLevel)));
			const unsigned xIndex(CLAMP(currentXPos/1088, 0, zoomLevel-1));
			const unsigned yIndex(CLAMP(currentYPos/420, 0, rowPerCol));
			currentRow = xIndex*rowPerCol + yIndex;
			
			eventActionPairs.insert(currentRow, p);

			rearrangeButtons( prevRow < currentRow ? prevRow : currentRow );
			
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
			bool changed(false);
			if (wheelEvent->delta() > 0 && zoomLevel > 1)
			{
				zoomLevel--;
				changed = true;
			}
			else if (wheelEvent->delta() < 0 && zoomLevel < 10)
			{
				zoomLevel++;
				changed = true;
			}
			if (changed)
			{
				relayout();
				emit zoomChanged();
				wheelEvent->accept();
			}
		}
	}
} } // namespace ThymioVPL / namespace Aseba
