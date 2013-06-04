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

#include "ThymioButtons.h"
#include "Card.h"
#include "EventCards.h"
#include "ActionCards.h"

namespace Aseba
{
	// Clickable button
	ThymioClickableButton::ThymioClickableButton (const QRectF rect, const ButtonType  type, QGraphicsItem *parent, const QColor& initBrushColor, const QColor& initPenColor) :
		QGraphicsObject(parent),
		buttonType(type),
		boundingRectangle(rect),
		curState(0),
		toggleState(true)
	{
		colors.push_back(qMakePair(initBrushColor, initPenColor));
	}
	
	void ThymioClickableButton::addState(const QColor& brushColor, const QColor& penColor)
	{
		colors.push_back(qMakePair(brushColor, penColor));
	}
	
	void ThymioClickableButton::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
	{
		painter->setBrush(colors[curState].first);
		painter->setPen(QPen(colors[curState].second, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); // outline
		
		switch (buttonType)
		{
			case CIRCULAR_BUTTON:
				painter->drawEllipse(boundingRectangle);
			break;
			case TRIANGULAR_BUTTON:
			{
				QPointF points[3];
				points[0] = (boundingRectangle.topLeft() + boundingRectangle.topRight())*0.5;
				points[1] = boundingRectangle.bottomLeft();
				points[2] = boundingRectangle.bottomRight();
				painter->drawPolygon(points, 3);
			}
			break;
			case QUARTER_CIRCLE_BUTTON:
			{
				const int x(boundingRectangle.x());
				const int y(boundingRectangle.y());
				const int w(boundingRectangle.width());
				const int h(boundingRectangle.height());
				painter->drawPie(x,y,2*w,2*h,90*16, 90*16);
			}
			break;
			default:
				painter->drawRect(boundingRectangle);
			break;
		}
	}

	void ThymioClickableButton::mousePressEvent ( QGraphicsSceneMouseEvent * event )
	{
		if( event->button() == Qt::LeftButton ) 
		{
			if( toggleState )
			{
				curState = (curState + 1) % colors.size();
				//parentItem()->update();
			}
			else 
			{
				curState = 1;
				for( QList<ThymioClickableButton*>::iterator itr = siblings.begin();
					 itr != siblings.end(); ++itr )
					(*itr)->setValue(0);
				//parentItem()->update();
			}
			emit stateChanged();
		}
	}

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
	Card* Card::createButton(const QString& name, bool advancedMode)
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
		else if ( name == "memory" )
			return new StateFilterActionCard();
		else
			return 0;
	}
	
	Card::Card(bool eventButton, bool advanced, QGraphicsItem *parent) : 
		QGraphicsObject(parent),
		buttonIR(),
		buttonColor(eventButton ? QColor(0,191,255) : QColor(218, 112, 214)),
		parentID(-1),
		trans(advanced ? 64 : 0)
	{
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		if (advanced)
			addAdvancedModeButtons();
	}
	

	Card::~Card() 
	{
		if( buttonIR != 0 ) 
			delete(buttonIR);
	}
	
	
	
	ThymioIRButton *Card::getIRButton()
	{
		if (!buttonIR)
		{
			const QString& name = getName();
			buttonIR = new ThymioIRButton(valuesCount(), getIRIdentifier());
			buttonIR->setBasename(name.toStdWString());
		}
		updateIRButton();
		
		return buttonIR;
	}

	void Card::updateIRButton()
	{ 
		if( buttonIR ) 
		{
			for(int i=0; i<valuesCount(); ++i) 
				buttonIR->setValue(i, getValue(i));
			buttonIR->setStateFilter(getStateFilter());
		}
	}
	
	void Card::updateIRButtonAndNotify()
	{
		updateIRButton();
		emit stateChanged();
	}
	
	void Card::addAdvancedModeButtons()
	{
		const int angles[4] = {0,90,270,180};
		for(uint i=0; i<4; i++)
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-20,-20,40,40), ThymioClickableButton::QUARTER_CIRCLE_BUTTON, this, Qt::lightGray, Qt::darkGray);
			button->setPos(310 + (i%2)*60, 100 + (i/2)*60);
			button->setRotation(angles[i]);
			button->addState(QColor(255,128,0));
			button->addState(Qt::white);

			stateButtons.push_back(button);
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButtonAndNotify()));
		}
	}
	
	void Card::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		//painter->setPen(QColor(147, 134, 115),2);
		painter->setPen(Qt::NoPen);
		painter->setBrush(buttonColor); // filling
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
		QByteArray buttonData;
		QDataStream dataStream(&buttonData, QIODevice::WriteOnly);
		dataStream << parentID << getName();
		
		dataStream << getStateFilter();
		
		dataStream << valuesCount();
		for(int i=0; i<valuesCount(); ++i)
			dataStream << getValue(i);
		
		QMimeData *mime = new QMimeData;
		mime->setData("thymiobutton", buttonData);
		mime->setData("thymiotype", getType().toLatin1());
		
		return mime;
	}

	void Card::setAdvanced(bool advanced)
	{
		trans = advanced ? 64 : 0;
		
		if( advanced && stateButtons.empty() )
		{
			addAdvancedModeButtons();
			updateIRButtonAndNotify();
		}
		else if( !advanced && !stateButtons.empty() )
		{
			for(int i=0; i<stateButtons.size(); i++)
			{
				disconnect(stateButtons[i], SIGNAL(stateChanged()), this, SLOT(updateIRButtonAndNotify()));
				stateButtons[i]->setParentItem(0);
				delete(stateButtons[i]);
			}
			stateButtons.clear();
			updateIRButtonAndNotify();
		}
	}

	bool Card::isAnyStateFilter() const
	{
		for (int i=0; i<stateButtons.size(); ++i)
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

		int val=0;
		for(int i=0; i<stateButtons.size(); ++i)
			val |= stateButtons[i]->getValue() << (i*2);

		return val;
	}
	
	void Card::setStateFilter(int val)
	{	
		if( val >= 0 )
		{
			setAdvanced(true);
			for (int i=0; i<stateButtons.size(); ++i)
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
	
	int CardWithButtons::valuesCount() const
	{
		return buttons.size();
	}
	
	int CardWithButtons::getValue(int i) const
	{
		if( i<buttons.size() )
			return buttons.at(i)->getValue();
		return -1;
	}

	void CardWithButtons::setValue(int i, int status)
	{
		if (i < buttons.size())
			buttons.at(i)->setValue(status);
		updateIRButtonAndNotify();
	}
	
	
	// Thymio Push Button
	ThymioPushButton::ThymioPushButton(const QString& name, QWidget *parent) : 
		QPushButton(parent), 
		thymioButton(Card::createButton(name))
	{
		setToolTip(QString("%0 %1").arg(thymioButton->getName()).arg(thymioButton->getType()));
		
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		
		setStyleSheet("QPushButton {border: none; }");
		
		const qreal factor = width() / 256.;
		setIcon(thymioButton->image(factor)); 
		
		setAcceptDrops(true);
	}
	
	ThymioPushButton::~ThymioPushButton()
	{ 
		if( thymioButton != 0 ) 
			delete(thymioButton); 
	}	
	
	void ThymioPushButton::changeButtonColor(const QColor& color) 
	{ 
		thymioButton->setButtonColor(color); 
		const qreal factor = width() / 256.;
		setIcon(thymioButton->image(factor));
	}	

	void ThymioPushButton::mouseMoveEvent( QMouseEvent *event )
	{
		#ifndef ANDROID
		if( thymioButton==0 )
			return;
		
		const qreal factor = width() / 256.;
		QDrag *drag = new QDrag(this);
		drag->setMimeData(thymioButton->mimeData());
		drag->setPixmap(thymioButton->image(factor));
		drag->setHotSpot(event->pos());
		drag->exec();
		#endif // ANDROID
	}

	void ThymioPushButton::dragEnterEvent( QDragEnterEvent *event )
	{
		if( event->mimeData()->hasFormat("thymiotype") &&
			event->mimeData()->data("thymiotype") == thymioButton->getType().toLatin1() )
			event->accept();
		else
			event->ignore();
	}

	void ThymioPushButton::dropEvent( QDropEvent *event )
	{
		if( event->mimeData()->hasFormat("thymiotype") &&
			event->mimeData()->data("thymiotype") == thymioButton->getType().toLatin1() )
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
		else
			event->ignore();
	}

};

