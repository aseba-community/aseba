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

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QMessageBox>
#include <QDebug>
#include <cmath>

#include "Scene.h"
#include "Block.h"
#include "ThymioVisualProgramming.h"
#include "Style.h"
#include "UsageLogger.h"

using namespace std;

namespace Aseba { namespace ThymioVPL
{

	Scene::Scene(ThymioVisualProgramming *vpl) : 
		QGraphicsScene(vpl),
		vpl(vpl),
		warningGraphicsItem(new QGraphicsSvgItem(":/images/vpl/missing_block.svgz")),
		errorGraphicsItem(new QGraphicsSvgItem(":/images/vpl/error.svgz")),
		referredGraphicsItem(new QGraphicsSvgItem(":/images/vpl/error.svgz")),
		referredLineItem(addLine(0, 0, 0, 0, QPen(Qt::red, 8, Qt::DashLine))),
		sceneModified(false),
		advancedMode(false),
		zoomLevel(1)
	{
		addItem(warningGraphicsItem);
		warningGraphicsItem->setVisible(false);
		addItem(errorGraphicsItem);
		errorGraphicsItem->setVisible(false);
		addItem(referredGraphicsItem);
		referredGraphicsItem->setVisible(false);
		referredLineItem->setVisible(false);
		
		// create initial set
		EventActionsSet *p(createNewEventActionsSet());
		buttonSetHeight = p->boundingRect().height();
		
		connect(this, SIGNAL(selectionChanged()), SIGNAL(highlightChanged()));
		USAGE_LOG(setScene(this));
	}
	
	Scene::~Scene()
	{
		clear(false);
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
				//if (!eventActionsSet->hasActionBlock(item->getName()))
				if (!eventActionsSet->hasAnyActionBlock())
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
	
	//! Makes sure that there is at least one empty event-actions set at the end of the scene
	void Scene::ensureOneEmptySetAtEnd()
	{
		EventActionsSet *newSet(0);
		if (eventActionsSets.empty())
			newSet = createNewEventActionsSet();
		else if (!eventActionsSets.last()->isEmpty())
			newSet = createNewEventActionsSet();
		
		if (newSet)
		{
			relayout();
		
			// make sure the newly set is visible
			QGraphicsView* view;
			foreach (view, views())
				view->ensureVisible(newSet);
		}
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
		connect(eventActionsSet, SIGNAL(undoCheckpoint()), this, SIGNAL(undoCheckpoint()));
	}
	
	//! Return whether we have to generate debug log information
	bool Scene::debugLog() const
	{
		return vpl->debugLog;
	}

	//! Return whether the scene is as when started
	bool Scene::isEmpty() const
	{
		if (eventActionsSets.isEmpty())
			return true;
			
		if (eventActionsSets.size() > 1 ||
			!eventActionsSets[0]->isEmpty())
			return false;

		return true;
	}

	//! Reset the scene to an empty one
	void Scene::reset()
	{
		clear(advancedMode);
		createNewEventActionsSet();
		lastCompilationResult = compiler.compile(this);
	}
	
	//! Remove everything from the scene, leaving it with no object (hence not usable directly), and set advanced mode or not
	void Scene::clear(bool advanced)
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
		// hide any error
		warningGraphicsItem->setVisible(false);
		errorGraphicsItem->setVisible(false);
		referredGraphicsItem->setVisible(false);
		referredLineItem->setVisible(false);
		
		connect(this, SIGNAL(selectionChanged()), SIGNAL(highlightChanged()));
		
		setModified(false);
		
		advancedMode = advanced;
	}
	
