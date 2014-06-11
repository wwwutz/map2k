all: map2k
CFLAGS=-g -Wall -O3
LDFLAGS=-lm
MAXS=16

.PHONY: nice    
nice:
	perl -pix -e 's/\r//g' map2k.c
	indent -kr -l0 --no-tabs  map2k.c 

.PHONY: test
test:
	for i in `seq -w 0 ${MAXS}`; do ./map2k -r 1 -s $${i} -S $$i > test/strat-$${i}.tst ;done
	for i in `seq -w 0 ${MAXS}`; do diff -sq test/strat-$${i}.{ref,tst} ;done
	

.PHONY: test-ref
test-ref:
	[ -d test ] || mkdir test
	for i in `seq -w 0 ${MAXS}`; do ./map2k -r 1 -s $${i} -S $${i} > test/strat-$${i}.ref ;done
	

.PHONY: stats
stats:
	for i in `seq -w 0 ${MAXS}`; do perl -lane '($$F[0] <= 9000) and  $$m++;$$n++; END{printf "$$m/$$n : %4.1f\%\n",100-(100/$$n*$$m)}' test/strat-$${i}.ref ;done
