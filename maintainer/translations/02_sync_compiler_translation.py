#!/usr/bin/env python

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

from __future__ import print_function

# System lib
import re
import sys
from string import Template

from . import path

comment_regexp = re.compile(r'\A\s*// (.*)')
error_regexp = re.compile(r'error_map\[(.*?)\]\s*=\s*L"(.*?)";')

output_code_file = """
#ifndef __ERRORS_H
#define __ERRORS_H

/*
This file was automatically generated. Do not modify it,
or your changes will be lost!
You can manually run the script 'sync_translation.py'
to generate a new version, based on errors.cpp
*/

// define error codes
namespace Aseba
{
    enum ErrorCode
    {
        // ${start_comment}
        ${id_0} = 0,
${id_others}
        ERROR_END
    };
};

#endif // __ERRORS_H
"""
output_code_file_template = Template(output_code_file)

output_qt_file = """
#include "CompilerTranslator.h"
#include "compiler/errors_code.h"

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
output_qt_file_template = Template(output_qt_file)

output_qt_element = """
            case ${error_id}:
                msg = tr("${error_msg}");
                break;
"""
output_qt_element_template = Template(output_qt_element)

# process input file
print("Reading " + path.errors_cpp)

case_vector = ""
count = 0
id_0 = None
start_comment = ""
id_others = ""

try:
    with open(path.errors_cpp) as fh:
        print("Processing...")
        for line in fh:
            # a comment?
            match = comment_regexp.search(line)
            if match:
                # add it to the error codes file
                if not id_0:
                    start_comment = match.group(1)
                else:
                    id_others += "        // " + match.group(1) + "\n"
                continue

            match = error_regexp.search(line)
            if match:
                count = count + 1
                code = match.group(1)
                msg = match.group(2)
                # error codes file
                if not id_0:
                    id_0 = code
                else:
                    id_others += "        " + code + ",\n"
                # qt file
                case_vector += output_qt_element_template.substitute(error_id=code, error_msg=msg)
except IOError:
    print("Invalid file " + path.errors_cpp, file=sys.stderr)
    exit(1)

print("Found " + str(count) + " string(s)")

# writing errors_code.h
result = output_code_file_template.substitute(start_comment=start_comment, id_0=id_0, id_others=id_others)
print("Writing to " + path.errors_code_h)
try:
    with open(path.errors_code_h, 'w') as fh:
        fh.write(result)
except IOError:
    print("Invalid file " + path.errors_code_h, file=sys.stderr)
    exit(1)

# writing the qt file
result = output_qt_file_template.substitute(elements=case_vector)
print("Writing to " + path.compiler_ts_cpp)
try:
    with open(path.compiler_ts_cpp, 'w') as fh:
        fh.write(result)
except IOError:
    print("Invalid file " + path.compiler_ts_cpp, file=sys.stderr)
    exit(1)

print("Done!")
