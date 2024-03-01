#ifndef CELL_H
#define CELL_H

#include "state.h"
typedef unsigned short cellFlags;

#define FREECELL    ((cellFlags) 0x01) /* this cell still has free choice */
#define FROZENCELL  ((cellFlags) 0x02) /* this cell is frozen in all gens */
#define CHOOSECELL  ((cellFlags) 0x04) /* can choose this cell if unknown */

typedef struct Cell
{
    State   state;  /* current state */
    short   gen;    /* generation of this cell */
    cellFlags   flags;  /* the (C)hoose, fro(Z)en, and (F)ree flags */
                        /*  in a bitfield : 0x00000CZF */
    short   row;    /* row of this cell */
    short   col;    /* column of this cell */
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
} Cell;

typedef struct sCrg
{
    short flags;
    short gen;
    short row;
    short col;
} sCrg;

/* O_STATE = 0 which is intended, so no need to set a define*/
#define O_GENFLAGS  1
#define O_COLROW    2
#define O_SUMNEAR   3
#define O_INDEX     4
#define O_PAST      5
#define O_FUTURE    6
#define O_CUL       7
#define O_CU        8
#define O_CUR       9
#define O_CL        10
#define O_CR        11
#define O_CDL       12
#define O_CD        13
#define O_CDR       14
#define O_LOOP      15

#define NULL_CELL   -1

#endif /* CELL_H */
