CC = /usr/bin/g++
CFLAGS = -g -I..

all: main exe

clean:
	-rm *.o

.c.o:
	${CC} ${CFLAGS} -c $*.c

stdinc.o : stdinc.h
int_set.o : stdinc.h int_set.h
link_set.o : stdinc.h link_set.h
sub_set.o : stdinc.h sub_set.h nfa.h dfa.h
spliter.o : stdinc.h spliter.h
parser.o : stdinc.h parser.h nfa.h dfa.h
nfa.o : stdinc.h nfa.h dfa.h
dfa.o : stdinc.h dfa.h
trace.o : trace.h

main: stdinc.o int_set.o link_set.o sub_set.o spliter.o parser.o nfa.o dfa.o trace.o main.o
	${CC} ${CFLAGS} stdinc.o int_set.o link_set.o sub_set.o spliter.o parser.o nfa.o dfa.o main.o trace.o -o regex

exe:
	-chmod a+x regex*
