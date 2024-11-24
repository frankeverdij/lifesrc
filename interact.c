/*
 * Life search program - user interactions module.
 * Author: David I. Bell.
 */

#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#define extern

#include "lifesrc.h"
/*#include "state.h"
#include "setstate.h"
#include "printasc.h"
#include "printblk.h"
#include "printrle.h"
#include "sortorder.h"
#include "sectohms.h"
#include "outputtimers.h"
*/
#define VERSION "3.8"
#define	DUMP_FILE	"lifesrc.dmp"

extern struct globals_struct g;

/*
 * Local data.
 */
static BOOL quiet;
static BOOL quitOk;
static BOOL debug;
static BOOL noWait;          /* don't wait for commands after loading */
static BOOL setAll;          /* set all cells from initial file */
static BOOL setDeep;
static BOOL isLife;          /* whether the rules are for standard Life */
static char ruleString[20];  /* rule string for printouts */
static long foundCount;      /* number of objects found */
static char * initFile;      /* file containing initial cells */
static char * loadFile;      /* file to load state from */
static BOOL blockOutput;     /* print Unicode blocks instead of character */
static BOOL RLEOutput;       /* print additional RLE code */
static BOOL augmentOutput;   /* print additional UTF8 code for stateList info */
static time_t startTime;
static char timeBuf[256] = {0};

/*
 * Local procedures
 */
static void usage(void);
static void getSetting(const char *);
static void getBackup(const char *);
static void getClear(const char *);
static void getExclude(const char *);
static void getFreeze(const char *);
static void excludeCone(int, int, int);
static void freezeCell(int, int);
static STATUS loadState(const char *);
static STATUS readFile(const char *);
static BOOL confirm(const char *);
static BOOL setRules(const char *);
static long getNum(const char **, int);
static const char * getStr(const char *, const char *);
static void fatal(const char *);

void alarm_handler(const int signo)
{
    if (signo == SIGUSR1) dumpFlag = TRUE;
    if (signo == SIGUSR2) viewFlag = TRUE;
}


/*
 * Table of addresses of parameters which are loaded and saved.
 * Changing this table may invalidate old dump files, unless new
 * parameters are added at the end and default to zero.
 * When changed incompatibly, the dump file version should be incremented.
 * The table is ended with a NULL pointer.
 */
static int * paramTable[] =
{
    &g.curstatus,
    &g.nrows, &g.ncols, &g.period, &g.rowtrans, &g.coltrans,
    &g.rowsym, &g.colsym, &g.pointsym, &g.fwdsym, &g.bwdsym,
    &g.fliprows, &g.flipcols, &g.flipquads,
    &g.parent, &g.allobjects, &g.nearcols, &g.maxcount,
    &g.userow, &g.usecol, &g.colcells, &g.colwidth, &g.follow,
    &g.orderwide, &g.ordergens, &g.ordermiddle, &g.followgens,
    &g.sortorder, NULL
};


