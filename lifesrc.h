/*
 * Life search program include file.
 * Author: David I. Bell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


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
#define	DEBUG_FLAG	1	/* nonzero for debugging features */
#endif


/*
 * Other definitions
 */
#define	DUMP_VERSION	6		/* version of dump file */

#define	ALLOC_SIZE	50000		/* chunk size for cell allocation */
#define	VIEW_MULT	1000		/* viewing frequency multiplier */
#define	DUMP_MULT	1000		/* dumping frequency multiplier */
#define	DUMP_FILE	"lifesrc.dmp"	/* default dump file name */
#define	LINE_SIZE	132		/* size of input lines */

#define	MAX_CELLS	((COL_MAX + 2) * (ROW_MAX + 2) * GEN_MAX)
#define	AUX_CELLS	(TRANS_MAX * (COL_MAX + ROW_MAX + 4) * 2)


/*
 * Debugging macros
 */
#if DEBUG_FLAG
#define	DPRINTF0(fmt)			if (debug) printf(fmt)
#define	DPRINTF1(fmt,a1)		if (debug) printf(fmt,a1)
#define	DPRINTF2(fmt,a1,a2)		if (debug) printf(fmt,a1,a2)
#define	DPRINTF3(fmt,a1,a2,a3)		if (debug) printf(fmt,a1,a2,a3)
#define	DPRINTF4(fmt,a1,a2,a3,a4)	if (debug) printf(fmt,a1,a2,a3,a4)
#define	DPRINTF5(fmt,a1,a2,a3,a4,a5)	if (debug) printf(fmt,a1,a2,a3,a4,a5)
#else
#define	DPRINTF0(fmt)
#define	DPRINTF1(fmt,a1)
#define	DPRINTF2(fmt,a1,a2)
#define	DPRINTF3(fmt,a1,a2,a3)
#define	DPRINTF4(fmt,a1,a2,a3,a4)
#define	DPRINTF5(fmt,a1,a2,a3,a4,a5)
#endif


#define	isDigit(ch)	(((ch) >= '0') && ((ch) <= '9'))
#define	isBlank(ch)	(((ch) == ' ') || ((ch) == '\t'))


typedef	char		Packedbool;
typedef	unsigned char	State;
typedef	unsigned int	Status;


/*
 * Status returned by routines
 */
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
	Packedbool	free;		/* this cell still has free choice */
	Packedbool	frozen;		/* this cell is frozen in all gens */
	Packedbool	choose;		/* can choose this cell if unknown */
	short		gen;		/* generation number of this cell */
	short		row;		/* row of this cell */
	short		col;		/* column of this cell */
	short		near;		/* count of cells this cell is near */
	Cell *		search;		/* cell next to be searched */
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
	RowInfo *	rowInfo;	/* info about this cell's row */
	ColInfo *	colInfo;	/* info about this cell's column */
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
EXTERN	bool	rowSym;		/* enable row symmetry starting at column */
EXTERN	bool	colSym;		/* enable column symmetry starting at row */
EXTERN	bool	pointSym;	/* enable symmetry with central point */
EXTERN	bool	fwdSym;		/* enable forward diagonal symmetry */
EXTERN	bool	bwdSym;		/* enable backward diagonal symmetry */
EXTERN	bool	flipRows;	/* flip rows at column number from last to first generation */
EXTERN	bool	flipCols;	/* flip columns at row number from last to first generation */
EXTERN	bool	flipQuads;	/* flip quadrants from last to first gen */
EXTERN	bool	parent;		/* only look for parents */
EXTERN	bool	allObjects;	/* look for all objects including subPeriods */
EXTERN	bool	setDeep;	/* set cleared cells deeply from init file */
EXTERN	int	nearCols;	/* maximum distance to be near columns */
EXTERN	int	maxCount;	/* maximum number of cells in generation 0 */
EXTERN	int	useRow;		/* row that must have at least one ON cell */
EXTERN	int	useCol;		/* column that must have at least one ON cell */
EXTERN	int	colCells;	/* maximum cells in a column */
EXTERN	int	colWidth;	/* maximum width of each column */
EXTERN	bool	follow;		/* follow average position of previous column */
EXTERN	bool	orderWide;	/* ordering tries to find wide objects */
EXTERN	bool	orderGens;	/* ordering tries all gens first */
EXTERN	bool	orderMiddle;	/* ordering tries middle columns first */
EXTERN	bool	followGens;	/* try to follow setting of other gens */


/*
 * These values are not affected when dumping and loading since they
 * do not affect the status of a search in progress.
 * They are either setTable on the command line or are computed.
 */
EXTERN	bool	quiet;		/* don't output */
EXTERN	bool	debug;		/* enable debugging output (if compiled so) */
EXTERN	bool	quitOk;		/* ok to quit without confirming */
EXTERN	bool	inited;		/* initialization has been done */
EXTERN	State	bornRules[9];	/* rules for whether a cell is to be born */
EXTERN	State	liveRules[9];	/* rules for whether a live cell stays alive */
EXTERN	int	curGen;		/* current generation for display */
EXTERN	int	outputCols;	/* number of columns to save for output */
EXTERN	int	outputLastCols;	/* last number of columns output */
EXTERN	int	cellCount;	/* number of live cells in generation 0 */
EXTERN	long	dumpFreq;	/* how often to perform dumps */
EXTERN	long	dumpcount;	/* counter for dumps */
EXTERN	long	viewFreq;	/* how often to view results */
EXTERN	long	viewCount;	/* counter for viewing */
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
extern	void	writeGen(const char *, bool);
extern	void	dumpState(const char *);
extern	void	adjustNear(Cell *, int);
extern	Status	search(void);
extern	Status	proceed(Cell *, State, bool);
extern	Status	go(Cell *, State, bool);
extern	Status	setCell(Cell *, State, bool);
extern	Cell *	findCell(int, int, int);
extern	Cell *	backup(void);
extern	bool	subPeriods(void);
extern	void	loopCells(Cell *, Cell *);
extern	void	fatal(const char *);
extern	bool	ttyOpen(void);
extern	bool	ttyCheck(void);
extern	bool	ttyRead(const char *, char *, int);
extern	void	ttyPrintf(const char *, ...);
extern	void	ttyStatus(const char *, ...);
extern	void	ttyWrite(const char *, int);
extern	void	ttyHome(void);
extern	void	ttyEEop(void);
extern	void	ttyFlush(void);
extern	void	ttyClose(void);

/* END CODE */
