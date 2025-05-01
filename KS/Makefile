# Makefile for life search program

CFLAGS = -O3 -fomit-frame-pointer -Wno-unused-result -Iinclude -I/usr/include/ncurses

all:	lifesrcdumb lifesrc

lifesrcdumb:	search.o implication.o interact.o dumbtty.o
	$(CC) -o lifesrcdumb search.o implication.o interact.o dumbtty.o

lifesrc:	search.o implication.o interact.o cursestty.o
	$(CC) -o lifesrc search.o implication.o interact.o cursestty.o -lncurses

clean:
	rm -f search.o implication.o interact.o cursestty.o dumbtty.o
	rm -f lifesrc lifesrcdumb

search.o:	lifesrc.h
implication.o: include/state.h include/bool.h include/tty.h include/implication.h
interact.o:	lifesrc.h
cursestty.o:	lifesrc.h
dumbtty.o:	lifesrc.h
