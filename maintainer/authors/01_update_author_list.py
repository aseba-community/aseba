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

import os.path
import re

top_level_path = "../.."
authors_source_file = os.path.join(top_level_path, "authors.txt")
authors_dest_file = os.path.join(top_level_path, "common/authors.h")

authors_file_head = \
"""
#ifndef __ASEBA_AUTHORS_H
#define __ASEBA_AUTHORS_H

#define ASEBA_AUTHORS_FULL_LIST "\
"""

authors_file_tail = \
"""\
"

#endif // __ASEBA_AUTHORS_H
"""

first = True
with open(authors_dest_file, 'wb') as f:
	f.write(authors_file_head)
	for line in open(authors_source_file):
		# only keep tabbed content
		if len(line) == 0 or line[0] != '\t':
			continue
		line = line.strip(' \t\n\r')
		# get first non-empty content
		if first and len(line) == 0:
			continue
		# drop lines starting with a parenthesis
		if len(line) > 0 and line[0] == '(':
			continue
		# once we reached the reference section, stop parsing
		if len(line) > 0 and line[0] == '[':
			break
		# remove emails and references
		line = line.split('<')[0]
		line = line.split('[')[0]
		f.write(line + '\\n')
		first = False
	f.write(authors_file_tail)