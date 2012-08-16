#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QDrag>
#include <QMimeData>
#include <QCursor>
#include <QApplication>

#include "ThymioButtons.h"
#include "ThymioButtons.moc"

namespace Aseba
{
	// Clickable button
	ThymioClickableButton::ThymioClickableButton ( QRectF rect, ThymioButtonType type,  int nstates, QGraphicsItem *parent ) :
		QGraphicsObject(parent),
		buttonType(type),
		buttonClicked(false),
		toggleState(true),
		numStates(nstates),
		boundingRectangle(rect),
		buttonColor(Qt::gray),
		buttonBeginColor(Qt::white)
	{		
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
	
		setCursor(Qt::ArrowCursor);
	}
	
	void ThymioClickableButton::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
	{		
		painter->setPen(QPen(QBrush(Qt::black), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); // outline
		
		// filled
		if( buttonClicked == 0 )
			painter->setBrush(Qt::white);
		else
		{
			double step = (numStates <= 2 ? 1 : (double)(buttonClicked-1)/(double)(numStates-2));
			painter->setBrush(QColor(buttonColor.red()*step + buttonBeginColor.red()*(1-step),
									 buttonColor.green()*step + buttonBeginColor.green()*(1-step),
									 buttonColor.blue()*step + buttonBeginColor.blue()*(1-step))); 
		}

		if ( buttonType == THYMIO_CIRCULAR_BUTTON )
			painter->drawEllipse(boundingRectangle);
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
			if( toggleState ) {
				if( ++buttonClicked == numStates )
					buttonClicked = 0;
				update();
			}
			else 
			{
				buttonClicked = 1;
				for( QList<ThymioClickableButton*>::iterator itr = siblings.begin();
					 itr != siblings.end(); ++itr )				
					(*itr)->setClicked(0);
				parentItem()->update();
			}
			emit stateChanged();
		}
	}
		