int
main(int argc, char ** argv)
{
    struct sigaction actDump, actView;
    struct sigevent sevDump, sevView;
    struct itimerspec itsDump, itsView;
    timer_t tidDump, tidView;

    time_t end;
    long dif = 0;
    const char *    str;

    setSigaction(&actDump, SIGUSR1, &alarm_handler);
    setSigaction(&actView, SIGUSR2, &alarm_handler);

    /*
     * echo the command line, before the program alters argc
     */
    ttyPrintf("Command line: \n");
    for (int i = 0; i < argc; i++) ttyPrintf("%s ", argv[i]);
    ttyPrintf("\n\n");

    if (--argc <= 0)
    {
        usage();
        exit(1);
    }

    argv++;

    if (!setRules("3/23"))
        fatal("Cannot set Life rules!");

    /*
     * Set a couple of defaults.
     */
    g.viewfreq = 10;
    g.dumpfreq = 0;
    g.ncols = 75;

    /*
     * Collect the command line options.
     */
    while (argc-- > 0)
    {
        str = *argv++;

        if (*str++ != '-')
        {
            usage();
            exit(1);
        }

        switch (*str++)
        {
            case 'b':
                /*
                 * Don't enter command mode.
                 */
                noWait = TRUE;
                break;

            case 'q':
                /*
                 * Don't output.
                 */
                quiet = TRUE;
                break;

            case 'r':
                /*
                 * Set number of rows.
                 */
                g.nrows = atoi(str);
                break;

            case 'c':
                /*
                 * Set number of columns.
                 */
                g.ncols = atoi(str);
                break;

            case 'g':
                /*
                 * Set number of generations.
                 */
                g.period = atoi(str);
                break;

            case 't':
                /*
                 * Set row or column translations.
                 */
                switch (*str++)
                {
                    case 'r':
                        g.rowtrans = atoi(str);
                        break;

                    case 'c':
                        g.coltrans = atoi(str);
                        break;

                    default:
                        fatal("Bad translate");
                }

                break;

            case 'f':
                /*
                 * Flip cells around an axis.
                 */
                switch (*str++)
                {
                    case 'r':
                        g.fliprows = 1;

                        if (*str)
                            g.fliprows = atoi(str);

                        break;

                    case 'c':
                        g.flipcols = 1;

                        if (*str)
                            g.flipcols = atoi(str);

                        break;

                    case 'q':
                        g.flipquads = TRUE;
                        break;

                    case 'g':
                        g.followgens = TRUE;
                        break;

                    case 'o':
                        chooseUnknown = ON;
                        break;

                    case '\0':
                        g.follow = TRUE;
                        break;

                    default:
                        fatal("Bad flip");
                }

                break;

            case 's':
                /*
                 * Set symmetry.
                 */
                switch (*str++)
                {
                    case 'r':
                        g.rowsym = 1;

                        if (*str)
                            g.rowsym = atoi(str);

                        break;

                    case 'c':
                        g.colsym = 1;

                        if (*str)
                            g.colsym = atoi(str);

                        break;

                    case 'p':
                        g.pointsym = TRUE;
                        break;

                    case 'f':
                        g.fwdsym = TRUE;
                        break;

                    case 'b':
                        g.bwdsym = TRUE;
                        break;

                    default:
                        fatal("Bad symmetry");
                }

                break;

            case 'n':
                /*
                 * Set near cells.
                 */
                switch (*str++)
                {
                    case 'c':
                        g.nearcols = atoi(str);
                        break;

                    default:
                        fatal("Bad near");
                }

                break;

            case 'w':
                /*
                 * Set max width of ON cells.
                 */
                switch (*str++)
                {
                    case 'c':
                        g.colwidth = atoi(str);
                        break;

                    default:
                        fatal("Bad width");
                }

                break;

            case 'u':
                /*
                 * Force use of row or column.
                 */
                switch (*str++)
                {
                    case 'r':
                        g.userow = atoi(str);
                        break;

                    case 'c':
                        g.usecol = atoi(str);
                        break;

                    default:
                        fatal("Bad use");
                }

                break;

            case 'd':
                /*
                 * Get dump frequency.
                 */
                g.dumpfreq = atoi(str);
                g.dumpfile = DUMP_FILE;

                if ((argc > 0) && (**argv != '-'))
                {
                    argc--;
                    g.dumpfile = *argv++;
                }

                break;

            case 'V':
                augmentOutput = TRUE;
            case 'v':
                /*
                 * Set view frequency.
                 */
                while ((*str) && !isdigit(*str))
                {
                    switch (*str++)
                    {
                        case 'b':
                            blockOutput = TRUE;
                            break;
                        case 'r':
                            RLEOutput = TRUE;
                            break;
                    }
                }
                if (*str)
                    g.viewfreq = atoi(str);

                break;

            case 'l':
                /*
                 * Load file.
                 */
                if (*str == 'n')
                    noWait = TRUE;

                if ((argc <= 0) || (**argv == '-'))
                    fatal("Missing load file name");

                loadFile = *argv++;
                argc--;
                break;

            case 'i':
                /*
                 * Read initial file.
                 */
                if (*str == 'd')
                {
                    setAll = TRUE;
                    setDeep = TRUE;
                }
                else if (*str != 'n')
                    setAll = TRUE;

                if ((argc <= 0) || (**argv == '-'))
                    fatal("Missing initial file name");

                initFile = *argv++;
                argc--;
                break;

            case 'o':
                /*
                 * Set output columns or file name.
                 */
                if ((*str == '\0') || isdigit(*str))
                {
                    /*
                     * Output file name
                     */
                    g.outputcols = atol(str);

                    if ((argc <= 0) || (**argv == '-'))
                        fatal("Missing output file name");

                    g.outputfile = *argv++;
                    argc--;
                    break;
                }

                /*
                 * An ordering option.
                 */
                while (*str)
                {
                    switch (*str++)
                    {
                        case 'w':
                            g.orderwide = TRUE;
                            break;

                        case 'g':
                            g.ordergens = TRUE;
                            break;

                        case 'i':
                            orderInvert = TRUE;
                            break;

                        case 'm':
                            g.ordermiddle = TRUE;
                            break;

                        case 'r':
                            g.sortorder = SORTORDER_TOPDOWN;
                            break;

                        case 'c':
                            g.sortorder = SORTORDER_LEFTRIGHT;
                            break;

                        case 'f':
                            g.sortorder = SORTORDER_DIAG;
                            break;

                        case 'b':
                            g.sortorder = SORTORDER_BACKDIAG;
                            break;

                        case 'O':
                            g.sortorder = SORTORDER_CENTEROUT;
                            break;

                        default:
                            fatal("Bad ordering or sorting option");
                    }
                }

                break;

            case 'm':
                /*
                 * Set maximum cell count.
                 */
                switch (*str++)
                {
                    case 'c':
                        g.colcells = atoi(str);
                        break;

                    case 't':
                        g.maxcount = atoi(str);
                        break;

                    default:
                        fatal("Bad maximum");
                }

                break;

            case 'p':
                /*
                 * Find parents only.
                 */
                g.parent = TRUE;
                break;

            case 'a':
                /*
                 * Find all objects.
                 */
                g.allobjects = TRUE;
                break;

            case 'D':
                /*
                 * Turn on debugging output.
                 */
                debug = TRUE;
                break;

            case 'R':
                /*
                 * Set rules.
                 */
                if (!setRules(str))
                    fatal("Bad rule string");

                break;

            default:
                ttyClose();

                fprintf(stderr, "Unknown option -%c\n",
                    str[-1]);

                exit(1);
        }
    }

    if (g.parent &&
        (g.rowtrans || g.coltrans || g.flipquads || g.fliprows || g.flipcols))
    {
        fatal("Cannot specify translations or flips with -p");
    }

    if ((g.pointsym != 0) + (g.rowsym || g.colsym) + (g.fwdsym || g.bwdsym) > 1)
        fatal("Conflicting symmetries specified");

    if ((g.fwdsym || g.bwdsym || g.flipfwd || g.flipbwd || g.flipquads) && (g.nrows != g.ncols))
        fatal("Rows must equal cols with -sf, -sb, or -fq");

    if ((g.rowtrans || g.coltrans) + (g.flipquads != 0) > 1)
        fatal("Conflicting translation or flipping specified");

    if ((g.rowtrans && g.fliprows) || (g.coltrans && g.flipcols))
        fatal("Conflicting translation or flipping specified");

    if ((g.userow < 0) || (g.userow > g.nrows))
        fatal("Bad row for -ur");

    if ((g.usecol < 0) || (g.usecol > g.ncols))
        fatal("Bad column for -uc");

    if (!noWait)
    {
        if (!ttyOpen())
            fatal("Cannot initialize terminal");
    }

    /*
     * Check for loading state from file or reading initial
     * object from file.
     */
    if (loadFile)
    {
        if (loadState(loadFile) != OK)
        {
            ttyClose();
            exit(1);
        }
    }
    else
    {
        initCells();

        if (initFile)
        {
            if (readFile(initFile) != OK)
            {
                ttyClose();
                exit(1);
            }

            g.baseset = g.nextset;
        }
    }

    /*
     * If we are looking for parents, then set the current generation
     * to the last one so that it can be input easily.  Then get the
     * commands to initialize the cells, unless we were told to not wait.
     */
    if (g.parent)
        g.curgen = g.period - 1;

    if (noWait)
    {
        if (!quiet)
            printGen(0);
    }
    else
        getCommands();

    inited = TRUE;

    /*
     * Arm the output timers
     */
    if (g.dumpfreq)
    {
        createTimer(&sevDump, &itsDump, &tidDump, SIGUSR1, g.dumpfreq);
        if (timer_settime(tidDump, 0, &itsDump, NULL) == -1)
        {
            perror("timer_settime Dump failed");
            exit(EXIT_FAILURE);
        }
    }

    createTimer(&sevView, &itsView, &tidView, SIGUSR2, g.viewfreq);
    if (timer_settime(tidView, 0, &itsView, NULL) == -1)
    {
        perror("timer_settime View failed");
        exit(EXIT_FAILURE);
    }

    /*
     * Initial commands are complete, now look for the object.
     */
    while (TRUE)
    {
        if (g.curstatus == OK)
        {
            time(&startTime);
            g.curstatus = search(noWait);
            time(&end);
            dif = end - startTime;
            secToHMS(dif, timeBuf);
        }

//        if ((g.curstatus == FOUND) && g.userow &&
//            (rowInfo[g.userow].onCount == 0))
//        {
//            g.curstatus = OK;
//            continue;
//        }

        if ((g.curstatus == FOUND) && !g.allobjects && subPeriods())
        {
            g.curstatus = OK;
            continue;
        }

        if (g.dumpfreq)
        {
            dumpState(g.dumpfile);
        }

        quitOk = (g.curstatus == NOTEXIST);

        g.curgen = 0;

        if (g.outputfile == NULL)
        {
            if (!noWait)
            {
                getCommands();
                continue;
            }
        }

        /*
         * Here if results are going to a file.
         */
        if (g.curstatus == FOUND)
        {
            g.curstatus = OK;

            if (!quiet)
            {
                printGen(0);
                ttyStatus("Object %ld found in%s.\n", ++foundCount, timeBuf);
            }

            writeGen(g.outputfile, TRUE);
            if (noWait)
            {
                if (g.allobjects)
                    continue;
            }
            else
                continue;
        }

        if (foundCount == 0)
        {
            printf("Total time searched%s.\n", timeBuf);
            fatal("No objects found.");
        }
        ttyClose();

        if (!quiet)
        {
            printf("Search completed, file \"%s\" contains %ld object%s\n",
                g.outputfile, foundCount, (foundCount == 1) ? "" : "s");
        }
        exit(0);
    }
}


