#!/usr/bin/env python

import sys
import os
import subprocess

iterations = 100    # by default
fuzzy_ratio = 0.01

# When executed from the command line
if __name__ == "__main__":
    # Input arguments
    asebatest_bin = None
    input_script = None
    if len(sys.argv) in [3, 4]:
        asebatest_bin = sys.argv[1]
        input_script = sys.argv[2]
        if len(sys.argv) == 4:
            iterations = int(sys.argv[3])
    else:
        print >> sys.stderr, "Wrong number of arguments.\n"
        print >> sys.stderr, "Usage:"
        print >> sys.stderr, "  {} asebatest_bin input_script".format(sys.argv[0])
        exit(1)

try:
    f = open(input_script, 'rb')
except IOError, e:
    print "Can not find the script file {}. Error.".format(input_script)
    exit(2)

# read the script
script = f.read()
f.close()

# run several fuzzing loops, changing the seed at each step
failed = False
for s in range(0,iterations):
    #subprocess.call(["zzuf", "-P", r"\n", "-R", r"\x00-\x1f\x7f-\xff", "-r{}".format(fuzzy_ratio), "-s", str(s), "cat", input_script])
    retcode = subprocess.call(["zzuf", "-P", r"\n", "-R", r"\x00-\x1f\x7f-\xff", "-r".format(fuzzy_ratio), "-s", str(s), asebatest_bin, input_script], \
            stdout=open(os.devnull), stderr=open(os.devnull))
    if retcode == 1:
        # oops, the child of zzuf crashed (sigsev?)
        print >> sys.stderr, "Compiler crached when fuzzing the input script."
        print >> sys.stderr, "Faulty script is generated with the seed (-s) {}:".format(s)
        subprocess.call(["zzuf", "-P", r"\n", "-R", r"\x00-\x1f\x7f-\xff", "-r{}".format(fuzzy_ratio), "-s", str(s), "cat", input_script])
        failed = True
        break

if failed == True:
    exit(3)
else:
    exit(0)

