# Makefile for life search program

CFLAGS = -Ofast -MP -MMD -Wall -Wno-unused-result -fomit-frame-pointer -Iinclude -I. -I..

#CFLAGS = -O -g -pg -MP -MMD -fno-inline -Wall -Wno-unused-result -Iinclude -I. -I.. -I/usr/include/ncurses
#LDFLAGS = -pg

#CFLAGS = -O -g --coverage -MP -MMD -Wall -Wno-unused-result -Iinclude -I. -I.. -I/usr/include/ncurses
#LDFLAGS = -lgcov

SOURCES = $(wildcard src/*.c)
SOURCES += init.c search.c interact.c
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d) 
COVERAGE = $(SOURCES:.c=.gcda)
COVERAGE += $(SOURCES:.c=.gcno)

all:	lifesrcdumb lifesrc

lifesrcdumb:	$(OBJECTS)
	$(CC) -o lifesrcdumb $(OBJECTS) $(LDFLAGS)

lifesrc:	$(OBJECTS) cursestty.o
	$(CC) -o lifesrc $(OBJECTS) $(LDFLAGS)
clean:
	rm -f $(OBJECTS) $(DEPS)
	rm -f lifesrc lifesrcdumb
cleancov:
	rm -f $(COVERAGE)
-include $(DEPS)
