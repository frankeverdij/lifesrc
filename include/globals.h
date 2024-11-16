#ifndef GLOBALS_H
#define GLOBALS_H

#include "state.h"
#include "enums.h"
#include "cell.h"

#define	ROW_MAX		49	/* maximum rows for search rectangle */
#define	COL_MAX		132	/* maximum columns for search rectangle */

/*
 * Current parameter values for the program to be saved over runs.
 * These values are dumped and loaded by the dump and load commands.
 * If you add another parameter, be sure to also add it to paramTable,
 * preferably at the end so as to minimize dump file incompatibilities.
 */
typedef struct globals {
    int	rowMax;       /* maximum number of rows */
    int	colMax;       /* maximum number of columns */
    int	genMax;       /* maximum number of generations */
    int	rowTrans;     /* translation of rows */
    int	colTrans;     /* translation of columns */
    Bool rowSym;      /* enable row symmetry starting at column */
    Bool colSym;      /* enable column symmetry starting at row */
    Bool pointSym;    /* enable symmetry with central point */
    Bool fwdSym;      /* enable forward diagonal symmetry */
    Bool bwdSym;      /* enable backward diagonal symmetry */
    Bool flipRows;    /* flip rows at column number from last to first generation */
    Bool flipCols;    /* flip columns at row number from last to first generation */
    Bool flipFwd;     /* flip forward diagonal (/) from last to first gen */
    Bool flipBwd;     /* flip backward diagonal (\) from last to first gen */
    Bool flipQuads;   /* flip quadrants from last to first gen */
    Bool parent;      /* only look for parents */
    Bool allObjects;  /* look for all objects including subPeriods */
    Bool setDeep;     /* set cleared cells deeply from init file */
    int	nearCols;     /* maximum distance to be near columns */
    int	maxCount;     /* maximum number of cells in generation 0 */
    int	useRow;       /* row that must have at least one ON cell */
    int	useCol;       /* column that must have at least one ON cell */
    int	colCells;     /* maximum cells in a column */
    int	colWidth;     /* maximum width of each column */
    Bool follow;      /* follow average position of previous column */
    Bool orderWide;   /* ordering tries to find wide objects */
    Bool orderGens;   /* ordering tries all gens first */
    Bool orderInvert; /* Inverts direction of non-wide orderings */
    Bool orderMiddle; /* ordering tries middle columns first */
    Bool followGens;  /* try to follow setting of other gens */
    State chooseUnknown; /* First choice for unknown cell, either ON or OFF */
    SortOrder sortOrder;    /* sort direction */

    Bool inited;
    int symmetry;
	int	newcellcount; /* number of cells ready for allocation */
	int	auxcellcount; /* number of cells in auxillary table */
	Cell * newcells; /* cells ready for allocation */
	Cell * searchlist; /* current list of cells to search */
	RowInfo	dummyrowinfo; /* dummy info for ignored cells */
	ColInfo	dummycolinfo; /* dummy info for ignored cells */
	Cell ** celltable; /* table of (MAXCELLS) usual cells */
	Cell ** auxtable; /* table of (auxtable_alloc) auxillary cells */
	int auxtable_alloc;
    int lifesrc_maxcells; // formerly the MAXCELLS macro

    Cell ** settable;	/* table of (MAXCELLS) cells whose value is set */
    Cell ** newset;		/* where to add new cells into setting table */
    Cell ** nextset;	/* next cell in setting table to examine */
    Cell ** searchtable; /* a stack of (MAXCELLS) searchlist positions */
    Cell ** searchset;
    RowInfo rowinfo[ROW_MAX];	/* information about rows of gen 0 */
    ColInfo colinfo[COL_MAX];	/* information about columns of gen 0 */
    int fullcolumns;	/* columns in gen 0 which are fully set */
    int combinedcells;
    int setcombinedcells;
    int differentcombinedcells;

} globals;

#endif /* GLOBALS_H */
