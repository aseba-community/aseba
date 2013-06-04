#ifndef VPL_CARD_H
#define VPL_CARD_H

#include <QObject>
#include <QGraphicsObject>
#include "ThymioIntermediateRepresentation.h"

class QMimeData;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class ThymioIRButton;
	class ThymioClickableButton;
	
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
	class Card : public QGraphicsObject
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
		
		static Card* createButton(const QString& name, bool advancedMode=false);
		
		Card(bool eventButton = true, bool advanced=false, QGraphicsItem *parent=0);
		virtual ~Card();
		
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		QRectF boundingRect() const { return QRectF(0, 0, 256, 256); }

		void setButtonColor(QColor color) { buttonColor = color; update(); }
		QColor getButtonColor() const { return buttonColor; }
		void setParentID(int id) { parentID = id; }
		int getParentID() const { return parentID; }
		QString getType() const { return data(0).toString(); }
		QString getName() const { return data(1).toString(); }
		
		virtual int valuesCount() const = 0;
		virtual int getValue(int i) const = 0;
		virtual void setValue(int i, int status) = 0;
		
		bool isAnyStateFilter() const;
		int getStateFilter() const;
		void setStateFilter(int val);
		void setAdvanced(bool advanced);
		
		void setScaleFactor(qreal factor);
		
		void render(QPainter& painter);
		virtual QPixmap image(qreal factor=1);
		QMimeData* mimeData() const;
		
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
		QList<ThymioClickableButton*> stateButtons;
		
		ThymioIRButton *buttonIR;
		QColor buttonColor;
		int parentID;
		qreal trans;

		virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
	};
	
	class CardWithBody: public Card
	{
	public:
		CardWithBody(bool eventButton, bool up, bool advanced, QGraphicsItem *parent);
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
	
	protected:
		bool up;
		QColor bodyColor;
	};
	
	class CardWithButtons: public CardWithBody
	{
	public:
		CardWithButtons(bool eventButton, bool up, bool advanced, QGraphicsItem *parent);
		
		virtual int valuesCount() const;
		virtual int getValue(int i) const;
		virtual void setValue(int i, int status);
		
	protected:
		QList<ThymioClickableButton*> buttons;
	};
	
	/*@}*/
}; // Aseba

#endif
