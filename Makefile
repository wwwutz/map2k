all: map2k
CFLAGS=-g -Wall -O3
LDFLAGS=-lm


.PHONY: nice    
nice:
	perl -pix -e 's/\r//g' map2k.c
	indent -kr -l0 --no-tabs  map2k.c 

.PHONY: test
test:
	for i in `seq -w 0 11`; do ./map2k -r 1 -s $${i} -S $$i > test/strat-$${i}.tst ;done
	for i in `seq -w 0 11`; do diff -q test/strat-$${i}.{ref,tst} ;done
	

test-ref:
	[ -d test ] || mkdir test
	for i in `seq -w 0 11`; do ./map2k -r 1 -s $${i} -S $${i} > test/strat-$${i}.ref ;done
	
