#ifndef CELL_H
#define CELL_H

#include "state.h"
typedef unsigned int cellFlags;

#define FREECELL    ((cellFlags) 0x01) /* this cell still has free choice */
#define FROZENCELL  ((cellFlags) 0x02) /* this cell is frozen in all gens */
#define CHOOSECELL  ((cellFlags) 0x04) /* can choose this cell if unknown */

typedef struct Cell
{
    State   state;  /* current state */
    cellFlags   flags;  /* the (C)hoose, fro(Z)en, and (F)ree flags */
                        /*  in a bitfield : 0x00000CZF */
    int     sumNear;    /* sum of states of neighbor cells */
    int     index;
    int     past;   /* cell in past at this location */
    int     future; /* cell in future at this location */
    int     cul;    /* cell to up and left */
    int     cu;	    /* cell to up */
    int     cur;    /* cell to up and right */
    int     cl;     /* cell to left */
    int     cr;     /* cell to right */
    int     cdl;    /* cell to down and left */
    int     cd;	    /* cell to down */
    int     cdr;    /* cell to down and right */
    int     loop;   /* next cell in this same loop */
    short   gen;    /* generation number of this cell*/
    short   rowcol; /* row and columns of this cell */
} Cell;

/* O_STATE = 0 which is intended, so no need to set a define*/
#define O_FLAGS   1
#define O_SUMNEAR 2
#define O_INDEX   3
#define O_PAST    4
#define O_FUTURE  5
#define O_CUL     6
#define O_CU      7
#define O_CUR     8
#define O_CL      9
#define O_CR      10
#define O_CDL     11
#define O_CD      12
#define O_CDR     13
#define O_LOOP    14
#define O_RC0G    15

#define NULL_CELL   -1

#endif /* CELL_H */
