/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VPL_ACTION_BLOCKS_H
#define VPL_ACTION_BLOCKS_H

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
		virtual QVector<quint16> getValuesCompressed() const;
		
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
		virtual QVector<quint16> getValuesCompressed() const;
		
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
	
	class SoundActionBlock : public Block
	{
	public:
		SoundActionBlock(QGraphicsItem *parent=0);
		
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		virtual unsigned valuesCount() const { return 6; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		virtual QVector<quint16> getValuesCompressed() const;
		
	protected:
		virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
		
		void idxAndValFromPos(const QPointF& pos, bool* ok, unsigned& noteIdx, unsigned& noteVal);
		void setNote(unsigned noteIdx, unsigned noteVal);
		void setDuration(unsigned noteIdx, unsigned durationVal);
		
	protected:
		bool dragging;
		unsigned notes[6];
		unsigned durations[6];
	};
	
	class TimerActionBlock: public Block
	{
		Q_OBJECT
		
	public:
		TimerActionBlock(QGraphicsItem *parent=0);
		
		void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		
		virtual unsigned valuesCount() const { return 1; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		virtual QVector<quint16> getValuesCompressed() const;
		
		virtual bool isAdvancedBlock() const { return true; }
		
	protected slots:
		void frameChanged(int frame);
		
	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
		
		unsigned durationFromPos(const QPointF& pos, bool* ok) const;
		void setDuration(unsigned duration);
		
	protected:
		bool dragging;
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
