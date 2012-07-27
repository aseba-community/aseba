#!/usr/bin/env python

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
from string import Template
import re

INPUT_FILE = 'errors.cpp'
OUTPUT_FILE = 'CompilerTranslator.cpp'
OUTPUT_DIRECTOR = '../studio'

error_regexp = re.compile(r'error_map\[(.*?)\]\s*=\s*L"(.*?)";')

output_file = \
"""
#include "CompilerTranslator.h"
#include "../compiler/errors_code.h"

#include <CompilerTranslator.moc>

namespace Aseba
{
	CompilerTranslator::CompilerTranslator()
	{

	}

	const std::wstring CompilerTranslator::translate(ErrorCode error)
	{
		QString msg;

		switch (error)
		{
${elements}
			default:
				msg = tr("Unknown error");
		}

		return msg.toStdWString();
	}
};
"""
output_file_template = Template(output_file)

output_element = \
"""
			case ${error_id}:
				msg = tr("${error_msg}");
				break;
"""
output_element_template = Template(output_element)

# process input file
filename = INPUT_FILE
print "Reading " + filename
try:
    fh = file(filename)
except:
    print >> sys.stderr, "Invalid file " + filename
    exit(1)

case_vector = ""
count = 0

print "Processing..."
for line in fh:
    match = error_regexp.search(line)
    if match:
        count = count + 1
        code = match.group(1)
        msg = match.group(2)
        case_vector += output_element_template.substitute(error_id=code, error_msg=msg)

fh.close()
print "Found " + str(count) + " string(s)"

result = output_file_template.substitute(elements=case_vector)

filename = os.path.join(OUTPUT_DIRECTOR, OUTPUT_FILE)
print "Writing to " + filename
try:
    fh = file(filename, 'w')
except:
    print >> sys.stderr, "Invalid file " + filename
    exit(1)

fh.write(result)
fh.close()
print "Done!"
