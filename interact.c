/*
 * Life search program - user interactions module.
 * Author: David I. Bell.
 */

#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "lifesrc.h"
#include "state.h"
#include "setstate.h"
#include "printasc.h"
#include "printblk.h"
#include "printrle.h"
#include "sortorder.h"
#include "sectohms.h"
#include "outputtimers.h"
#include "loopcells.h"
#include "subperiods.h"
#include "findcell.h"
#include "tty.h"
#include "isedge.h"

#define VERSION "4.0"


/*
 * Local data.
 */
static Bool noWait;          /* don't wait for commands after loading */
static Bool setAll;          /* set all cells from initial file */
static Bool isLife;          /* whether the rules are for standard Life */
static char ruleString[20];  /* rule string for printouts */
static long foundCount;      /* number of objects found */
static char * initFile;      /* file containing initial cells */
static char * loadFile;      /* file to load state from */
static Bool blockOutput;     /* print Unicode blocks instead of character */
static Bool RLEOutput;       /* print additional RLE code */
static Bool augmentOutput;   /* print additional UTF8 code for stateList info */
static Bool setDeep;         /* set cleared cells deeply from init file */
static time_t startTime;
static char timeBuf[256] = {0};
static char * argstr;
static int  dumpFreq;        /* how often to perform dumps in seconds */
static int  viewFreq;        /* how often to view results in seconds */


/*
 * Local procedures
 */
static void usage(void);
static void getSetting(const char *);
static void getClear(const char *);
static void getExclude(const char *);
static void getFreeze(const char *);
static void excludeCone(int, int, int);
static void freezeCell(int, int);
static Status loadState(const char *);
static Status readFile(const char *);
static Bool confirm(const char *);
static Bool setRules(const char *);
static long getNum(const char **, int);
static const char * getStr(const char *, const char *);
static void writeGen(const char *, Bool);
static Bool initEdgeCells(void);

static Status (*pSearch)(const Bool);
static Bool (*pProceed)(Cell *, State, Bool);
static Cell * (*pBackup)(void);
static Bool (*pSetCell)(Cell * const , const State, const Bool);


/*
 * Signal handler for output
 */
void alarm_handler(const int signo)
{
    if (signo == SIGUSR1)
    {
        dumpFlag = TRUE;
    }
    if (signo == SIGUSR2)
    {
        viewFlag = TRUE;
    }
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
    &curStatus,
    &rowMax, &colMax, &genMax, &edgeDiagOffset, &rowTrans, &colTrans,
    &rowSym, &colSym, &pointSym, &quadSym, &fwdSym, &bwdSym,
    &flipRows, &flipCols, &flipFwd, &flipBwd, &flipPoint, &flipQuads,
    &parent, &allObjects,
    &orderWide, &orderGens, &orderInvert, &orderMiddle, &followGens,
    &chooseUnknown, &sortOrder, NULL
};


// copy my format to dbells format...
// ... and make a backup of the current state (KAS)
Bool set_initial_cells(void)
{
    Cell * cell;
    Cell ** setpos;
    Bool change;
    int i,j,g;

    newSet = setTable;
    nextSet = setTable;

    // now let's try all UNK cells for ON and OFF state
    // set those which allow only one

    setpos = newSet;
    do {
        change = FALSE;
        for (g = 0; g < genMax; g++)
        {
            for (i = 0; i < colMax; i++)
            {
                for (j = 0; j < rowMax; j++)
                {
                    cell = findCell(j + 1, i + 1, g);
                    if (cell->active && (cell->state == UNK))
                    {
                        if (pProceed(cell, OFF, TRUE))
                        {
                            pBackup();
                            if (pProceed(cell, ON, TRUE))
                            {
                                pBackup();
                            } else {
                                // OFF possible, ON impossible
                                if (setpos != newSet)
                                {
                                    pBackup();
                                }
                                if (pProceed(cell, OFF, TRUE))
                                {
                                    change = TRUE;
                                } else {
                                    // we should never get here
                                    // because it's already tested that the OFF state is possible
                                    printf("Program inconsistency found\n");

                                    return FALSE;
                                }
                            }
                        } else {
                            // can't set OFF state
                            // let's try ON state
                            if (setpos != newSet)
                            {
                                pBackup();
                            }
                            if (pProceed(cell, ON, TRUE))
                            {
                                change = TRUE;
                            } else {
                                // can't set neither ON nor OFF state
                                printf("Inconsistent UNK state for cell (col %d,row %d,gen %d)\n", i + 1, j + 1, g);

                                return FALSE;
                            }
                        }
                    }
                }
            }
        }
    } while (change);

    newSet = setTable;
    nextSet = setTable;

    return TRUE;
}


