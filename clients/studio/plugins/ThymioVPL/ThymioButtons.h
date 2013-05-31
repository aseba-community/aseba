#ifndef THYMIO_BUTTONS_H
#define THYMIO_BUTTONS_H

#include <QGraphicsItem>
#include <QPushButton>

#include "ThymioIntermediateRepresentation.h"

class QSlider;
class QTimeLine;
class QMimeData;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	enum ThymioCardType 
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
		ThymioClickableButton (const QRectF rect, const ThymioCardType type, QGraphicsItem *parent=0, const QColor& initBrushColor = Qt::white, const QColor& initPenColor = Qt::black);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return boundingRectangle; }

		int getValue() const { return curState; }
		void setValue(int state) { curState = state; update(); }
		void setToggleState(bool state) { toggleState = state; update(); }

		void addState(const QColor& brushColor = Qt::white, const QColor& penColor = Qt::black);

		void addSibling(ThymioClickableButton *s) { siblings.push_back(s); }
	
	signals:
		void stateChanged();
	
	protected:
		const ThymioCardType buttonType;
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

	/**
		An "event" or "action" card.
		
		These cards have a type (event or action) and a name (prox, etc.)
		and may provide several values (set/get by setValue()/getValue()).
		These values are set by the user through buttons (typically
		ThymioClickableButton), sliders, or specific widgets.
		
		In addition, event cards can have an associated state filter,
		which is an array of 4 tri-bool value (yes/no/don't care) "serialized"
		in a single integer (2 bits per value). The state filter
		can be set/get using setStateFilter()/getStateFilter().
	*/
	class ThymioCard : public QGraphicsObject
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
			
			static void drawBody(QPainter * painter, int xShift, int yShift, bool up, const QColor& bodyColor);
			
			QColor bodyColor;
			
		private:
			const int yShift;
			bool up;
		};
		
		static ThymioCard* createButton(const QString& name, bool advancedMode=false);
		
		ThymioCard(bool eventButton = true, bool advanced=false, QGraphicsItem *parent=0);
		virtual ~ThymioCard();
		
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }

		void setButtonColor(QColor color) { buttonColor = color; update(); }
		QColor getButtonColor() const { return buttonColor; }
		void setParentID(int id) { parentID = id; }
		int getParentID() const { return parentID; }
		QString getType() const { return data(0).toString(); }
		QString getName() const { return data(1).toString(); }
		
		virtual int valuesCount() const { return thymioButtons.size(); }
		virtual int getValue(int i) const;
		virtual void setValue(int i, int status);
		
		bool isAnyStateFilter() const;
		int getStateFilter() const;
		void setStateFilter(int val);
		void setAdvanced(bool advanced);
		
		
		void setScaleFactor(qreal factor);
		
		void render(QPainter& painter);
		virtual QPixmap image(qreal factor=1);
		QMimeData* mimeData() const;
	
		virtual bool isValid(); // returns true if the current button state is valid, i.e., at least one button is on
	
		ThymioIRButton *getIRButton();

	signals:
		void stateChanged();

	protected slots:
		void updateIRButtonAndNotify();
		
	protected:
		void updateIRButton();
		void addAdvancedModeButtons();
		virtual ThymioIRButtonName getIRIdentifier() const = 0;
		
	protected:
		// TODO: add only this in a subclass
		QList<ThymioClickableButton*> thymioButtons;
		QList<ThymioClickableButton*> stateButtons;
		
		ThymioIRButton *buttonIR;
		QColor buttonColor;
		int parentID;
		qreal trans;

		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
	};
	
	class ThymioCardWithBody: public ThymioCard
	{
	public:
		ThymioCardWithBody(bool eventButton, bool up, bool advanced, QGraphicsItem *parent);
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
	
	protected:
		bool up;
	};
	
	class ThymioPushButton : public QPushButton
	{
		Q_OBJECT
		
	public:
		ThymioPushButton(const QString& name, QWidget *parent=0);
		~ThymioPushButton();
		
		void changeButtonColor(const QColor& color);
		
	protected:
		virtual void mouseMoveEvent( QMouseEvent *event );
		virtual void dragEnterEvent( QDragEnterEvent *event );
		virtual void dropEvent( QDropEvent *event );

	private:
		ThymioCard *thymioButton;
	};
	
	// Buttons Event
	class ThymioButtonsEvent : public ThymioCardWithBody
	{
	public:
		ThymioButtonsEvent(QGraphicsItem *parent=0, bool advanced=false);
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_BUTTONS_IR; }
	};
	
	// Proximity Event
	class ThymioProxEvent : public ThymioCardWithBody
	{
	public:
		ThymioProxEvent(QGraphicsItem *parent=0, bool advanced=false);
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_PROX_IR; }
	};

	// Proximity Ground Event
	class ThymioProxGroundEvent : public ThymioCardWithBody
	{
	public:
		ThymioProxGroundEvent(QGraphicsItem *parent=0, bool advanced=false);
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_PROX_GROUND_IR; }
	};

	// Tap Event
	class ThymioTapEvent : public ThymioCard
	{
	public:
		ThymioTapEvent(QGraphicsItem *parent=0, bool advanced=false);
	
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_TAP_IR; }
	};

	// Clap Event
	class ThymioClapEvent : public ThymioCard
	{
	public:
		ThymioClapEvent(QGraphicsItem *parent=0, bool advanced=false);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_CLAP_IR; }
	};
	
	// Move Action
	class ThymioMoveAction : public ThymioCard
	{
		Q_OBJECT
	
	public:
		ThymioMoveAction(QGraphicsItem *parent=0);
		virtual ~ThymioMoveAction();
	
		virtual int valuesCount() const { return 2; }
		virtual int getValue(int i) const;
		virtual void setValue(int i, int status);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_MOVE_IR; }

	private slots:
		void frameChanged(int frame);
		void valueChangeDetected();
		
	private:
		QList<QSlider *> sliders;
		QTimeLine *timer;
		ThymioBody* thymioBody;
	};
	
	// Color Action
	class ThymioColorAction : public ThymioCardWithBody
	{
		Q_OBJECT
		
	protected:
		ThymioColorAction(QGraphicsItem *parent, bool top);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
	
	public:
		virtual int valuesCount() const { return 3; }
		virtual int getValue(int i) const;
		virtual void setValue(int i, int value);
		
	private slots:
		void valueChangeDetected();
	
	private:
		QList<QSlider *> sliders;
	};
	
	struct ThymioColorTopAction : public ThymioColorAction
	{
		ThymioColorTopAction(QGraphicsItem *parent=0);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_COLOR_TOP_IR; }
	};
	
	struct ThymioColorBottomAction : public ThymioColorAction
	{
		ThymioColorBottomAction(QGraphicsItem *parent=0);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_COLOR_BOTTOM_IR; }
	};
	
	// Sound Action
	class ThymioSoundAction : public ThymioCardWithBody
	{
	public:
		ThymioSoundAction(QGraphicsItem *parent=0);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		virtual int valuesCount() const { return 8; }
		virtual int getValue(int i) const;
		virtual void setValue(int i, int value);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_SOUND_IR; }
		void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		
	protected:
		int notes[8];
	};

	// Memory Action
	class ThymioMemoryAction : public ThymioCardWithBody
	{
	public:
		ThymioMemoryAction(QGraphicsItem *parent=0);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_MEMORY_IR; }
	};
	
	/*@}*/
}; // Aseba

#endif
