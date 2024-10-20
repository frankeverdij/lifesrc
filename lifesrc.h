/*
 * Life search program include file.
 * Author: David I. Bell.
 */
 
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "state.h"


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

#define	ALLOC_SIZE	50000		/* chunk size for cell allocation */
#define	VIEW_MULT	1000000		/* viewing frequency multiplier */
#define	DUMP_MULT	1000000		/* dumping frequency multiplier */
#define	DUMP_FILE	"lifesrc.dmp"	/* default dump file name */
#define	LINE_SIZE	132		/* size of input lines */

#define	MAX_CELLS	((COL_MAX + 2) * (ROW_MAX + 2) * GEN_MAX)
#define	AUX_CELLS	(TRANS_MAX * (COL_MAX + ROW_MAX + 4) * 2)

/*
 * Flag bits
 */
typedef unsigned short cellFlags;

#define FREECELL	((cellFlags) 0x01) /* this cell still has free choice */
#define FROZENCELL	((cellFlags) 0x02) /* this cell is frozen in all gens */
#define CHOOSECELL	((cellFlags) 0x04) /* can choose this cell if unknown */

/*
 * Debugging macros
 */
#if DEBUG_FLAG
#define	DPRINTF(fmt, ...)   if (debug) printf(fmt, ##__VA_ARGS__ )
#else
#define	DPRINTF(fmt, ...)
#endif

#define	isBlank(ch)	(((ch) == ' ') || ((ch) == '\t'))

/*
 * Bool type
 */
typedef	int		Bool;

#define	FALSE		((Bool) 0)
#define	TRUE		((Bool) 1)


/*
 * Status returned by routines
 */
typedef	unsigned int	Status;

#define	OK		((Status) 0)
#define	ERROR		((Status) 1)
#define	CONSISTENT	((Status) 2)
#define	NOT_EXIST	((Status) 3)
#define	FOUND		((Status) 4)


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

struct Cell
{
	State		state;		/* current state */
    cellFlags	flags;		/* the (C)hoose, fro(Z)en, and (F)ree flags */
							/*  in a bitfield : 0x00000CZF */
	short		gen;		/* generation number of this cell */
	short		row;		/* row of this cell */
	short		col;		/* column of this cell */
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
};

#define	NULL_CELL	((Cell *) 0)


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
EXTERN	Bool	setDeep;	/* set cleared cells deeply from init file */
EXTERN	int	nearCols;	/* maximum distance to be near columns */
EXTERN	int	maxCount;	/* maximum number of cells in generation 0 */
EXTERN	int	useRow;		/* row that must have at least one ON cell */
EXTERN	int	useCol;		/* column that must have at least one ON cell */
EXTERN	int	colCells;	/* maximum cells in a column */
EXTERN	int	colWidth;	/* maximum width of each column */
EXTERN	Bool	follow;		/* follow average position of previous column */
EXTERN	Bool	orderWide;	/* ordering tries to find wide objects */
EXTERN	Bool	orderGens;	/* ordering tries all gens first */
EXTERN	Bool	orderInvert;	/* Inverts direction of non-wide orderings */
EXTERN	Bool	orderMiddle;	/* ordering tries middle columns first */
EXTERN	Bool	followGens;	/* try to follow setting of other gens */
EXTERN	State   chooseUnknown;  /* First choice for unknown cell, either ON or OFF */
EXTERN  long stepConfl; /* step counter for one Proceed-Backup action */
EXTERN  int sortOrder; /* sort direction */

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
EXTERN	int	outputCols;	/* number of columns to save for output */
EXTERN	int	outputLastCols;	/* last number of columns output */
EXTERN	int	cellCount;	/* number of live cells in generation 0 */
EXTERN	int	dumpFreq;	/* how often to perform dumps */
EXTERN	sig_atomic_t	dumpFlag;	/* sigaction flag for dumps */
EXTERN	int	viewFreq;	/* how often to view results */
EXTERN	sig_atomic_t	viewFlag;	/* sigaction flag for viewing */
EXTERN	char *	dumpFile;	/* dump file name */
EXTERN	char *	outputFile;	/* file to output results to */


/*
 * Data about all of the cells.
 */
EXTERN	Cell *	setTable[MAX_CELLS];	/* table of cells whose value is set */
EXTERN	Cell **	newSet;		/* where to add new cells into setting table */
EXTERN	Cell **	nextSet;	/* next cell in setting table to examine */
EXTERN	Cell **	baseSet;	/* base of changeable part of setting table */
EXTERN	Cell *	fullSearchList;	/* complete list of cells to search */
EXTERN	RowInfo	rowInfo[ROW_MAX];	/* information about rows of gen 0 */
EXTERN	ColInfo	colInfo[COL_MAX];	/* information about columns of gen 0 */
EXTERN	int	fullColumns;	/* columns in gen 0 which are fully set */


/*
 * Global procedures
 */
extern	void	getCommands(void);
extern	void	initCells(void);
extern	void	printGen(int);
extern	void	writeGen(const char *, Bool);
extern	void	dumpState(const char *);
extern	void	adjustNear(Cell *, int);
extern	Status	search(const Bool);
extern	Status	proceed(Cell *, State, Bool);
extern	Status	go(Cell *, State, Bool);
extern	Status	setCell(Cell * const , const State, const Bool);
extern	Cell *	findCell(int, int, int);
extern	Cell *	backup(void);
extern	Bool	subPeriods(void);
extern	void	loopCells(Cell *, Cell *);
extern	void	fatal(const char *);
extern	Bool	ttyOpen(void);
extern	Bool	ttyCheck(void);
extern	Bool	ttyRead(const char *, char *, int);
extern	void	ttyPrintf(const char *, ...);
extern	void	ttyStatus(const char *, ...);
extern	void	ttyWrite(const char *, int);
extern	void	ttyHome(void);
extern	void	ttyEEop(void);
extern	void	ttyFlush(void);
extern	void	ttyClose(void);

/* END CODE */
