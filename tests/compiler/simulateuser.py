#!/usr/bin/env python

import sys
import os
import tempfile
import subprocess

# When executed from the command line
if __name__ == "__main__":
    # Input arguments
    asebatest_bin = None
    input_script = None
    if len(sys.argv) == 3:
        asebatest_bin = sys.argv[1]
        input_script = sys.argv[2]
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

# open a tempory file
tmp = tempfile.NamedTemporaryFile(mode='wb', prefix='tmp', delete=False)
tmp.close()

# write the script, character by character
tmp_script = ''
failed = False
for c in script:
    # generate the incremental script
    tmp_script += c
    # write to temp file
    tmp = open(tmp.name, 'wb')
    tmp.write(script)
    tmp.close()
    # run the compiler on it
    retcode = subprocess.call([asebatest_bin, tmp.name], stdout=open(os.devnull), stderr=open(os.devnull))
    if retcode < 0:
        # oops, received a signal (sigsev?)
        print >> sys.stderr, "Compiler crached for the following code:"
        print >> sys.stderr, tmp_script
        failed = True
        break

# remove the temp file
os.unlink(tmp.name)

if failed == True:
    exit(retcode)
else:
    exit(0)