int main(int argc, char ** argv)
{
    struct sigaction actDump, actView;
    struct sigevent sevDump, sevView;
    struct itimerspec itsDump, itsView;
    timer_t tidDump, tidView;

    time_t end;
    long dif = 0;
    const char * str;

    size_t asize = 1;

    /*
     * Set a couple of defaults.
     */
    viewFreq = 10;
    dumpFreq = 0;
    colMax = 75;
    edgeDiagOffset = 0;
    smartOn = 0;
    smartWindow = 50;
    smartThreshold = 4;

    pProceed = &Proceed;
    pBackup = &Backup;
    pSearch = &Search;
    pSetCell = &setCell;

    setSigaction(&actDump, SIGUSR1, &alarm_handler);
    setSigaction(&actView, SIGUSR2, &alarm_handler);

    /*
     * echo the command line, before the program alters argc
     */
    for (int i = 1; i < argc; i++) {
        asize += strlen(argv[i]) + 1;
    }
    argstr = (char *) malloc(asize);
    if (!argstr) {
        fatal("No memory");
    }
    asize = 0;
    for (int i = 1; i < argc; i++) {
        asize += sprintf(argstr + asize, "%s ", argv[i]);
        assert(argstr[asize] == '\0');
    }
    if (asize > 0)
    {
        argstr[--asize] = '\0';
    }

    ttyPrintf("Command line: \n");
    ttyPrintf("%s\n", argstr);

    if (--argc <= 0)
    {
        usage();
        exit(1);
    }

    argv++;

    if (!setRules("3/23"))
    {
        fatal("Cannot set Life rules!");
    }

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
                rowMax = atoi(str);
                break;

            case 'c':
                /*
                 * Set number of columns.
                 */
                colMax = atoi(str);
                break;

            case 'g':
                /*
                 * Set number of generations.
                 */
                genMax = atoi(str);
                break;

            case 'e':
                /*
                 * Set offset for diagonal areas.
                 */
                edgeDiagOffset = atoi(str);
                break;

            case 't':
                /*
                 * Set row or column translations.
                 */
                switch (*str++)
                {
                    case 'r':
                        rowTrans = atoi(str);
                        break;

                    case 'c':
                        colTrans = atoi(str);
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
                        flipRows = 1;
                        break;

                    case 'c':
                        flipCols = 1;
                        break;

                    case 'f':
                        flipFwd = TRUE;
                        break;

                    case 'b':
                        flipBwd = TRUE;
                        break;

                    case 'p':
                        flipPoint = TRUE;
                        break;

                    case 'q':
                        flipQuads = TRUE;
                        break;

                    default:
                        fatal("Bad flip");
                }
                break;

            case 'u':
                /*
                 * Set choose strategy for getNormalUnknown search method.
                 */
                switch (*str++)
                {
                    case 'g':
                        followGens = TRUE;
                        break;

                    case 'o':
                        chooseUnknown = ON;
                        break;
                }
                break;

            case 's':
                /*
                 * Set symmetry.
                 */
                switch (*str++)
                {
                    case 'r':
                        rowSym = 1;
                        break;

                    case 'c':
                        colSym = 1;
                        break;

                    case 'p':
                        pointSym = TRUE;
                        break;

                    case 'q':
                        quadSym = TRUE;
                        break;

                    case 'f':
                        fwdSym = TRUE;
                        break;

                    case 'b':
                        bwdSym = TRUE;
                        break;

                    default:
                        fatal("Bad symmetry");
                }
                break;

            case 'W':
                /*
                 * Set search method from WinLifeSearch.
                 */
                smartOn += 1;
                smartWindow = 50;
                if (*str)
                {
                    smartThreshold = atoi(str);
                }
            case 'w':
                smartOn += 1;
                pProceed = &proceed;
                pBackup = &backup;
                pSearch = &search;
                pSetCell = &setcell;
                break;

            case 'd':
                /*
                 * Get dump frequency.
                 */
                dumpFreq = atoi(str);
                dumpFile = DUMP_FILE;

                if ((argc > 0) && (**argv != '-'))
                {
                    argc--;
                    dumpFile = *argv++;
                }
                break;

            case 'V':
                blockOutput = TRUE;
            case 'v':
                /*
                 * Set view frequency.
                 */
                while ((*str) && !isdigit(*str))
                {
                    switch (*str++)
                    {
                        case 'a':
                            augmentOutput = TRUE;
                            break;

                        case 'r':
                            RLEOutput = TRUE;
                            break;
                    }
                }

                if (*str)
                {
                    viewFreq = atoi(str);
                }
                break;

            case 'l':
                /*
                 * Load file.
                 */
                if (*str == 'n')
                {
                    noWait = TRUE;
                }

                if ((argc <= 0) || (**argv == '-'))
                {
                    fatal("Missing load file name");
                }

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
                {
                    setAll = TRUE;
                }

                if ((argc <= 0) || (**argv == '-'))
                {
                    fatal("Missing initial file name");
                }

                initFile = *argv++;
                argc--;
                break;

            case 'o':
                /*
                 * Set output file name
                 */
                if ((argc <= 0) || (**argv == '-'))
                {
                    fatal("Missing output file name");
                }

                outputFile = *argv++;
                argc--;
                break;

            case 'M':
                oldSortOrder = TRUE;
            case 'm':
                /*
                 * An ordering option.
                 */
                while (*str)
                {
                    switch (*str++)
                    {
                        case 'w':
                            orderWide = TRUE;
                            break;

                        case 'g':
                            orderGens = TRUE;
                            break;

                        case 'i':
                            orderInvert = TRUE;
                            break;

                        case 'm':
                            orderMiddle = TRUE;
                            break;

                        case 'r':
                            sortOrder = TOPDOWN;
                            break;

                        case 'c':
                            sortOrder = LEFTRIGHT;
                            break;

                        case 'f':
                            sortOrder = DIAG;
                            break;

                        case 'b':
                            sortOrder = BACKDIAG;
                            break;

                        case 'O':
                            sortOrder = CENTEROUT;
                            break;

                        default:
                            fatal("Bad ordering or sorting option");
                    }
                }
                break;

            case 'p':
                /*
                 * Find parents only.
                 */
                parent = TRUE;
                break;

            case 'a':
                /*
                 * Find all objects.
                 */
                allObjects = TRUE;
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
                {
                    fatal("Bad rule string");
                }
                break;

            default:
                ttyClose();
                fprintf(stderr, "Unknown option -%c\n", str[-1]);
                exit(1);
        }
    }

    if (parent && (rowTrans || colTrans || flipPoint || flipQuads ||
        flipRows || flipCols || flipFwd || flipBwd))
    {
        fatal("Cannot specify translations or flips with -p");
    }

    if (pointSym + quadSym + (rowSym || colSym) + (fwdSym || bwdSym) > 1)
    {
        fatal("Conflicting symmetries specified");
    }

    if ((fwdSym || bwdSym || flipFwd || flipBwd || flipQuads) &&
        (rowMax != colMax))
    {
        fatal("Rows must equal cols with -sf, -sb, -ff, -fb or -fq");
    }

    if ((rowTrans || colTrans) + flipPoint + flipQuads > 1)
    {
        fatal("Conflicting translation or flipping specified");
    }

    if ((rowTrans && flipRows) || (colTrans && flipCols))
    {
        fatal("Conflicting translation or flipping specified");
    }

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

            baseSet = nextSet;
        }
        else
        {
            if (edgeDiagOffset)
            {
                initEdgeCells();
            }
        }

        if (smartOn)
        {
            set_initial_cells();

            /*
             * set_initial_cells() cannot be called if the searchlist is not
             * initialised, but then set cells will not be excluded from the
             * searchlist, so let's call initsearchorder() again.
             */
            initSearchOrder();
        }
    }

    /*
     * If we are looking for parents, then set the current generation
     * to the last one so that it can be input easily.  Then get the
     * commands to initialize the cells, unless we were told to not wait.
     */
    if (parent)
    {
        curGen = genMax - 1;
    }

    if (noWait)
    {
        if (!quiet)
        {
            printGen(0);
        }
    }
    else
    {
        getCommands();
    }

    inited = TRUE;

    /*
     * Arm the output timers
     */
    if (dumpFreq)
    {
        createTimer(&sevDump, &itsDump, &tidDump, SIGUSR1, dumpFreq);
        if (timer_settime(tidDump, 0, &itsDump, NULL) == -1)
        {
            perror("timer_settime Dump failed");
            exit(EXIT_FAILURE);
        }
    }

    createTimer(&sevView, &itsView, &tidView, SIGUSR2, viewFreq);
    if (timer_settime(tidView, 0, &itsView, NULL) == -1)
    {
        perror("timer_settime View failed");
        exit(EXIT_FAILURE);
    }

    time(&startTime);

    /*
     * Initial commands are complete, now look for the object.
     */
    while (TRUE)
    {
        if (curStatus == OK)
        {
            curStatus = pSearch(noWait);
            time(&end);
            dif = end - startTime;
            secToHMS(dif, timeBuf);
        }

        if ((curStatus == FOUND) && !allObjects && subPeriods())
        {
            curStatus = OK;
            continue;
        }

        if (dumpFreq)
        {
            dumpState(dumpFile);
        }

        quitOk = (curStatus == NOT_EXIST);

        curGen = 0;

        if (outputFile == NULL)
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
        if (curStatus == FOUND)
        {
            curStatus = OK;

            if (!quiet)
            {
                printGen(0);
                ttyStatus("Object %ld found in%s.\n", ++foundCount, timeBuf);
            }

            writeGen(outputFile, TRUE);
            if (noWait)
            {
                if (allObjects)
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
                outputFile, foundCount, (foundCount == 1) ? "" : "s");
            printf("Total time searched: %s.\n", timeBuf);
        }
        exit(0);
    }
}