	// Face Button
	ThymioFaceButton::ThymioFaceButton ( QRectF rect, ThymioSmileType type, QGraphicsItem *parent ) :
		ThymioClickableButton( rect, THYMIO_CIRCULAR_BUTTON, 2, parent )
	{
		qreal x = boundingRectangle.x();
		qreal y = boundingRectangle.y();
		qreal w = boundingRectangle.width();
		qreal h = boundingRectangle.height();
		qreal s = (w+h)*0.05;

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
		painter->setPen(QPen(QBrush(Qt::black), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); // outline
		
		// filled
		if ( buttonClicked )
			painter->setBrush(buttonColor);
		else
			painter->setBrush(Qt::white);

		painter->drawEllipse(boundingRectangle);

		painter->setBrush(Qt::black);		
		painter->drawEllipse(leftEye);
		painter->drawEllipse(rightEye);
		painter->drawArc(mouth, arcStart, arcEnd);

	}

	void ThymioButton::ThymioBody::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{			
		Q_UNUSED(option);
		Q_UNUSED(widget);
     		
		// button shape
		painter->setPen(Qt::NoPen);
		painter->setBrush(bodyColor);
		painter->drawChord(-124, -118, 248, 144, 0, 2880);
		painter->drawRoundedRect(-124, -52, 248, 176, 5, 5);
		
		if(!up) 
		{
			painter->setBrush(Qt::black);
			painter->drawRect(-128, 30, 18, 80);
			painter->drawRect(110, 30, 18, 80);
		}
	}

	ThymioButton::ThymioButton(bool eventButton, qreal scale, bool up, bool advanced, QGraphicsItem *parent) : 
		QGraphicsSvgItem(parent),
		buttonIR(),
		buttonColor(eventButton ? QColor(0,191,255) : QColor(218, 112, 214)),
		parentID(-1),
		scaleFactor(0.5),
		trans(advanced ? 64 : 0)
	{
		thymioBody = new ThymioBody(this);
		thymioBody->setUp(up);
		thymioBody->setScale(scale);
		thymioBody->setPos(128,128);

		setFlag(QGraphicsItem::ItemIsFocusable);		
		setFlag(QGraphicsItem::ItemIsSelectable);	

		setCursor(Qt::OpenHandCursor);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		if( advanced )
		{
			for(uint i=0; i<4; i++)
			{
//				ThymioClickableButton *button = new ThymioClickableButton(QRectF(-15,-25,30,50), THYMIO_CIRCULAR_BUTTON, 2, this);//THYMIO_RECTANGULAR_BUTTON , 2, this);
//				button->setPos(320 + (2-i)*(i%2)*35, 128 + (i-1)*((i+1)%2)*45);
//				button->setRotation(90*(i+1));				
				
				ThymioClickableButton *button = new ThymioClickableButton(QRectF(-20,-20,40,40), THYMIO_CIRCULAR_BUTTON, 2, this);
				button->setPos(295, i*60 + 40);
				button->setButtonColor(QColor(255,200,0));

				stateButtons.push_back(button);
				connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
			}
		}
	}

	ThymioButton::~ThymioButton() 
	{
		if( buttonIR != 0 ) 
			delete(buttonIR);
	}

	void ThymioButton::setScaleFactor(qreal factor) 
	{ 
		scaleFactor = factor; 
	} 

	void ThymioButton::setClicked(int i, int status)
	{
		//qDebug() << "in thymio button set clicked";
		if( i < thymioButtons.size()  )
			thymioButtons.at(i)->setClicked(status);			
	}
	
	ThymioIRButton *ThymioButton::getIRButton()
	{
		int num = getNumButtons();
		if( buttonIR == 0 )
		{
			QString name = getName();

			//qDebug() << "getIRButton num: " << num;

			if( name == "button" )
				buttonIR = new ThymioIRButton(num, THYMIO_BUTTONS_IR, 2);
			else if( name == "prox" )
				buttonIR = new ThymioIRButton(num, THYMIO_PROX_IR, 3);
			else if( name == "proxground" )
				buttonIR = new ThymioIRButton(num, THYMIO_PROX_GROUND_IR, 3);
			else if( name == "tap" )
				buttonIR = new ThymioIRButton(num, THYMIO_TAP_IR, 0);
			else if( name == "clap" )
				buttonIR = new ThymioIRButton(num, THYMIO_CLAP_IR, 0);
			else if( name == "move" )
				buttonIR = new ThymioIRButton(num, THYMIO_MOVE_IR, 0);
			else if( name == "color" )
				buttonIR = new ThymioIRButton(num, THYMIO_COLOR_IR, 0);
			else if( name == "circle" )
				buttonIR = new ThymioIRButton(num, THYMIO_CIRCLE_IR, 5);
			else if( name == "sound" )
				buttonIR = new ThymioIRButton(num, THYMIO_SOUND_IR, 2);
			else if( name == "memory" )
				buttonIR = new ThymioIRButton(num, THYMIO_MEMORY_IR);

			buttonIR->setBasename(name.toStdWString());
		}

		for(int i=0; i<num; ++i)
			buttonIR->setClicked(i, isClicked(i));

		if( !stateButtons.empty() )
		{
			int val=0;
			for(int i=0; i<stateButtons.size(); ++i)
				val += (stateButtons[i]->isClicked()*(0x01<<i));
			buttonIR->setMemoryState(val);
		}
		
		return buttonIR;
	}

	void ThymioButton::updateIRButton()
	{ 
		if( buttonIR ) 
		{
			for(int i=0; i<getNumButtons(); ++i) 
				buttonIR->setClicked(i, isClicked(i));

			buttonIR->setMemoryState(getState());		
		
			emit stateChanged();
		}
	}
	
	void ThymioButton::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
		
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(buttonColor); // filling
		painter->drawRoundedRect(0, 0, 256, 256, 5, 5);		

		QGraphicsSvgItem::paint(painter, option, widget);
	}
	
