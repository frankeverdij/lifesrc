/*
 * Life search program include file.
 * Author: David I. Bell.
 */
 
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "description.h"
#include "state.h"
#include "cell.h"
#include "enums.h"
#include "flags.h"


/*
 * Maximum dimensions of the search
 */
#define	ROW_MAX		49	/* maximum rows for search rectangle */
#define	COL_MAX		132	/* maximum columns for search rectangle */
#define	GEN_MAX		8	/* maximum number of generations */
#define	TRANS_MAX	4	/* largest translation value allowed */


/*
 * Build options
 */
#ifndef DEBUG_FLAG
#define	DEBUG_FLAG	0	/* nonzero for debugging features */
#endif


/*
 * Other definitions
 */
#define	DUMP_VERSION	7		/* version of dump file */

#define	DUMP_FILE	"lifesrc.dmp"	/* default dump file name */
#define	LINE_SIZE	132		/* size of input lines */

#define	MAX_CELLS	((COL_MAX + 2) * (ROW_MAX + 2) * GEN_MAX)
#define	AUX_CELLS	(TRANS_MAX * (COL_MAX + ROW_MAX + 4) * 2)

/*
 * Debugging macros
 */
#if DEBUG_FLAG
#define	DPRINTF(fmt, ...)   if (debug) printf(fmt, ##__VA_ARGS__ )
#else
#define	DPRINTF(fmt, ...)
#endif


/*
 * Declare this macro so that by default the variables are defined external.
 * In the main program, this is defined as a null value so as to actually
 * define the variables.
 */
#ifndef	EXTERN
#define	EXTERN	extern
#endif


/*
 * Current parameter values for the program to be saved over runs.
 * These values are dumped and loaded by the dump and load commands.
 * If you add another parameter, be sure to also add it to paramTable,
 * preferably at the end so as to minimize dump file incompatibilities.
 */
EXTERN	Status	curStatus;	/* current status of search */
EXTERN	int	rowMax;		/* maximum number of rows */
EXTERN	int	colMax;		/* maximum number of columns */
EXTERN	int	genMax;		/* maximum number of generations */
EXTERN  int edgeDiagOffset; /* treat lower-right and upper-left corner triangles with n cell-bases as deadcells. A negative number select lower-left and upper-right triangles */
EXTERN	int	rowTrans;	/* translation of rows */
EXTERN	int	colTrans;	/* translation of columns */
EXTERN	Bool	rowSym;		/* enable row symmetry starting at column */
EXTERN	Bool	colSym;		/* enable column symmetry starting at row */
EXTERN	Bool	pointSym;	/* enable symmetry with central point */
EXTERN	Bool	fwdSym;		/* enable forward diagonal symmetry */
EXTERN	Bool	bwdSym;		/* enable backward diagonal symmetry */
EXTERN	Bool	flipRows;	/* flip rows at column number from last to first generation */
EXTERN	Bool	flipCols;	/* flip columns at row number from last to first generation */
EXTERN	Bool	flipFwd;	/* flip forward diagonal (/) from last to first gen */
EXTERN	Bool	flipBwd;	/* flip backward diagonal (\) from last to first gen */
EXTERN	Bool	flipQuads;	/* flip quadrants from last to first gen */
EXTERN	Bool	parent;		/* only look for parents */
EXTERN	Bool	allObjects;	/* look for all objects including subPeriods */

EXTERN	Bool	orderWide;	/* ordering tries to find wide objects */
EXTERN	Bool	orderGens;	/* ordering tries all gens first */
EXTERN	Bool	orderInvert;	/* Inverts direction of non-wide orderings */
EXTERN	Bool	orderMiddle;	/* ordering tries middle columns first */
EXTERN	Bool	followGens;	/* try to follow setting of other gens */
EXTERN	State   chooseUnknown;  /* First choice for unknown cell, either ON or OFF */
EXTERN  long stepConfl; /* step counter for one Proceed-Backup action */
EXTERN  int sortOrder; /* sort direction */

EXTERN Bool smartOn;      /* use smart method (KAS) */
EXTERN  int smartWindow; /* no. of cells to check */
EXTERN  int smartThreshold; /* check threshold */


/*
 * These values are not affected when dumping and loading since they
 * do not affect the status of a search in progress.
 * They are either setTable on the command line or are computed.
 */
EXTERN	Bool	quiet;		/* don't output */
EXTERN	Bool	debug;		/* enable debugging output (if compiled so) */
EXTERN	Bool	quitOk;		/* ok to quit without confirming */
EXTERN	Bool	inited;		/* initialization has been done */
EXTERN	State	bornRules[9];	/* rules for whether a cell is to be born */
EXTERN	State	liveRules[9];	/* rules for whether a live cell stays alive */
EXTERN	int	curGen;		/* current generation for display */

EXTERN	sig_atomic_t	dumpFlag;	/* sigaction flag for dumps */
EXTERN	sig_atomic_t	viewFlag;	/* sigaction flag for viewing */
EXTERN	char *	dumpFile;	/* dump file name */
EXTERN	char *	outputFile;	/* file to output results to */
EXTERN    long    viewcount;    /* counter for viewing */


/*
 * Data about all of the cells.
 */
EXTERN	Cell *	setTable[MAX_CELLS];	/* table of cells whose value is set */
EXTERN	Cell **	newSet;		/* where to add new cells into setting table */
EXTERN	Cell **	nextSet;	/* next cell in setting table to examine */
EXTERN	Cell **	baseSet;	/* base of changeable part of setting table */
EXTERN  Cell *  searchTable[MAX_CELLS]; /* a stack of searchlist positions */
EXTERN  Cell ** searchSet;


/*
 * Other local data.
 */
EXTERN    Cell **    searchList;    /* current list of cells to search */
EXTERN    int    searchIdx;      /* index of first unknown cell in searchList[] */
EXTERN Cell *    cellTable[MAX_CELLS];    /* table of usual cells */

/*
 * Table of transitions.
 * Given the state of a cell and its neighbors in one generation,
 * this table determines the state of the cell in the next generation.
 * The table is indexed by the descriptor value of a cell.
 */
EXTERN State transit[TRIMSIZE];

/*
 * Table of implications.
 * Given the state of a cell and its neighbors in one generation,
 * this table determines deductions about the cell and its neighbors
 * in the previous generation.
 * The table is indexed by the descriptor value of a cell.
 */
EXTERN Flags implic[TRIMSIZE];

/*
 * Global procedures
 */
extern	void	getCommands(void);
extern	void	initCells(void);
extern	void	printGen(int);
extern	void	dumpState(const char *);
extern	Cell *	findCell(int, int, int);
extern	void	fatal(const char *);
extern  void    dumparray(void);
extern	Status	Search(const Bool);
extern	Status	search(const Bool);
extern	Bool	Proceed(Cell *, State, Bool);
extern	Bool	proceed(Cell *, State, Bool);
extern	Bool	setCell(Cell * const , const State, const Bool);
extern	Bool	setcell(Cell * const , const State, const Bool);
extern	Cell *	Backup(void);
extern	Cell *	backup(void);

/* END CODE */