Bool initEdgeCells(void)
{
    for (int col = 1; col <= colMax; col++)
    {
        for (int row = 1; row <= rowMax; row++)
        {
            if (isEdge(row, col))
            {
                for (int gen = 0; gen < genMax; gen++)
                {
                    if (!pProceed(findCell(row, col, gen), OFF, FALSE))
                    {
                        ttyStatus("Inconsistent state for cell %d %d\n", row, col);

                        return FALSE;
                    }
                }
            }
        }
    }

    return TRUE;
}


/*
 * Get one or more user commands.
 * Commands are ended by a blank line.
 */
void getCommands(void)
{
    const char * cp;
    const char * cmd;
    char buf[LINE_SIZE];

    printGen(curGen);

    while (TRUE)
    {
        if (!ttyRead("> ", buf, LINE_SIZE))
        {
            ttyClose();
            exit(0);
        }

        cp = buf;

        while (isblank(*cp))
        {
            cp++;
        }

        cmd = cp;

        if (*cp)
        {
            cp++;
        }

        while (isblank(*cp))
        {
            cp++;
        }

        switch (*cmd)
        {
            case 'p':
                /*
                 * Print previous generation.
                  */
                printGen((curGen + genMax - 1) % genMax);
                break;

            case 'n':
                /*
                 * Print next generation.
                 */
                printGen((curGen + 1) % genMax);
                break;

            case 's':
                /*
                 * Add a cell setting.
                 */
                getSetting(cp);
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
                viewFreq = atoi(cp);
                printGen(curGen);
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
                if (curStatus == FOUND)
                {
                    curStatus = OK;
                }

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
static void getSetting(const char * cp)
{
    int row;
    int col;
    State state;

    cp = getStr(cp, "Cell to set (row col [state]): ");

    if (*cp == '\0')
        return;


    row = getNum(&cp, -1);

    if (*cp == ',')
    {
        cp++;
    }

    col = getNum(&cp, -1);

    if (*cp == ',')
    {
        cp++;
    }

    state = getNum(&cp, 1);

    while (isblank(*cp))
    {
        cp++;
    }

    if (*cp != '\0')
    {
        ttyStatus("Bad input line format\n");

        return;
    }

    if ((row <= 0) || (row > rowMax) || (col <= 0) || (col > colMax) ||
        ((state != 0) && (state != 1)))
    {
        ttyStatus("Illegal cell value\n");

        return;
    }

    if (!pProceed(findCell(row, col, curGen), state, FALSE))
    {
        ttyStatus("Inconsistent state for cell\n");

        return;
    }

    baseSet = nextSet;
    printGen(curGen);
}


/*
 * Clear all remaining unknown cells in the current generation or all
 * generations, or else just the specified rectangular area.  If
 * clearing the whole area, then confirmation is required.
 */
static void getClear(const char * cp)
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
    Cell * cell;

    /*
     * Assume we are doing just this generation, but if the 'cg'
     * command was given, then clear in all generations.
     */
    beggen = curGen;
    endGen = curGen;

    if (*cp == 'g')
    {
        cp++;
        beggen = 0;
        endGen = genMax - 1;
    }

    while (isblank(*cp))
    {
        cp++;
    }

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
        endRow = rowMax;
        endCol = colMax;
    }

    if ((begRow < 1) || (begRow > endRow) || (endRow > rowMax) ||
        (begCol < 1) || (begCol > endCol) || (endCol > colMax))
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

                if (!pProceed(cell, OFF, FALSE))
                {
                    ttyStatus("Inconsistent state for cell\n");

                    return;
                }
            }
        }
    }

    baseSet = nextSet;
    printGen(curGen);
}