	QPixmap ThymioButton::image(bool on)
	{
		QPixmap pixmap(256+trans, 256);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(buttonColor);
		painter.drawRoundedRect(0,0,256,256,5,5);

		int state[getNumButtons()];
		if(on)
			for(int i=0; i<getNumButtons(); ++i)
			{
				state[i] = thymioButtons.at(i)->isClicked();
				thymioButtons.at(i)->setClicked(thymioButtons.at(i)->getNumStates()-1);
			}

		if(thymioBody != 0) 
		{
			painter.translate(thymioBody->pos());
			painter.scale(thymioBody->scale(),thymioBody->scale());
			painter.rotate(thymioBody->rotation());
			thymioBody->paint(&painter, 0, 0);
			painter.resetTransform();
		}

		for(QList<ThymioClickableButton*>::iterator itr = thymioButtons.begin();
			itr != thymioButtons.end(); ++itr)
		{			
			painter.translate((*itr)->pos());
			painter.rotate((*itr)->rotation());
			(*itr)->paint(&painter, 0, 0);
			painter.resetTransform();
		}

		for(QList<ThymioClickableButton*>::iterator itr = stateButtons.begin();
			itr != stateButtons.end(); ++itr)
		{			
			painter.translate((*itr)->pos());
			(*itr)->paint(&painter, 0, 0);
			painter.resetTransform();
		}

		if(on)
			for(int i=0; i<getNumButtons(); ++i)
				thymioButtons.at(i)->setClicked(state[i]);
				
		if( renderer() ) 
			renderer()->render(&painter, QRectF(0,0,256,256)); 

		return pixmap;
	}
	
	bool ThymioButton::isValid()
	{
		if( thymioButtons.isEmpty() )
			return true;

		for(QList<ThymioClickableButton*>::iterator itr = thymioButtons.begin();
			itr != thymioButtons.end(); ++itr)
			if( (*itr)->isClicked() > 0 )
				return true;
				
		return false;
	}

	void ThymioButton::setAdvanced(bool advanced)
	{
		trans = advanced ? 64 : 0;	
		
		if( advanced && stateButtons.empty() )
		{
			for(int i=0; i<4; i++)
			{
//				ThymioClickableButton *button = new ThymioClickableButton(QRectF(-10,-15,20,30), THYMIO_RECTANGULAR_BUTTON , 2, this);
//				button->setPos(128 + (2-i)*(i%2)*35, 128 + (i-1)*((i+1)%2)*35);
//				button->setRotation(90*(i+1));
				
				ThymioClickableButton *button = new ThymioClickableButton(QRectF(-20,-20,40,40), THYMIO_CIRCULAR_BUTTON, 2, this);
				button->setPos(295, i*60 + 40);
				button->setButtonColor(QColor(255,200,0));

				stateButtons.push_back(button);
				connect(button, SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
			}
			updateIRButton();
		}	
		else if( !advanced && !stateButtons.empty() )
		{
			for(int i=0; i<stateButtons.size(); i++)
			{
				disconnect(stateButtons[i], SIGNAL(stateChanged()), this, SLOT(updateIRButton()));
				stateButtons[i]->setParentItem(0);
				delete(stateButtons[i]);
			}
			stateButtons.clear();
		}
	}

	int ThymioButton::getState() const
	{
		if( stateButtons.empty() )
			return -1;

		int val=0;
		for(int i=0; i<stateButtons.size(); ++i)
			val += (stateButtons[i]->isClicked()*(0x01<<i));

		return val;
	}
	
	void ThymioButton::setState(int val)
	{	
		if( val >= 0 )
		{
			setAdvanced(true);
			
			for(int i=0; i<4; ++i)
				stateButtons.at(i)->setClicked(val&(0x01<<i)? 1 : 0);			
		}
		else
			setAdvanced(false);
		
	}

	void ThymioButton::mousePressEvent ( QGraphicsSceneMouseEvent * event )
	{
		setCursor(Qt::ClosedHandCursor);
	}

	void ThymioButton::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
	{
		setCursor(Qt::OpenHandCursor);
	}

	void ThymioButton::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;

		QByteArray buttonData;
		QDataStream dataStream(&buttonData, QIODevice::WriteOnly);
		dataStream << parentID << getName() << getNumButtons();
		
		for(int i=0; i<getNumButtons(); i++)
			dataStream << isClicked(i);	

		QMimeData *mime = new QMimeData;
		mime->setData("thymiobutton", buttonData);
		mime->setData("thymiotype", getType().toLatin1());
	
		if( renderer() ) 
		{
			QByteArray rendererData;
			QDataStream rendererStream(&rendererData, QIODevice::WriteOnly);
			quint64 location  = (quint64)(renderer());
			rendererStream << location;
			mime->setData("thymiorenderer", rendererData);
		}
		
		QDrag *drag = new QDrag(event->widget());
		QPixmap img = image(false);		
		drag->setMimeData(mime);
		drag->setPixmap(img.scaled((256+trans)*scaleFactor, 256*scaleFactor));
		drag->setHotSpot(QPoint((256+trans)*scaleFactor, 256*scaleFactor));

		if (drag->exec(Qt::MoveAction | Qt::CopyAction , Qt::CopyAction) == Qt::MoveAction)
			parentID = -1;
		
		setCursor(Qt::OpenHandCursor);
		parentItem()->update();
	}