/*
 * Get one or more user commands.
 * Commands are ended by a blank line.
 */
void
getCommands(void)
{
    const char * cp;
    const char * cmd;
    char buf[LINESIZE];

    printGen(g.curgen);

    while (TRUE)
    {
        if (!ttyRead("> ", buf, LINESIZE))
        {
            ttyClose();
            exit(0);
        }

        cp = buf;

        while (isblank(*cp))
            cp++;

        cmd = cp;

        if (*cp)
            cp++;

        while (isblank(*cp))
            cp++;

        switch (*cmd)
        {
            case 'p':
                /*
                 * Print previous generation.
                  */
                printGen((g.curgen + g.period - 1) % g.period);
                break;

            case 'n':
                /*
                 * Print next generation.
                 */
                printGen((g.curgen + 1) % g.period);
                break;

            case 's':
                /*
                 * Add a cell setting.
                 */
                getSetting(cp);
                break;

            case 'b':
                /*
                 * Back up the search.
                 */
                getBackup(cp);
                break;

            case 'c':
                /*
                 * Clear an area.
                 */
                getClear(cp);
                break;

            case 'v':
                /*
                 * Set viewing frequency.
                 */
                g.viewfreq = atol(cp) * VIEW_MULT;
                printGen(g.curgen);
                break;

            case 'w':
                /*
                 * Write generation to a file.
                 */
                writeGen(cp, FALSE);
                break;

            case 'd':
                /*
                 * Dump state to a file.
                 */
                dumpState(cp);
                break;

            case 'N':
                /*
                 * Find next object.
                 */
                if (g.curstatus == FOUND)
                    g.curstatus = OK;

                return;

            case 'q':
            case 'Q':
                /*
                 * Quit program.
                 */
                if (quitOk || confirm("Really quit? "))
                {
                    ttyClose();
                    exit(0);
                }

                break;

            case 'x':
                /*
                 * Exclude cells from the search.
                 */
                getExclude(cp);
                break;

            case 'f':
                /*
                 * Free state of cells.
                 */
                getFreeze(cp);
                break;

            case '\n':
            case '\0':
                /*
                 * Return from commands to the search.
                 */
                return;

            default:
                /*
                 * If a digit, set that cell.
                 */
                if (isdigit(*cmd))
                {
                    getSetting(cmd);
                    break;
                }

                ttyStatus("Unknown command\n");
                break;
        }
    }
}


/*
 * Get a cell to be set in the current generation.
 * The state of the cell is defaulted to ON.
 * Warning: Use of this routine invalidates backing up over
 * the setting, so that the setting is permanent.
 */
static void
getSetting(const char * cp)
{
    int row;
    int col;
    STATE state;

    cp = getStr(cp, "Cell to set (row col [state]): ");

    if (*cp == '\0')
        return;

    row = getNum(&cp, -1);

    if (*cp == ',')
        cp++;

    col = getNum(&cp, -1);

    if (*cp == ',')
        cp++;

    state = getNum(&cp, 1);

    while (isblank(*cp))
        cp++;

    if (*cp != '\0')
    {
        ttyStatus("Bad input line format\n");

        return;
    }

    if ((row <= 0) || (row > g.nrows) || (col <= 0) || (col > g.ncols) ||
        ((state != 0) && (state != 1)))
    {
        ttyStatus("Illegal cell value\n");

        return;
    }

    if (proceed(findCell(row, col, g.curgen), state, FALSE) != OK)
    {
        ttyStatus("Inconsistent state for cell\n");

        return;
    }

    g.baseset = g.nextset;
    printGen(g.curgen);
}


/*
 * Backup the search to the nth latest free choice.
 * Notice: This skips examinination of some of the possibilities, thus
 * maybe missing a solution.  Therefore this should only be used when it
 * is obvious that the current search state is useless.
 */
