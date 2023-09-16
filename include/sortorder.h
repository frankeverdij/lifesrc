#ifndef SORTORDER_H
#define SORTORDER_H

#include "state.h"

int orderSortFunc(const void * addr1, const void * addr2, void * gvars);

typedef	struct globals_struct globals_struct;

struct globals_struct {
/*
 * Current parameter values for the program to be saved over runs.
 * These values are dumped and loaded by the dump and load commands.
 * If you add another parameter, be sure to also add it to param_table,
 * preferably at the end so as to minimize dump file incompatibilities.
 */
	Status	curStatus;	/* current status of search */
	int	rowMax;		/* maximum number of rows */
	int	colMax;		/* maximum number of columns */
	int	genMax;		/* maximum number of generations */
	int	rowTrans;	/* translation of rows */
	int	colTrans;	/* translation of columns */
	Bool	rowSym;		/* enable row symmetry starting at column */
	Bool	colSym;		/* enable column symmetry starting at row */
	Bool	pointSym;	/* enable symmetry with central point */
	Bool	fwdSym;		/* enable forward diagonal symmetry */
	Bool	bwdSym;		/* enable backward diagonal symmetry */
	Bool	flipRows;	/* flip rows at column number from last to first generation */
	Bool	flipCols;	/* flip columns at row number from last to first generation */
	Bool	flipQuads;	/* flip quadrants from last to first gen */
	Bool	parent;		/* only look for parents */
	Bool	allObjects;	/* look for all objects including subPeriods */
	Bool	setDeep;	/* set cleared cells deeply from init file */
	int	nearCols;	/* maximum distance to be near columns */
	int	maxCount;	/* maximum number of cells in generation 0 */
	int	useRow;		/* row that must have at least one ON cell */
	int	useCol;		/* column that must have at least one ON cell */
	int	colCells;	/* maximum cells in a column */
	int	colWidth;	/* maximum width of each column */
	Bool	follow;		/* follow average position of previous column */
	Bool	orderWide;	/* ordering tries to find wide objects */
	Bool	orderGens;	/* ordering tries all gens first */
	Bool	orderMiddle;	/* ordering tries middle columns first */
	Bool	followGens;	/* try to follow setting of other gens */

#define SORTORDER_DEFAULT 0
#define SORTORDER_DIAG    1
#define SORTORDER_KNIGHT  2
#define SORTORDER_TOPDOWN 3
#define SORTORDER_CENTEROUT 4
#define SORTORDER_MIDDLECOLOUT 5
	int sortOrder;
};

#endif /* SORTORDER_H */
