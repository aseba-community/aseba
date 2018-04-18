/*
    Aseba - an event-based framework for distributed robot control
    Copyright (C) 2007--2013:
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

#ifndef __PLAYGROUND_ROBOTS_H
#define __PLAYGROUND_ROBOTS_H

#include "DashelAsebaGlue.h"
#include "DirectAsebaGlue.h"
#include "robots/thymio2/Thymio2.h"
#include "robots/e-puck/EPuck.h"

namespace Enki {
// typedefs for robots

typedef DirectlyConnected<AsebaThymio2> DirectAsebaThymio2;
typedef DirectlyConnected<AsebaFeedableEPuck> DirectAsebaFeedableEPuck;

typedef DashelConnected<AsebaThymio2> DashelAsebaThymio2;
typedef DashelConnected<AsebaFeedableEPuck> DashelAsebaFeedableEPuck;
}  // namespace Enki

#endif  // __PLAYGROUND_ROBOTS_H
