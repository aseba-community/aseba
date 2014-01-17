#ifndef VPL_ACTION_CARDS_H
#define VPL_ACTION_CARDS_H

#include <QList>

#include "Block.h"

class QSlider;
class QTimeLine;

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/
	
	class MoveActionBlock : public Block
	{
		Q_OBJECT
	
	public:
		MoveActionBlock(QGraphicsItem *parent=0);
		virtual ~MoveActionBlock();
	
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
	
	class ColorActionBlock : public BlockWithBody
	{
		Q_OBJECT
		
	protected:
		ColorActionBlock(QGraphicsItem *parent, bool top);
		
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
	
	struct TopColorActionBlock : public ColorActionBlock
	{
		TopColorActionBlock(QGraphicsItem *parent=0);
	};
	
	struct BottomColorActionBlock : public ColorActionBlock
	{
		BottomColorActionBlock(QGraphicsItem *parent=0);
	};
	
	class SoundActionBlock : public BlockWithBody
	{
	public:
		SoundActionBlock(QGraphicsItem *parent=0);
		
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
	
	class TimerActionBlock: public BlockWithBody
	{
		Q_OBJECT
		
	public:
		TimerActionBlock(QGraphicsItem *parent=0);
		
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

	class StateFilterActionBlock : public StateFilterBlock
	{
	public:
		StateFilterActionBlock(QGraphicsItem *parent=0);
	};
	
	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif