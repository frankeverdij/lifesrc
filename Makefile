# Makefile for life search program

CFLAGS = -Ofast -MP -MMD -Wall -Wno-unused-result -fomit-frame-pointer -Iinclude -I. -I.. -I/usr/include/ncurses

#CFLAGS = -O -g -pg -MP -MMD -fno-inline -Wall -Wno-unused-result -Iinclude -I. -I.. -I/usr/include/ncurses
#LDFLAGS = -pg

#CFLAGS = -O -g --coverage -MP -MMD -Wall -Wno-unused-result -Iinclude -I. -I.. -I/usr/include/ncurses
#LDFLAGS = -lgcov

#SOURCES = $(wildcard src/*.c)
SOURCES += search.c interact.c
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d) 
COVERAGE = $(SOURCES:.c=.gcda)
COVERAGE += $(SOURCES:.c=.gcno)

all:	lifesrc

lifesrc:	$(OBJECTS)
	$(CC) -o lifesrc $(OBJECTS) $(LDFLAGS)

clean:
	rm -f cursestty.o dumbtty.o $(OBJECTS) $(DEPS)
	rm -f lifesrc lifesrcdumb
cleancov:
	rm -f $(COVERAGE) cursestty.gcda cursestty.gcno dumbtty.gcda dumbtty.gcno
-include $(DEPS)
