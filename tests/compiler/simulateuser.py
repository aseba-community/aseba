#!/usr/bin/env python

from __future__ import print_function

import os
import subprocess
import sys
import tempfile

# When executed from the command line
if __name__ == "__main__":
    # Input arguments
    asebatest_bin = None
    input_script = None
    if len(sys.argv) == 3:
        asebatest_bin = sys.argv[1]
        input_script = sys.argv[2]
    else:
        print("Wrong number of arguments.\n", file=sys.stderr)
        print("Usage:", file=sys.stderr)
        print("  {} asebatest_bin input_script".format(sys.argv[0]), file=sys.stderr)
        exit(1)

try:
    f = open(input_script, 'rb')
except IOError:
    print("Can not find the script file {}. Error.".format(input_script))
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
    tmp_script += str(c)
    # write to temp file
    tmp = open(tmp.name, 'wb')
    tmp.write(script)
    tmp.close()
    # run the compiler on it
    retcode = subprocess.call([asebatest_bin, tmp.name], stdout=open(os.devnull), stderr=open(os.devnull))
    if retcode < 0:
        # oops, received a signal (sigsev?)
        print("Compiler crached for the following code:", file=sys.stderr)
        print(tmp_script, file=sys.stderr)
        failed = True
        break

# remove the temp file
os.unlink(tmp.name)

if failed:
    exit(retcode)
else:
    exit(0)
