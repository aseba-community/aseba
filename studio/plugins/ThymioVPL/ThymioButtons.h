#ifndef THYMIO_BUTTONS_H
#define THYMIO_BUTTONS_H

#include <QGraphicsItem>
#include <QGraphicsSvgItem>
#include <QGraphicsLayoutItem>
#include <QtCore/qmath.h>
#include <QMimeData>
#include <QPushButton>
#include <QResizeEvent>
#include <QDebug>

#include "ThymioIntermediateRepresentation.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	enum ThymioButtonType 
	{
		THYMIO_CIRCULAR_BUTTON = 0,
		THYMIO_TRIANGULAR_BUTTON,
		THYMIO_RECTANGULAR_BUTTON
	};

	enum ThymioSmileType 
	{
		THYMIO_SMILE_BUTTON = 0,
		THYMIO_NEUTRAL_BUTTON,
		THYMIO_FROWN_BUTTON
	};

	class ThymioClickableButton : public QGraphicsObject
	{	
		Q_OBJECT
		
	public:
		ThymioClickableButton ( QRectF rect, ThymioButtonType type, int nstates = 2, QGraphicsItem *parent=0 );
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);

		int isClicked() { return buttonClicked; }
		void setClicked(int clicked) { buttonClicked = clicked; }
		void setToggleState(bool state) { toggleState = state; }
		void setNumStates(int num) { numStates = num > 2 ? num : 2; }
		int getNumStates() { return numStates; }

		QRectF boundingRect() const { return boundingRectangle; }
		void setButtonColor(QColor color) { buttonColor = color; }

		void addSibling(ThymioClickableButton *s) { siblings.push_back(s); }
	
	signals:
		void stateChanged();
	
	protected:
		ThymioButtonType buttonType;
		int buttonClicked;
		bool toggleState;
		int numStates;
		
		QRectF boundingRectangle;		
		QColor buttonColor;

		QList<ThymioClickableButton*> siblings;

		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
	};

	class ThymioFaceButton : public ThymioClickableButton
	{
	public:
		ThymioFaceButton ( QRectF rect, ThymioSmileType type, QGraphicsItem *parent=0 );
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
	private:
		QRectF leftEye;
		QRectF rightEye;
		QRectF mouth;
		qreal arcStart;
		qreal arcEnd;
	};

	// Button
	class ThymioButton : public QGraphicsSvgItem
	{
		Q_OBJECT
		
	public:
		class ThymioBody : public QGraphicsItem
		{
		public:
			ThymioBody(QGraphicsItem *parent=0) : QGraphicsItem(parent), up(true), bodyColor(Qt::white) { }
			virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }
			void setUp(bool u) { up = u; }
			
			QColor bodyColor;
		private:
			bool up;
		};
		
		ThymioButton(bool eventButton = true, qreal scale=1.0, bool up=true, QGraphicsItem *parent=0);
		~ThymioButton();

		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }

		void setButtonColor(QColor color) { buttonColor = color; update(); }
		QColor getButtonColor() { return buttonColor; }
		void setClicked(int i, int status);
		int isClicked(int i) { if( i<thymioButtons.size() ) return thymioButtons.at(i)->isClicked(); return -1; }

		void setParentID(int id) { parentID = id; }
		int getParentID() { return parentID; }
		QString getType() { return data(0).toString(); }
		QString getName() { return data(1).toString(); }
		int getNumButtons() { return thymioButtons.size(); }
		void setScaleFactor(qreal factor) { scaleFactor = factor; } 
		
		virtual QPixmap image(bool on=true);
	
		virtual bool isValid(); // returns true if the current button state is valid, i.e., at least one button is on
	
		ThymioIRButton *getIRButton();

	signals:
		void stateChanged();

	private slots:
		void updateIRButton();
		
	protected:
		QList<ThymioClickableButton*> thymioButtons;
		ThymioBody *thymioBody;
		ThymioIRButton *buttonIR;
		QColor buttonColor;
		int parentID;
		qreal scaleFactor;

		QPointF dragStartPosition;

		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );
		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
	};
	
	class ThymioButtonSet : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		class ThymioRemoveButton : public QGraphicsItem
		{
		public:
			ThymioRemoveButton(QGraphicsItem *parent=0);
			virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);			
			QRectF boundingRect() const { return QRectF(-64, -64, 128, 128); }
		};

		class ThymioAddButton : public QGraphicsItem
		{
		public:
			ThymioAddButton(QGraphicsItem *parent=0);
			virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);			
			QRectF boundingRect() const { return QRectF(-64, -64, 128, 128); }
		};
			
		ThymioButtonSet(int row, QGraphicsItem *parent=0);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);		
		QRectF boundingRect() const { return QRectF(0, 0, 1000, 400); }

		void addEventButton(ThymioButton *event);
		void addActionButton(ThymioButton *action);
		void setRow(int row);
		int getRow() { return data(1).toInt(); }
		ThymioButton *getEventButton() { return eventButton; }
		ThymioButton *getActionButton() { return actionButton; }
		
		bool eventExists() { return eventButton == 0 ? false : true; }
		bool actionExists() { return actionButton == 0 ? false : true; }
		void setColorScheme(QColor eventColor, QColor actionColor);
		
		virtual QPixmap image();
		void setScale(qreal factor);
		
		ThymioIRButtonSet *getIRButtonSet() { return &buttonSetIR; }

		void setErrorStatus(bool flag) { errorFlag = flag; }
		
	signals:
		void buttonUpdated();
	
	public slots:
		void stateChanged();
	
	private:
		ThymioButton *eventButton;
		ThymioButton *actionButton;
		ThymioRemoveButton *deleteButton;
		ThymioAddButton *addButton;
		ThymioIRButtonSet buttonSetIR;
		
		bool highlightEventButton;
		bool highlightActionButton;
		bool errorFlag;
		
		QColor eventButtonColor;
		QColor actionButtonColor;
		
		virtual void mousePressEvent ( QGraphicsSceneMouseEvent *event );
		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
		virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );

		virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
		virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
	};
	
	class ThymioPushButton : public QPushButton
	{
		Q_OBJECT
				
	public:
		ThymioPushButton(QString name, QSvgRenderer *renderer=0, QWidget *parent=0);
		~ThymioPushButton();
			
		void changeButtonColor(QColor color);
		
	protected:
		virtual void mouseMoveEvent( QMouseEvent *event );
		virtual void dragEnterEvent( QDragEnterEvent *event );
		virtual void dropEvent( QDropEvent *event );

	private:
		ThymioButton *thymioButton;
		int prevSpan;
	};	
	
	// Buttons Event
	class ThymioButtonsEvent : public ThymioButton
	{
	public:
		ThymioButtonsEvent(QGraphicsItem *parent=0);
	};
	
	// Proximity Event
	class ThymioProxEvent : public ThymioButton
	{
	public:
		ThymioProxEvent(QGraphicsItem *parent=0);		
	};

	// Proximity Ground Event
	class ThymioProxGroundEvent : public ThymioButton
	{
	public:
		ThymioProxGroundEvent(QGraphicsItem *parent=0);
	};

	// Tap Event
	class ThymioTapEvent : public ThymioButton
	{
	public:
		ThymioTapEvent(QGraphicsItem *parent=0);
	};

	// Clap Event
	class ThymioClapEvent : public ThymioButton
	{
	public:
		ThymioClapEvent(QGraphicsItem *parent=0);
	};
	
	// Move Action
	class ThymioMoveAction : public ThymioButton
	{
	public:
		ThymioMoveAction(QGraphicsItem *parent=0);
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
	};
	
	// Color Action
	class ThymioColorAction : public ThymioButton
	{
	public:
		ThymioColorAction(QGraphicsItem *parent=0);
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);		
	};
	
	// Circle Action
	class ThymioCircleAction : public ThymioButton
	{
	public:
		ThymioCircleAction(QGraphicsItem *parent=0);
	};

	// Sound Action
	class ThymioSoundAction : public ThymioButton
	{
	public:
		class Speaker : public QGraphicsItem
		{
		public:
			Speaker(QGraphicsItem *parent=0) : QGraphicsItem(parent) {}
			virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }
		};
	
		ThymioSoundAction(QGraphicsItem *parent=0);
		virtual QPixmap image(bool on=true);
		
	protected:
		Speaker *speaker;
	};

	// Reset Action
//	class ThymioResetAction : public ThymioButton
//	{		
//	public:
//		class Reset : public QGraphicsItem
//		{
//		public:
//			Reset(QGraphicsItem *parent=0) : QGraphicsItem(parent) {}
//			virtual void paint (QPainter *painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
//			QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }
//		};
//	
//		ThymioResetAction(QGraphicsItem *parent=0);
//		virtual QPixmap image(bool on=true);
//
//	protected:
//		Reset *logo;
//	};
//	
	/*@}*/
}; // Aseba

#endif
