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
#include <QX11Info>
#include <QDebug>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "Block.h"
#include "Buttons.h"
#include "EventBlocks.h"
#include "StateBlocks.h"
#include "ActionBlocks.h"
#include "EventActionsSet.h"
#include "Style.h"
#include "ResizingView.h"
#include "../../../../common/utils/utils.h"

namespace Aseba { namespace ThymioVPL
{
	void Block::ThymioBody::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		drawBody(painter, 0, yShift, up, bodyColor);
	}
	
	void Block::ThymioBody::drawBody(QPainter * painter, int xShift, int yShift, bool up, const QColor& bodyColor)
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
	Block* Block::createBlock(const QString& name, bool advanced, QGraphicsItem *parent)
	{
		if ( name == "button" )
			return new ArrowButtonsEventBlock(parent);
		else if ( name == "prox" )
			return new ProxEventBlock(advanced, parent);
		else if ( name == "proxground" )
			return new ProxGroundEventBlock(advanced, parent);
		else if ( name == "acc" )
			return new AccEventBlock(advanced, parent);
		else if ( name == "clap" )
			return new ClapEventBlock(parent);
		else if ( name == "timeout" )
			return new TimeoutEventBlock(parent);
		else if ( name == "statefilter" )
			return new StateFilterCheckBlock(parent);
		else if ( name == "move" )
			return new MoveActionBlock(parent);
		else if ( name == "colortop" )
			return new TopColorActionBlock(parent);
		else if ( name == "colorbottom" )
			return new BottomColorActionBlock(parent);
		else if ( name == "sound" )
			return new SoundActionBlock(parent);
		else if ( name == "timer" )
			return new TimerActionBlock(parent);
		else if ( name == "setstate" )
			return new StateFilterActionBlock(parent);
		else
			return 0;
	}
	