static void
getBackup(const char * cp)
{
    CELL * cell;
    STATE state;
    int count;
    int blanksToo;

    blanksToo = TRUE;
#if 0
    /*
     * This doesn't work!
     */
    blanksToo = FALSE;

    if (*cp == 'b')
    {
        blanksToo = TRUE;
        cp++;
    }
#endif
    count = getNum(&cp, 0);

    if ((count <= 0) || *cp)
    {
        ttyStatus("Must back up at least one cell\n");

        return;
    }

    while (count > 0)
    {
        cell = backup();

        if (cell == NULL_CELL)
        {
            printGen(g.curgen);
            ttyStatus("Backed up over all possibilities\n");

            return;
        }

        state = 1 - cell->state;

        if (blanksToo || (state == ON))
            count--;

        setState(cell, UNK);

        if (go(cell, state, FALSE) != OK)
        {
            printGen(g.curgen);
            ttyStatus("Backed up over all possibilities\n");

            return;
        }
    }

    printGen(g.curgen);
}


/*
 * Clear all remaining unknown cells in the current generation or all
 * generations, or else just the specified rectangular area.  If
 * clearing the whole area, then confirmation is required.
 */
static void
getClear(const char * cp)
{
    int beggen;
    int begRow;
    int begCol;
    int endGen;
    int endRow;
    int endCol;
    int gen;
    int row;
    int col;
    CELL * cell;

    /*
     * Assume we are doing just this generation, but if the 'cg'
     * command was given, then clear in all generations.
     */
    beggen = g.curgen;
    endGen = g.curgen;

    if (*cp == 'g')
    {
        cp++;
        beggen = 0;
        endGen = g.period - 1;
    }

    while (isblank(*cp))
        cp++;

    /*
     * Get the coordinates.
     */
    if (*cp)
    {
        begRow = getNum(&cp, -1);
        begCol = getNum(&cp, -1);
        endRow = getNum(&cp, -1);
        endCol = getNum(&cp, -1);
    }
    else
    {
        if (!confirm("Clear all unknown cells ?"))
            return;

        begRow = 1;
        begCol = 1;
        endRow = g.nrows;
        endCol = g.ncols;
    }

    if ((begRow < 1) || (begRow > endRow) || (endRow > g.nrows) ||
        (begCol < 1) || (begCol > endCol) || (endCol > g.ncols))
    {
        ttyStatus("Illegal clear coordinates");

        return;
    }

    for (row = begRow; row <= endRow; row++)
    {
        for (col = begCol; col <= endCol; col++)
        {
            for (gen = beggen; gen <= endGen; gen++)
            {
                cell = findCell(row, col, gen);

                if (cell->state != UNK)
                    continue;

                if (proceed(cell, OFF, FALSE) != OK)
                {
                    ttyStatus("Inconsistent state for cell\n");

                    return;
                }
            }
        }
    }

    g.baseset = g.nextset;
    printGen(g.curgen);
}


/*
 * Exclude cells in a rectangular area from searching.
 * This simply means that such cells will not be selected for setting.
 */
static void
getExclude(const char * cp)
{
    int begRow;
    int begCol;
    int endRow;
    int endCol;
    int row;
    int col;

    while (isblank(*cp))
        cp++;

    if (*cp == '\0')
    {
        ttyStatus("Coordinates needed for exclusion");

        return;
    }

    begRow = getNum(&cp, -1);
    begCol = getNum(&cp, -1);
    endRow = begRow;
    endCol = begCol;

    while (isblank(*cp))
        cp++;

    if (*cp)
    {
        endRow = getNum(&cp, -1);
        endCol = getNum(&cp, -1);
    }

    if ((begRow < 1) || (begRow > endRow) || (endRow > g.nrows) ||
        (begCol < 1) || (begCol > endCol) || (endCol > g.ncols))
    {
        ttyStatus("Illegal exclusion coordinates");

        return;
    }

    for (row = begRow; row <= endRow; row++)
    {
        for (col = begCol; col <= endCol; col++)
            excludeCone(row, col, g.curgen);
    }

    printGen(g.curgen);
}


/*
 * Exclude all cells within the previous light cone centered at the
 * specified cell from searching.
 */
static void
excludeCone(int row, int col, int gen)
{
    int tGen;
    int tRow;
    int tCol;
    int dist;
    CELL * cell;

    for (tGen = g.period; tGen >= gen; tGen--)
    {
        dist = tGen - gen;

        for (tRow = row - dist; tRow <= row + dist; tRow++)
        {
            for (tCol = col - dist; tCol <= col + dist; tCol++)
            {
                cell = findCell(tRow, tCol, tGen);
                /* cell->choose = FALSE; */
            }
        }
    }
}


/*
 * Freeze cells in a rectangular area so that their states in all
 * generations are the same.
 */
static void
getFreeze(const char * cp)
{
    int begRow;
    int begCol;
    int endRow;
    int endCol;
    int row;
    int col;

    while (isblank(*cp))
        cp++;

    if (*cp == '\0')
    {
        ttyStatus("Coordinates needed for freezing");

        return;
    }

    begRow = getNum(&cp, -1);
    begCol = getNum(&cp, -1);
    endRow = begRow;
    endCol = begCol;

    while (isblank(*cp))
        cp++;

    if (*cp)
    {
        endRow = getNum(&cp, -1);
        endCol = getNum(&cp, -1);
    }

    if ((begRow < 1) || (begRow > endRow) || (endRow > g.nrows) ||
        (begCol < 1) || (begCol > endCol) || (endCol > g.ncols))
    {
        ttyStatus("Illegal freeze coordinates");

        return;
    }

    for (row = begRow; row <= endRow; row++)
    {
        for (col = begCol; col <= endCol; col++)
            freezeCell(row, col);
    }

    printGen(g.curgen);
}


/*
 * Freeze all generations of the specified cell.
 * A frozen cell can be ON or OFF, but must be the same in all generations.
 * This routine marks them as frozen, and also inserts all the cells of
 * the generation into the same loop so that they will be forced
 * to have the same state.
 */
void
freezeCell(int row, int col)
{
    int gen;
    CELL * cell0;
    CELL * cell;

    cell0 = findCell(row, col, 0);

    for (gen = 0; gen < g.period; gen++)
    {
        cell = findCell(row, col, gen);

        cell->frozen = TRUE;

        loopCells(cell0, cell);
    }
}


/*
 * Print out the current status of the specified generation.
 * This also sets the current generation.
 */
