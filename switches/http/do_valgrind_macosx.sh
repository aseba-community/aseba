#!/bin/bash -x

TEMP=/tmp/dVG.$$
VALGRIND="valgrind --dsymutil=yes --suppressions=valgrind-macosx.supp --leak-check=yes"
function CURL    () { curl -s --output $TEMP $1 > /dev/null; }
function CURL_H  () { curl -s --output $TEMP -H 'Connection: close' $1 > /dev/null; }
function SIEGE   () { siege -q -b -c 10 -r 10 $1; }

trap "rm -f $TEMP; killall valgrind; killall asebadummynode" 0 1 2 3 15

TEST_AESL=$PWD/dummynode-1-tick.aesl
TEST2_AESL=$PWD/dummynode-1.aesl
TEST_TARGET='tcp:;port=33334'

killall asebadummynode
asebadummynode 1 &

function start_valgrind () {
    $VALGRIND ./asebahttp --aesl ${2:-$TEST_AESL} ${3:-$TEST_TARGET} > out.$1 2>&1 &
    sleep 12 # time to load aesl
    echo $!
}


# Test nodes info
#
pid=$(start_valgrind vg.nodes.log)
echo PID = $pid
for r in http://127.0.0.1:3000/nodes{,/dummynode-1}; do
    CURL {,,,,,,,,,}$r
    CURL_H {,,,,,,,,,}$r
    SIEGE $r
done
wait $pid  # kill -HUP $pid


# Test get variable
#
pid=$(start_valgrind vg.getvar.log)
for r in http://127.0.0.1:3000/nodes/dummynode-1/clock; do
    CURL {,,,,,,,,,}$r
    CURL_H {,,,,,,,,,}$r
    SIEGE $r
done
wait $pid  # kill -HUP $pid


# Test set/get variable
#
pid=$(start_valgrind vg.setvar.log)
for r in http://127.0.0.1:3000/nodes/dummynode-1/vec{/9/1/80,}; do
    CURL {,,,,,,,,,}$r
    CURL_H {,,,,,,,,,}$r
    SIEGE $r
done
wait $pid  # kill -HUP $pid


# Test send event
#
pid=$(start_valgrind vg.sendevent.log)
for r in http://127.0.0.1:3000/nodes/dummynode-1/reset/0; do
    CURL {,,,,,,,,,}$r
    CURL_H {,,,,,,,,,}$r
    SIEGE $r
done
wait $pid  # kill -HUP $pid


# Test subscribe to SSE stream
#
pid=$(start_valgrind vg.events.log)
for r in http://127.0.0.1:3000/events; do
    CURL {,}$r & sleep 4; kill %%
    siege -q -c 2 -r 2 -b $r & sleep 6; kill %%
    CURL_H {,}$r & sleep 4; kill %%
done
wait $pid  # kill -HUP $pid


# Test upload bytecode
#
pid=$(start_valgrind vg.sse.log)
for r in http://127.0.0.1:3000/nodes/dummynode-1; do
    for f in $TEST_AESL $TEST2_AESL; do 
	curl --data-ascii "file=$(cat $f)" -X PUT $r
	sleep 2
	kill $!
    done
done
wait $pid  # kill -HUP $pid

grep -A5 'LEAK SUMMARY' out.*.log
