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
import subprocess
import shutil
import re

from path import *
import translation_tools

# some tags inside the files to help us
cpp_tag = re.compile(r"/\* insert translation here")
studio_qrc_tag = re.compile(r"<!-- insert translation here")
studio_qrc_compiler_tag = re.compile(r"<!-- insert compiler translation here")
sync_compiler_py_tag = re.compile(r"# insert translation here")
updatedoc_tag = sync_compiler_py_tag
challenge_qrc_tag = studio_qrc_tag


def insert_before_tag(input_file, output_file, tag_re, inserted_text):
    with open(input_file) as in_file:
        with open(output_file, "w") as out_file:
            found = False
            for line in in_file:
                if tag_re.search(line):
                    found = True
                    # insert the text before the tag
                    out_file.write(inserted_text)
                out_file.write(line)
    if not found:
        print sys.stderr, "Tag not found in ", input_file
    return found


print "*****"
print "This will add a new language to the available translations."
print "We need first a few information."
print "*****"
print ""

translation_tools.init_commands()

name_en = raw_input("\nLanguage name (in English): ")
name = raw_input("Language name, as displayed to users (ex: Français): ")
print "\n***"
print "Hint: for a list a locales, you can look here: http://www.roseindia.net/tutorials/i18n/locales-list.shtml"
print "***\n"
code_lang = raw_input("Language code (ex: fr): ")
code_region = raw_input("Region code, if any (ex: ch): ")

if code_region == '':
    code = code_lang
else:
    code = code_lang + "_" + code_region

wikidot_url = ""
if raw_input("\nIs the wikidot user manual also translated and to be added to the offline help? [y/N] ").lower() == 'y':
    wikidot_url = "https://aseba.wikidot.com/{}:asebausermanual".format(code)   # should be good
    temp = raw_input("What is its url? [{}] ".format(wikidot_url))
    if temp:
        wikidot_url = temp

if raw_input("\nAre you happy with your input? [y/N] ").lower() != 'y':
    exit(2)

print "Generating files...\n"
os.chdir(studio_path)

# lupdate / lrelease asebastudio_x.{ts,qm}
translation_tools.do_lupdate_lrelease("asebastudio", code, " ".join([studio_path, plugin_path, vpl_path, blockly_path]))

# lupdate / lrelease compiler_x.{ts,qm}
translation_tools.do_lupdate_lrelease("compiler", code, compiler_ts_path)

os.chdir(challenge_path)
# lupdate / lrelease asebachallenge_x.{ts,qm}
translation_tools.do_lupdate_lrelease("asebachallenge", code, challenge_cpp)


print "Modifying source files...\n"

# We have to update DashelTarget.cpp
print "Updating DashelTarget.cpp..."
os.chdir(studio_path)
tmp_file = "DashelTarget.cpp.tmp"
insert_before_tag(dashel_target, tmp_file, cpp_tag, """\t\tlanguageSelectionBox->addItem(QString::fromUtf8("{}"), "{}");\n""".format(name, code))
os.remove(dashel_target)
shutil.move(tmp_file, dashel_target)
translation_tools.do_git_add(dashel_target)
print "Done\n"

# We have to update asebastudio.qrc
print "Updating asebastudio.qrc..."
tmp_file1 = "asebastudio.qrc.tmp1"
tmp_file2 = "asebastudio.qrc.tmp2"
insert_before_tag(studio_qrc, tmp_file1, studio_qrc_tag, """\t<file>asebastudio_{}.qm</file>\n""".format(code))
insert_before_tag(tmp_file1, tmp_file2, studio_qrc_compiler_tag, """\t<file>compiler_{}.qm</file>\n""".format(code))
os.remove(tmp_file1)
os.remove(studio_qrc)
shutil.move(tmp_file2, studio_qrc)
translation_tools.do_git_add(studio_qrc)
print "Done\n"

# We have to update challenge.cpp
print "Updating challenge.cpp..."
os.chdir(challenge_path)
tmp_file = "challenge.cpp.tmp"
insert_before_tag(challenge_cpp, tmp_file, cpp_tag, """\tlanguageSelectionBox->addItem(QString::fromUtf8("{}"), "{}");\n""".format(name, code))
os.remove(challenge_cpp)
shutil.move(tmp_file, challenge_cpp)
translation_tools.do_git_add(challenge_cpp)
print "Done\n"

# We have to update challenge-textures.qrc
print "Updating challenge-textures.qrc..."
tmp_file = "challenge-textures.qrc.tmp"
insert_before_tag(challenge_qrc, tmp_file, challenge_qrc_tag, """\t<file>asebachallenge_{}.qm</file>\n""".format(code))
os.remove(challenge_qrc)
shutil.move(tmp_file, challenge_qrc)
translation_tools.do_git_add(challenge_qrc)
print "Done\n"

# Update updatedoc.py, if asked by the user
if wikidot_url:
    os.chdir(updatedoc_path)
    print "Updating updatedoc.py"
    tmp_file = "updatedoc.py.tmp"
    insert_before_tag(updatedoc, tmp_file, updatedoc_tag, """                '{}':'{}',\n""".format(code, wikidot_url))
    os.remove(updatedoc)
    shutil.move(tmp_file, updatedoc)
    translation_tools.do_git_add(updatedoc)
    print "Done\n"


print "We are done! Now edit the .ts files with Qt Linguist, then run lrelease on them to generate valid .qm."
print "Finally, do not forget to commit the .ts and .qm files."
print "Have fun :-)\n"
print "*****\n"


