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

import sys
import os
import os.path
import re
import glob

from path import *

translated_re = re.compile(r"""<translation>""")
type_re = re.compile(r"""<translation type="(\w+?)">""")
file_re = re.compile(r"""(\S+?)_(\w+).ts""")

def file_statistics(f):
#    print "\n*** {} ***\n".format(f)
    fd = open(f)
    stat = dict()
    stat['translated'] = 0
    stat['unfinished'] = 0
    stat['obsolete'] = 0
    for line in fd:
        # search translated entries
        s = translated_re.search(line)
        if s:
            stat['translated'] += 1
            continue
        # search untransalted / obsolete entries
        s = type_re.search(line)
        if s:
            #print s.group(1)
            state = s.group(1)
            if state == "unfinished":
                stat['unfinished'] += 1
            elif state == "obsolete":
                stat['obsolete'] += 1
            else:
                print sys.stderr, "Unknown type: ", state
            continue
    return stat

def directory_statistics(directory, stats):
    os.chdir(directory)
    for f in glob.glob("*.ts"):
        # get filename + language
        s = file_re.match(f)
        if not s:
            continue
        name = s.group(1)
        lang = s.group(2)

        if name not in stats:
            stats[name] = dict()

        stats[name][lang] = file_statistics(f)

    return stats

print "*****"
print "Statistics for translation files..."
print "*****"
print ""

stats = dict()

stats = directory_statistics(studio_path, stats)
stats = directory_statistics(challenge_path, stats)


print "       \t{:*^20}\t{:*^20}\t{:*^20}\t{:*^20}".format("Translated", "Total", "Percent", "Obsolete")
# loop on files
for f in stats.keys():
    print "\n[{}]\n".format(f)
    # loop on languages
    lang = stats[f].keys()
    lang.sort()
    for l in lang:
        stat = stats[f][l]
        total = stat["translated"] + stat["unfinished"]
        percent = float(stat["translated"]) / total * 100
        print "--> {}:\t{:^20}\t{:^20}\t{:^20.1f}%\t{:^20}".format(l, stat["translated"], total, percent, stat["obsolete"])

