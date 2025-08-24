#ifndef CELL_H
#define CELL_H

#include "state.h"
#include "bool.h"

/*
 * Information about one cell of the search.
 */
typedef    unsigned char cellFlags;
typedef    struct Cell Cell;

struct Cell
{
    // state is the most used field so let's put it first

    State    state;        /* current state */

    // it makes one byte
    // let's align the address before the pointers start

    cellFlags free;    /* TRUE if this cell still has free choice */
    cellFlags frozen;    /* TRUE if this cell is frozen in all gens */
    cellFlags active; /* FALSE if mirror by a symmetry */
    cellFlags choose; /* TRUE for unchecked cells */

    // aligned to two bytes - let's round it up to four

    short    gen;        /* generation number of this cell */
    short    row;        /* row of this cell */
    short    col;        /* column of this cell */

    int     sumNear;    /* sum of states of adjacent cells */
    int     index;      /* cell index in searchlist[] */
    // and now for the pointers

    Cell *    past;        /* cell in the past at this location */
    Cell *    future;        /* cell in the future at this location */
    Cell *    cul;        /* cell to up and left */
    Cell *    cu;            /* cell to up */
    Cell *    cur;        /* cell to up and right */
    Cell *    cl;            /* cell to left */
    Cell *    cr;            /* cell to right */
    Cell *    cdl;        /* cell to down and left */
    Cell *    cd;            /* cell to down */
    Cell *    cdr;        /* cell to down and right */
    Cell *    loop;        /* next cell in same loop as this one */
    Cell *    search;        /* cell next to be searched for setting */
};

#endif /* CELL_H */
