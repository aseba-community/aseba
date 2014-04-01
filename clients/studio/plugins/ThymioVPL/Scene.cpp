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

using namespace std;

namespace Aseba { namespace ThymioVPL
{

	Scene::Scene(ThymioVisualProgramming *vpl) : 
		QGraphicsScene(vpl),
		vpl(vpl),
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
		connect(eventActionsSet, SIGNAL(undoCheckpoint()), this, SIGNAL(undoCheckpoint()));
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
		clear(false);
		createNewEventActionsSet();
		// TODO: fixme
		lastCompilationResult = Compiler::CompilationResult();
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
			else if(element.tagName() == "buttonset")
			{
				QMessageBox::warning(0, "Feature not implemented", "Loading of files from 1.3 is not implementd yet, it will be soon, please be patient. Thank you.");
				break;
				//addEventActionsSetOldFormat_1_3(element);
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
		setSceneRect(r.adjusted(-40,-40,40,Style::addRemoveButtonHeight+Style::blockSpacing+40));
		emit sceneSizeChanged();
	}

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
		
		for(int i=0; i<eventActionsSets.size(); i++)
			eventActionsSets[i]->setErrorStatus(false);
		
		if (!lastCompilationResult.isSuccessful())
			eventActionsSets.at(lastCompilationResult.errorLine)->setErrorStatus(true);
		
		emit contentRecompiled();
	}
} } // namespace ThymioVPL / namespace Aseba
