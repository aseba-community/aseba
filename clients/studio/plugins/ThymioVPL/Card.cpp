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
#include <QDebug>
#include <cassert>
#include <cmath>

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
		else if ( name == "timeout" )
			return new TimeoutEventCard(0, advancedMode);
		else if ( name == "move" )
			return new MoveActionCard();
		else if ( name == "colortop" )
			return new TopColorActionCard();
		else if ( name == "colorbottom" )
			return new BottomColorActionCard();
		else if ( name == "sound" )
			return new SoundActionCard();
		else if ( name == "timer" )
			return new TimerActionCard();
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
		setFlag(QGraphicsItem::ItemIsMovable);
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

	bool Card::isAnyAdvancedFeature() const
	{
		for (unsigned i = 0; i < (unsigned)stateButtons.size(); ++i)
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
		for (unsigned i = 0; i < (unsigned)stateButtons.size(); ++i)
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
			for (unsigned i = 0; i < (unsigned)stateButtons.size(); ++i)
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
		drag->exec();
		#endif // ANDROID
	}
	
	
	CardWithNoValues::CardWithNoValues(bool eventButton, bool advanced, QGraphicsItem *parent):
		Card(eventButton, advanced, parent)
	{}
	
	
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
		if (i < (unsigned)buttons.size())
			return buttons.at(i)->getValue();
		return -1;
	}

	void CardWithButtons::setValue(unsigned i, int value)
	{
		if (i < (unsigned)buttons.size())
			buttons.at(i)->setValue(value);
		emit contentChanged();
	}
	
	
	CardWithButtonsAndRange::CardWithButtonsAndRange(bool eventButton, bool up, int lowerBound, int upperBound, int defaultLow, int defaultHigh, bool advanced, QGraphicsItem *parent) :
		CardWithButtons(eventButton, up, advanced, parent),
		lowerBound(lowerBound),
		upperBound(upperBound),
		range(upperBound-lowerBound),
		defaultLow(defaultLow),
		defaultHigh(defaultHigh),
		low(defaultLow),
		high(defaultHigh),
		lastPressedIn(false),
		showRangeControl(advanced)
	{
	}
	
	void CardWithButtonsAndRange::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		// paint parent
		CardWithButtons::paint(painter, option, widget);
		
		if (showRangeControl)
		{
			// draw selectors
			const int x(32);
			const int y(100);
			const int w(192);
			const int h(96);
			const int lowPos(valToPixel(low));
			const int highPos(valToPixel(high));
			//qDebug() << range << low << lowPos << high << highPos;
			// background ranges
			painter->fillRect(x,y+20,highPos,h-40,Qt::red);
			painter->fillRect(x+highPos,y+20,lowPos-highPos,h-40,Qt::darkGray);
			painter->fillRect(x+lowPos,y+20,w-lowPos,h-40,Qt::lightGray);
			// cursors
			painter->setPen(QPen(Qt::black, 4, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
			painter->setBrush(Qt::white);
			QPolygon highCursor;
			highCursor << QPoint(x+highPos+2, y+2) << QPoint(x+highPos+46, y+2) << QPoint(x+highPos+2, y+46);
			painter->drawConvexPolygon(highCursor);
			QPolygon lowCursor;
			lowCursor << QPoint(x+lowPos-2, y+50) << QPoint(x+lowPos-46, y+94) << QPoint(x+lowPos-2, y+94);
			painter->drawConvexPolygon(lowCursor);
			// rectangle
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(x+2,y+2,w-4,h-4);
		}
	}
	
	void CardWithButtonsAndRange::mousePressEvent(QGraphicsSceneMouseEvent * event)
	{
		if (!showRangeControl)
		{
			CardWithButtons::mousePressEvent(event);
			return;
		}
		QPointF pos(event->pos());
		lastPressedIn = (event->button() == Qt::LeftButton && rangeRect().contains(pos));
		if (lastPressedIn)
			mouseMoveEvent(event);
	}
	
	void CardWithButtonsAndRange::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
	{
		if (!showRangeControl || !lastPressedIn)
		{
			CardWithButtons::mouseMoveEvent(event);
			return;
		}
		
		const QRectF& r(rangeRect());
		QPointF pos(event->pos());
		if (!((event->buttons() & Qt::LeftButton) && r.contains(pos)))
		{
			pos -= r.topLeft();
			if (pos.y() >= 48 && pos.x() >= r.width())
			{
				low = lowerBound;
				update();
				emit contentChanged();
			}
			if (pos.y() < 48 && pos.x() <= 0)
			{
				high = upperBound;
				update();
				emit contentChanged();
			}
			return;
		}
		
		pos -= r.topLeft();
		if (pos.y() >= 48)
		{
			low = pixelToVal(pos.x());
			low = std::min<int>(low, pixelToVal(50));
			high = std::max(low, high);
		}
		else
		{
			high = pixelToVal(pos.x());
			high = std::max<int>(high, pixelToVal(r.width()-50));
			low = std::min(low, high);
		}
		update();
		emit contentChanged();
	}
	
	unsigned CardWithButtonsAndRange::valuesCount() const
	{
		return CardWithButtons::valuesCount() + 2;
	}
	
	int CardWithButtonsAndRange::getValue(unsigned i) const
	{
		const unsigned buttonsCount(buttons.size());
		if (i < buttonsCount)
			return CardWithButtons::getValue(i);
		i -= buttonsCount;
		if (i == 0)
			return low;
		else
			return high;
	}

	void CardWithButtonsAndRange::setValue(unsigned i, int value)
	{
		const unsigned buttonsCount(buttons.size());
		if (i < buttonsCount)
		{
			CardWithButtons::setValue(i, value);
		}
		else
		{
			i -= buttonsCount;
			if (i == 0)
				low = value;
			else
				high = value;
			emit contentChanged();
		}
	}
	
	bool CardWithButtonsAndRange::isAnyValueSet() const
	{
		for (unsigned i=0; i<CardWithButtons::valuesCount(); ++i)
		{
			if (getValue(i) > 0)
				return true;
		}
		return false;
	}
	
	bool CardWithButtonsAndRange::isAnyAdvancedFeature() const
	{
		return (low != defaultLow) || (high != defaultHigh);
	}
	
	void CardWithButtonsAndRange::setAdvanced(bool advanced)
	{
		if (!advanced)
		{
			low = defaultLow;
			high = defaultHigh;
		}
		CardWithButtons::setAdvanced(advanced);
		showRangeControl = advanced;
		emit contentChanged();
	}
	
	QRectF CardWithButtonsAndRange::rangeRect() const
	{
		return QRectF(32,100,192,96);
	}
	
	float CardWithButtonsAndRange::pixelToVal(float pixel) const
	{
		const float factor(1. - pixel/rangeRect().width());
		return range * factor * factor + lowerBound;
	}
	
	float CardWithButtonsAndRange::valToPixel(float val) const
	{
		return rangeRect().width()*(1. - sqrt((val-lowerBound)/float(range)));
	}

} } // namespace ThymioVPL / namespace Aseba