/*
 * Exclude cells in a rectangular area from searching.
 * This simply means that such cells will not be selected for setting.
 */
static void getExclude(const char * cp)
{
    int begRow;
    int begCol;
    int endRow;
    int endCol;
    int row;
    int col;

    while (isblank(*cp))
    {
        cp++;
    }

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
    {
        cp++;
    }

    if (*cp)
    {
        endRow = getNum(&cp, -1);
        endCol = getNum(&cp, -1);
    }

    if ((begRow < 1) || (begRow > endRow) || (endRow > rowMax) ||
        (begCol < 1) || (begCol > endCol) || (endCol > colMax))
    {
        ttyStatus("Illegal exclusion coordinates");

        return;
    }

    for (row = begRow; row <= endRow; row++)
    {
        for (col = begCol; col <= endCol; col++)
        {
            excludeCone(row, col, curGen);
        }
    }

    printGen(curGen);
}


/*
 * Exclude all cells within the previous light cone centered at the
 * specified cell from searching.
 */
static void excludeCone(const int row, const int col, const int gen)
{
    int tGen;
    int tRow;
    int tCol;
    int dist;
    Cell * cell;

    for (tGen = genMax; tGen >= gen; tGen--)
    {
        dist = tGen - gen;

        for (tRow = row - dist; tRow <= row + dist; tRow++)
        {
            for (tCol = col - dist; tCol <= col + dist; tCol++)
            {
                cell = findCell(tRow, tCol, tGen);
                cell->choose = FALSE;
            }
        }
    }
}


