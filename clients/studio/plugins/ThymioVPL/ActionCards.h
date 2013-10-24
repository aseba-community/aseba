#ifndef VPL_ACTION_CARDS_H
#define VPL_ACTION_CARDS_H

#include <QList>

#include "Card.h"

class QSlider;
class QTimeLine;

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class MoveActionCard : public Card
	{
		Q_OBJECT
	
	public:
		MoveActionCard(QGraphicsItem *parent=0);
		virtual ~MoveActionCard();
	
		virtual unsigned valuesCount() const { return 2; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		
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
		
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
	
	public:
		virtual unsigned valuesCount() const { return 3; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		
	private slots:
		void valueChangeDetected();
	
	private:
		QList<QSlider *> sliders;
	};
	
	struct TopColorActionCard : public ColorActionCard
	{
		TopColorActionCard(QGraphicsItem *parent=0);
	};
	
	struct BottomColorActionCard : public ColorActionCard
	{
		BottomColorActionCard(QGraphicsItem *parent=0);
	};
	
	class SoundActionCard : public CardWithBody
	{
	public:
		SoundActionCard(QGraphicsItem *parent=0);
		
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		virtual unsigned valuesCount() const { return 6; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		
	protected:
		void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		
	protected:
		unsigned notes[6];
		unsigned durations[6];
	};
	
	class TimerActionCard: public CardWithBody
	{
		Q_OBJECT
		
	public:
		TimerActionCard(QGraphicsItem *parent=0);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		virtual unsigned valuesCount() const { return 1; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
	
	protected slots:
		void frameChanged(int frame);
		
	protected:
		void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		void durationUpdated();
		
	protected:
		unsigned duration;
		QTimeLine *timer;
	};

	class StateFilterActionCard : public CardWithButtons
	{
	public:
		StateFilterActionCard(QGraphicsItem *parent=0);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif