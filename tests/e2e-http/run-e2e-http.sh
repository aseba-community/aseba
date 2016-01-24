#!/bin/bash

killall aseba{dummynode,switch,massloader}
for i in 0 1 2 3; do asebadummynode $i & done
asebaswitch -p 33332 $(for i in 0 1 2 3; do echo 'tcp:;port='$[ 33333 + $i ]; done) &
asebamassloader ping0123.aesl 'tcp:;port=33332' & sleep 10 ; kill $!
asebaswitch -d -p 33331 'tcp:;port=33332' | head -20

#../../switches/http/Debug/asebahttp -a ping0123.aesl 'tcp:;port=33332' &
asebahttp -a ping0123.aesl 'tcp:;port=33332' & sleep 15

jasmine-node --verbose --color *_spec.js