	//! Set the modified boolean to mod, emit status change
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
			USAGE_LOG(logSetAdvanced(advanced));
			emit undoCheckpoint();
		}
	}
	
	//! Return whether the scene is using any advanced feature
	bool Scene::isAnyAdvancedFeature() const
	{
		for (SetConstItr itr(setsBegin()); itr != setsEnd(); ++itr)
			if ((*itr)->isAnyAdvancedFeature())
				return true;
		return false;
	}
	
	//! Serialize the scene to DOM
	QDomElement Scene::serialize(QDomDocument& document) const
	{
		QDomElement program = document.createElement("program");
		program.setAttribute("advanced_mode", advancedMode);
		
		for (Scene::SetConstItr itr(setsBegin()); itr != setsEnd(); ++itr)
			program.appendChild((*itr)->serialize(document));
		
		return program;
	}
	
	//! Clear the existing scene and deserialize from DOM
	void Scene::deserialize(const QDomElement& programElement)
	{
		// remove current content and set mode
		clear(programElement.attribute("advanced_mode", "false").toInt());
		
		// deserialize
		QDomElement element(programElement.firstChildElement());
		while (!element.isNull())
		{
			if (element.tagName() == "set")
			{
				addEventActionsSet(element);
			}
			
			element = element.nextSiblingElement();
		}
		
		// reset visuals
		ensureOneEmptySetAtEnd();
	}
	
	//! Serialize to string through DOM document
	QString Scene::toString() const
	{
		QDomDocument document("scene");
		document.appendChild(serialize(document));
		return document.toString();
	}
	
	//! Deserialize from string through DOM document
	void Scene::fromString(const QString& text)
	{
		QDomDocument document("scene");
		document.setContent(text);
		deserialize(document.firstChildElement());
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
		USAGE_LOG(logRemoveSet(row));
		emit undoCheckpoint();
	}
	
	void Scene::insertSet(int row) 
	{
		Q_ASSERT( row <= eventActionsSets.size() );

		EventActionsSet *p(new EventActionsSet(row, advancedMode));
		eventActionsSets.insert(row, p);
		
		addItem(p);
		
		connect(p, SIGNAL(contentChanged()), this, SLOT(recompile()));
		connect(p, SIGNAL(undoCheckpoint()), this, SIGNAL(undoCheckpoint()));
		
		rearrangeSets(row+1);
		relayout();
		
		recompile();
		USAGE_LOG(logInsertSet(row));
		
		emit undoCheckpoint();
		
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
		setSceneRect(r.adjusted(-(40+errorGraphicsItem->boundingRect().width()+40),-40,40,Style::addRemoveButtonHeight+Style::blockSpacing+40));
		emit sceneSizeChanged();
	}
	
	//! Is a set, given by its id, the last non-empty set of the scene?
	bool Scene::isSetLast(unsigned setId) const
	{
		if (setId+1 >= (unsigned)eventActionsSets.size())
			return true;
		for (unsigned i=setId+1; i<(unsigned)eventActionsSets.size(); ++i)
			if (!eventActionsSets[i]->isEmpty())
				return false;
		return true;
	}
	
	/*void Scene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
	{
		Scene::dragEnterEvent(event);
		if (isDnDValid(event))
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
	}
	
	void Scene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
	{
		Scene::dragMoveEvent(event);
		if (isDnDValid(event))
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
	}
	
	void Scene::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		Scene::dropEvent(event);
		if (isDnDValid(event))
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
	}
	
	bool Scene::isDnDValid(QGraphicsSceneDragDropEvent *event) const
	{
		if (event->mimeData()->hasFormat("EventActionsSet") ||
			event->mimeData()->hasFormat("Block")
		)
		{
			if (items(event->pos(), Qt::ContainsItemBoundingRect, Qt::AscendingOrder).empty())
				return true;
		}
		return false;
	}*/

	void Scene::rearrangeSets(int row)
	{
		for(int i=row; i<eventActionsSets.size(); ++i) 
			eventActionsSets.at(i)->setRow(i);
	}
	
	void Scene::relayout()
	{
		for(SetItr itr(setsBegin()); itr != setsEnd(); ++itr)
			(*itr)->repositionElements();
		recomputeSceneRect();
	}
	
	QList<QString> Scene::getCode() const
	{
		QList<QString> out;
		
		for (std::vector<std::wstring>::const_iterator itr = compiler.beginCode();
			itr != compiler.endCode(); ++itr)
			out.push_back(QString::fromStdWString(*itr));
	
		return out;
	}

	int Scene::getSelectedSetCodeId() const 
	{ 
		EventActionsSet* selectedSet(getSelectedSet());
		if (selectedSet)
			return compiler.getSetToCodeIdMap(selectedSet->getRow());
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
		//qDebug() << "recompiling";
		lastCompilationResult = compiler.compile(this);
		
		const bool isWarning(
			isSetLast(lastCompilationResult.errorLine) &&
			((lastCompilationResult.errorType == Compiler::MISSING_ACTION) || (lastCompilationResult.errorType == Compiler::MISSING_EVENT))
		);
		
		warningGraphicsItem->setVisible(isWarning);
		errorGraphicsItem->setVisible(!isWarning && lastCompilationResult.errorLine != -1);
		referredGraphicsItem->setVisible(lastCompilationResult.referredLine != -1);
		referredLineItem->setVisible(lastCompilationResult.referredLine != -1);

		EventActionsSet* warningEventActionsSet(0);
		EventActionsSet* errorEventActionsSet(0);
		EventActionsSet* referredEventActionsSet(0);
		for (int i = 0; i < eventActionsSets.size(); ++i)
		{
			EventActionsSet* eventActionsSet(eventActionsSets[i]);
			Compiler::ErrorType errorType;
			if (i == lastCompilationResult.errorLine)
			{
				errorType = lastCompilationResult.errorType;
				if (isWarning)
					warningEventActionsSet = eventActionsSet;
				else
					errorEventActionsSet = eventActionsSet;
			}
			else if (i == lastCompilationResult.referredLine)
			{
				errorType = lastCompilationResult.errorType;
				referredEventActionsSet = eventActionsSet;
			}
			else
			{
				errorType = Compiler::NO_ERROR;
			}
			eventActionsSet->setErrorType(errorType);
		}

		// force update when error notification changed position to work around issue 360
		bool forceUpdate(false);
		if (warningEventActionsSet)
		{
			QSizeF errorSize(errorGraphicsItem->boundingRect().size());
			
			qreal errorY(warningEventActionsSet->scenePos().y() + warningEventActionsSet->innerBoundingRect().height() / 2);
			qreal oldPosY(warningGraphicsItem->pos().y());
			warningGraphicsItem->setPos(-(errorSize.width() + 40), errorY - errorSize.height() / 2);
			if (warningGraphicsItem->pos().y() != oldPosY)
				forceUpdate = true;
		}
		if (errorEventActionsSet)
		{
			QSizeF errorSize(errorGraphicsItem->boundingRect().size());
			
			qreal errorY(errorEventActionsSet->scenePos().y() + errorEventActionsSet->innerBoundingRect().height() / 2);
			qreal oldPosY(errorGraphicsItem->pos().y());
			errorGraphicsItem->setPos(-(errorSize.width() + 40), errorY - errorSize.height() / 2);
			if (errorGraphicsItem->pos().y() != oldPosY)
				forceUpdate = true;
			
			if (referredEventActionsSet)
			{
				qreal referredY(referredEventActionsSet->scenePos().y() + referredEventActionsSet->innerBoundingRect().height() / 2);
				oldPosY = referredGraphicsItem->pos().y();
				referredGraphicsItem->setPos(-(errorSize.width() + 40), referredY - errorSize.height() / 2);
				if (referredGraphicsItem->pos().y() != oldPosY)
					forceUpdate = true;
				
				referredLineItem->setLine(-(errorSize.width() / 2 + 40), referredY + errorSize.height() / 2 + 10, -(errorSize.width() / 2 + 40), errorY - errorSize.height() / 2 - 10);
			}
		}
		
		if (forceUpdate)
			update();
		
		emit contentRecompiled();
	}
} } // namespace ThymioVPL / namespace Aseba
