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

#ifndef VPL_STYLE_H
#define VPL_STYLE_H

#include <QColor>

namespace Aseba { namespace ThymioVPL {
	
	//! Style metrics for VPL
	struct Style
	{
		static const int blockWidth;
		static const int blockHeight;
		static const int blockSpacing;
		static const int blockDropAreaBorderWidth;
		static const qreal blockDropAreaSaturationFactor;
		static const qreal blockDropAreaValueFactor;
		
		static const int addRemoveButtonWidth;
		static const int addRemoveButtonHeight;
		
		static const int removeBlockButtonWidth;
		static const int removeBlockButtonHeight;
		
		static const int eventActionsSetColumnWidth;
		static const int eventActionsSetRowStep;
		static const int eventActionsSetCornerSize;
		static const QColor eventActionsSetBackgroundColors[2];
		static const QColor eventActionsSetForegroundColors[2];
		
		static const int addRemoveButtonCornerSize;
		static const QColor addRemoveButtonBackgroundColor;
		
		static QColor blockCurrentColor(const QString& type);
		static QColor blockColor(const QString& type, unsigned index);
		static void blockSetCurrentColorIndex(unsigned index);
		static unsigned blockColorsCount();
		
		static QColor unusedButtonFillColor;
		static QColor unusedButtonStrokeColor;
	};
	
} }  // namespace ThymioVPL / namespace Aseba

#endif // VPL_STYLE_H