void
printGen(int gen)
{
    int row;
    int col;
    int count = 0, unkCount = 0;
    const CELL * cell;
    const char * msg;
    time_t mark;
    long dif;

    g.curgen = gen;

    switch (g.curstatus)
    {
        case NOTEXIST:
            msg = "No such object";
            break;
        case FOUND:
            time(&mark);
            dif = mark - startTime;
            secToHMS(dif, timeBuf);
            msg = "Found object";
            break;
        default:
            msg = "";
            break;
    }

    for (row = 1; row <= g.nrows; row++)
    {
        for (col = 1; col <= g.ncols; col++)
        {
            cell = findCell(row, col, gen);
            count += (cell->state == ON);
            unkCount += (cell->state == UNK);
        }
    }

    ttyHome();
    ttyEEop();

    if (isLife)
    {
        if (g.curstatus == FOUND)
        {
            ttyPrintf("%s%s (gen %d, cells %d unk %d confl %ld)", msg, timeBuf, gen, count, unkCount, stepConfl);
        }
        else
        {
            ttyPrintf("%s (gen %d, cells %d unk %d confl %ld)", msg, gen, count, unkCount, stepConfl);
        }
    }
    else
    {
        if (g.curstatus == FOUND)
        {
            ttyPrintf("%s%s (rule %s, gen %d, cells %d unk %d confl %ld)",
            msg, timeBuf, ruleString, gen, count, unkCount, stepConfl);
        }
        else
        {
            ttyPrintf("%s (rule %s, gen %d, cells %d unk %d confl %ld)",
            msg, ruleString, gen, count, unkCount, stepConfl);
        }
    }

    ttyPrintf(" -r%d -c%d -g%d", g.nrows, g.ncols, g.period);

    if (g.rowtrans)
        ttyPrintf(" -tr%d", g.rowtrans);

    if (g.coltrans)
        ttyPrintf(" -tc%d", g.coltrans);

    if (g.fliprows == 1)
        ttyPrintf(" -fr");

    if (g.fliprows > 1)
        ttyPrintf(" -fr%d", g.fliprows);

    if (g.flipcols == 1)
        ttyPrintf(" -fc");

    if (g.flipcols > 1)
        ttyPrintf(" -fc%d", g.flipcols);

    if (g.flipfwd)
        ttyPrintf(" -ff");

    if (g.flipbwd)
        ttyPrintf(" -fb");

    if (g.flipquads)
        ttyPrintf(" -fq");

    if (g.rowsym == 1)
        ttyPrintf(" -sr");

    if (g.rowsym > 1)
        ttyPrintf(" -sr%d", g.rowsym);

    if (g.colsym == 1)
        ttyPrintf(" -sc");

    if (g.colsym > 1)
        ttyPrintf(" -sc%d", g.colsym);

    if (g.pointsym)
        ttyPrintf(" -sp");

    if (g.fwdsym)
        ttyPrintf(" -sf");

    if (g.bwdsym)
        ttyPrintf(" -sb");

    if (g.ordergens || g.orderwide || orderInvert || g.ordermiddle || (g.sortorder != DEFAULT))
    {
        ttyPrintf(" -o");

        if (g.ordergens)
            ttyPrintf("g");

        if (g.orderwide)
            ttyPrintf("w");

        if (orderInvert)
            ttyPrintf("i");

        if (g.ordermiddle)
            ttyPrintf("m");

        if (g.sortorder == SORTORDER_DIAG)
            ttyPrintf("f");
        else if (g.sortorder == SORTORDER_BACKDIAG)
            ttyPrintf("b");
        else if (g.sortorder == SORTORDER_TOPDOWN)
            ttyPrintf("r");
        else if (g.sortorder == SORTORDER_LEFTRIGHT)
            ttyPrintf("c");
        else if (g.sortorder == SORTORDER_CENTEROUT)
            ttyPrintf("O");
    }

    if (g.follow)
        ttyPrintf(" -f");

    if (g.followgens)
        ttyPrintf(" -fg");

    if (chooseUnknown)
        ttyPrintf(" -fo");

    if (g.parent)
        ttyPrintf(" -p");

    if (g.allobjects)
        ttyPrintf(" -a");

    if (g.userow)
        ttyPrintf(" -ur%d", g.userow);

    if (g.usecol)
        ttyPrintf(" -uc%d", g.usecol);

    if (g.nearcols)
        ttyPrintf(" -nc%d", g.nearcols);

    if (g.maxcount)
        ttyPrintf(" -mt%d", g.maxcount);

    if (g.colcells)
        ttyPrintf(" -mc%d", g.colcells);

    if (g.colwidth)
        ttyPrintf(" -wc%d", g.colwidth);

    if (g.viewfreq)
        ttyPrintf(" -v%d", g.viewfreq);

    if (g.dumpfreq)
        ttyPrintf(" -d%d %s", g.dumpfreq, g.dumpfile);

    if (g.outputfile)
    {
        if (g.outputcols)
            ttyPrintf(" -o%d %s", g.outputcols, g.outputfile);
        else
            ttyPrintf(" -o %s", g.outputfile);

        if (foundCount)
            ttyPrintf(" [%d]", foundCount);
    }

    ttyPrintf("\n");

    if (!blockOutput)
    {
        printAsc(gen, augmentOutput);
    }
    else
    {
        printBlk(gen, augmentOutput);
    }

    if (RLEOutput)
    {
        printRLE(gen, ruleString);
    }

    ttyHome();
    ttyFlush();
}


/*
 * Write the current generation to the specified file.
 * Empty rows and columns are not written.
 * If no file is specified, it is asked for.
 * Filename of "." means write to stdout.
 */
