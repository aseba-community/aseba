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

namespace Aseba
{
	// Clickable button
	ThymioClickableButton::ThymioClickableButton (const QRectF rect, const ThymioCardType  type, QGraphicsItem *parent, const QColor& initBrushColor, const QColor& initPenColor) :
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
		
		if ( buttonType == THYMIO_CIRCULAR_BUTTON )
		{
			painter->drawEllipse(boundingRectangle);
		}
		else if ( buttonType == THYMIO_TRIANGULAR_BUTTON )
		{
			QPointF points[3];
			points[0] = (boundingRectangle.topLeft() + boundingRectangle.topRight())*0.5;
			points[1] = boundingRectangle.bottomLeft();
			points[2] = boundingRectangle.bottomRight();
			painter->drawPolygon(points, 3);
		}
		else
			painter->drawRect(boundingRectangle);
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
	
	// Face Button
	ThymioFaceButton::ThymioFaceButton ( QRectF rect, ThymioSmileType type, QGraphicsItem *parent ) :
		ThymioClickableButton( rect, THYMIO_CIRCULAR_BUTTON, parent )
	{
		const qreal x = boundingRectangle.x();
		const qreal y = boundingRectangle.y();
		const qreal w = boundingRectangle.width();
		const qreal h = boundingRectangle.height();
		const qreal s = (w+h)*0.05;

		leftEye = QRectF(x+w*0.3, y+h*0.3, s, s);
		rightEye = QRectF(x+w*0.7-s, y+h*0.3, s, s);
		
		if( type == THYMIO_SMILE_BUTTON ) 
		{
			mouth = QRectF(x+w*0.2, y+h*0.2, w*0.6, h*0.6);
			arcStart = 0;
			arcEnd = -2880;
		} 
		else if( type == THYMIO_FROWN_BUTTON )
		{
			mouth = QRectF(x+w*0.2, y+h*0.5, w*0.6, h*0.6);
			arcStart = 1;
			arcEnd = 2879;
		}
		else
		{
			mouth = QRectF(x+w*0.2, y+h*0.6, w*0.6, h*0.05);
			arcStart = 0;
			arcEnd = -2880;
		}
	}

	void ThymioFaceButton::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_ASSERT(curState < colors.size());
		painter->setBrush(colors[curState].first);
		painter->setPen(QPen(colors[curState].second, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); // outline

		painter->drawEllipse(boundingRectangle);

		painter->setBrush(Qt::black);
		painter->drawEllipse(leftEye);
		painter->drawEllipse(rightEye);
		painter->drawArc(mouth, arcStart, arcEnd);
	}

	void ThymioCard::ThymioBody::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		// button shape
		painter->setPen(Qt::NoPen);
		painter->setBrush(bodyColor);
		painter->drawChord(-124, -118+yShift, 248, 144, 0, 2880);
		painter->drawRoundedRect(-124, -52+yShift, 248, 176, 5, 5);
		
