# Makefile for life search program

CFLAGS = -O3 -MP -MMD -Wall -Wno-unused-result -fomit-frame-pointer -Iinclude -I. -I.. -I/usr/include/ncurses

SOURCES = $(wildcard src/*.c)
SOURCES += search.c interact.c
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d) 

all:	lifesrcdumb lifesrc

lifesrcdumb:	$(OBJECTS) dumbtty.o
	$(CC) -o lifesrcdumb $(OBJECTS) dumbtty.o

lifesrc:	$(OBJECTS) cursestty.o
	$(CC) -o lifesrc $(OBJECTS) cursestty.o -lncurses
clean:
	rm -f cursestty.o dumbtty.o $(OBJECTS) $(DEPS)
	rm -f lifesrc lifesrcdumb
-include $(DEPS)
