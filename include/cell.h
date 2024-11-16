#ifndef CELL_H
#define CELL_H

#include "state.h"
#include "cellflags.h"

/*
 * Information about a row.
 */
typedef	struct
{
	int	onCount;	/* number of cells which are set on */
} RowInfo;


/*
 * Information about a column.
 */
typedef struct
{
	int	setCount;	/* number of cells which are set */
	int	onCount;	/* number of cells which are set on */
	int	sumPos;		/* sum of row positions for on cells */
} ColInfo;

/*
 * Information about one cell of the search.
 */
typedef	struct Cell Cell;

struct Cell {
	State		state;		/* current state */
    State       combined;
    cellFlags	flags;		/* Bitfield: (U)nchecked, (A)ctive, (C)hoose, */
							/* fro(Z)en and (F)ree : 0x000UACZF */
	short		gen;		/* generation number of this cell */
	short		row;		/* row of this cell */
	short		col;		/* column of this cell */
    short   	near1;		/* count of cells this cell is near */
	int			sumNear;	/* sum of states of neighbor cells */
	int			index;
	Cell *		past;		/* cell in past at this location */
	Cell *		future;		/* cell in future at this location */
	Cell *		cul;		/* cell to up and left */
	Cell *		cu;		/* cell to up */
	Cell *		cur;		/* cell to up and right */
	Cell *		cl;		/* cell to left */
	Cell *		cr;		/* cell to right */
	Cell *		cdl;		/* cell to down and left */
	Cell *		cd;		/* cell to down */
	Cell *		cdr;		/* cell to down and right */
	Cell *		loop;		/* next cell in this same loop */
	Cell *	    search;		/* cell next to be searched for setting */

	RowInfo *   rowInfo;	/* information about this cell's row */
	ColInfo *   colInfo;	/* information about this cell's column */
};

#define	NULL_CELL	((Cell *) 0)

#endif /* CELL_H */
