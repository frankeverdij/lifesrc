#ifndef CELL_H
#define CELL_H

#include "state.h"
#include "bool.h"

/*
 * Information about a row.
 */
typedef    struct
{
    int    oncount;    /* number of cells which are set on */
} ROWINFO;


/*
 * Information about a column.
 */
typedef struct
{
    int    setcount;    /* number of cells which are set */
    int    oncount;    /* number of cells which are set on */
    int    sumpos;        /* sum of row positions for on cells */
} COLINFO;


/*
 * Information about one cell of the search.
 */
typedef    char        PACKED_BOOL;
typedef    struct Cell Cell;

struct Cell
{
    // state is the most used field so let's put it first

    State    state;        /* current state */

    // it makes one byte
    // let's align the address before the pointers start

    PACKED_BOOL free;    /* TRUE if this cell still has free choice */
    PACKED_BOOL frozen;    /* TRUE if this cell is frozen in all gens */
    PACKED_BOOL active; /* FALSE if mirror by a symmetry */
    PACKED_BOOL unchecked; /* TRUE for unchecked cells */

    // aligned to two bytes - let's round it up to four

    short    gen;        /* generation number of this cell */
    short    row;        /* row of this cell */
    short    col;        /* column of this cell */

    int     sumnear;    /* sum of states of adjacent cells */
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

    ROWINFO * rowinfo;    /* information about this cell's row */
    COLINFO * colinfo;    /* information about this cell's column */

    short    near1;        /* count of cells this cell is near */


    State combined;

    long potential;
};

#endif /* CELL_H */
