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
import re

_lupdate = ""
_lrelease = ""
_git = ""

def _verbose_call(cmd):
    print cmd
    return subprocess.call(cmd, shell=True)

def _silent_call(cmd):
    with open(os.devnull, 'w') as tempf:
        return subprocess.call(cmd, shell=True, stdout=tempf, stderr=tempf)

def _find_cmd(possible_cmds, args):
    for cmd in possible_cmds:
        print "Trying... [{}]".format(cmd)
        retcode = _silent_call("{} {}".format(cmd, args))
        if retcode == 0:
            print "OK\n"
            return cmd
        else:
            print "No"
    return ""

def do_lupdate_lrelease(output_name, lang_code, input_files):
    filename = "{}_{}.ts".format(output_name, lang_code)
    filename_qm = "{}_{}.qm".format(output_name, lang_code)
    # _lupdate
    retcode = _verbose_call("{} -no-recursive {} -target-language {} -ts {}".format(_lupdate, input_files, lang_code, filename))
    if retcode != 0:
        print sys.stderr, "Ooops... Something wrong happened with lupdate {}".format(filename)
        exit(3)
    if _git:
        _verbose_call("{} add {}".format(_git, filename))
    # _lrelease
    retcode = _verbose_call("{} {} -qm {}".format(_lrelease, filename, filename_qm))
    if retcode != 0:
        print sys.stderr, "Ooops... Something wrong happened with lrelease {}".format(filename)
        exit(4)
    if _git:
        _verbose_call("{} add {}".format(_git, filename_qm))


def do_lupdate_lrelease_all(directory, generic_name, input_files):
    old_path = os.getcwd()
    os.chdir(directory)
    # search all the existing languages corresponding to this file
    file_re = re.compile("{}_(\w+?).ts".format(generic_name))
    files = [ f for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f)) ]
    for f in files:
        match = file_re.match(f)
        if not match:
            continue
        # get the language
        lang = match.group(1)
        do_lupdate_lrelease(generic_name, lang, input_files)
    os.chdir(old_path)


def do_git_add(filename):
    if _git:
        _verbose_call("{} add {}".format(_git, filename))


def init_commands():
    global _lupdate
    global _lrelease
    global _git

    print "First, we will test for a valid lupdate binary..."
    _lupdate = _find_cmd(["lupdate", "lupdate-qt4"], "-version")
    if _lupdate == "":
        print sys.stderr, "No valide lupdate found :-("
        exit(1)

    print "...and for a valid lrelease..."
    _lrelease = _find_cmd(["lrelease", "lrelease-qt4"], "-version")
    if _lrelease == "":
        print sys.stderr, "No valide lrelease found :-("
        exit(1)

    print "Trying to find a valid git binary (not mandatory)..."
    _git = _find_cmd(["git"], "--version")
    if _git == "":
        print "No git found, won't be able to add newly created files.\n"
    else:
        if raw_input("Git found: do you want to automatically 'git add' the files? [y/N] ") != 'y':
            _git = ""


