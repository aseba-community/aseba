#! /usr/bin/env python

#   Aseba - an event-based framework for distributed robot control
#   Copyright (C) 2007--2011:
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

# System lib
import os
import os.path
import sys
from shutil import rmtree
from shutil import copy

# Custom lib
from wikidot.fetch import fetchwikidot

#language_map = [('fr','http://aseba.wikidot.com/fr:asebausermanual')]
language_map = [
                ('en','http://aseba.wikidot.com/en:asebausermanual'),
                ('fr','http://aseba.wikidot.com/fr:asebausermanual'),
                ('de','http://aseba.wikidot.com/de:asebausermanual')
                ]
OUTPUT_DIR = 'doc'
# Clean
rmtree(OUTPUT_DIR, True)
os.mkdir(OUTPUT_DIR)

for x in language_map:
    print >> sys.stderr, "\n*** Getting doc for language ", x[0], " ***"
    output = OUTPUT_DIR + '_' + x[0]
    rmtree(output, True)
    fetchwikidot(x[1], output)
    # Copy from output to OUTPUT_DIR
    # copytree not possible, as OUTPUT_DIR already exists
    listing = os.listdir(output)
    for y in listing:
        copy(os.path.join(output, y), os.path.join(OUTPUT_DIR, y))

