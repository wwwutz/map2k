all: map2k
CFLAGS=-g -Wall -O3
LDFLAGS=-lm

nice:
	perl -pix -e 's/\r//g' map2k.c
	indent -kr -l0 --no-tabs  map2k.c 
