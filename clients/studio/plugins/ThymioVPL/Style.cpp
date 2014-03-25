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
	const int Style::blockDropAreaBorderWidth = 20;
	
	
	//! The width of the column in the middle of the set
	const int Style::eventActionsSetColumnWidth = 40;
	
	//! Step size between event-actions sets
	const int Style::eventActionsSetRowStep = Style::blockHeight + 3*Style::blockSpacing + 64;
	
	//! Size of the rounded corner of event-actions sets
	const int Style::eventActionsSetCornerSize = 5;
	
	//! The normal/selected background colors of event-actions sets
	const QColor Style::eventActionsSetBackgroundColors[2] = { QColor(231, 231, 231), QColor(255, 220, 211) };
	
	//! The normal/selected foreground colors of event-actions sets
	const QColor Style::eventActionsSetForegroundColors[2] = { QColor(213, 213, 213), QColor(237, 172, 140) };
	
	
	//! Size of the rounded corner of add/remove buttons
	const int Style::addRemoveButtonCornerSize = 5;
	
	//! Background color of add/remove buttons
	const QColor Style::addRemoveButtonBackgroundColor(170, 170, 170);
	
	
	// global colors
	
	static struct PrivateStyle
	{
		typedef QList<QColor> ColorList;
		
		ColorList eventColors;
		ColorList actionColors;
		unsigned currentColor;
		
		// FIXME: define colors for events
		PrivateStyle():
			eventColors(ColorList()
				<< QColor(0,191,255)
				<< QColor(155,48,255)
				<< QColor(67,205,128)
				<< QColor(255,215,0)
				<< QColor(255,97,3)
				<< QColor(125,158,192)
			),
			actionColors(ColorList()
				<< QColor(218,112,214)
				<< QColor(159,182,205)
				<< QColor(0,197,205)
				<< QColor(255,99,71)
				<< QColor(142,56,142)
				<< QColor(56,142,142)
			),
			currentColor(0)
		{
			assert(eventColors.size() == actionColors.size());
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
			return QColor(122, 204, 0);
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
	
} }  // namespace ThymioVPL / namespace Aseba

