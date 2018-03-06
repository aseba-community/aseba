/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

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

#ifndef VPL_EVENT_BLOCKS_H
#define VPL_EVENT_BLOCKS_H

#include "Block.h"

class QGraphicsSvgItem;

namespace Aseba { namespace ThymioVPL
{
	/** \addtogroup studio */
	/*@{*/

	class ArrowButtonsEventBlock : public BlockWithButtons
	{
		Q_OBJECT

	public:
		enum
		{
			MODE_ARROW = 0,
			MODE_RC_ARROW,
			MODE_RC_KEYPAD
		};

	public:
		ArrowButtonsEventBlock(bool advanced, QGraphicsItem *parent=0);

		virtual unsigned valuesCount() const { return 7; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		virtual QVector<quint16> getValuesCompressed() const;

		virtual bool isAnyAdvancedFeature() const;
		virtual void setAdvanced(bool advanced);

		unsigned getMode() const { return mode; }
		int getSelectedRCArrowButton() const;
		int getSelectedRCKeypadButton() const;

	protected:
		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
		virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);

		void setMode(unsigned mode);
		void setButtonsPos(bool advanced);

	protected slots:
		void ensureSingleRCArrowButtonSelected(int current = -1);
		void ensureSingleRCKeypadButtonSelected(int current = -1);

	protected:
		static const QRectF buttonPoses[3];

		bool advanced;
		unsigned mode;
	};

	class ProxEventBlock : public BlockWithButtonsAndRange
	{
	public:
		ProxEventBlock(bool advanced, QGraphicsItem *parent=0);
	};

	class ProxGroundEventBlock : public BlockWithButtonsAndRange
	{
	public:
		ProxGroundEventBlock(bool advanced, QGraphicsItem *parent=0);
	};

	class AccEventBlock : public Block
	{
	public:
		enum
		{
			MODE_TAP = 0,
			MODE_ROLL,
			MODE_PITCH
		};
		static const int resolution;

	public:
		AccEventBlock(bool advanced, QGraphicsItem *parent=0);

		virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);

		virtual unsigned valuesCount() const { return 2; }
		virtual int getValue(unsigned i) const;
		virtual void setValue(unsigned i, int value);
		virtual QVector<uint16_t> getValuesCompressed() const;

		virtual bool isAnyAdvancedFeature() const;
		virtual void setAdvanced(bool advanced);

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

		int orientationFromPos(const QPointF& pos, bool* ok) const;
		void setMode(unsigned mode);
		void setOrientation(int orientation);

	protected:
		static const QRectF buttonPoses[3];

		unsigned mode;
		int orientation;
		bool dragging;

		QGraphicsSvgItem* tapSimpleSvg;
		QGraphicsSvgItem* tapAdvancedSvg;
		QGraphicsSvgItem* quadrantSvg;
		QGraphicsSvgItem* pitchSvg;
		QGraphicsSvgItem* rollSvg;
	};

	class ClapEventBlock : public BlockWithNoValues
	{
	public:
		ClapEventBlock(QGraphicsItem *parent=0);
	};

	class TimeoutEventBlock : public BlockWithNoValues
	{
	public:
		TimeoutEventBlock(QGraphicsItem *parent=0);

		virtual bool isAdvancedBlock() const { return true; }
	};

	/*@}*/
} } // namespace ThymioVPL / namespace Aseba

#endif
