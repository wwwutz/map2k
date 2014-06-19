#!/bin/bash
BIN=${HOME}/2048
CLUSTERDISK=/scratch/cluster/wwwutz
NUM=2000

GROUPCOUNTER=${CLUSTERDISK}/.submit.sh.group-id.cnt

cp ${BIN}/map2k ${CLUSTERDISK}/
ARG=$1
eval $(${CLUSTERDISK}/map2k -B)

LAST=$((${MAP2K_NUM_STRATS}-1))

STRAT=${ARG:=${LAST}}

NAME=${MAP2K_STRATS_NAME[${STRAT}]}


rm ${CLUSTERDISK}/job.{stdout,stderr,sorted}.[0-9]*
JC=$((1+`cat ${GROUPCOUNTER}`))
echo ${JC}>${GROUPCOUNTER}

GROUPID=${NAME}-${JC}
for i in `seq -w 1 ${NUM}`; do
 JF=${CLUSTERDISK}/job.stdout.$i 
 JE=${CLUSTERDISK}/job.stderr.$i
 mxq_submit --stderr=${JE} --stdout=${JF} --group-id=${GROUPID} --threads=1 --memory=20 --time=15 \
 ${CLUSTERDISK}/map2k -s ${STRAT} -i 10000 -S -R ${GROUPCOUNTER}${i}0000
done

echo "waiting for ${NUM} jobs in group \"${GROUPID}\" to finish"

${BIN}/finwait.pl ${NUM}

if [ ! -d results ]; then
  mkdir results
fi

if [ ${NUM} -le 30 ]; then
  for i in job.stdout.[0-9]*; do
    sort -n $i > ${i/stdout/sorted}
  done
  J=( job.sorted.* )
  X=${J[@]/%/\' u 1 with lines};X=${X//j/\'j};X=${X//s \'/s, \'};
  echo "# plot ${NUM} curves"
  echo "gnuplot -p -e \"plot $X\""
fi

K=( `perl -lane 'if (/barrier.*?(\d+)/) {$x=$1;($F)=$ARGV=~m/(\d+)/;if ($x>$a){$a=$x;$A=$F;$b or $b=$x}if($x<$b){$b=$x;$B=$F};END{print "job.sorted.$A job.sorted.$B"} }' *.stderr* ` )
X=${K[@]/%/\' u 1 with lines};X=${X//j/\'j};X=${X//s \'/s, \'};
for i in ${K[@]}; do
  if [ ! -e $i ]; then
    sort -n ${i/sorted/stdout} > $i
    cp ${i/sorted/stderr} results/${GROUPID}-$i
  fi
done
echo "# plot max / min curve"
echo "gnuplot -p -e \"plot $X\""