void
writeGen(const char * file, BOOL append)
{
    FILE * fp;
    const CELL * cell;
    int row;
    int col;
    int ch;
    int minRow;
    int maxRow;
    int minCol;
    int maxCol;

    file = getStr(file, "Write object to file: ");

    if (*file == '\0')
        return;

    fp = stdout;

    if (strcmp(file, "."))
        fp = fopen(file, append ? "a" : "w");

    if (fp == NULL)
    {
        ttyStatus("Cannot create \"%s\"\n", file);

        return;
    }

    /*
     * First find the minimum bounds on the object.
     */
    minRow = g.nrows;
    minCol = g.ncols;
    maxRow = 1;
    maxCol = 1;

    for (row = 1; row <= g.nrows; row++)
    {
        for (col = 1; col <= g.ncols; col++)
        {
            cell = findCell(row, col, g.curgen);

            if (cell->state == OFF)
                continue;

            if (row < minRow)
                minRow = row;

            if (row > maxRow)
                maxRow = row;

            if (col < minCol)
                minCol = col;

            if (col > maxCol)
                maxCol = col;
        }
    }

    if (minRow > maxRow)
    {
        minRow = 1;
        maxRow = 1;
        minCol = 1;
        maxCol = 1;
    }

    if (fp == stdout)
        fprintf(fp, "#\n");

    /*
     * Now write out the bounded area.
     */
    for (row = minRow; row <= maxRow; row++)
    {
        for (col = minCol; col <= maxCol; col++)
        {
            cell = findCell(row, col, g.curgen);

            switch (cell->state)
            {
                case OFF:    ch = '.'; break;
                case ON:    ch = '*'; break;
                case UNK:    ch = '?'; break;
                        break;
                default:
                    ttyStatus("Bad cell state");
                    fclose(fp);

                    return;
            }

            fputc(ch, fp);
        }

        fputc('\n', fp);
    }

    if (append)
        fprintf(fp, "\n");

    if ((fp != stdout) && fclose(fp))
    {
        ttyStatus("Error writing \"%s\"\n", file);

        return;
    }

    if (fp != stdout)
        ttyStatus("\"%s\" written\n", file);

    quitOk = TRUE;
}


/*
 * Dump the current state of the search in the specified file.
 * If no file is specified, it is asked for.
 */
void
dumpState(const char * file)
{
    FILE * fp;
    CELL ** set;
    const CELL * cell;
    int row;
    int col;
    int gen;
    int ** param;

    file = getStr(file, "Dump state to file: ");

    if (*file == '\0')
        return;

    fp = fopen(file, "w");

    if (fp == NULL)
    {
        ttyStatus("Cannot create \"%s\"\n", file);

        return;
    }

    /*
     * Dump out the version so we can detect incompatible formats.
     */
    fprintf(fp, "V %d\n", DUMPVERSION);

    /*
     * Dump out the life rule if it is not the normal one.
     */
    if (!isLife)
        fprintf(fp, "R %s\n", ruleString);

    /*
     * Dump out the parameter values.
     */
    fprintf(fp, "P");

    for (param = paramTable; *param; param++)
        fprintf(fp, " %d", **param);

    fprintf(fp, "\n");

    /*
     * Dump out those cells which have a setting.
     */
    set = g.settable;

    while (set != g.nextset)
    {
        cell = *set++;

        fprintf(fp, "S %d %d %d %d %d\n", cell->row, cell->col,
            cell->gen, cell->state, (cell->free) ? 1 : 0);
    }

    /*
     * Dump out those cells which are being excluded from the search.
     */
    for (row = 1; row <= g.nrows; row++)
        for (col = 1; col < g.ncols; col++)
            for (gen = 0; gen < g.period; gen++)
    {
        cell = findCell(row, col, gen);

        /*if (cell->choose)
            continue;*/

        fprintf(fp, "X %d %d %d\n", row, col, gen);
    }

    /*
     * Dump out those cells in generation 0 which are frozen.
     * It isn't necessary to remember frozen cells in other
     * generations since they will be copied from generation 0.
     */
    for (row = 1; row <= g.nrows; row++)
        for (col = 1; col < g.ncols; col++)
    {
        cell = findCell(row, col, 0);

        if (cell->frozen)
            fprintf(fp, "F %d %d\n", row, col);
    }

    /*
     * Finish up with the setting offsets and the final line.
     */
    fprintf(fp, "T %ld %ld\n", g.baseset - g.settable, g.nextset - g.settable);
    fprintf(fp, "E\n");

    if (fclose(fp))
    {
        ttyStatus("Error writing \"%s\"\n", file);

        return;
    }

    ttyStatus("State dumped to \"%s\"\n", file);
    quitOk = TRUE;
}


/*
 * Load a previously dumped state from a file.
 * Warning: Almost no checks are made for validity of the state.
 * Returns OK on success, ERROR on failure.
 */