		if(!up) 
		{
			painter->setBrush(Qt::black);
			painter->drawRect(-128, 30+yShift, 18, 80);
			painter->drawRect(110, 30+yShift, 18, 80);
		}
	}
	
	//! Factory for buttons
	ThymioCard* ThymioCard::createButton(const QString& name, bool advancedMode)
	{
		if ( name == "button" )
			return new ThymioButtonsEvent(0, advancedMode);
		else if ( name == "prox" )
			return new ThymioProxEvent(0, advancedMode);
		else if ( name == "proxground" )
			return new ThymioProxGroundEvent(0, advancedMode);
		else if ( name == "tap" )
			return new ThymioTapEvent(0, advancedMode);
		else if ( name == "clap" )
			return new ThymioClapEvent(0, advancedMode);
		else if ( name == "move" )
			return new ThymioMoveAction();
		else if ( name == "colortop" )
			return new ThymioColorTopAction();
		else if ( name == "colorbottom" )
			return new ThymioColorBottomAction();
		else if ( name == "sound" )
			return new ThymioSoundAction();
		else if ( name == "memory" )
			return new ThymioMemoryAction();
		else
			return 0;
	}
	
	ThymioCard::ThymioCard(bool eventButton, bool advanced, QGraphicsItem *parent) : 
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
	

	ThymioCard::~ThymioCard() 
	{
		if( buttonIR != 0 ) 
			delete(buttonIR);
	}
	
	int ThymioCard::getValue(int i) const
	{
		if( i<thymioButtons.size() )
			return thymioButtons.at(i)->getValue();
		return -1;
	}

	void ThymioCard::setValue(int i, int status)
	{
		if (i < thymioButtons.size())
			thymioButtons.at(i)->setValue(status);
		updateIRButtonAndNotify();
	}
	
	ThymioIRButton *ThymioCard::getIRButton()
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

	void ThymioCard::updateIRButton()
	{ 
		if( buttonIR ) 
		{
			for(int i=0; i<valuesCount(); ++i) 
				buttonIR->setValue(i, getValue(i));
			buttonIR->setStateFilter(getStateFilter());
		}
	}
	
	void ThymioCard::updateIRButtonAndNotify()
	{
		updateIRButton();
		emit stateChanged();
	}
	
	void ThymioCard::addAdvancedModeButtons()
	{
		for(uint i=0; i<4; i++)
		{
			ThymioClickableButton *button = new ThymioClickableButton(QRectF(-20,-20,40,40), THYMIO_CIRCULAR_BUTTON, this, Qt::lightGray, Qt::darkGray);
			button->setPos(310 + (i%2)*60, 100 + (i/2)*60);
			button->addState(QColor(255,128,0));
			button->addState(Qt::white);

			stateButtons.push_back(button);
			connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButtonAndNotify()));
		}
	}
	
	void ThymioCard::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		//painter->setPen(QColor(147, 134, 115),2);
		painter->setPen(Qt::NoPen);
		painter->setBrush(buttonColor); // filling
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);
	}
	
	QPixmap ThymioCard::image(qreal factor)
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
	
	void ThymioCard::render(QPainter& painter)
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
	
	QMimeData* ThymioCard::mimeData() const
	{
		QByteArray buttonData;
		QDataStream dataStream(&buttonData, QIODevice::WriteOnly);
		const int numButtons = valuesCount();
		dataStream << -1 << getName();
		
		dataStream << 0;
		
		dataStream << numButtons;
		for(int i=0; i<numButtons; ++i)
			dataStream << getValue(i);
		
		QMimeData *mime = new QMimeData;
		mime->setData("thymiobutton", buttonData);
		mime->setData("thymiotype", getType().toLatin1());
		
		return mime;
	}
	
	bool ThymioCard::isValid()
	{
		if( thymioButtons.isEmpty() )
			return true;

		for(QList<ThymioClickableButton*>::iterator itr = thymioButtons.begin();
			itr != thymioButtons.end(); ++itr)
			if( (*itr)->getValue() > 0 )
				return true;
		
		return false;
	}

	void ThymioCard::setAdvanced(bool advanced)
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

	bool ThymioCard::isAnyStateFilter() const
	{
		for (size_t i=0; i<stateButtons.size(); ++i)
		{
			if (stateButtons[i]->getValue() != 0)
				return true;
		}
		return false;
	}
	
	int ThymioCard::getStateFilter() const
	{
		if (stateButtons.empty())
			return -1;

		int val=0;
		for(int i=0; i<stateButtons.size(); ++i)
			val |= stateButtons[i]->getValue() << (i*2);

		return val;
	}
	
	void ThymioCard::setStateFilter(int val)
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

	void ThymioCard::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
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
	
	ThymioCardWithBody::ThymioCardWithBody(bool eventButton, bool up, bool advanced, QGraphicsItem *parent) :
		ThymioCard(eventButton, advanced, parent)
	{
		thymioBody = new ThymioBody(this);
		thymioBody->setPos(128,128);
		thymioBody->setUp(up);
	}
	
	ThymioCardWithBody::~ThymioCardWithBody()
	{
	}

	// Thymio Push Button
	ThymioPushButton::ThymioPushButton(const QString& name, QWidget *parent) : 
		QPushButton(parent), 
		thymioButton(ThymioCard::createButton(name))
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

