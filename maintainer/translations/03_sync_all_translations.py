#!/usr/bin/env python
# -*- coding: UTF-8 -*-

#   Aseba - an event-based framework for distributed robot control
#   Copyright (C) 2007--2015:
#           Stephane Magnenat <stephane at magnenat dot net>
#           (http://stephane.magnenat.net)
#           and other contributors, see authors.txt for details
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, version 3 of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program. If not, see <http://www.gnu.org/licenses/>.

import os
import os.path
import sys

from path import *
import translation_tools

translation_tools.init_commands()

# qtabout
translation_tools.do_lupdate_all(qtabout_path, "qtabout", qtabout_path)

# asebastudio
translation_tools.do_lupdate_all(studio_path, "asebastudio", " ".join([studio_path, plugin_path, vpl_path]))

# compiler
translation_tools.do_lupdate_all(studio_path, "compiler", compiler_ts_path)

# playground
translation_tools.do_lupdate_all(playground_path, "asebaplayground", playground_path)

# challenge
translation_tools.do_lupdate_all(challenge_path, "asebachallenge", challenge_cpp)

# upgrader
translation_tools.do_lupdate_all(thymioupgrader_path, "thymioupgrader", thymioupgrader_path)

# wnetconfig
translation_tools.do_lupdate_all(thymiownetconfig_path, "thymiownetconfig", thymiownetconfig_path)



