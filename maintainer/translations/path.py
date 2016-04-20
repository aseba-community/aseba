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

import sys
import os.path

STUDIO_PATH = "../../clients/studio/"
COMPILER_PATH = '../../compiler'
CHALLENGE_PATH = "../../targets/challenge/"
THYMIOUPGRADER_PATH = "../../clients/thymioupgrader/"
THYMIOWNETCONFIG_PATH = "../../clients/thymiownetconfig/"

# will be the working directory
my_path = os.path.dirname(os.path.abspath(sys.argv[0])) # getcwd won't work in all cases
updatedoc_path = os.path.abspath(os.path.join(my_path, "../updatedoc/"))
compiler_path = os.path.abspath(COMPILER_PATH)
studio_path = os.path.abspath(STUDIO_PATH)
compiler_ts_path = os.path.join(studio_path, "translations/")
challenge_path = os.path.abspath(CHALLENGE_PATH)
thymioupgrader_path = os.path.abspath(THYMIOUPGRADER_PATH)
thymiownetconfig_path = os.path.abspath(THYMIOWNETCONFIG_PATH)

# files used when updating translations for the compiler
# errors_cpp -> errors_code_h -> compiler_ts_cpp
errors_cpp = os.path.join(compiler_path, 'errors.cpp')
errors_code_h = os.path.join(compiler_path, 'errors_code.h')
compiler_ts_cpp = os.path.join(compiler_ts_path, 'CompilerTranslator.cpp')

# path for strings to be translated
plugin_path = os.path.join(studio_path, "plugins/")
vpl_path = os.path.join(plugin_path, "ThymioVPL/")
blockly_path = os.path.join(plugin_path, "ThymioBlockly/")

# path to file that must be updated when adding a new language
dashel_target = os.path.join(studio_path, "DashelTarget.cpp")
studio_qrc = os.path.join(studio_path, "asebastudio.qrc")
sync_compiler_py = os.path.join(my_path, "sync_compiler_translation.py")
updatedoc = os.path.join(updatedoc_path, "updatedoc.py")
challenge_cpp = os.path.join(challenge_path, "challenge.cpp")
challenge_qrc = os.path.join(challenge_path, "challenge-textures.qrc")