	Block::Block(const QString& type, const QString& name, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		type(type),
		name(name),
		beingDragged(false),
		changed(false)
	{
		setFlag(QGraphicsItem::ItemIsMovable);
		setAcceptedMouseButtons(Qt::LeftButton);
	}

	Block::~Block() 
	{
		// doing nothing
	}
	
	//! Return a 4-bit unsigned integer encoding the name
	unsigned Block::getNameAsUInt4() const
	{
		// 0 is reserved
		if ( name == "button" )
			return 1;
		else if ( name == "prox" )
			return 2;
		else if ( name == "proxground" )
			return 3;
		else if ( name == "acc" )
			return 4;
		else if ( name == "clap" )
			return 5;
		else if ( name == "timeout" )
			return 6;
		else if ( name == "statefilter" )
			return 7;
		else if ( name == "move" )
			return 9;
		else if ( name == "colortop" )
			return 10;
		else if ( name == "colorbottom" )
			return 11;
		else if ( name == "sound" )
			return 12;
		else if ( name == "timer" )
			return 13;
		else if ( name == "setstate" )
			return 14;
		else
			throw std::runtime_error("unknown name");
	}
	
	bool Block::isAnyValueSet() const
	{
		for (unsigned i=0; i<valuesCount(); ++i)
		{
			if (getValue(i) > 0)
				return true;
		}
		return false;
	}
	
	void Block::resetValues()
	{
		for (unsigned i=0; i<valuesCount(); ++i)
			setValue(i, 0);
	}
	
	QMimeData* Block::mimeData() const
	{
		// create a DOM document and serialize the content of this block in it
		QDomDocument document("block");
		document.appendChild(serialize(document));
		
		// create a MIME data for this block
		QMimeData *mime = new QMimeData;
		mime->setData("Block", document.toByteArray());
		
		return mime;
	}
	
	QDomElement Block::serialize(QDomDocument& document) const
	{
		// create element
		QDomElement element = document.createElement("block");
		
		// populate attributes from this block's informations
		element.setAttribute("name", getName());
		element.setAttribute("type", getType());
		for (unsigned i=0; i<valuesCount(); ++i)
			element.setAttribute(QString("value%0").arg(i), getValue(i));
		
		return element;
	}
	
	Block* Block::deserialize(const QDomElement& element, bool advanced)
	{
		// create a block
		Block *block(createBlock(element.attribute("name"), advanced));
		if (!block)
			return 0;
		
		// set that block's informations from element's attributes
		for (unsigned i=0; i<block->valuesCount(); ++i)
			block->setValue(i, element.attribute(QString("value%0").arg(i), QString("%0").arg(block->getValue(i))).toInt());
		
		return block;
	}
	
	Block* Block::deserialize(const QByteArray& data, bool advanced)
	{
		QDomDocument document;
		document.setContent(data);
		return deserialize(document.documentElement(), advanced);
	}
	
	QString Block::deserializeType(const QByteArray& data)
	{
		QDomDocument document;
		document.setContent(data);
		const QDomElement element(document.documentElement());
		return element.attribute("type");
	}
	
	QString Block::deserializeName(const QByteArray& data)
	{
		QDomDocument document;
		document.setContent(data);
		const QDomElement element(document.documentElement());
		return element.attribute("name");
	}
	
		void Block::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		painter->setPen(Qt::NoPen);
		painter->setBrush(Style::blockCurrentColor(type));
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);
	}
	
	QImage Block::image(qreal factor)
	{
		const QRectF br(boundingRect());
		QImage img(br.width()*factor, br.height()*factor, QImage::Format_ARGB32);
		img.fill(Qt::transparent);
		QPainter painter(&img);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.scale(factor, factor);
		painter.translate(-br.topLeft());
		render(painter);
		painter.end();
		
		return img;
	}
	
	QImage Block::translucidImage(qreal factor)
	{
		QImage img(image(factor));
		
		// Set transparent block FIXME: test ugly
		#ifdef Q_WS_X11
		if (!QX11Info::isCompositingManagerRunning())
		{
			// Note: disabled for now as this is slow
			/*for (int y = 0; y < img.height(); ++y) {
				QRgb *row = (QRgb*)img.scanLine(y);
				for (int x = 0; x < img.width(); ++x) {
					((unsigned char*)&row[x])[3] = (x+y) % 2 == 0 ? 255 : 0;
				}
			}*/
		}
		else
			#endif // Q_WS_X11
		{
			for (int y = 0; y < img.height(); ++y) {
				QRgb *row = (QRgb*)img.scanLine(y);
				for (int x = 0; x < img.width(); ++x) {
					((unsigned char*)&row[x])[3] = 128;
				}
			}
		}
		
		return img;
	}
	
	//! Manual rendering of this block and its children, do not use a scene
	void Block::render(QPainter& painter)
	{
		QStyleOptionGraphicsItem opt;
		opt.exposedRect = boundingRect();
		paint(&painter, &opt, 0);
		renderChildItems(painter, this, opt);
	}
	
	//! Manual rendering of this block and its children, do not use a scene, for child items
	void Block::renderChildItems(QPainter& painter, QGraphicsItem* item, QStyleOptionGraphicsItem& opt)
	{
		if (!item)
			return;
		QGraphicsItem *child;
		foreach(child, item->childItems())
		{
			if (!child->isVisible())
				continue;
			painter.save();
			painter.translate(child->pos());
			painter.translate(child->transformOriginPoint());
			painter.rotate(child->rotation());
			painter.translate(-child->transformOriginPoint());
			painter.scale(child->scale(), child->scale());
			renderChildItems(painter, child, opt);
			child->paint(&painter, &opt, 0);
			painter.restore();
		}
	}
	
	void Block::clearChangedFlag()
	{
		changed = false;
	}
	
	void Block::setChangedFlag()
	{
		changed = true;
	}
	
	void Block::emitUndoCheckpointAndClearIfChanged()
	{
		if (changed)
			emit undoCheckpoint();
		changed = false;
	}

	void Block::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		#ifndef ANDROID
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;
		
		Q_ASSERT(scene());
		Q_ASSERT(scene()->views().size() > 0);
		ResizingView* view(polymorphic_downcast<ResizingView*>(scene()->views()[0]));
		const QRectF sceneRect(mapRectToScene(boundingRect()));
		const QRect viewRect(view->mapFromScene(sceneRect).boundingRect());
		const QPoint hotspot(view->mapFromScene(event->scenePos()) - viewRect.topLeft());
		
		const bool isCopy((event->modifiers() & Qt::ControlModifier) || (name == "statefilter"));
		QDrag *drag = new QDrag(event->widget());
		drag->setMimeData(mimeData());
		drag->setHotSpot(hotspot);
		drag->setPixmap(QPixmap::fromImage(translucidImage(view->getScale())));
		
		beingDragged = true;
		keepAfterDrop = false;
		Qt::DropAction dragResult(drag->exec(isCopy ? Qt::CopyAction : Qt::MoveAction));
		if (dragResult != Qt::IgnoreAction)
		{
			if (!isCopy && parentItem())
			{
				EventActionsSet* eventActionsSet(polymorphic_downcast<EventActionsSet*>(parentItem()));
				if (eventActionsSet && !keepAfterDrop)
					eventActionsSet->removeBlock(this);
			}
			emit contentChanged();
			emit undoCheckpoint();
		}
		beingDragged = false;
		#endif // ANDROID
	}
	
	/////
	
	BlockWithNoValues::BlockWithNoValues(const QString& type, const QString& name, QGraphicsItem *parent):
		Block(type, name, parent)
	{}
	
	
	BlockWithBody::BlockWithBody(const QString& type, const QString& name, bool up, QGraphicsItem *parent) :
		Block(type, name, parent),
		up(up),
		bodyColor(Qt::white)
	{
	}
	
	void BlockWithBody::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Block::paint(painter, option, widget);
		ThymioBody::drawBody(painter, 128, 128, up, bodyColor);
	}
	
	
	BlockWithButtons::BlockWithButtons(const QString& type, const QString& name, bool up, QGraphicsItem *parent) :
		BlockWithBody(type, name, up, parent)
	{
	}
	
	unsigned BlockWithButtons::valuesCount() const
	{
		return buttons.size();
	}
	
	int BlockWithButtons::getValue(unsigned i) const
	{
		if (i < (unsigned)buttons.size())
			return buttons.at(i)->getValue();
		return -1;
	}

	void BlockWithButtons::setValue(unsigned i, int value)
	{
		if (i < (unsigned)buttons.size())
			buttons.at(i)->setValue(value);
		emit contentChanged();
	}
	
	QVector<quint16> BlockWithButtons::getValuesCompressed() const
	{
		unsigned value(0);
		for (int i=0; i<buttons.size(); ++i)
		{
			value *= buttons[i]->valuesCount();
			value += buttons[i]->getValue();
		}
		assert(value <= 65535);
		return QVector<quint16>(1, value);
	}
	
	
	BlockWithButtonsAndRange::BlockWithButtonsAndRange(const QString& type, const QString& name, bool up, int lowerBound, int upperBound, int defaultLow, int defaultHigh, const QColor& lowColor, const QColor& highColor, bool advanced, QGraphicsItem *parent) :
		BlockWithButtons(type, name, up, parent),
		lowerBound(lowerBound),
		upperBound(upperBound),
		range(upperBound-lowerBound),
		defaultLow(defaultLow),
		defaultHigh(defaultHigh),
		lowColor(lowColor),
		highColor(highColor),
		low(defaultLow),
		high(defaultHigh),
		lastPressedIn(false),
		showRangeControl(advanced)
	{
	}
	
	void BlockWithButtonsAndRange::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		// paint parent
		BlockWithButtons::paint(painter, option, widget);
		
		if (showRangeControl)
		{
			// draw selectors
			const int x(32);
			const int y(100);
			const int w(192);
			const int h(96);
			const int lowPos(valToPixel(low));
			const int highPos(valToPixel(high));
			painter->fillRect(x,y,highPos,48,Qt::lightGray);
			painter->fillRect(x+highPos,y,w-highPos,48,highColor);
			painter->fillRect(x,y+48,lowPos,48,lowColor);
			painter->fillRect(x+lowPos,y+48,w-lowPos,48,Qt::lightGray);
			
			// rectangle
			painter->setPen(QPen(Qt::darkGray, 4, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(x,y,w,h);
			painter->drawLine(x,y+48,x+w,y+48);
			
			// cursors
			painter->setPen(QPen(Qt::black, 4, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
			painter->setBrush(Qt::white);
			QPolygon highCursor;
			highCursor << QPoint(x+highPos, y) << QPoint(x+highPos-46, y) << QPoint(x+highPos-46, y+23) << QPoint(x+highPos, y+46);
			painter->drawConvexPolygon(highCursor);
			QPolygon lowCursor;
			lowCursor << QPoint(x+lowPos, y+50) << QPoint(x+lowPos+46, y+50+23) << QPoint(x+lowPos+46, y+96) << QPoint(x+lowPos, y+96);
			painter->drawConvexPolygon(lowCursor);
		}
	}
	
	void BlockWithButtonsAndRange::mousePressEvent(QGraphicsSceneMouseEvent * event)
	{
		if (!showRangeControl)
		{
			BlockWithButtons::mousePressEvent(event);
			return;
		}
		QPointF pos(event->pos());
		lastPressedIn = (event->button() == Qt::LeftButton && rangeRect().contains(pos));
		if (lastPressedIn)
			mouseMoveEvent(event);
	}
	
	void BlockWithButtonsAndRange::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
	{
		if (!showRangeControl || !lastPressedIn)
		{
			BlockWithButtons::mouseMoveEvent(event);
			return;
		}
		
		const QRectF& r(rangeRect());
		QPointF pos(event->pos());
		if (!((event->buttons() & Qt::LeftButton) && r.contains(pos)))
		{
			pos -= r.topLeft();
			if (pos.y() < 48 && pos.x() >= r.width())
			{
				high = upperBound;
				update();
				emit contentChanged();
			}
			if (pos.y() >= 48 && pos.x() <= 0)
			{
				low = lowerBound;
				update();
				emit contentChanged();
			}
			return;
		}
		
		pos -= r.topLeft();
		if (pos.y() >= 48)
		{
			low = pixelToVal(pos.x());
			low = std::min<int>(low, pixelToVal(r.width()-46));
			high = std::max(low, high);
		}
		else
		{
			high = pixelToVal(pos.x());
			high = std::max<int>(high, pixelToVal(47));
			low = std::min(low, high);
		}
		update();
		emit contentChanged();
	}
	
	void BlockWithButtonsAndRange::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
	{
		BlockWithButtons::mouseReleaseEvent(event);
		emit undoCheckpoint();
	}
	
	unsigned BlockWithButtonsAndRange::valuesCount() const
	{
		return BlockWithButtons::valuesCount() + 2;
	}
	
	int BlockWithButtonsAndRange::getValue(unsigned i) const
	{
		const unsigned buttonsCount(buttons.size());
		if (i < buttonsCount)
			return BlockWithButtons::getValue(i);
		i -= buttonsCount;
		if (i == 0)
			return low;
		else
			return high;
	}

	void BlockWithButtonsAndRange::setValue(unsigned i, int value)
	{
		const unsigned buttonsCount(buttons.size());
		if (i < buttonsCount)
		{
			BlockWithButtons::setValue(i, value);
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
	
	QVector<quint16> BlockWithButtonsAndRange::getValuesCompressed() const
	{
		QVector<quint16> values(BlockWithButtons::getValuesCompressed());
		values.append(low);
		values.append(high);
		return values;
	}
	
	bool BlockWithButtonsAndRange::isAnyValueSet() const
	{
		for (unsigned i=0; i<BlockWithButtons::valuesCount(); ++i)
		{
			if (getValue(i) > 0)
				return true;
		}
		return false;
	}
	
	bool BlockWithButtonsAndRange::isAnyAdvancedFeature() const
	{
		return (low != defaultLow) || (high != defaultHigh);
	}
	
	void BlockWithButtonsAndRange::setAdvanced(bool advanced)
	{
		if (!advanced)
		{
			low = defaultLow;
			high = defaultHigh;
		}
		BlockWithButtons::setAdvanced(advanced);
		showRangeControl = advanced;
		emit contentChanged();
	}
	
	QRectF BlockWithButtonsAndRange::rangeRect() const
	{
		return QRectF(32,100,192,96);
	}
	
	float BlockWithButtonsAndRange::pixelToVal(float pixel) const
	{
		const float factor(pixel/rangeRect().width());
		return range * factor * factor + lowerBound;
	}
	
	float BlockWithButtonsAndRange::valToPixel(float val) const
	{
		return rangeRect().width()*(sqrt((val-lowerBound)/float(range)));
	}
	
	//! Create a point with a gradient representing a LED on the robot
	QGraphicsItem* BlockWithButtonsAndRange::createIndicationLED(int x, int y)
	{
		QGraphicsEllipseItem* ledIndication = new QGraphicsEllipseItem(x-12,y-12,24,24,this);
		QRadialGradient grad(x,y,12);
		grad.setColorAt(0, Qt::red);
		grad.setColorAt(1, Qt::transparent);
		ledIndication->setBrush(grad);
		ledIndication->setPen(Qt::NoPen);
		return ledIndication;
	}
	
	//! For every button, update the indication LED accordingly
	void BlockWithButtonsAndRange::updateIndicationLEDsOpacity(void)
	{
		// we need to have one LED per button for this function to work
		if (indicationLEDs.size() != buttons.size())
			return;
		
		for (int i=0; i<indicationLEDs.size(); ++i)
		{
			switch (buttons[i]->getValue())
			{
				case 0: indicationLEDs[i]->setOpacity(0); break;
				case 1: indicationLEDs[i]->setOpacity(1); break;
				case 2: indicationLEDs[i]->setOpacity(0.4); break;
				default: assert(false);
			}
		}
	}
	
	// State Filter Action
	
	StateFilterBlock::StateFilterBlock(const QString& type, const QString& name, QGraphicsItem *parent) : 
		BlockWithButtons(type, name, true, parent)
	{
		const int angles[4] = {0,90,270,180};
		for(uint i=0; i<4; i++)
		{
			GeometryShapeButton *button = new GeometryShapeButton(QRectF(-30,-30,60,60), GeometryShapeButton::QUARTER_CIRCLE_BUTTON, this, Qt::lightGray, Qt::darkGray);
			button->setPos(128 + (i%2)*90 - 45, 128 + (i/2)*90 - 45 + 5);
			button->setRotation(angles[i]);
			button->addState(QColor(255,128,0));
			button->addState(Qt::white);

			buttons.push_back(button);
			connect(button, SIGNAL(stateChanged()), SIGNAL(contentChanged()));
			connect(button, SIGNAL(stateChanged()), SIGNAL(undoCheckpoint()));
		}
	}

} } // namespace ThymioVPL / namespace Aseba
