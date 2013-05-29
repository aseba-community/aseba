#ifndef THYMIO_BUTTONS_H
#define THYMIO_BUTTONS_H

#include <QGraphicsItem>
#include <QPushButton>

#include "ThymioIntermediateRepresentation.h"

class QSlider;
class QTimeLine;

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
		//! Create a button with initially one state
		ThymioClickableButton (const QRectF rect, const ThymioButtonType type, QGraphicsItem *parent=0, const QColor& initBrushColor = Qt::white, const QColor& initPenColor = Qt::black);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return boundingRectangle; }

		int isClicked() { return curState; }
		void setClicked(int state) { curState = state; update(); }
		void setToggleState(bool state) { toggleState = state; update(); }

		void addState(const QColor& brushColor = Qt::white, const QColor& penColor = Qt::black);

		void addSibling(ThymioClickableButton *s) { siblings.push_back(s); }
	
	signals:
		void stateChanged();
	
	protected:
		const ThymioButtonType buttonType;
		const QRectF boundingRectangle;
		
		int curState;
		bool toggleState;
		
		//! a (brush,pen) color pair
		typedef QPair<QColor, QColor> ColorPair;
		QList<ColorPair> colors;

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
	class ThymioButton : public QGraphicsObject
	{
		Q_OBJECT
		
	public:
		class ThymioBody : public QGraphicsItem
		{
		public:
			ThymioBody(QGraphicsItem *parent = 0, int yShift = 0) : QGraphicsItem(parent), bodyColor(Qt::white), yShift(yShift), up(true) { }
			
			void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
			QRectF boundingRect() const { return QRectF(-128, -128+yShift, 256, 256); }
			void setUp(bool u) { up = u; }
			
			QColor bodyColor;
		private:
			const int yShift;
			bool up;
		};
		
		ThymioButton(bool eventButton = true, bool advanced=false, QGraphicsItem *parent=0);
		virtual ~ThymioButton();
		
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }

		void setButtonColor(QColor color) { buttonColor = color; update(); }
		QColor getButtonColor() const { return buttonColor; }
		virtual void setClicked(int i, int status);
		virtual int isClicked(int i) { if( i<thymioButtons.size() ) return thymioButtons.at(i)->isClicked(); return -1; }
		// TODO: changed clicked to state

		void setParentID(int id) { parentID = id; }
		int getParentID() const { return parentID; }
		QString getType() const { return data(0).toString(); }
		QString getName() const { return data(1).toString(); }
		virtual int getNumButtons() { return thymioButtons.size(); }
		void setScaleFactor(qreal factor);
		void setAdvanced(bool advanced);
		int getState() const;
		void setState(int val);
		
		void render(QPainter& painter);
		virtual QPixmap image(qreal factor=1);
	
		virtual bool isValid(); // returns true if the current button state is valid, i.e., at least one button is on
	
		ThymioIRButton *getIRButton();

	signals:
		void stateChanged();

	protected slots:
		void updateIRButton();
		
	protected:
		void addAdvancedModeButtons();
		
	protected:
		QList<ThymioClickableButton*> thymioButtons;
		QList<ThymioClickableButton*> stateButtons;
		
		ThymioIRButton *buttonIR;
		QColor buttonColor;
		int parentID;
		qreal trans;

		QPointF dragStartPosition;

		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
	};
	
	class ThymioButtonWithBody: public ThymioButton
	{
	public:
		ThymioButtonWithBody(bool eventButton, bool up, bool advanced, QGraphicsItem *parent);
		~ThymioButtonWithBody();
		
	protected:
		ThymioBody *thymioBody;
	};
	
	class ThymioPushButton : public QPushButton
	{
		Q_OBJECT
		
	public:
		ThymioPushButton(QString name, QWidget *parent=0);
		~ThymioPushButton();
		
		void changeButtonColor(QColor color);
		
	protected:
		virtual void mouseMoveEvent( QMouseEvent *event );
		virtual void dragEnterEvent( QDragEnterEvent *event );
		virtual void dropEvent( QDropEvent *event );

	private:
		ThymioButton *thymioButton;
	};	
	
	// Buttons Event
	class ThymioButtonsEvent : public ThymioButtonWithBody
	{
	public:
		ThymioButtonsEvent(QGraphicsItem *parent=0, bool advanced=false);
	};
	
	// Proximity Event
	class ThymioProxEvent : public ThymioButtonWithBody
	{
	public:
		ThymioProxEvent(QGraphicsItem *parent=0, bool advanced=false);
	};

	// Proximity Ground Event
	class ThymioProxGroundEvent : public ThymioButtonWithBody
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
		virtual void setClicked(int i, int status);
		virtual int isClicked(int i);
		virtual int getNumButtons() { return 2; }

	private slots:
		void frameChanged(int frame);
		void valueChangeDetected();
		
	private:
		QList<QSlider *> sliders;
		QTimeLine *timer;
		ThymioBody* thymioBody;
	};
	
	// Color Action
	class ThymioColorAction : public ThymioButtonWithBody
	{
		Q_OBJECT
		
	public:
		ThymioColorAction(QGraphicsItem *parent=0);
		virtual void setClicked(int i, int status);
		virtual int isClicked(int i);
		virtual	int getNumButtons() { return 3; }
			
	private slots:
		void valueChangeDetected();
	
	private:
		QList<QSlider *> sliders;
	};
	
	// Circle Action
	class ThymioCircleAction : public ThymioButtonWithBody
	{
	public:
		ThymioCircleAction(QGraphicsItem *parent=0);
	};

	// Sound Action
	class ThymioSoundAction : public ThymioButtonWithBody
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
		
	protected:
		Speaker *speaker;
	};

	// Memory Action
	class ThymioMemoryAction : public ThymioButtonWithBody
	{		
	public:
		ThymioMemoryAction(QGraphicsItem *parent=0);
	};
	
	/*@}*/
}; // Aseba

#endif
