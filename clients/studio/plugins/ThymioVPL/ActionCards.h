#ifndef VPL_ACTION_CARDS_H
#define VPL_ACTION_CARDS_H

#include "Card.h"
#include <QList>

class QSlider;
class QTimeLine;

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class MoveActionCard : public Card
	{
		Q_OBJECT
	
	public:
		MoveActionCard(QGraphicsItem *parent=0);
		virtual ~MoveActionCard();
	
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
	
	class ColorActionCard : public CardWithBody
	{
		Q_OBJECT
		
	protected:
		ColorActionCard(QGraphicsItem *parent, bool top);
		
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
	
	struct TopColorActionCard : public ColorActionCard
	{
		TopColorActionCard(QGraphicsItem *parent=0);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_COLOR_TOP_IR; }
	};
	
	struct BottomColorActionCard : public ColorActionCard
	{
		BottomColorActionCard(QGraphicsItem *parent=0);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_COLOR_BOTTOM_IR; }
	};
	
	class SoundActionCard : public CardWithBody
	{
	public:
		SoundActionCard(QGraphicsItem *parent=0);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		virtual int valuesCount() const { return 6; }
		virtual int getValue(int i) const;
		virtual void setValue(int i, int value);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_SOUND_IR; }
		void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		
	protected:
		unsigned notes[6];
		unsigned durations[6];
	};

	class StateFilterActionCard : public CardWithButtons
	{
	public:
		StateFilterActionCard(QGraphicsItem *parent=0);
		
	protected:
		ThymioIRButtonName getIRIdentifier() const { return THYMIO_MEMORY_IR; }
	};
	
	/*@}*/
}; // Aseba

#endif