	// Button Set
	ThymioButtonSet::ThymioRemoveButton::ThymioRemoveButton(QGraphicsItem *parent) : 
		QGraphicsItem(parent) 
	{ 		
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);	
		setData(0, "remove"); 
		
		setCursor(Qt::ArrowCursor);		
		setAcceptedMouseButtons(Qt::LeftButton);
	}
			
	void ThymioButtonSet::ThymioRemoveButton::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);

		qreal alpha = hasFocus() ? 255 : 150;
					
		QLinearGradient linearGrad(QPointF(-64, -64), QPointF(64, 64));
		linearGrad.setColorAt(0, QColor(209, 196, 180, alpha));
		linearGrad.setColorAt(1, QColor(167, 151, 128, alpha));
     
		painter->setPen(QColor(147, 134, 115));
		painter->setBrush(linearGrad);
		painter->drawRoundedRect(-64,-64,128,128,4,4);
		
		QRadialGradient radialGrad(QPointF(0, 0), 80);
		radialGrad.setColorAt(0, Qt::red);
		radialGrad.setColorAt(1, Qt::darkRed);		

		painter->setPen(Qt::black);
		painter->setBrush(radialGrad);
		painter->drawRect(-40,-20,80,40);
	}

	ThymioButtonSet::ThymioAddButton::ThymioAddButton(QGraphicsItem *parent) : 
		QGraphicsItem(parent) 
	{ 		
		setFlag(QGraphicsItem::ItemIsFocusable);		
		setFlag(QGraphicsItem::ItemIsSelectable);	
		setData(0, "add"); 

		setCursor(Qt::ArrowCursor);
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
		trans(advanced ? 64 : 0),
		xpos(advanced ? 5 : 15)
	{ 
		setData(0, "buttonset"); 
		setData(1, row);
		
		deleteButton = new ThymioRemoveButton(this);
		addButton = new ThymioAddButton(this);
		
		deleteButton->setPos(896+trans, 168);
		addButton->setPos(500+trans, 368);
		
		setFlag(QGraphicsItem::ItemIsFocusable);
		setFlag(QGraphicsItem::ItemIsSelectable);
		setAcceptDrops(true);
		setCursor(Qt::OpenHandCursor);
		setAcceptedMouseButtons(Qt::LeftButton);
		
		setPos(xpos, row*400*scale()+20);
	}

	void ThymioButtonSet::paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
	{
		Q_UNUSED(option);
		Q_UNUSED(widget);
     		
		qreal alpha = (errorFlag ? 255 : hasFocus() ? 150 : 50);

		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(255, 196, 180, alpha));		
		painter->drawRoundedRect(0, 0, 1000+trans, 336, 5, 5);

		painter->setBrush(QColor(167, 151, 128, alpha));
		painter->drawRect(340 + trans, 143, 55, 55);

		QPointF pts[3];
		pts[0] = QPointF(394.5+trans, 118);
		pts[1] = QPointF(394.5+trans, 218);
		pts[2] = QPointF(446+trans, 168);
		painter->drawPolygon(pts, 3);

		painter->setPen(errorFlag ? Qt::black : Qt::gray);
		painter->setFont(QFont("Arial", 30));
		painter->drawText(QRect(800+trans, 20, 180, 40), Qt::AlignRight, QString("%0").arg(getRow()));

		if( eventButton && eventButton->getParentID() < 0 ) 
		{
			disconnect(eventButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem(eventButton);
			delete(eventButton);
			eventButton = 0;
		
			buttonSetIR.addEventButton(0);
			emit buttonUpdated();
		}
		if( actionButton && actionButton->getParentID() < 0 )
		{
			disconnect(actionButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem(actionButton);
			delete(actionButton);
			actionButton = 0;			

			buttonSetIR.addActionButton(0);
			emit buttonUpdated();
		}
		
		if( eventButton == 0 )
		{
			if( !highlightEventButton ) eventButtonColor.setAlpha(50);
			painter->setPen(QPen(eventButtonColor, 10, Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));				
			painter->setBrush(Qt::NoBrush);
			painter->drawRoundedRect(45, 45, 246, 246, 5, 5);
			eventButtonColor.setAlpha(255);
		}

		if( actionButton == 0 )
		{
			if( !highlightActionButton ) actionButtonColor.setAlpha(50);	
			painter->setPen(QPen(actionButtonColor, 10,	Qt::DotLine, Qt::SquareCap, Qt::RoundJoin));
			painter->setBrush(Qt::NoBrush);
			painter->drawRoundedRect(505+trans, 45, 246, 246, 5, 5);
			actionButtonColor.setAlpha(255);	
		}

		highlightEventButton = false;
		highlightActionButton = false;
	}

	void ThymioButtonSet::setColorScheme(QColor eventColor, QColor actionColor)
	{
		eventButtonColor = eventColor;
		actionButtonColor = actionColor;
		
		if( eventButton )	eventButton->setButtonColor(eventColor);
		if( actionButton )	actionButton->setButtonColor(actionColor);
	}

	void ThymioButtonSet::setRow(int row) 
	{ 
		setData(1,row); 
		if( eventButton ) eventButton->setParentID(row);
		if( actionButton ) actionButton->setParentID(row);
		setPos(xpos, row*400*scale()+20);	
	}

	void ThymioButtonSet::addEventButton(ThymioButton *event) 
	{ 
		if( eventButton ) 
		{
			disconnect(eventButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem( eventButton );
			delete( eventButton );
		}
		
		event->setButtonColor(eventButtonColor);
		event->setPos(40, 40);
		event->setScaleFactor(scale());
		event->setEnabled(true);
		event->setParentItem(this);
		event->setParentID(data(1).toInt());
		eventButton = event;
		
		buttonSetIR.addEventButton(event->getIRButton());
		emit buttonUpdated();
		connect(eventButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));	
	}

	void ThymioButtonSet::addActionButton(ThymioButton *action) 
	{ 
		if( actionButton )
		{
			disconnect(actionButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
			scene()->removeItem( actionButton );
			delete(actionButton);
		}
		
		action->setButtonColor(actionButtonColor);
		action->setPos(500+trans, 40);
		action->setScaleFactor(scale());
		action->setEnabled(true);
		action->setParentItem(this);
		action->setParentID(data(1).toInt());
		actionButton = action;
		
		buttonSetIR.addActionButton(action->getIRButton());
		emit buttonUpdated();
		connect(actionButton, SIGNAL(stateChanged()), this, SLOT(stateChanged()));
	}

	void ThymioButtonSet::setScale(qreal factor)
	{ 
		QGraphicsItem::setScale(factor); 
		setPos(xpos, getRow()*400*scale()+20);
		
		if( eventButton ) 
			eventButton->setScaleFactor(factor);

		if( actionButton )
			actionButton->setScaleFactor(factor);
	}
	
	void ThymioButtonSet::setAdvanced(bool advanced)
	{
		advancedMode = advanced;
		if( eventButton )
			eventButton->setAdvanced(advanced);
		
		if( advanced ) 
		{			
			trans = 64;
			xpos = 5;
		} 
		else
		{
			trans = 0;
			xpos = 15;
		}

		setPos(xpos, getRow()*400*scale()+20);		
		
		deleteButton->setPos(896+trans, 168);
		addButton->setPos(500+trans/2, 368);
		
		if( actionButton )
			actionButton->setPos(500+trans, 40);
		
		update();
	}

	void ThymioButtonSet::stateChanged()
	{
		emit buttonUpdated();
	}
	
	QPixmap ThymioButtonSet::image()
	{
		QPixmap pixmap(advancedMode? 1064 : 1000, 336);
		pixmap.fill();
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);	
		
		paint(&painter,0,0);
		
		painter.translate(deleteButton->pos());
		deleteButton->paint(&painter, 0, 0);
		painter.resetTransform();
		
		if( eventButton )
			painter.drawImage(eventButton->pos(), (eventButton->image(false)).toImage());

		if( actionButton )
			painter.drawImage(actionButton->pos(), (actionButton->image(false)).toImage()); 
			
		return pixmap;
	}
		
	void ThymioButtonSet::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
	{
		if ( event->mimeData()->hasFormat("thymiobutton") || 
			 event->mimeData()->hasFormat("thymiobuttonset") )
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
			update();
					
			scene()->setFocusItem(this);
		}
		else
			event->ignore();
	}	

	void ThymioButtonSet::dragMoveEvent( QGraphicsSceneDragDropEvent *event )
	{
		if ( event->mimeData()->hasFormat("thymiobutton") || 
			 event->mimeData()->hasFormat("thymiobuttonset") )
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
			update();
					
			scene()->setFocusItem(this);
		}
		else
			event->ignore();
	}	
	
	void ThymioButtonSet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
	{
		if ( event->mimeData()->hasFormat("thymiobutton") || 
			 event->mimeData()->hasFormat("thymiobuttonset") )
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
			update();					
		} 
		else
			event->ignore();		
	}

	void ThymioButtonSet::dropEvent(QGraphicsSceneDragDropEvent *event)
	{
		//qDebug() << "ThymioButtonSet -- drop event";

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
				int numButtons;
				dataStream >> buttonName >> numButtons;
				
				ThymioButton *button = 0;

				if ( buttonName == "button" )
					button = new ThymioButtonsEvent(0,advancedMode);
				else if ( buttonName == "prox" )
					button = new ThymioProxEvent(0,advancedMode);
				else if ( buttonName == "proxground" )
					button = new ThymioProxGroundEvent(0,advancedMode);
				else if ( buttonName == "tap" )
				{					
					button = new ThymioTapEvent(0,advancedMode);
					QByteArray rendererData = event->mimeData()->data("thymiorenderer");
					QDataStream rendererStream(&rendererData, QIODevice::ReadOnly);
					quint64 location;
					rendererStream >> location;
					QSvgRenderer *tapSvg;
					tapSvg = (QSvgRenderer*)location;
					button->setSharedRenderer(tapSvg);
				}
				else if ( buttonName == "clap" )
				{
					button = new ThymioClapEvent(0,advancedMode);
					QByteArray rendererData = event->mimeData()->data("thymiorenderer");
					QDataStream rendererStream(&rendererData, QIODevice::ReadOnly);
					quint64 location;
					rendererStream >> location;
					QSvgRenderer *clapSvg;
					clapSvg = (QSvgRenderer*)location;
					button->setSharedRenderer(clapSvg);
				}
				else if ( buttonName == "move" )
					button = new ThymioMoveAction();
				else if ( buttonName == "color" )
					button = new ThymioColorAction();
				else if ( buttonName == "circle" )
					button = new ThymioCircleAction();
				else if ( buttonName == "sound" )			
					button = new ThymioSoundAction();
				else if( buttonName == "memory" )
					button = new ThymioMemoryAction();
							
				//qDebug() << "  == TBS -- drop event : created " << buttonName << " button, # " << numButtons;

				if( button ) 
				{
					event->setDropAction(Qt::MoveAction);
					event->accept();
					
					//qDebug() << "  == TBS -- drop event : accepted the button.";
					
					if( event->mimeData()->data("thymiotype") == QString("event").toLatin1() )
					{
						addEventButton(button);
						highlightEventButton = false;
					}
					else if( event->mimeData()->data("thymiotype") == QString("action").toLatin1() )
					{
						addActionButton(button);
						highlightActionButton = false;
					}
					
					for( int i=0; i<numButtons; ++i ) 
					{
						int status;
						dataStream >> status;	
						button->setClicked(i, status);
					}

				}
				//qDebug() << "  == TBS -- drop event : added the button.";
			}
			
			update();
		}
		else
			event->ignore();

		//qDebug() << "  == TBS -- drop event : done";
	}

	void ThymioButtonSet::mousePressEvent( QGraphicsSceneMouseEvent * event )
	{
		setCursor(Qt::ClosedHandCursor);
		QGraphicsItem::mousePressEvent(event);
	}

	void ThymioButtonSet::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
	{
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()) 
			return;

		QByteArray buttonData;
		QDataStream dataStream(&buttonData, QIODevice::WriteOnly);
		
		dataStream << getRow();

		QMimeData *mime = new QMimeData;
		mime->setData("thymiobuttonset", buttonData);
		
		QDrag *drag = new QDrag(event->widget());		
		QPixmap img = image();
		drag->setMimeData(mime);
		drag->setPixmap(img.scaled((1000+trans)*scale(),336*scale()));
		drag->setHotSpot(QPoint((500+trans*0.5)*scale(), 168*scale()));

		drag->exec(Qt::MoveAction);
		setCursor(Qt::OpenHandCursor);
	}

	void ThymioButtonSet::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
	{
		setCursor(Qt::OpenHandCursor);
		QGraphicsItem::mouseReleaseEvent(event);		
	}

	// Thymio Push Button
	ThymioPushButton::ThymioPushButton(QString name, QSvgRenderer *renderer, QWidget *parent) : 
		QPushButton(parent), 
		thymioButton(0),
		prevSpan(128)
	{ 
		if( name == "button" )
			thymioButton = new ThymioButtonsEvent();
		else if( name == "prox" )
			thymioButton = new ThymioProxEvent();
		else if( name == "proxground" )
			thymioButton = new ThymioProxGroundEvent();
		else if( name == "tap" )		
			thymioButton = new ThymioTapEvent();
		else if( name == "clap" )
			thymioButton = new ThymioClapEvent();
		else if( name == "move" )
			thymioButton = new ThymioMoveAction();
		else if( name == "color" )
			thymioButton = new ThymioColorAction();
		else if( name == "circle" )
			thymioButton = new ThymioCircleAction();
		else if( name == "sound" )
			thymioButton = new ThymioSoundAction();
		else if( name == "memory" )
			thymioButton = new ThymioMemoryAction();

		if( renderer )
			thymioButton->setSharedRenderer(renderer);		

		setIcon(thymioButton->image()); 
		setToolTip(QString("%0 %1").arg(thymioButton->getName()).arg(thymioButton->getType()));
		
		setMinimumSize(64,64);
		setMaximumSize(256,256);
		setIconSize(QSize(128,128));
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		
		setAcceptDrops(true);
	}
	
	ThymioPushButton::~ThymioPushButton()
	{ 
		if( thymioButton != 0 ) 
			delete(thymioButton); 
	}	
	
	void ThymioPushButton::changeButtonColor(QColor color) 
	{ 
		thymioButton->setButtonColor(color); 
		setIcon(thymioButton->image()); 
	}	

	void ThymioPushButton::mouseMoveEvent( QMouseEvent *event )
	{
		if( thymioButton==0 )
			return;
			
		QByteArray buttonData;
		QDataStream dataStream(&buttonData, QIODevice::WriteOnly);
		int numButtons = thymioButton->getNumButtons();
		dataStream << -1 << thymioButton->getName() << numButtons;
		
		for(int i=0; i<numButtons; ++i)
			dataStream << thymioButton->isClicked(i);			

		QMimeData *mime = new QMimeData;
		mime->setData("thymiobutton", buttonData);
		mime->setData("thymiotype", thymioButton->getType().toLatin1());

		if( thymioButton->renderer() ) 
		{
			QByteArray rendererData;
			QDataStream rendererStream(&rendererData, QIODevice::WriteOnly);
			quint64 location  = (quint64)(thymioButton->renderer());
			rendererStream << location;
			mime->setData("thymiorenderer", rendererData);
		}	

		QDrag *drag = new QDrag(this);		
		QPixmap img = thymioButton->image(false);		
		drag->setMimeData(mime);
		drag->setPixmap(img.scaled(iconSize()));
		drag->setHotSpot(QPoint(iconSize().width(), iconSize().height()));

		drag->exec();
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

