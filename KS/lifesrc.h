/*
 * Life search program include file.
 * Author: David I. Bell.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>        //JES, for isdigit()

#include "bool.h"
#include "state.h"
#include "cell.h"
#include "implication.h"


/*
 * Maximum dimensions of the search
 */
#define    ROW_MAX        80    /* maximum rows for search rectangle */
#define    COL_MAX        132    /* maximum columns for search rectangle */
#define    GEN_MAX        19    /* maximum number of generations */
#define    TRANS_MAX    8    /* largest translation value allowed */
#define MAX_PATH    80


/*
 * Build options
 */
#ifndef DEBUGFLAG
#define    DEBUGFLAG    0    /* nonzero for debugging features */
#endif


/*
 * Other definitions
 */
#define    DUMPVERSION    100        /* version of dump file   JES-was 6 */

#define    LINESIZE    132        /* size of input lines */
#define    VIEWMULT    1000        /* viewing frequency multiplier */
#define    DUMPMULT    1000        /* dumping frequency multiplier */
#define    DUMPFILE    "lifesrc.dmp"    /* default dump file name */

#define    MAX_CELLS    ((COL_MAX + 2) * (ROW_MAX + 2) * GEN_MAX)
#define    AUX_CELLS    (TRANS_MAX * (COL_MAX + ROW_MAX + 4) * 2)


/*
 * Debugging macros
 */
#if DEBUGFLAG
#define DPRINTF(fmt, ...)   if (debug) printf(fmt, ##__VA_ARGS__ )
#else
#define DPRINTF(fmt, ...)
#endif


//#define    isblank(ch)    (((ch) == ' ') || ((ch) == '\t'))

typedef    unsigned int    Status;

/*
 * Status returned by routines
 */
#define    OK        ((Status) 0)

// JES
#define    ERROR        ((Status) 1)
#define    CONSISTENT    ((Status) 2)
#define    NOTEXIST    ((Status) 3)
#define    FOUND        ((Status) 4)


/*
 * Declare this macro so that by default the variables are defined external.
 * In the main program, this is defined as a null value so as to actually
 * define the variables.
 */
#ifndef    EXTERN
#define    EXTERN    extern
#endif


/*
 * Current parameter values for the program to be saved over runs.
 * These values are dumped and loaded by the dump and load commands.
 * If you add another parameter, be sure to also add it to param_table,
 * preferably at the end so as to minimize dump file incompatibilities.
 */
EXTERN    Status    curstatus;    /* current status of search */
EXTERN    int    rowmax;        /* maximum number of rows */
EXTERN    int    colmax;        /* maximum number of columns */
EXTERN    int    genmax;        /* maximum number of generations */
EXTERN    int    rowtrans;    /* translation of rows */
EXTERN    int    coltrans;    /* translation of columns */
EXTERN    Bool    rowsym;        /* enable row symmetry starting at column */
EXTERN    Bool    colsym;        /* enable column symmetry starting at row */
EXTERN    Bool    pointsym;    /* enable symmetry with central point */
EXTERN    Bool    fwdsym;        /* enable forward diagonal symmetry */
EXTERN    Bool    bwdsym;        /* enable backward diagonal symmetry */
EXTERN    Bool    fliprows;    /* flip rows at column number from last to first generation */
EXTERN    Bool    flipcols;    /* flip columns at row number from last to first generation */
EXTERN    Bool    flipquads;    /* flip quadrants from last to first gen */
EXTERN    Bool    parent;        /* only look for parents */
EXTERN    Bool    allobjects;    /* look for all objects including subperiods */

EXTERN    Bool    orderwide;    /* ordering tries to find wide objects */
EXTERN    Bool    ordergens;    /* ordering tries all gens first */
EXTERN    Bool    ordermiddle;    /* ordering tries middle columns first */
EXTERN    Bool    followgens;    /* try to follow setting of other gens */
EXTERN	State   chooseUnknown;  /* First choice for unknown cell, either ON or OFF */

EXTERN  Bool    smarton;      /* use smart method (KAS) */
EXTERN  int smartwindow; /* no. of cells to check */
EXTERN  int smartthreshold; /* check threshold */

EXTERN  int  diagsort;       /* JES - optimize for diagonal objects */
EXTERN  int  knightsort;     /* JES */

/*
 * These values are not affected when dumping and loading since they
 * do not affect the status of a search in progress.
 * They are either settable on the command line or are computed.
 */
EXTERN    Bool    quiet;        /* don't output */
EXTERN    Bool    debug;        /* enable debugging output (if compiled so) */
EXTERN    Bool    quitok;        /* ok to quit without confirming */
EXTERN    Bool    inited;        /* initialization has been done */
EXTERN    State    bornrules[9];    /* rules for whether a cell is to be born */
EXTERN    State    liverules[9];    /* rules for whether a live cell stays alive */
EXTERN    int    curgen;        /* current generation for display */

EXTERN    long    dumpfreq;    /* how often to perform dumps */
EXTERN    long    dumpcount;    /* counter for dumps */
EXTERN    long    viewfreq;    /* how often to view results */
EXTERN    long    viewcount;    /* counter for viewing */
EXTERN    char *    dumpfile;    /* dump file name */
EXTERN    char *    outputfile;    /* file to output results to */

/*
 * Data about all of the cells.
 */
EXTERN    Cell *    settable[MAX_CELLS];    /* table of cells whose value is set */
EXTERN    Cell **    newset;        /* where to add new cells into setting table */
EXTERN    Cell **    nextset;    /* next cell in setting table to examine */
EXTERN  Cell *  searchtable[MAX_CELLS]; /* a stack of searchlist positions */
EXTERN  Cell ** searchset;

/*
 * Other local data.
 */
EXTERN    Cell **    searchlist;    /* current list of cells to search */
EXTERN    int    searchidx;      /* index of first unknown cell in searchlist[] */
EXTERN Cell *    cellTable[MAX_CELLS];    /* table of usual cells */

/*
 * Table of implications.
 * Given the state of a cell and its neighbors in one generation,
 * this table determines deductions about the cell and its neighbors
 * in the previous generation.
 * The table is indexed by the descriptor value of a cell.
 */
EXTERN FLAGS implic[2304];

/*
 * Global procedures
 */
extern    void    initcells(void);
extern  void    initsearchorder(void);
extern    void    printgen(int);
extern    void    writegen(char *, Bool);
extern    void    dumpstate(const char *);
extern    Status    search(void);
extern    Bool    proceed(Cell *, State, Bool);
extern    Bool    setcell(Cell *, State, Bool);
extern  Status  examinenext(void);
extern    Cell *    findcell(int, int, int);
extern    Cell *    backup(void);
extern void setState(Cell * const cell, const State state);

/* END CODE */
