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

#include <QMenu>
#include <QtDebug>

#include "Plugin.h"
#include "DashelTarget.h"
#include "../../common/utils/utils.h"
#include "../../common/productids.h"
#include "NamedValuesVectorModel.h"

// plugins

namespace Aseba
{	
	/** \addtogroup studio */
	/*@{*/
	
	//! Returns whether there is a tool named name in the list
	bool NodeToolInterfaces::containsNamed(const QString& name) const
	{
		for (const_iterator it(begin()); it != end(); ++it)
			if ((*it)->name == name)
				return true;
		return false;
	}
	
	//! Returns a tool of a given name
	NodeToolInterface* NodeToolInterfaces::getNamed(const QString& name) const
	{
		for (const_iterator it(begin()); it != end(); ++it)
			if ((*it)->name == name)
				return *it;
		return 0;
	}
	
	/*@}*/
} // namespace Aseba
