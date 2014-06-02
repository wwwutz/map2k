#!/bin/bash
cp /home/wwwutz/2048/map2k /scratch/cluster/wwwutz/
NUM=2000
rm /scratch/cluster/wwwutz/job.stdout.[0-9]*
JC=$((1+`cat /scratch/cluster/wwwutz.jobcnt`))
echo ${JC}>/scratch/cluster/wwwutz.jobcnt
for i in `seq -w 1 ${NUM}`; do
 JF=/scratch/cluster/wwwutz/job.stdout.$i
 mxq_submit --stdout=${JF} --group-id=sim2k-Randdir-${JC} --threads=1 --memory=20 --time=15 \
 /scratch/cluster/wwwutz/map2k -s 11 -i 10000 -S -Q -R ${i}0000

done
#cd /scratch/cluster/wwwutz
#/home/wwwutz/2048/finwait.pl ${NUM};sort -n job.stdout* > /home/wwwutz/2048/stats.Randdir;
