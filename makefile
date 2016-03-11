all:rsum.o
rsum.o:rsum.s
	as -o $@ $<

all:isort.o
isort.o:isort.s
	as -o $@ $<

all:fact_iterative.o
fact_iterative.o:fact_iterative.s
	as -o $@ $<

all:fact_recursive.o
fact_recursive.o:fact_recursive.s
	as -o $@ $<

all:armemu
armemu:armemu.c
	gcc -o $@ $+ fact_recursive.o fact_iterative.o isort.o rsum.o
clean:
	rm *.o

