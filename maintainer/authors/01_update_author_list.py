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

SCRIPT_DIRECTORY = sys.path[0]

top_level_path = os.path.join(SCRIPT_DIRECTORY, "..", "..")
authors_source_file = os.path.join(top_level_path, "authors.txt")
authors_dest_file = os.path.join(top_level_path, "aseba/common/authors.h")

authors_file_head = \
"""\
#ifndef __ASEBA_AUTHORS_H
#define __ASEBA_AUTHORS_H

#include <vector>
#include <set>

namespace Aseba
{
	struct AuthorInfo
	{
		using Tags = std::set<std::string>;

		std::string name;
		std::string email;
		std::string web;
		std::string role;
		Tags tags;
	};
	using AuthorInfos = std::vector<AuthorInfo>;

	struct InstitutionInfo
	{
		std::string name;
		std::string web;
	};
	using InstitutionInfos = std::vector<InstitutionInfo>;

	static const AuthorInfos authorList = {
"""

authors_file_contrib_step = \
"""\
	};

	static const AuthorInfos thankToList = {
"""

authors_file_tail = \
"""\
	};
} // namespace Aseba

#endif // __ASEBA_AUTHORS_H
"""

# enum for sections
AUTHORS, CONTRIBUTORS, INSTITUITONS = range(3)
author = None
authorLine = -1
currentSection = AUTHORS
with open(authors_dest_file, 'wb') as f:
	f.write(authors_file_head)
	for line in open(authors_source_file):
		# strip newlines
		line = line.strip('\n\r')
		# switch sections
		if line == '# Contributors':
			currentSection = CONTRIBUTORS
			f.write(authors_file_contrib_step)
		elif line == '# Institutions':
			currentSection = CONTRIBUTORS
		# if first author line
		if len(line) > 2 and line[0:2] == '*\t':
			authorLine = 0
			line = line[2:]
		# otherwise only keep tabbed content
		elif len(line) != 0 and line[0] == '\t':
			line = line[1:]
		else:
			continue
		# strip tabs and spaces
		line = line.strip(' \t')
		# if first line
		if authorLine == 0:
			author = {}
			nameLine = line.split('<')
			if len(nameLine) < 2:
				author["name"] = line.split('[')[0].strip(' \t\n\r')
			else:
				name, rest = nameLine
				if name[0] == '[':
					name, web = name.split('(')
					author["name"] = name.strip(' \t\n\r[]')
					author["web"] = web.strip(' \t\n\r()')
				else:
					author["name"] = name.strip(' \t\n\r')
				if '>' in rest:
					email = rest.split('>')[0]
					author["email"] = email.strip(' \t\n\r')
		elif authorLine == 1:
			author["role"] = line
			pass
		elif authorLine == 2:
			line = line[len('contributed to: '):]
			tags = line.split(', ')
			author["tags"] = tags
			if currentSection == AUTHORS:
				print 'Adding author ' + author["name"]
			else:
				print 'Adding contributor ' + author["name"]
			f.write('		{\n')
			f.write('			u8"' + author.get("name","") + '",\n')
			f.write('			u8"' + author.get("email","") + '",\n')
			f.write('			u8"' + author.get("web","") + '",\n')
			f.write('			u8"' + author.get("role","") + '",\n')
			f.write('			{ ')
			for tag in author.get("tags",[]):
				f.write('u8"' + tag + '", ')
			f.write('}\n')
			f.write('		},\n')
		else:
			continue
		authorLine += 1
	f.write(authors_file_tail)
