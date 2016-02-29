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

#include "Style.h"
#include <cassert>
#include <QList>
#include <QColor>

namespace Aseba { namespace ThymioVPL {
	
	// private 
	
	// sizes
	
	//! Width of a block
	const int Style::blockWidth = 256;
	
	//! Height of a block
	const int Style::blockHeight = 256;
	
	//! Spacing between block in an event-actions set
	const int Style::blockSpacing = 40;
	
	//! The width of the border of the block drop area
	const int Style::blockDropAreaBorderWidth = 10;
	
	//! The multiplicative factor to apply to the color saturation (in HSV) of the block drop area when it is not highlighted
	const qreal Style::blockDropAreaSaturationFactor = 0.7;
	
	//! The multiplicative factor to apply to the color value (in HSV) of the block drop area when it is not highlighted
	const qreal Style::blockDropAreaValueFactor = 1.0;
	
	
	//! The width of the add/remove button
	const int Style::addRemoveButtonWidth = 64;
	
	//! The height of the add/remove button
	const int Style::addRemoveButtonHeight = 64;
	
	//! The width of the remove block button
	const int Style::removeBlockButtonWidth = 58;
	
	//! The height of the remove block button
	const int Style::removeBlockButtonHeight = 58;
	
	
	//! The width of the column in the middle of the set
	const int Style::eventActionsSetColumnWidth = 40;
	
	//! Step size between event-actions sets
	const int Style::eventActionsSetRowStep = Style::blockHeight + 3*Style::blockSpacing + 64;
	
	//! Size of the rounded corner of event-actions sets
	const int Style::eventActionsSetCornerSize = 5;
	
	//! The normal/selected background colors of event-actions sets
	const QColor Style::eventActionsSetBackgroundColors[2] = { QColor(234, 234, 234), QColor(255, 255, 180) };
	
	//! The normal/selected foreground colors of event-actions sets
	const QColor Style::eventActionsSetForegroundColors[2] = { QColor(204, 204, 204), QColor(237, 237, 80) };
	
	
	//! Size of the rounded corner of add/remove buttons
	const int Style::addRemoveButtonCornerSize = 5;
	
	//! Background color of add/remove buttons
	const QColor Style::addRemoveButtonBackgroundColor(170, 170, 170);
	
	
	// global colors
	
	static struct PrivateStyle
	{
		typedef QList<QColor> ColorList;
		
		ColorList eventColors;
		ColorList stateColors;
		ColorList actionColors;
		unsigned currentColor;
		
		PrivateStyle():
			eventColors(ColorList()
				//<< QColor(0,191,255)
				<<  QColor(246,135,31)
				<< QColor(155,48,255)
				<< QColor(67,205,128)
				<< QColor(255,215,0)
				<< QColor(255,97,3)
				<< QColor(125,158,192)
			),
			stateColors(ColorList()
				<< QColor(0, 218, 3)
				<< QColor(122, 204, 0)
				<< QColor(122, 204, 0)
				<< QColor(122, 204, 0)
				<< QColor(122, 204, 0)
				<< QColor(122, 204, 0)
			),
			actionColors(ColorList()
				//<< QColor(218,112,214)
				<< QColor(0,150,219)
				<< QColor(159,182,205)
				<< QColor(0,197,205)
				<< QColor(255,99,71)
				<< QColor(142,56,142)
				<< QColor(56,142,142)
			),
			currentColor(0)
		{
			assert(eventColors.size() == actionColors.size());
			assert(eventColors.size() == stateColors.size());
		}
	} privateStyle;
	
	
	//! Return the current color of a block of a given type
	QColor Style::blockCurrentColor(const QString& type)
	{
		return blockColor(type, privateStyle.currentColor);
	}
	
	//! Return the color of a block of a given type at a certain index
	QColor Style::blockColor(const QString& type, unsigned index)
	{
		if (type == "event")
			return privateStyle.eventColors[index];
		else if (type == "state")
			return privateStyle.stateColors[index];
		else
			return privateStyle.actionColors[index];
	}
	
	//! Set the current index for block colors
	void Style::blockSetCurrentColorIndex(unsigned index)
	{
		if (int(index) < privateStyle.eventColors.size())
			privateStyle.currentColor = index;
	}
	
	//! Return the number of colors
	unsigned Style::blockColorsCount()
	{
		return privateStyle.eventColors.size();
	}
	
	//! The color to fill unused buttons
	QColor Style::unusedButtonFillColor = QColor(220,220,220);
	
	//! The color to draw the stroke of unused buttons
	QColor Style::unusedButtonStrokeColor = Qt::lightGray;
	
} }  // namespace ThymioVPL / namespace Aseba

