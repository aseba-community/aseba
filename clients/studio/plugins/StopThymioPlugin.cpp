/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2014:
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

#include "StopThymioPlugin.h"
#include "../TargetModels.h"
#include <QPushButton>
#include <QIcon>

namespace Aseba
{
	StopThymioPlugin::StopThymioPlugin(DevelopmentEnvironmentInterface *_de) :
		de(_de)
	{}
	
	QWidget* StopThymioPlugin::createMenuEntry()
	{
		QPushButton *button(new QPushButton(QIcon(":/images/icons/stop.svgz"), tr("Stop Thymio")));
		button->setIconSize(QSize(32,32));
		connect(button, SIGNAL(clicked()), SLOT(stopThymio()));
		return button;
	}
	
	void StopThymioPlugin::stopThymio()
	{
		de->stop();
		const unsigned leftSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.left.target");
		de->setVariableValues(leftSpeedVarPos, VariablesDataVector(1, 0));
		const unsigned rightSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.right.target");
		de->setVariableValues(rightSpeedVarPos, VariablesDataVector(1, 0));
	}
} // namespace Aseba
