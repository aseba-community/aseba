#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QDrag>
#include <QMimeData>
#include <QCursor>
#include <QApplication>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsView>
#include <cassert>

#include "ThymioButtonSet.h"
#include "ThymioButtons.h"

namespace Aseba
{
	ThymioButtonSet::ThymioRemoveButton::ThymioRemoveButton(QGraphicsItem *parent) : 
		QGraphicsItem(parent) 
	{
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setData(0, "remove"); 
		
		setAcceptedMouseButtons(Qt::LeftButton);
	}
	
	void ThymioButtonSet::ThymioRemoveButton::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);

		qreal alpha = hasFocus() ? 255 : 150;
		
		QLinearGradient linearGrad(QPointF(-32, -32), QPointF(32, 32));
		linearGrad.setColorAt(0, QColor(209, 196, 180, alpha));
		linearGrad.setColorAt(1, QColor(167, 151, 128, alpha));
     
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(linearGrad);
		painter->drawEllipse(-32,-32,64,64);
		
		painter->setPen(QPen(QColor(147, 134, 115),5,Qt::SolidLine,Qt::RoundCap));
		painter->drawLine(-11,-11,11,11);
		painter->drawLine(-11,11,11,-11);
	}

	ThymioButtonSet::ThymioAddButton::ThymioAddButton(QGraphicsItem *parent) : 
		QGraphicsItem(parent) 
	{ 
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setData(0, "add"); 

		setAcceptedMouseButtons(Qt::LeftButton);
	}
	
	void ThymioButtonSet::ThymioAddButton::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		qreal alpha = hasFocus() ? 255 : 150;
		
		QLinearGradient linearGrad(QPointF(-32, -32), QPointF(32, 32));
		linearGrad.setColorAt(0, QColor(209, 196, 180, alpha));
		linearGrad.setColorAt(1, QColor(167, 151, 128, alpha));
     
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(linearGrad);
		painter->drawEllipse(-32,-32,64,64);
		
		painter->setPen(QPen(QColor(147, 134, 115),5,Qt::SolidLine,Qt::RoundCap));
		painter->drawLine(-16,0,16,0);
		painter->drawLine(0,-16,0,16);
	}

	ThymioButtonSet::ThymioButtonSet(int row, bool advanced, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		eventButton(0),
		actionButton(0),
		buttonSetIR(),
		eventButtonColor(QColor(0,191,255)),
		actionButtonColor(QColor(218,112,214)),
		highlightEventButton(false),
		highlightActionButton(false),
		errorFlag(false),
		advancedMode(advanced),
		trans(0),
		xpos(0)
	{ 
		setData(0, "buttonset"); 
		setData(1, row);
		
		deleteButton = new ThymioRemoveButton(this);
		addButton = new ThymioAddButton(this);
		
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setAcceptDrops(true);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		repositionElements();
	}
	
	void ThymioButtonSet::repositionElements()
	{
		if (advancedMode)
		{
			trans = 128;
			xpos = 5;
		} 
		else
		{
			trans = 0;
			xpos = 15;
		}
		
		setPos(xpos, (getRow()*420+20));
		deleteButton->setPos(830+trans, 70);
		addButton->setPos(450+trans/2, 378);
		
		if (actionButton)
			actionButton->setPos(500+trans, 40);
		
		prepareGeometryChange();
	}
	
	void ThymioButtonSet::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		// first check whether some buttons have been removed
		if( eventButton && eventButton->getParentID() < 0 ) 
		{
			disconnect(eventButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem(eventButton);
			delete eventButton;
			eventButton = 0;
		
			buttonSetIR.addEventButton(0);
			emit buttonUpdated();
		}
		if( actionButton && actionButton->getParentID() < 0 )
		{
			disconnect(actionButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem(actionButton);
			delete actionButton;
			actionButton = 0;

			buttonSetIR.addActionButton(0);
			emit buttonUpdated();
		}
		
		// then proceed with painting
		
		// select colors (error, selected, normal)
		const int bgColors[][3] = {
			{255, 180, 180},
			{255, 220, 211},
			{255, 237, 233},
		};
		const int fgColors[][3] = {
			{237, 120, 120},
			{237, 172, 140},
			{237, 208, 194},
		};
		const int i = (errorFlag ? 0 : hasFocus() ? 1 : 2);
		
		// background
		if (errorFlag)
			painter->setPen(QPen(Qt::red, 4));
		else
			painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(bgColors[i][0], bgColors[i][1], bgColors[i][2]));
		painter->drawRoundedRect(0, 0, 900+trans, 336, 5, 5);

		// arrow
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(fgColors[i][0], fgColors[i][1], fgColors[i][2]));
		painter->drawRect(350+trans, 143, 55, 55);
		QPointF pts[3];
		pts[0] = QPointF(404.5+trans, 118);
		pts[1] = QPointF(404.5+trans, 218);
		pts[2] = QPointF(456+trans, 168);
		painter->drawPolygon(pts, 3);
		
		if( eventButton == 0 )
		{
			if( !highlightEventButton )
				eventButtonColor.setAlpha(100);
			painter->setPen(QPen(eventButtonColor, 10, Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->setBrush(Qt::NoBrush);
			painter->drawRoundedRect(45, 45, 246, 246, 5, 5);
			eventButtonColor.setAlpha(255);
		}

		if( actionButton == 0 )
		{
			if( !highlightActionButton )
				actionButtonColor.setAlpha(100);
			painter->setPen(QPen(actionButtonColor, 10,	Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->setBrush(Qt::NoBrush);
			painter->drawRoundedRect(505+trans, 45, 246, 246, 5, 5);
			actionButtonColor.setAlpha(255);
		}
	}
	
	bool ThymioButtonSet::isAnyStateFilter() const
	{
		if (eventButton && eventButton->isAnyStateFilter())
			return true;
		if (actionButton && (actionButton->getName() == "memory"))
			return true;
		return false;
	}

	void ThymioButtonSet::setColorScheme(QColor eventColor, QColor actionColor)
	{
		eventButtonColor = eventColor;
		actionButtonColor = actionColor;
		
		if( eventButton )
			eventButton->setButtonColor(eventColor);
		if( actionButton )
			actionButton->setButtonColor(actionColor);
	}

	void ThymioButtonSet::setRow(int row) 
	{ 
		setData(1,row); 
		if( eventButton )
			eventButton->setParentID(row);
		if( actionButton )
			actionButton->setParentID(row);
		setPos(xpos, (row*420+20)*scale());
	}

	void ThymioButtonSet::addEventButton(ThymioCard *event) 
	{ 
		if( eventButton ) 
		{
			disconnect(eventButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem( eventButton );
			eventButton->deleteLater();
		}
		
		event->setButtonColor(eventButtonColor);
		event->setPos(40, 40);
		event->setEnabled(true);
		event->setParentItem(this);
		event->setParentID(data(1).toInt());
		eventButton = event;
		
		buttonSetIR.addEventButton(event->getIRButton());
		emit buttonUpdated();
		connect(eventButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
	}

	void ThymioButtonSet::addActionButton(ThymioCard *action) 
	{ 
		if( actionButton )
		{
			disconnect(actionButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem( actionButton );
			actionButton->deleteLater();
		}
		
		action->setButtonColor(actionButtonColor);
		action->setPos(500+trans, 40);
		action->setEnabled(true);
		action->setParentItem(this);
		action->setParentID(data(1).toInt());
		actionButton = action;
		
		buttonSetIR.addActionButton(action->getIRButton());
		emit buttonUpdated();
		connect(actionButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
	}
	
	void ThymioButtonSet::setAdvanced(bool advanced)
	{
		advancedMode = advanced;
		if( eventButton )
			eventButton->setAdvanced(advanced);
		repositionElements();
	}

	void ThymioButtonSet::stateChanged()
	{
		emit buttonUpdated();
	}
	
	void ThymioButtonSet::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
	{
		if ( event->mimeData()->hasFormat("thymiobuttonset") || 
			 event->mimeData()->hasFormat("thymiobutton") )
		{
			if( event->mimeData()->hasFormat("thymiotype") )
			{
				if( event->mimeData()->data("thymiotype") == QString("event").toLatin1() )
					highlightEventButton = true;
				else if( event->mimeData()->data("thymiotype") == QString("action").toLatin1() )
					highlightActionButton = true;
			}

			event->setDropAction(Qt::MoveAction);
			event->accept();
			scene()->setFocusItem(this);
			update();
		}
		else
			event->ignore();
	}

	void ThymioButtonSet::dragMoveEvent( QGraphicsSceneDragDropEvent *event )
	{
		/*if ( event->mimeData()->hasFormat("thymiobuttonset") ||
			 event->mimeData()->hasFormat("thymiobutton") )
		{
			if( event->mimeData()->hasFormat("thymiotype") )
			{
				if( event->mimeData()->data("thymiotype") == QString("event").toLatin1() )
					highlightEventButton = true;
				else if( event->mimeData()->data("thymiotype") == QString("action").toLatin1() )
					highlightActionButton = true;
			}
			
			event->setDropAction(Qt::MoveAction);
			event->accept();
			scene()->setFocusItem(this);
			update();
		}
		else
			event->ignore();*/
		QGraphicsObject::dragMoveEvent(event);
	}
	
	void ThymioButtonSet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("thymiobutton") || 
			 event->mimeData()->hasFormat("thymiobuttonset") )
		{
			highlightEventButton = false;
			highlightActionButton = false;
			event->setDropAction(Qt::MoveAction);
			event->accept();
			update();
		} 
		else
			event->ignore();
	}

	void ThymioButtonSet::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		scene()->setFocusItem(0);
	
		if ( event->mimeData()->hasFormat("thymiobutton") )
		{
			QByteArray buttonData = event->mimeData()->data("thymiobutton");
			QDataStream dataStream(&buttonData, QIODevice::ReadOnly);
			
			int parentID;
			dataStream >> parentID;
			
			if( parentID != data(1).toInt() )
			{
				QString buttonName;
				int state;
				dataStream >> buttonName >> state;
				
				ThymioCard *button(ThymioCard::createButton(buttonName, advancedMode));
				if( button ) 
				{
					event->setDropAction(Qt::MoveAction);
					event->accept();
					
					if( event->mimeData()->data("thymiotype") == QString("event").toLatin1() )
					{
						if (advancedMode)
						{
							if (parentID == -1 && eventButton)
								button->setStateFilter(eventButton->getStateFilter());
							else
								button->setStateFilter(state);
						}
						addEventButton(button);
						highlightEventButton = false;
					}
					else if( event->mimeData()->data("thymiotype") == QString("action").toLatin1() )
					{
						addActionButton(button);
						highlightActionButton = false;
					}
					
					int numButtons;
					dataStream >> numButtons;
					for( int i=0; i<numButtons; ++i ) 
					{
						int status;
						dataStream >> status;
						button->setValue(i, status);
					}
				}
			}
			
			update();
		}
		else
			event->ignore();
	}

	void ThymioButtonSet::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		#ifndef ANDROID
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;
		
		QByteArray buttonData;
		QDataStream dataStream(&buttonData, QIODevice::WriteOnly);
		
		dataStream << getRow();

		QMimeData *mime = new QMimeData;
		mime->setData("thymiobuttonset", buttonData);
		
		Q_ASSERT(scene()->views().size() > 0);
		QGraphicsView* view(scene()->views()[0]);
		const QRectF sceneRect(mapRectToScene(innerBoundingRect()));
		const QRect viewRect(view->mapFromScene(sceneRect).boundingRect());
		const QPoint hotspot(view->mapFromScene(event->scenePos()) - viewRect.topLeft());
		QPixmap pixmap(viewRect.size());
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		scene()->render(&painter, QRectF(), sceneRect);
		painter.end();
		
		QDrag *drag = new QDrag(event->widget());
		drag->setMimeData(mime);
		drag->setHotSpot(hotspot);
		drag->setPixmap(pixmap);
		drag->exec(Qt::MoveAction);
		#endif // ANDROID
	}
};

