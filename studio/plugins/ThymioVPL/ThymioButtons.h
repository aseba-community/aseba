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
#include <QSlider>
#include <QGraphicsProxyWidget>
#include <QGraphicsItemAnimation>
#include <QTimeLine>

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
		int getNumStates() const { return numStates; }

		QRectF boundingRect() const { return boundingRectangle; }
		void setButtonColor(QColor color) { buttonColor = color; }
		void setBeginButtonColor(QColor color) { buttonBeginColor = color; }

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
		QColor buttonBeginColor;

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
			ThymioBody(QGraphicsItem *parent=0) : QGraphicsItem(parent), bodyColor(Qt::white), up(true) { }
			virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }
			void setUp(bool u) { up = u; }
			
			QColor bodyColor;
		private:
			bool up;
		};
		
		ThymioButton(bool eventButton = true, qreal scale=1.0, bool up=true, bool advanced=false, QGraphicsItem *parent=0);
		~ThymioButton();

		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return QRectF(0, 0, 256+trans, 256); }

		void setButtonColor(QColor color) { buttonColor = color; update(); }
		QColor getButtonColor() const { return buttonColor; }
		virtual void setClicked(int i, int status);
		virtual int isClicked(int i) { if( i<thymioButtons.size() ) return thymioButtons.at(i)->isClicked(); return -1; }

		void setParentID(int id) { parentID = id; }
		int getParentID() const { return parentID; }
		QString getType() const { return data(0).toString(); }
		QString getName() const { return data(1).toString(); }
		virtual int getNumButtons() { return thymioButtons.size(); }
		void setScaleFactor(qreal factor);
		void setAdvanced(bool advanced);
		int getState() const;
		void setState(int val);
		
		virtual QPixmap image(bool on=true);
	
		virtual bool isValid(); // returns true if the current button state is valid, i.e., at least one button is on
	
		ThymioIRButton *getIRButton();

	signals:
		void stateChanged();

	private slots:
		void updateIRButton();
		
	protected:
		QList<ThymioClickableButton*> thymioButtons;
		QList<ThymioClickableButton*> stateButtons;
		ThymioBody *thymioBody;
		ThymioIRButton *buttonIR;
		QColor buttonColor;
		int parentID;
		qreal scaleFactor;
		qreal trans;

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
			
		ThymioButtonSet(int row, bool advanced, QGraphicsItem *parent=0);
		
		virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);		
		QRectF boundingRect() const { return advancedMode? QRectF(0, 0, 1064, 400) : QRectF(0, 0, 1000, 400); }

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
		void setAdvanced(bool advanced);
		
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
		
		QColor eventButtonColor;
		QColor actionButtonColor;
		
		bool highlightEventButton;
		bool highlightActionButton;
		bool errorFlag;
		bool advancedMode;
		qreal trans;
		qreal xpos;
		
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
		ThymioButtonsEvent(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	// Proximity Event
	class ThymioProxEvent : public ThymioButton
	{
	public:
		ThymioProxEvent(QGraphicsItem *parent=0, bool advanced=false);
	};

	// Proximity Ground Event
	class ThymioProxGroundEvent : public ThymioButton
	{
	public:
		ThymioProxGroundEvent(QGraphicsItem *parent=0, bool advanced=false);
	};

	// Tap Event
	class ThymioTapEvent : public ThymioButton
	{
	public:
		ThymioTapEvent(QGraphicsItem *parent=0, bool advanced=false);
	};

	// Clap Event
	class ThymioClapEvent : public ThymioButton
	{
	public:
		ThymioClapEvent(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	// Move Action
	class ThymioMoveAction : public ThymioButton
	{
		Q_OBJECT
						
	public:
		ThymioMoveAction(QGraphicsItem *parent=0);
		virtual ~ThymioMoveAction();
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		virtual QPixmap image(bool on=true);
		virtual void setClicked(int i, int status);
		virtual int isClicked(int i);
		virtual	int getNumButtons() { return 2; }

	private slots:
		void valueChangeDetected();
				
	private:
		QList<QGraphicsProxyWidget *> widgets;
		QList<QSlider *> sliders;
		QGraphicsItemAnimation *animation;
		QTimeLine *timer;
	};
	
	// Color Action
	class ThymioColorAction : public ThymioButton
	{
		Q_OBJECT
		
	public:
		ThymioColorAction(QGraphicsItem *parent=0);
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		virtual QPixmap image(bool on=true);
		virtual void setClicked(int i, int status);
		virtual int isClicked(int i);
		virtual	int getNumButtons() { return 3; }
			
	private slots:
		void valueChangeDetected();
	
	private:
		QList<QGraphicsProxyWidget *> widgets;
		QList<QSlider *> sliders;
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

	// Memory Action
	class ThymioMemoryAction : public ThymioButton
	{		
	public:
		ThymioMemoryAction(QGraphicsItem *parent=0);
	};
	
	/*@}*/
}; // Aseba

#endif