/*
 * Freeze cells in a rectangular area so that their states in all
 * generations are the same.
 */
static void getFreeze(const char * cp)
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

    if ((begRow < 1) || (begRow > endRow) || (endRow > rowMax) ||
        (begCol < 1) || (begCol > endCol) || (endCol > colMax))
    {
        ttyStatus("Illegal freeze coordinates");

        return;
    }

    for (row = begRow; row <= endRow; row++)
    {
        for (col = begCol; col <= endCol; col++)
        {
            freezeCell(row, col);
        }
    }

    printGen(curGen);
}


/*
 * Freeze all generations of the specified cell.
 * A frozen cell can be ON or OFF, but must be the same in all generations.
 * This routine marks them as frozen, and also inserts all the cells of
 * the generation into the same loop so that they will be forced
 * to have the same state.
 */
void freezeCell(const int row, const int col)
{
    int gen;
    Cell * cell0;
    Cell * cell;

    cell0 = findCell(row, col, 0);

    for (gen = 0; gen < genMax; gen++)
    {
        cell = findCell(row, col, gen);
        cell->frozen = TRUE;
        loopCells(smartOn, cell0, cell);
    }
}


/*
 * Print out the current status of the specified generation.
 * This also sets the current generation.
 */
void printGen(int gen)
{
    int row;
    int col;
    int count = 0, unkCount = 0;
    const Cell * cell;
    const char * msg;
    time_t mark;
    long dif;

    curGen = gen;

    switch (curStatus)
    {
        case NOT_EXIST:
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

    for (row = 1; row <= rowMax; row++)
    {
        for (col = 1; col <= colMax; col++)
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
        if (curStatus == FOUND)
        {
            ttyPrintf("%s%s (gen %d, cells %d unk %d confl %ld count %lld)",
                msg, timeBuf, gen, count, unkCount, stepConfl, viewCount);
        }
        else
        {
            ttyPrintf("%s (gen %d, cells %d unk %d confl %ld count %lld)",
                msg, gen, count, unkCount, stepConfl, viewCount);
        }
    }
    else
    {
        if (curStatus == FOUND)
        {
            ttyPrintf("%s%s (rule %s, gen %d, cells %d unk %d confl %ld count %lld)",
                msg, timeBuf, ruleString, gen, count, unkCount, stepConfl, viewCount);
        }
        else
        {
            ttyPrintf("%s (rule %s, gen %d, cells %d unk %d confl %ld count %lld)",
                msg, ruleString, gen, count, unkCount, stepConfl, viewCount);
        }
    }

    if (outputFile)
    {
        if (foundCount)
        {
            ttyPrintf(" [%d]", foundCount);
        }
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
void writeGen(const char * file, const Bool append)
{
    FILE * fp;
    const Cell * cell;
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
    {
        fp = fopen(file, append ? "a" : "w");
    }

    if (fp == NULL)
    {
        ttyStatus("Cannot create \"%s\"\n", file);

        return;
    }

    /*
     * First find the minimum bounds on the object.
     */
    minRow = rowMax;
    minCol = colMax;
    maxRow = 1;
    maxCol = 1;

    for (row = 1; row <= rowMax; row++)
    {
        for (col = 1; col <= colMax; col++)
        {
            cell = findCell(row, col, curGen);

            if (cell->state == OFF)
                continue;

            if (row < minRow)
            {
                minRow = row;
            }

            if (row > maxRow)
            {
                maxRow = row;
            }

            if (col < minCol)
            {
                minCol = col;
            }

            if (col > maxCol)
            {
                maxCol = col;
            }
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
    {
        fprintf(fp, "#\n");
    }

    /*
     * Now write out the bounded area.
     */
    for (row = minRow; row <= maxRow; row++)
    {
        for (col = minCol; col <= maxCol; col++)
        {
            cell = findCell(row, col, curGen);
            switch (cell->state)
            {
                case OFF:
                    ch = '.';
                    break;

                case ON:
                    ch = '*';
                    break;

                case UNK:
                    ch = (cell->choose ? '?' : 'X');
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
    {
        fprintf(fp, "\n");
    }

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
void dumpState(const char * file)
{
    FILE * fp;
    Cell ** set;
    const Cell * cell;
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
    fprintf(fp, "V %d\n", DUMP_VERSION);

    /*
     * Dump out the life rule if it is not the normal one.
     */
    if (!isLife)
    {
        fprintf(fp, "R %s\n", ruleString);
    }

    /*
     * Dump out the parameter values.
     */
    fprintf(fp, "P");

    for (param = paramTable; *param; param++)
    {
        fprintf(fp, " %d", **param);
    }

    fprintf(fp, "\n");

    /*
     * Dump out those cells which have a setting.
     */
    set = setTable;

    while (set != nextSet)
    {
        cell = *set++;

        fprintf(fp, "S %d %d %d %d %d\n", cell->row, cell->col,
            cell->gen, cell->state, cell->free ? 1 : 0);
    }

    /*
     * Dump out those cells which are being excluded from the search.
     */
    for (row = 1; row <= rowMax; row++)
    {
        for (col = 1; col < colMax; col++)
        {
            for (gen = 0; gen < genMax; gen++)
            {
                cell = findCell(row, col, gen);
                if (cell->choose)
                    continue;

                fprintf(fp, "X %d %d %d\n", row, col, gen);
            }
        }
    }

    /*
     * Dump out those cells in generation 0 which are frozen.
     * It isn't necessary to remember frozen cells in other
     * generations since they will be copied from generation 0.
     */
    for (row = 1; row <= rowMax; row++)
    {
        for (col = 1; col < colMax; col++)
        {
            cell = findCell(row, col, 0);
            if (cell->frozen)
            {
                fprintf(fp, "F %d %d\n", row, col);
            }
        }
    }

    /*
     * Finish up with the setting offsets and the final line.
     */
    fprintf(fp, "T %ld %ld\n", baseSet - setTable, nextSet - setTable);
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
static Status loadState(const char * file)
{
    FILE * fp;
    const char * cp;
    int row;
    int col;
    int gen;
    int len;
    State state;
    Bool free;
    Cell * cell;
    int ** param;
    char buf[LINE_SIZE];

    file = getStr(file, "Load state from file: ");

    if (*file == '\0')
        return OK;

    fp = fopen(file, "r");

    if (fp == NULL)
    {
        ttyStatus("Cannot open state file \"%s\"\n", file);

        return ERROR;
    }

    buf[0] = '\0';
    fgets(buf, LINE_SIZE, fp);

    if (buf[0] != 'V')
    {
        ttyStatus("Missing version line in file \"%s\"\n", file);
        fclose(fp);

        return ERROR;
    }

    cp = &buf[1];

    if (getNum(&cp, 0) != DUMP_VERSION)
    {
        ttyStatus("Unknown version in state file \"%s\"\n", file);
        fclose(fp);

        return ERROR;
    }

    fgets(buf, LINE_SIZE, fp);

    /*
     * Set the life rules if they were specified.
     * This line is optional.
     */
    if (buf[0] == 'R')
    {
        len = strlen(buf) - 1;

        if (buf[len] == '\n')
        {
            buf[len] = '\0';
        }

        cp = &buf[1];

        while (isblank(*cp))
        {
            cp++;
        }

        if (!setRules(cp))
        {
            ttyStatus("Bad Life rules in state file\n");
            fclose(fp);

            return ERROR;
        }

        fgets(buf, LINE_SIZE, fp);
    }

    /*
     * Load up all of the parameters from the parameter line.
     * If parameters are missing at the end, they are defaulted to zero.
     */
    if (buf[0] != 'P')
    {
        ttyStatus("Missing parameter line in state file\n");
        fclose(fp);

        return ERROR;
    }

    cp = &buf[1];

    for (param = paramTable; *param; param++)
    {
        **param = getNum(&cp, 0);
    }

    /*
     * Initialize the cells.
     */
    initCells();

    if (edgeDiagOffset)
    {
        initEdgeCells();
    }

    /*
     * Handle cells which have been set.
     */
    newSet = setTable;

    for (;;)
    {
        buf[0] = '\0';
        fgets(buf, LINE_SIZE, fp);

        if (buf[0] != 'S')
            break;

        cp = &buf[1];
        row = getNum(&cp, 0);
        col = getNum(&cp, 0);
        gen = getNum(&cp, 0);
        state = getNum(&cp, 0);
        free = getNum(&cp, 0);

        cell = findCell(row, col, gen);

        if (!pSetCell(cell, state, free))
        {
            ttyStatus("Inconsistently setting cell at r%d c%d g%d \n",
                row, col, gen);
            fclose(fp);

            return ERROR;
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
        cell->choose = FALSE;

        buf[0] = '\0';
        fgets(buf, LINE_SIZE, fp);
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
        fgets(buf, LINE_SIZE, fp);
    }

    if (buf[0] != 'T')
    {
        ttyStatus("Missing table line in state file\n");
        fclose(fp);

        return ERROR;
    }

    cp = &buf[1];
    baseSet = &setTable[getNum(&cp, 0)];
    nextSet = &setTable[getNum(&cp, 0)];

    fgets(buf, LINE_SIZE, fp);

    if (buf[0] != 'E')
    {
        ttyStatus("Missing end of file line in state file\n");
        fclose(fp);

        return ERROR;
    }

    if (fclose(fp))
    {
        ttyStatus("Error reading \"%s\"\n", file);

        return ERROR;
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
static Status readFile(const char * file)
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
    State state;
    char buf[LINE_SIZE];

    file = getStr(file, "Read initial object from file: ");

    if (*file == '\0')
        return OK;

    fp = fopen(file, "r");

    if (fp == NULL)
    {
        ttyStatus("Cannot open \"%s\"\n", file);

        return ERROR;
    }

    activeGen = (parent ? (genMax - 1) : 0);
    row = 0;

    while (fgets(buf, LINE_SIZE, fp))
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
            if ((row > rowMax) || (col > colMax))
            {
                if ((ch == '.') || (ch == ' ') || (ch == ':') || (ch == '?'))
                    continue;

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
                        maxGen = genMax;
                    }

                    state = OFF;
                    break;

                case ':':
                    minGen = 0;
                    maxGen = genMax;
                    state = OFF;
                    break;

                case 'O':
                case 'o':
                case '*':
                    state = ON;
                    break;

                default:
                    ttyStatus("Bad file format in line %d\n", row);
                    fclose(fp);

                    return ERROR;
            }

            for (gen = minGen; gen <= maxGen; gen++)
            {
                if (!pProceed(findCell(row, col, gen), state, FALSE))
                {
                    ttyStatus("Inconsistent state for cell %d %d\n", row, col);
                    fclose(fp);

                    return ERROR;
                }
            }
        }
    }

    if (fclose(fp))
    {
        ttyStatus("Error reading \"%s\"\n", file);

        return ERROR;
    }

    return OK;
}


/*
 * Check a string for being NULL, and if so, ask the user to specify a
 * value for it.  Returned string may be static and thus is overwritten
 * for each call.  Leading spaces in the string are skipped over.
 */
static const char * getStr(const char * str, const char * prompt)
{
    static char buf[LINE_SIZE];

    if ((str == NULL) || (*str == '\0'))
    {
        if (!ttyRead(prompt, buf, LINE_SIZE))
        {
            buf[0] = '\0';

            return buf;
        }

        str = buf;
    }

    while (isblank(*str))
    {
        str++;
    }

    return str;
}


/*
 * Confirm an action by prompting with the specified string and reading
 * an answer.  Entering 'y' or 'Y' indicates TRUE, everything else FALSE.
 */
static Bool confirm(const char * prompt)
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
    Bool isNeg;

    isNeg = FALSE;
    cp = *cpp;

    while (isblank(*cp))
    {
        cp++;
    }

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
    {
        num = num * 10 + (*cp++ - '0');
    }

    if (isNeg)
    {
        num = -num;
    }

    while (isblank(*cp))
    {
        cp++;
    }

    *cpp = cp;

    return num;
}


/*
 * Parse a string and set the Life rules from it.
 * Returns TRUE on success, or FALSE on an error.
 * The rules can be "mmm,nnn",  "mmm/nnn", "Bmmm,Snnn", "Bmmm/Snnn",
 * or a hex number in the Wolfram encoding.
 */
static Bool setRules(const char * cp)
{
    char * cpTemp;
    int i;
    unsigned int bits;

    for (i = 0; i < 9; i++)
    {
        bornRules[i] = OFF;
        liveRules[i] = OFF;
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
            {
                bits += *cp - '0';
            }
            else if ((*cp >= 'a') && (*cp <= 'f'))
            {
                bits += *cp - 'a' + 10;
            }
            else if ((*cp >= 'A') && (*cp <= 'F'))
            {
                bits += *cp - 'A' + 10;
            }
            else
                return FALSE;
        }

        if (i & ~0x3ff)
            return FALSE;

        for (i = 0; i < 9; i++)
        {
            if (bits & 0x01)
            {
                bornRules[i] = ON;
            }

            if (bits & 0x02)
            {
                liveRules[i] = ON;
            }

            bits >>= 2;
        }
    }
    else
    {
        /*
         * It is in normal born/survive format.
         */
        if ((*cp == 'b') || (*cp == 'B'))
        {
            cp++;
        }

        while ((*cp >= '0') && (*cp <= '8'))
        {
            bornRules[*cp++ - '0'] = ON;
        }

        if ((*cp != ',') && (*cp != '/'))
            return FALSE;

        cp++;

        if ((*cp == 's') || (*cp == 'S'))
        {
            cp++;
        }

        while ((*cp >= '0') && (*cp <= '8'))
        {
            liveRules[*cp++ - '0'] = ON;
        }

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
        if (bornRules[i] == ON)
        {
            *cpTemp++ = '0' + i;
        }
    }

    *cpTemp++ = '/';
    *cpTemp++ = 'S';

    for (i = 0; i < 9; i++)
    {
        if (liveRules[i] == ON)
        {
            *cpTemp++ = '0' + i;
        }
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
void fatal(const char * msg)
{
    ttyClose();
    fprintf(stderr, "%s\n", msg);
    exit(1);
}


/*
 * Print usage text.
 */
static void usage(void)
{
    const char * const * cpp;

    static const char * const text[] =
    {
    "Original code by David Bell, developed from an algorithm by",
    " Dean Hickerson coded in 6502 assembly.",
    "This code contains smartsearch segments from WinLifeSearch",
    " created by Jason Summers and Karel Suhajda.",
    "Extensively modified by Frank Everdij.",
    "",
    "lifesrc -r# -c# -g# [other options]",
    "lifesrc -l[n] file -v# -o# file -d# file",
    "",
    "   -r n Number of rows",
    "   -c n Number of columns",
    "   -g n Number of generations",
    "   -e n Clear corner triangles from the search space by setting cells to OFF",
    "        The triangles have n cells as base. A positive number clears the",
    "        NW-SE corners (|/ /|), a negative number clears NE-SW (|\\ \\|)",
    "   -tr  Translate rows between last and first generation",
    "   -tc  Translate columns between last and first generation",
    "   -fr  Flip rows between last and first generation",
    "   -fc  Flip columns between last and first generation",
    "   -ff  Flip forward diagonals (/) between last and first generation",
    "   -fb  Flip backward diagonals (\\) between last and first generation",
    "   -fq  Flip quadrants (+) between last and first generation",
    "   -sr  Enforce symmetry on rows",
    "   -sc  Enforce symmetry on columns",
    "   -sp  Enforce point symmetry around center",
    "   -sf  Enforce symmetry on forward diagonal",
    "   -sb  Enforce symmetry on backward diagonal",
    "   -ug  First follow settings of previous or next generation",
    "   -uo  First choice for unknown cell should be ON instead of OFF",
    "   -w   Select WinLifeSearch's getNormalUnknown search method",
    "        (Default is DB/JS getNormalUnknown search method)",
    "   -W   Select WinLifeSearch's getSmartUnknown search method",
    "   -mg  Set search order from lowest generation to highest",
    "   -Mg  Like -og but select old search order code",
    "   -mw  Set search order to find wide objects first",
    "   -mm  Set search order from middle column outwards",
    "   -mr  Set search order from top to bottom",
    "   -mc  Set search order from left to right",
    "   -mf  Set search order from top left to down right",
    "   -mb  Set search order from top right to down left",
    "   -mO  Set search order circular outwards from the center",
    "   -mi  Invert search order",
    "   -p   Only look for parents of last generation",
    "   -a   Find all objects (even those with subPeriods)",
    "   -b   Batch. Don't enter command mode",
    "   -D   Enter debug mode if the code is compiled with -DDEBUGFLAG",
    "   -R   Use Life rules specified by born,live values",
    "   -vn  View object every n seconds using expanded lif format",
    "   -Vn  Like -vn but shows object in UTF8 block characters",
    "   -van Shows additional information in lif and block output:",
    "        Symmetry is show in bright and dark",
    "        First cell to be searched is highlighted",
    "   -vrn Also prints rle of object",
    "   -dn file  Dump status to file every n seconds",
    "   -l  file  Load status from file",
    "   -ln file  Load status from file without entering command mode",
    "   -i  file  Read initial object setting both ON and OFF cells",
    "   -in file  Read initial object from file setting only ON cells",
    "   -id file  Read initial object setting OFF cells deeply (all gens)",
    "   -o  file  Output objects to file (appending mode)",
    NULL
    };

    fprintf(stderr,
        "Program to search for Life oscillators or spaceships (version %s)\n\n",
        VERSION);

    for (cpp = text; *cpp; cpp++)
    {
        fprintf(stderr, "%s\n", *cpp);
    }
}

/* END CODE */