static STATUS
loadState(const char * file)
{
    FILE * fp;
    const char * cp;
    int row;
    int col;
    int gen;
    int len;
    STATE state;
    BOOL free;
    CELL * cell;
    int ** param;
    char buf[LINESIZE];

    file = getStr(file, "Load state from file: ");

    if (*file == '\0')
        return OK;

    fp = fopen(file, "r");

    if (fp == NULL)
    {
        ttyStatus("Cannot open state file \"%s\"\n", file);

        return ERROR1;
    }

    buf[0] = '\0';
    fgets(buf, LINESIZE, fp);

    if (buf[0] != 'V')
    {
        ttyStatus("Missing version line in file \"%s\"\n", file);
        fclose(fp);

        return ERROR1;
    }

    cp = &buf[1];

    if (getNum(&cp, 0) != DUMPVERSION)
    {
        ttyStatus("Unknown version in state file \"%s\"\n", file);
        fclose(fp);

        return ERROR1;
    }

    fgets(buf, LINESIZE, fp);

    /*
     * Set the life rules if they were specified.
     * This line is optional.
     */
    if (buf[0] == 'R')
    {
        len = strlen(buf) - 1;

        if (buf[len] == '\n')
            buf[len] = '\0';

        cp = &buf[1];

        while (isblank(*cp))
            cp++;

        if (!setRules(cp))
        {
            ttyStatus("Bad Life rules in state file\n");
            fclose(fp);

            return ERROR1;
        }

        fgets(buf, LINESIZE, fp);
    }

    /*
     * Load up all of the parameters from the parameter line.
     * If parameters are missing at the end, they are defaulted to zero.
     */
    if (buf[0] != 'P')
    {
        ttyStatus("Missing parameter line in state file\n");
        fclose(fp);

        return ERROR1;
    }

    cp = &buf[1];

    for (param = paramTable; *param; param++)
        **param = getNum(&cp, 0);

    /*
     * Initialize the cells.
     */
    initCells();

    /*
     * Handle cells which have been set.
     */
    g.newset = g.settable;

    for (;;)
    {
        buf[0] = '\0';
        fgets(buf, LINESIZE, fp);

        if (buf[0] != 'S')
            break;

        cp = &buf[1];
        row = getNum(&cp, 0);
        col = getNum(&cp, 0);
        gen = getNum(&cp, 0);
        state = getNum(&cp, 0);
        free = getNum(&cp, 0);

        cell = findCell(row, col, gen);

        if (setCell(cell, state, free) != OK)
        {
            ttyStatus(
                "Inconsistently setting cell at r%d c%d g%d \n",
                row, col, gen);

            fclose(fp);

            return ERROR1;
        }
    }

    /*
     * Handle non-choosing cells.
     */
    while (buf[0] == 'X')
    {
        cp = &buf[1];
        row = getNum(&cp, 0);
        col = getNum(&cp, 0);
        gen = getNum(&cp, 0);

        cell = findCell(row, col, gen);
        /* cell->choose = FALSE; */

        buf[0] = '\0';
        fgets(buf, LINESIZE, fp);
    }

    /*
     * Handle frozen cells.
     */
    while (buf[0] == 'F')
    {
        cp = &buf[1];
        row = getNum(&cp, 0);
        col = getNum(&cp, 0);

        freezeCell(row, col);

        buf[0] = '\0';
        fgets(buf, LINESIZE, fp);
    }

    if (buf[0] != 'T')
    {
        ttyStatus("Missing table line in state file\n");
        fclose(fp);

        return ERROR1;
    }

    cp = &buf[1];
    g.baseset = &g.settable[getNum(&cp, 0)];
    g.nextset = &g.settable[getNum(&cp, 0)];

    fgets(buf, LINESIZE, fp);

    if (buf[0] != 'E')
    {
        ttyStatus("Missing end of file line in state file\n");
        fclose(fp);

        return ERROR1;
    }

    if (fclose(fp))
    {
        ttyStatus("Error reading \"%s\"\n", file);

        return ERROR1;
    }

    ttyStatus("State loaded from \"%s\"\n", file);
    quitOk = TRUE;

    return OK;
}


/*
 * Read a file containing initial settings for either gen 0 or the last gen.
 * If setAll is TRUE, both the ON and the OFF cells will be set.
 * If setDeep is TRUE, then OFF cells will be set deeply (in all generations).
 * Returns OK on success, ERROR on error.
 */
static STATUS
readFile(const char * file)
{
    FILE * fp;
    const char * cp;
    char ch;
    int row;
    int col;
    int activeGen;
    int minGen;
    int maxGen;
    int gen;
    STATE state;
    char buf[LINESIZE];

    file = getStr(file, "Read initial object from file: ");

    if (*file == '\0')
        return OK;

    fp = fopen(file, "r");

    if (fp == NULL)
    {
        ttyStatus("Cannot open \"%s\"\n", file);

        return ERROR1;
    }

    activeGen = (g.parent ? (g.period - 1) : 0);
    row = 0;

    while (fgets(buf, LINESIZE, fp))
    {
        row++;
        cp = buf;
        col = 0;

        while (*cp && (*cp != '\n'))
        {
            minGen = activeGen;
            maxGen = activeGen;

            col++;
            ch = *cp++;

            /*
             * Check for out of range coordinates.
             * OFF and UNK cells are allowed for convenience.
             */
            if ((row > g.nrows) || (col > g.ncols))
            {
                if ((ch == '.') || (ch == ' ') ||
                    (ch == ':') || (ch == '?'))
                {
                    continue;
                }

                fatal("File sets cells beyond defined area");
            }


            /*
             * OK, handle the character.
             */
            switch (ch)
            {
                case '?':
                    continue;

                case 'x':
                case 'X':
                    excludeCone(row, col, activeGen);
                    continue;

                case '+':
                    freezeCell(row, col);
                    continue;

                case '.':
                case ' ':
                    if (!setAll)
                        continue;

                    if (setDeep)
                    {
                        minGen = 0;
                        maxGen = g.period;
                    }

                    state = OFF;
                    break;

                case ':':
                    minGen = 0;
                    maxGen = g.period;
                    state = OFF;
                    break;

                case 'O':
                case 'o':
                case '*':
                    state = ON;
                    break;

                default:
                    ttyStatus("Bad file format in line %d\n",
                        row);
                    fclose(fp);

                    return ERROR1;
            }

            for (gen = minGen; gen <= maxGen; gen++)
            {
                if (proceed(findCell(row, col, gen),
                    state, FALSE) != OK)
                {
                    ttyStatus(
                    "Inconsistent state for cell %d %d\n",
                        row, col);

                    fclose(fp);

                    return ERROR1;
                }
            }
        }
    }

    if (fclose(fp))
    {
        ttyStatus("Error reading \"%s\"\n", file);

        return ERROR1;
    }

    return OK;
}


/*
 * Check a string for being NULL, and if so, ask the user to specify a
 * value for it.  Returned string may be static and thus is overwritten
 * for each call.  Leading spaces in the string are skipped over.
 */
static const char *
getStr(const char * str, const char * prompt)
{
    static char buf[LINESIZE];

    if ((str == NULL) || (*str == '\0'))
    {
        if (!ttyRead(prompt, buf, LINESIZE))
        {
            buf[0] = '\0';

            return buf;
        }

        str = buf;
    }

    while (isblank(*str))
        str++;

    return str;
}


/*
 * Confirm an action by prompting with the specified string and reading
 * an answer.  Entering 'y' or 'Y' indicates TRUE, everything else FALSE.
 */
static BOOL
confirm(const char * prompt)
{
    int ch;

    ch = *getStr(NULL, prompt);

    if ((ch == 'y') || (ch == 'Y'))
        return TRUE;

    return FALSE;
}


/*
 * Read a number from a string, eating any leading or trailing blanks.
 * Returns the value, and indirectly updates the string pointer.
 * Returns specified default if no number was found.
 */
