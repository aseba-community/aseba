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
#include <QSlider>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <cassert>

#include "Card.h"
#include "Buttons.h"
#include "EventCards.h"
#include "ActionCards.h"

namespace Aseba { namespace ThymioVPL
{
	void Card::ThymioBody::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		drawBody(painter, 0, yShift, up, bodyColor);
	}
	
	void Card::ThymioBody::drawBody(QPainter * painter, int xShift, int yShift, bool up, const QColor& bodyColor)
	{
		// button shape
		painter->setPen(Qt::NoPen);
		painter->setBrush(bodyColor);
		painter->drawChord(-124+xShift, -118+yShift, 248, 144, 0, 2880);
		painter->drawRoundedRect(-124+xShift, -52+yShift, 248, 176, 5, 5);
		
		if(!up) 
		{
			painter->setBrush(Qt::black);
			painter->drawRect(-128+xShift, 30+yShift, 18, 80);
			painter->drawRect(110+xShift, 30+yShift, 18, 80);
		}
	}
	
	//! Factory for buttons
	Card* Card::createCard(const QString& name, bool advancedMode)
	{
		if ( name == "button" )
			return new ArrowButtonsEventCard(0, advancedMode);
		else if ( name == "prox" )
			return new ProxEventCard(0, advancedMode);
		else if ( name == "proxground" )
			return new ProxGroundEventCard(0, advancedMode);
		else if ( name == "tap" )
			return new TapEventCard(0, advancedMode);
		else if ( name == "clap" )
			return new ClapEventCard(0, advancedMode);
		else if ( name == "move" )
			return new MoveActionCard();
		else if ( name == "colortop" )
			return new TopColorActionCard();
		else if ( name == "colorbottom" )
			return new BottomColorActionCard();
		else if ( name == "sound" )
			return new SoundActionCard();
		else if ( name == "statefilter" )
			return new StateFilterActionCard();
		else
			return 0;
	}
	
	Card::Card(bool eventButton, bool advanced, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		backgroundColor(eventButton ? QColor(0,191,255) : QColor(218, 112, 214)),
		parentID(-1),
		trans(advanced ? 64 : 0)
	{
		setFlag(QGraphicsItem::ItemIsSelectable);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		if (advanced)
			addAdvancedModeButtons();
	}

	Card::~Card() 
	{
		// doing nothing
	}
	
	void Card::addAdvancedModeButtons()
	{
		const int angles[4] = {0,90,270,180};
		for(uint i=0; i<4; i++)
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-20,-20,40,40), GeometryShapeButton::QUARTER_CIRCLE_BUTTON, this, Qt::lightGray, Qt::darkGray);
			button->setPos(310 + (i%2)*60, 100 + (i/2)*60);
			button->setRotation(angles[i]);
			button->addState(QColor(255,128,0));
			button->addState(Qt::white);

