#!/bin/bash -x

SET=${1:-thymio}
DIR=${2:-.}
AESL=${3:-thymio_motion.aesl}

killall aseba{switch,rec,play,scratch}

asebaswitch -d 'ser:name=Thymio' > ${DIR}/sw.${SET}.txt &
asebarec 'tcp:;port=33333' > ${DIR}/rec.${SET}.txt &
../Debug/asebascratch --http 3002 --aesl ${AESL} 'tcp:;port=33333' &
sleep 30

curl --silent --output ${DIR}/ev.${SET}.txt http://localhost:3002/events &

perl run-generate-messages.perl
sleep 5

killall aseba{switch,rec,play,scratch} # also terminates curl

egrep -v '^(a011|900c)' ${DIR}/sw.${SET}.txt | tee ${DIR}/sw.${SET}.noping.txt |
    grep 'user message' | tee ${DIR}/sw.${SET}.user.txt |
    grep 'from 0 ' > ${DIR}/sw.${SET}.user_0.txt
grep -v 'from 0 ' ${DIR}/sw.${SET}.user.txt > ${DIR}/sw.${SET}.user_N.txt
