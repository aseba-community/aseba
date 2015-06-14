#!/bin/bash

# run this from cmake
# bash works on Windows if we run under MSYS2 / MINGW

../targets/dummy/asebadummynode 0 &
sleep 2
./test-asebahttp
status=$?
kill %%
exit $status