			stateButtons.push_back(button);
			connect(button, SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
		}
	}
	
	void Card::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		//painter->setPen(QColor(147, 134, 115),2);
		painter->setPen(Qt::NoPen);
		painter->setBrush(backgroundColor); // filling
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);
	}
	
	QPixmap Card::image(qreal factor)
	{
		const QRectF br(boundingRect());
		QPixmap pixmap(br.width()*factor, br.height()*factor);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.scale(factor, factor);
		painter.translate(-br.topLeft());
		render(painter);
		painter.end();
		
		return pixmap;
	}
	
	void Card::render(QPainter& painter)
	{
		QStyleOptionGraphicsItem opt;
		opt.exposedRect = boundingRect();
		paint(&painter, &opt, 0);
		QGraphicsItem *child;
		foreach(child, childItems())
		{
			painter.save();
			painter.translate(child->pos());
			painter.rotate(child->rotation());
			painter.scale(child->scale(), child->scale());
			child->paint(&painter, &opt, 0);
			painter.restore();
		}
	}
	
	QMimeData* Card::mimeData() const
	{
		QByteArray data;
		QDataStream dataStream(&data, QIODevice::WriteOnly);
		dataStream << parentID << getName();
		
		dataStream << getStateFilter();
		
		dataStream << valuesCount();
		for (unsigned i=0; i<valuesCount(); ++i)
			dataStream << getValue(i);
		
		QMimeData *mime = new QMimeData;
		mime->setData("Card", data);
		mime->setData("CardType", getType().toLatin1());
		
		return mime;
	}

	void Card::setAdvanced(bool advanced)
	{
		trans = advanced ? 64 : 0;
		
		if( advanced && stateButtons.empty() )
		{
			addAdvancedModeButtons();
			//emit contentChanged;
		}
		else if( !advanced && !stateButtons.empty() )
		{
			for(int i=0; i<stateButtons.size(); i++)
			{
				disconnect(stateButtons[i], SIGNAL(stateChanged()), this, SIGNAL(contentChanged()));
				stateButtons[i]->setParentItem(0);
				delete(stateButtons[i]);
			}
			stateButtons.clear();
			emit contentChanged();
		}
	}
	
	unsigned Card::stateFilterCount() const
	{
		return stateButtons.size();
	}
	
	bool Card::isAnyValueSet() const
	{
		for (unsigned i=0; i<valuesCount(); ++i)
		{
			if (getValue(i) > 0)
				return true;
		}
		return false;
	}

	bool Card::isAnyStateFilter() const
	{
		for (unsigned i=0; i<stateButtons.size(); ++i)
		{
			if (stateButtons[i]->getValue() != 0)
				return true;
		}
		return false;
	}
	
	int Card::getStateFilter() const
	{
		if (stateButtons.empty())
			return -1;

		unsigned val=0;
		for (unsigned i=0; i<stateButtons.size(); ++i)
			val |= stateButtons[i]->getValue() << (i*2);

		return val;
	}
	
	int Card::getStateFilter(unsigned i) const
	{
		return stateButtons[i]->getValue();
	}
	
	void Card::setStateFilter(int val)
	{	
		if( val >= 0 )
		{
			setAdvanced(true);
			for (unsigned i=0; i<stateButtons.size(); ++i)
			{
				const int v((val >> (i*2)) & 0x3);
				stateButtons[i]->setValue(v);
			}
		}
		else
			setAdvanced(false);
		
	}

	void Card::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		#ifndef ANDROID
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;
		
		Q_ASSERT(scene());
		Q_ASSERT(scene()->views().size() > 0);
		QGraphicsView* view(scene()->views()[0]);
		const QRectF sceneRect(mapRectToScene(boundingRect()));
		const QRect viewRect(view->mapFromScene(sceneRect).boundingRect());
		const QPoint hotspot(view->mapFromScene(event->scenePos()) - viewRect.topLeft());
		QPixmap pixmap(viewRect.size());
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		scene()->render(&painter, QRectF(), sceneRect);
		painter.end();
		
		QDrag *drag = new QDrag(event->widget());
		drag->setMimeData(mimeData());
		drag->setHotSpot(hotspot);
		drag->setPixmap(pixmap);

		if (drag->exec(Qt::MoveAction | Qt::CopyAction , Qt::CopyAction) == Qt::MoveAction)
			parentID = -1;
		
		if (parentItem())
			parentItem()->update();
		#endif // ANDROID
	}
	
	
	CardWithBody::CardWithBody(bool eventButton, bool up, bool advanced, QGraphicsItem *parent) :
		Card(eventButton, advanced, parent),
		up(up),
		bodyColor(Qt::white)
	{
	}
	
	void CardWithBody::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Card::paint(painter, option, widget);
		ThymioBody::drawBody(painter, 128, 128, up, bodyColor);
	}
	
	
	CardWithButtons::CardWithButtons(bool eventButton, bool up, bool advanced, QGraphicsItem *parent) :
		CardWithBody(eventButton, up, advanced, parent)
	{
	}
	
	unsigned CardWithButtons::valuesCount() const
	{
		return buttons.size();
	}
	
	int CardWithButtons::getValue(unsigned i) const
	{
		if( i<buttons.size() )
			return buttons.at(i)->getValue();
		return -1;
	}

	void CardWithButtons::setValue(unsigned i, int value)
	{
		if (i < buttons.size())
			buttons.at(i)->setValue(value);
		emit contentChanged();
	}
	
	
	CardWithNoValues::CardWithNoValues(bool eventButton, bool advanced, QGraphicsItem *parent):
		Card(eventButton, advanced, parent)
	{}

} } // namespace ThymioVPL / namespace Aseba
