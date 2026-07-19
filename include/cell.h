#ifndef CELL_H
#define CELL_H

#include "state.h"

typedef unsigned char cellFlags;

/*
 * Information about one cell of the search.
 */
typedef struct Cell Cell;

struct Cell {
    State     state;    /* current state */
    short     pad;      /* padding field to zero the MSB's of (int) state */
    cellFlags free;     /* TRUE if this cell still has free choice */
    cellFlags frozen;   /* TRUE if this cell is frozen in all gens */
    cellFlags active;   /* FALSE if mirror by a symmetry */
    cellFlags choose;   /* TRUE if can choose this cell if unknown */

    short     gen;      /* generation number of this cell */
    short     row;      /* row of this cell */
    short     col;      /* column of this cell */
    int       sumNear;  /* sum of states of neighbor cells */
    int       index;    /* position in searchlist */

    Cell *    past;     /* cell in past at this location */
    Cell *    future;   /* cell in future at this location */
    Cell *    cul;      /* cell to up and left */
    Cell *    cu;       /* cell to up */
    Cell *    cur;      /* cell to up and right */
    Cell *    cl;       /* cell to left */
    Cell *    cr;       /* cell to right */
    Cell *    cdl;      /* cell to down and left */
    Cell *    cd;       /* cell to down */
    Cell *    cdr;      /* cell to down and right */
    Cell *    loop;     /* next cell in this same loop */
};

#endif /* CELL_H */
