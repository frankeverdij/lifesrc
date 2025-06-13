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
#include "tty.h"
#include "cell.h"


/*
 * Maximum dimensions of the search
 */
#define    ROWMAX        80    /* maximum rows for search rectangle */
#define    COLMAX        132    /* maximum columns for search rectangle */
#define    GENMAX        19    /* maximum number of generations */
#define    TRANSMAX    8    /* largest translation value allowed */
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

#define    MAXCELLS    ((COLMAX + 2) * (ROWMAX + 2) * GENMAX)
#define    AUXCELLS    (TRANSMAX * (COLMAX + ROWMAX + 4) * 2)


/*
 * Debugging macros
 */
#if DEBUGFLAG
#define    DPRINTF0(fmt)            if (debug) printf(fmt)
#define    DPRINTF1(fmt,a1)        if (debug) printf(fmt,a1)
#define    DPRINTF2(fmt,a1,a2)        if (debug) printf(fmt,a1,a2)
#define    DPRINTF3(fmt,a1,a2,a3)        if (debug) printf(fmt,a1,a2,a3)
#define    DPRINTF4(fmt,a1,a2,a3,a4)    if (debug) printf(fmt,a1,a2,a3,a4)
#define    DPRINTF5(fmt,a1,a2,a3,a4,a5)    if (debug) printf(fmt,a1,a2,a3,a4,a5)
#else
#define    DPRINTF0(fmt)
#define    DPRINTF1(fmt,a1)
#define    DPRINTF2(fmt,a1,a2)
#define    DPRINTF3(fmt,a1,a2,a3)
#define    DPRINTF4(fmt,a1,a2,a3,a4)
#define    DPRINTF5(fmt,a1,a2,a3,a4,a5)
#endif


//#define    isblank(ch)    (((ch) == ' ') || ((ch) == '\t'))

typedef    char        PACKED_Bool;
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
EXTERN    int    nearcols;    /* maximum distance to be near columns */
EXTERN    int    maxcount;    /* maximum number of cells in generation 0 */
EXTERN    int    userow;        /* row that must have at least one ON cell */
EXTERN    int    usecol;        /* column that must have at least one ON cell */
EXTERN    int    colcells;    /* maximum cells in a column */
EXTERN    int    colwidth;    /* maximum width of each column */
EXTERN    Bool    follow;        /* follow average position of previous column */
EXTERN    Bool    orderwide;    /* ordering tries to find wide objects */
EXTERN    Bool    ordergens;    /* ordering tries all gens first */
EXTERN    Bool    ordermiddle;    /* ordering tries middle columns first */
EXTERN    Bool    followgens;    /* try to follow setting of other gens */
EXTERN	State   chooseUnknown;  /* First choice for unknown cell, either ON or OFF */

EXTERN  Bool    smart;      /* use smart method (KAS) */
EXTERN  Bool    smarton;
EXTERN  Bool    combine;
EXTERN  Bool    combining;
EXTERN  int smartwindow; /* no. of cells to check */
EXTERN  int smartthreshold; /* check threshold */
EXTERN  int smartstatlen;
EXTERN  int smartstatwnd;
EXTERN  int smartstatsumlen;
EXTERN  int smartstatsumwnd;
EXTERN  int smartstatsumlenc;
EXTERN  int smartstatsumwndc;


EXTERN  int  diagsort;       /* JES - optimize for diagonal objects */
EXTERN  int  knightsort;     /* JES */
EXTERN  int  symmetry;       /* JES */
EXTERN  int  trans_rotate;   /* JES */
EXTERN  int  trans_flip;     /* JES */
EXTERN  int  trans_x;        /* JES */
EXTERN  int  trans_y;        /* JES */

/*
 * These values are not affected when dumping and loading since they
 * do not affect the status of a search in progress.
 * They are either settable on the command line or are computed.
 */
EXTERN    Bool    quiet;        /* don't output */
EXTERN    Bool    quitok;        /* ok to quit without confirming */
EXTERN    Bool    debug;        /* enable debugging output (if compiled so) */
EXTERN    Bool    inited;        /* initialization has been done */
EXTERN    Bool    bornrules[16];    /* rules for whether a cell is to be born */
EXTERN    Bool    liverules[16];    /* rules for whether a live cell stays alive */
EXTERN    int    curgen;        /* current generation for display */
EXTERN    int    outputcols;    /* number of columns to save for output */
EXTERN    int    outputlastcols;    /* last number of columns output */
EXTERN    int    g0oncellcount;    /* number of live cells in generation 0 */
EXTERN  int cellcount; /* number of set cells */
EXTERN    long    dumpfreq;    /* how often to perform dumps */
EXTERN    long    dumpcount;    /* counter for dumps */
EXTERN    long    viewfreq;    /* how often to view results */
EXTERN    long    viewcount;    /* counter for viewing */
EXTERN    char *    dumpfile;    /* dump file name */
EXTERN    char *    outputfile;    /* file to output results to */

EXTERN  int smartlen0;
EXTERN  int smartlen1;
EXTERN  int smartcomb;
EXTERN  State smartchoice; /* preferred state for the selected cell */

EXTERN  State prevstate; /* the state of the last free cell before backup() */

/*
 * Data about all of the cells.
 */
EXTERN    Cell *    settable[MAXCELLS];    /* table of cells whose value is set */
EXTERN    Cell **    newset;        /* where to add new cells into setting table */
EXTERN    Cell **    nextset;    /* next cell in setting table to examine */
EXTERN  Cell *  searchtable[MAXCELLS]; /* a stack of searchlist positions */
EXTERN  Cell ** searchset;
EXTERN    int    fullcolumns;    /* columns in gen 0 which are fully set */
EXTERN  int combinedcells;
EXTERN  int setcombinedcells;
EXTERN  int differentcombinedcells;


/*
 * Global procedures
 */


extern    void    getcommands(void);
extern    void    initcells(void);
extern  void    initsearchorder(void);
extern    void    printgen(int);
extern    void    writegen(char *, Bool);
extern    void    dumpstate(const char *);
extern    void    adjustnear(Cell *, int);
extern    Status    search(const Bool);
extern    Bool    proceed(Cell *, State, Bool);
extern    Bool    go(Cell *, State, Bool);
extern    Bool    setcell(Cell *, State, Bool);
extern  Status  examinenext(void);
extern    Cell *    findcell(int, int, int);
extern    Cell *    backup(void);
extern    Bool    subperiods(void);
extern    void    loopcells(Cell *, Cell *);
extern void setState(Cell * const cell, const State state);

//JES
//void    freezecell(int, int);
//Bool    setrules(char *);
//Bool    loadstate(void);
//void    getbackup(char *cp);

extern int currfield[GENMAX][COLMAX][ROWMAX];
/* END CODE */
