#!/bin/bash
BIN=${HOME}/2048
CLUSTERDISK=/scratch/cluster/wwwutz
NUM=20

GROUPCOUNTER=${CLUSTERDISK}/.submit.sh.group-id.cnt

cp ${BIN}/map2k ${CLUSTERDISK}/
ARG=$1
eval $(${CLUSTERDISK}/map2k -B)

LAST=$((${MAP2K_NUM_STRATS}-1))

STRAT=${ARG:=${LAST}}

NAME=${MAP2K_STRATS_NAME[${STRAT}]}


rm ${CLUSTERDISK}/job.stdout.[0-9]*
JC=$((1+`cat ${GROUPCOUNTER}`))
echo ${JC}>${GROUPCOUNTER}

GROUPID=${NAME}-${JC}
for i in `seq -w 1 ${NUM}`; do
 JF=${CLUSTERDISK}/job.stdout.$i
 mxq_submit --stdout=${JF} --group-id=${GROUPID} --threads=1 --memory=20 --time=15 \
 ${CLUSTERDISK}/map2k -s ${STRAT} -i 10000 -S -Q -R ${i}0000
done

echo "waiting for ${NUM} jobs in group \"${GROUPID}\" to finish"

${BIN}/finwait.pl ${NUM}