static long
getNum(const char ** cpp, int defnum)
{
    const char * cp;
    long num;
    BOOL isNeg;

    isNeg = FALSE;
    cp = *cpp;

    while (isblank(*cp))
        cp++;

    if (*cp == '-')
    {
        cp++;
        isNeg = TRUE;
    }

    if (!isdigit(*cp))
    {
        *cpp = cp;

        return defnum;
    }

    num = 0;

    while (isdigit(*cp))
        num = num * 10 + (*cp++ - '0');

    if (isNeg)
        num = -num;

    while (isblank(*cp))
        cp++;

    *cpp = cp;

    return num;
}


/*
 * Parse a string and set the Life rules from it.
 * Returns TRUE on success, or FALSE on an error.
 * The rules can be "mmm,nnn",  "mmm/nnn", "Bmmm,Snnn", "Bmmm/Snnn",
 * or a hex number in the Wolfram encoding.
 */
static BOOL
setRules(const char * cp)
{
    char * cpTemp;
    int i;
    unsigned int bits;

    for (i = 0; i < 9; i++)
    {
        g.bornrules[i] = OFF;
        g.liverules[i] = OFF;
    }

    if (*cp == '\0')
        return FALSE;

    /*
     * See if the string contains a comma or a slash.
     * If not, then assume Wolfram's hex format.
     */
    if ((strchr(cp, ',') == NULL) && (strchr(cp, '/') == NULL))
    {
        bits = 0;

        for (; *cp; cp++)
        {
            bits <<= 4;

            if ((*cp >= '0') && (*cp <= '9'))
                bits += *cp - '0';
            else if ((*cp >= 'a') && (*cp <= 'f'))
                bits += *cp - 'a' + 10;
            else if ((*cp >= 'A') && (*cp <= 'F'))
                bits += *cp - 'A' + 10;
            else
                return FALSE;
        }

        if (i & ~0x3ff)
            return FALSE;

        for (i = 0; i < 9; i++)
        {
            if (bits & 0x01)
                g.bornrules[i] = ON;

            if (bits & 0x02)
                g.liverules[i] = ON;

            bits >>= 2;
        }
    }
    else
    {
        /*
         * It is in normal born/survive format.
         */
        if ((*cp == 'b') || (*cp == 'B'))
            cp++;

        while ((*cp >= '0') && (*cp <= '8'))
            g.bornrules[*cp++ - '0'] = ON;

        if ((*cp != ',') && (*cp != '/'))
            return FALSE;

        cp++;

        if ((*cp == 's') || (*cp == 'S'))
            cp++;

        while ((*cp >= '0') && (*cp <= '8'))
            g.liverules[*cp++ - '0'] = ON;

        if (*cp)
            return FALSE;
    }

    /*
     * Construct the rule string for printouts and see if this
     * is the normal Life rule.
     */
    cpTemp = ruleString;

    *cpTemp++ = 'B';

    for (i = 0; i < 9; i++)
    {
        if (g.bornrules[i] == ON)
            *cpTemp++ = '0' + i;
    }

    *cpTemp++ = '/';
    *cpTemp++ = 'S';

    for (i = 0; i < 9; i++)
    {
        if (g.liverules[i] == ON)
            *cpTemp++ = '0' + i;
    }

    *cpTemp = '\0';

    isLife = (strcmp(ruleString, "B3/S23") == 0);

    return TRUE;
}


/*
 * Print out a fatal message and exit.
 * The terminal is closed before the message is printed.
 * A newline is added after the supplied message.
 */
void
fatal(const char * msg)
{
    ttyClose();

    fprintf(stderr, "%s\n", msg);

    exit(1);
}


/*
 * Print usage text.
 */
static void
usage(void)
{
    const char * const * cpp;

    static const char * const text[] =
    {
    "",
    "lifesrc -r# -c# -g# [other options]",
    "lifesrc -l[n] file -v# -o# file -d# file",
    "",
    "   -r   Number of rows",
    "   -c   Number of columns",
    "   -g   Number of generations",
    "   -tr  Translate rows between last and first generation",
    "   -tc  Translate columns between last and first generation",
    "   -fr  Flip rows between last and first generation",
    "   -fc  Flip columns between last and first generation",
    "   -ff  Flip forward diagonals (/) between last and first generation",
    "   -fb  Flip backward diagonals (\\) between last and first generation",
    "   -fq  Flip quadrants between last and first generation",
    "   -sr  Enforce symmetry on rows",
    "   -sc  Enforce symmetry on columns",
    "   -sp  Enforce symmetry around central point",
    "   -sf  Enforce symmetry on forward diagonal",
    "   -sb  Enforce symmetry on backward diagonal",
    "   -nc  Near N cells of live cells in previous columns for generation 0",
    "   -wc  Maximum width of live cells in each column for generation 0",
    "   -mt  Maximum total live cells for generation 0",
    "   -mc  Maximum live cells in any column for generation 0",
    "   -ur  Force using at least one ON cell in the given row for generation 0",
    "   -uc  Force using at least one ON cell in the given column for generation 0",
    "   -f   First follow the average location of the previous column's cells",
    "   -fg  First follow settings of previous or next generation",
    "   -fo  First choice for unknown cell should be ON instead of OFF",
    "   -ow  Set search order to find wide objects first",
    "   -og  Set search order to examine all gens in a column before next column",
    "   -om  Set search order to examine from middle column outwards",
    "   -or  Set search order to examine from top to bottom",
    "   -oc  Set search order to examine from left to right",
    "   -of  Set search order to examine from top left forward diagonal",
    "   -ob  Set search order to examine from top right backward diagonal",
    "   -oO  Set search order to examine outwards from the center",
    "   -p   Only look for parents of last generation",
    "   -a   Find all objects (even those with subPeriods)",
    "   -v   View object every N million searches",
    "   -d   Dump status to file every N million searches",
    "   -l   Load status from file",
    "   -ln  Load status without entering command mode",
    "   -b   Batch. Don't enter command mode",
    "   -i   Read initial object setting both ON and OFF cells",
    "   -in  Read initial object from file setting only ON cells",
    "   -id  Read initial object setting OFF cells deeply (all gens)",
    "   -o   Output objects to file (appending) every N columns",
    "   -R   Use Life rules specified by born,live values",
    NULL
    };

    fprintf(stderr,
        "Program to search for Life oscillators or spaceships (version %s)\n",
        VERSION);

    for (cpp = text; *cpp; cpp++)
        fprintf(stderr, "%s\n", *cpp);
}

/* END CODE */
