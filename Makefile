# Makefile for life search program

CFLAGS = -O -g -Wall -Wno-unused-result -fomit-frame-pointer -Iinclude -I. -I.. -I/usr/include/ncurses

SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

all:	lifesrcdumb lifesrc

lifesrcdumb:	search.o interact.o $(OBJECTS) dumbtty.o
	$(CC) -o lifesrcdumb search.o interact.o $(OBJECTS) dumbtty.o

lifesrc:	search.o interact.o $(OBJECTS) cursestty.o
	$(CC) -o lifesrc search.o interact.o $(OBJECTS) cursestty.o -lncurses

clean:
	rm -f search.o interact.o cursestty.o dumbtty.o $(OBJECTS)
	rm -f lifesrc lifesrcdumb

search.o:	lifesrc.h
interact.o:	lifesrc.h
cursestty.o:	lifesrc.h
dumbtty.o:	lifesrc.h
