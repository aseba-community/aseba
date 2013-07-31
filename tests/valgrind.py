#!/usr/bin/env python

# This function performs a valgrind (leak-check) on asebatest with the specified script
# If script is a directory, perform the test on all the .txt files inside this directory
# Return 0 on success (no definitely or indirectly lost blocks), non-zero on error

import sys
import os
import os.path
import stat
import glob
import subprocess

# Input arguments
asebatest_bin = None
input_scripts = None
if len(sys.argv) == 3:
    asebatest_bin = sys.argv[1]
    input_scripts = sys.argv[2]
else:
    print >> sys.stderr, "Wrong number of arguments.\n"
    print >> sys.stderr, "Usage:"
    print >> sys.stderr, "  {} asebatest_bin input_script".format(sys.argv[0])
    exit(1)

# input_scripts: single file, or directory?
s = os.stat(input_scripts)
if stat.S_ISDIR(s.st_mode):
    # list *.txt files inside the directory
    g = os.path.join(input_scripts, "*.txt")
    input_scripts = glob.glob(g)
else:
    input_scripts = [input_scripts]


import re

valgrind_num = r"([\d',]+)"

definitely_lost_re = re.compile(r"definitely lost: " + valgrind_num)
indirectly_lost_re = re.compile(r"indirectly lost: " + valgrind_num)
possibly_lost_re = re.compile(r"possibly lost: " + valgrind_num)

def search_number(string, regexp):
    # convenient function to search a number with a regex
    # remove ' and , to successfully parse numbers
    match = regexp.search(string)
    if match:
        return int(match.group(1).translate(None, ',\''))

#
def valgrind_test(input_script):
    print "Processing {}...".format(input_script)
    try:
        valgrind_output = subprocess.check_output(["valgrind", "--leak-check=summary", asebatest_bin, input_script], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        # Non-zero exit status. With valgrind, it is the return code of the checked binary
        # return code can be cheched with e.returncode
        valgrind_output = e.output

    definitely_lost = search_number(valgrind_output, definitely_lost_re)
    indirectly_lost = search_number(valgrind_output, indirectly_lost_re)
    possibly_lost = search_number(valgrind_output, possibly_lost_re)

    if definitely_lost > 0 or indirectly_lost > 0:
        print "***"
        print "{} failed: {} bytes definitely lost, {} bytes indirectly lost".format(input_script, definitely_lost, indirectly_lost)
        print "  for details run: valgrind --leak-check=full {} {}".format(asebatest_bin, input_script)
        print "***"
        return True # failed
    else:
        return False # no memleak


# create a process pool
from multiprocessing import Pool
pool = Pool()

# test all the scripts using the process pool
failed = pool.map(valgrind_test, input_scripts)

if True in failed:
    exit(2)
else:
    exit(0)

