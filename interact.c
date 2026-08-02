/*
 * Life search program - user interactions module.
 * Author: David I. Bell.
 */

#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

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
static char initFile[256] = {0};   /* file containing initial cells */
static char loadFile[256] = {0};   /* file to load state from */
static char outputFile[256] = {0}; /* file to output results to */
static char dumpFile[256] = {0};   /* dump file name */
static Bool blockOutput;     /* print Unicode blocks instead of character */
static Bool RLEOutput;       /* print additional RLE code */
static Bool augmentOutput;   /* print additional UTF8 code for stateList info */
static Bool sortOrderOutput; /* print sort order in ASCII form */
static Bool setDeep;         /* set cleared cells deeply from init file */
static time_t startTime;
static char timeBuf[256] = {0};
static int  dumpFreq;        /* how often to perform dumps in seconds */
static int  viewFreq;        /* how often to view results in seconds */
static int  edgeDiagOffset;  /* turn lower-right and upper-left corner triangles with n cell-bases as OFF cells. A negative number select lower-left and upper-right triangles */

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
static Bool initEdgeCells(const int);

static Status (*pSearch)(const Bool);
static Bool (*pProceed)(Cell *, State, Bool);
static Cell * (*pBackup)(void);
static Bool (*pSetCell)(Cell * const , const State, const Bool);


/*
 * Switch to KS functions if smartON is set
 */
void setKSInterface(const int smart)
{
    if (smart)
    {
        pProceed = &proceedKS;
        pBackup = &backupKS;
        pSearch = &searchKS;
        pSetCell = &setCellKS;
    }
}

/*
 * Signal handler for output
 */
void alarm_handler(const int signo)
{
    if (signo == SIGUSR1)
    {
        viewFlag = TRUE;
    }
    if (signo == SIGUSR2)
    {
        dumpFlag = TRUE;
    }
    if (signo == SIGTERM)
    {
        dumpFlag = TRUE;
        termFlag = TRUE;
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
    &chooseUnknown, &sortOrder, &smartOn, NULL
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
    struct sigaction actDump, actView, actTerm;
    struct sigevent sevDump, sevView;
    struct itimerspec itsDump, itsView;
    timer_t tidDump, tidView;

    time_t end;
    long dif = 0;
    int opt;

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

    pProceed = &proceed;
    pBackup = &backup;
    pSearch = &search;
    pSetCell = &setCell;

    setSigaction(&actView, SIGUSR1, &alarm_handler);

    if (!setRules("3/23"))
    {
        fatal("Cannot set Life rules!");
    }
    /*
     * Check for any arguments
     */
    if (argc < 2)
    {
        usage();
        fatal("\nNo arguments provided.");
    }
    /*
     * echo the command line
     */
    ttyPrintf("Command line: \n");
    for (int i = 1; i < argc; i++) {
        ttyPrintf("%s ", argv[i]);
    }
    ttyPrintf("\n");

    /*
     * Collect the command line options.
     */
    while ((opt = getopt(argc, argv,
        "abc:d:De:f:g:h:H:i:I:j:l:m:o:O:pqr:R:s:S:t:u:v:V:w:")) != -1)
    {
        switch (opt)
        {
            case 'a':
                allObjects = TRUE; /* Find all objects. */
                break;
            case 'b':
                noWait = TRUE; /* Don't enter command mode. */
                break;
            case 'c':
                colMax = atoi(optarg); /* Set number of columns. */
                break;
            case 'd':
                /* set dump frequency and dumpfile */
                void * dummy = &dumpFile;
                int retv = sscanf(optarg, "%d:%s", &dumpFreq, (char *)dummy);
                if (retv == 1)
                {
                    strncpy(dummy, DUMP_FILE, strlen(DUMP_FILE) + 1);
                }
                else if (retv != 2)
                {
                    fatal("incorrect argument for dump option");
                }
                break;
            case 'D':
                debug = TRUE; /* Turn on debugging output. */
                break;
            case 'e':
                switch (*optarg)
                {
                    case 'r':
                        rowSym = 1; /* Enforce row symmetry. */
                        break;
                    case 'c':
                        colSym = 1; /* Enforce column symmetry. */
                        break;
                    case 'p':
                        pointSym = TRUE; /* Enforce point symmetry. */
                        break;
                    case 'q':
                        quadSym = TRUE; /* Enforce (rotation) quadrant symmetry */
                        break;
                    case 'f':
                        fwdSym = TRUE; /* Enforce forward diagonal symmetry */
                        break;
                    case 'b':
                        bwdSym = TRUE; /* Enforce backward diagonal symmetry */
                        break;
                    default:
                        fatal("Bad symmetry");
                }
                break;
            case 'f':
                switch (*optarg)
                {
                    case 'r':
                        flipRows = 1; /* Flip cells along central row. */
                        break;
                    case 'c':
                        flipCols = 1; /* Flip cells along central column. */
                        break;
                    case 'f':
                        flipFwd = TRUE; /* Flip cells along forward diagonal. */
                        break;
                    case 'b':
                        flipBwd = TRUE; /* Flip cells along backward diagonal. */
                        break;
                    case 'p':
                        flipPoint = TRUE; /* Flip cells around central point. */
                        break;
                    case 'q':
                        flipQuads = TRUE; /* Rotate cells 90 degrees around central point. */
                        break;
                    default:
                        fatal("Bad flip");
                }
                break;
            case 'g':
                genMax = atoi(optarg); /* Set number of generations. */
                break;
            case 'H':
                /* Set offset for clearing the triangular areas. */
                edgeDiagOffset = -1;
            case 'h':
                if (edgeDiagOffset == -1)
                {
                    edgeDiagOffset = -atoi(optarg); /* Clears NE-SW corner. */ 
                }
                else
                {
                    edgeDiagOffset = atoi(optarg); /* Clears NW-SE corner. */
                }
                break;
            case 'j':
                setDeep = TRUE;
            case 'i':
                setAll = TRUE;
            case 'I':
                if (optarg)
                {
                    strncpy(initFile, optarg, 64); /* set initial pattern filename */
                }
                else
                {
                    fatal("Missing initial file name");
                }
                break;
            case 'l':
                if (optarg)
                {
                    strncpy(loadFile, optarg, 64); /*set state load filename */
                }
                else
                {
                    fatal("Missing load file name");
                }
                break;
            case 'o':
                if (optarg)
                {
                    strncpy(outputFile, optarg, 64); /* set pattern output filename */
                }
                else
                {
                    fatal("Missing output file name");
                }
                break;
            case 'p':
                parent = TRUE; /* Find parents only. */
                break;
            case 'q':
                quiet = TRUE; /* Don't output. */
                break;
            case 'r':
                rowMax = atoi(optarg); /* Set number of rows. */
                break;
            case 'R':
                /* Set life rules. */
                if (!setRules(optarg))
                {
                    fatal("Bad rule string");
                }
                break;
            case 'S':
                oldSortOrder = TRUE;
            case 's':
                char * sopt = optarg;
                while (*sopt)
                {
                    switch (*sopt++)
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
            case 't':
                int shift;
                char topt;
                /* set translation direction and shift. */
                int rett = sscanf(optarg, "%d:%c", &shift, &topt);
                if (rett == 2)
                {
                    switch (topt)
                    {
                        case 'r': /* display in LIF format. */
                            rowTrans = shift; /* Translate # of rows. */
                            break;
                        case 'c':
                            colTrans = shift; /* Translate # of columns. */
                            break;
                        default:
                            fatal("Bad translate");
                    }
                }
                else
                {
                    fatal("Bad translate direction or shift");
                }
                break;
            case 'u':
                 /* Set choice strategy for getNormalUnknown search method. */
                switch (*optarg)
                {
                    case 'g':
                        followGens = TRUE; /* Choose the same as following or preceding generation of the cell. */
                        break;
                    case 'o':
                        chooseUnknown = ON; /* Choose ON for unknown cells. */
                        break;
                    case 'd': /* (default) Choose OFF for unknown cells. */
                    default:
                        chooseUnknown = OFF;
                        followGens = FALSE;
                        break;
                }
                break;
            case 'V':
                RLEOutput = TRUE;
            case 'v':
                char vopt;
                /* set view frequency. */
                int ret = sscanf(optarg, "%d:%c", &viewFreq, &vopt);
                if (ret == 2)
                {
                    switch (vopt)
                    {
                        case 'l': /* display in LIF format. */
                            break;
                        case 'L':
                            augmentOutput = TRUE;
                            break;
                        case 'b': /* display in UTF8 block format. */
                            blockOutput = TRUE;
                            break;
                        case 'B':
                            blockOutput = TRUE;
                            augmentOutput = TRUE;
                            break;
                        case 's': /* display sortorder of cells. */
                            sortOrderOutput = TRUE;
                            break;
                        default:
                            fatal("Bad view option.");
                    }
                }
                else if (ret == 1)
                {
                    break;
                }
                else
                {
                    fatal("Bad frequency.");
                }
                break;
            case 'w':
                switch (*optarg)
                {
                    case 's': /* KS smartUnknown method from WinLifeSearch. */
                        smartOn += 1;
                        smartWindow = 50;
                    case 'n': /* KS normalUnknown method from WinLifeSearch. */
                        smartOn += 1;
                        break;
                    case 'd': /* (default) DB/JS normalUnknown search. */
                    default:
                        break;
                }
                break;
            default: /* ? */
                ttyClose();
                fprintf(stderr, "Unknown option -%c\n", opt);
                usage();
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
     * make sure that function pointer are pointing at KS functions
     * if smartON is set
     */
    setKSInterface(smartOn);

    /*
     * Check for loading state from file or reading initial
     * object from file.
     */
    if (strlen(loadFile))
    {
        if (loadState(loadFile) != OK)
        {
            ttyClose();
            exit(1);
        }
    }
    else
    {
        initCells(edgeDiagOffset);

        if (strlen(initFile))
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
                initEdgeCells(edgeDiagOffset);
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
            if (sortOrderOutput)
            {
                printOrder(0, augmentOutput);
            }
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
    createTimer(&sevView, &itsView, &tidView, SIGUSR1, viewFreq);
    if (timer_settime(tidView, 0, &itsView, NULL) == -1)
    {
        perror("timer_settime View failed");
        exit(EXIT_FAILURE);
    }

    if (dumpFreq)
    {
        /*
         * Set the dump-to-file interrupt handlers
         */
        setSigaction(&actDump, SIGUSR2, &alarm_handler);
        setSigaction(&actTerm, SIGTERM, &alarm_handler);

        createTimer(&sevDump, &itsDump, &tidDump, SIGUSR2, dumpFreq);
        if (timer_settime(tidDump, 0, &itsDump, NULL) == -1)
        {
            perror("timer_settime Dump failed");
            exit(EXIT_FAILURE);
        }
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
            if (strlen(dumpFile))
            {
                dumpState();
            }
            else
            {
                fatal("no dumpfile specified.");
            }
        }

        quitOk = (curStatus == NOT_EXIST);

        curGen = 0;

        if (strlen(outputFile) == 0)
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


Bool initEdgeCells(const int offset)
{
    for (int col = 1; col <= colMax; col++)
    {
        for (int row = 1; row <= rowMax; row++)
        {
            if (isEdge(row, col, offset))
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
                cp = getStr(cp, "Dump state to file: ");
                strncpy(dumpFile, cp, 255);
                dumpState();
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

    if (strlen(outputFile))
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
void dumpState(void)
{
    FILE * fp;
    Cell ** set;
    const Cell * cell;
    int row;
    int col;
    int gen;
    int ** param;

    if (!strlen(dumpFile))
        return;

    fp = fopen(dumpFile, "w");

    if (fp == NULL)
    {
        ttyStatus("Cannot create \"%s\"\n", dumpFile);

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
        ttyStatus("Error writing \"%s\"\n", dumpFile);

        return;
    }

    ttyStatus("State dumped to \"%s\"\n", dumpFile);
    quitOk = TRUE;

    /* abort program if SIGTERM is set */
    if (termFlag)
    {
        exit(0);
    }
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
     * make sure that function pointer are pointing at KS functions
     * if smartON is set
     */
    setKSInterface(smartOn);

    /*
     * Initialize the cells.
     */
    initCells(edgeDiagOffset);

    if (smartOn)
    {
        initEdgeCells(edgeDiagOffset);
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
    "lifesrc -l file -v#:l -o file -d#:file",
    "",
    "   -r n Number of rows",
    "   -c n Number of columns",
    "   -g n Number of generations",
    "   -p   Only look for parents of last generation",
    "   -h n Make the search area hexagonal by clearing the NW-SE corner"
    "        triangles (|/ /|) from the search space by setting cells to OFF",
    "        The triangles have n cells as base.",
    "   -H n Same as -h but clears the NE-SW corner triangles (|\\ \\|).",
    "",
    "   -t n:r  Translate n rows between last and first generation",
    "   -t n:c  Translate n columns between last and first generation",
    "",
    "   -fr  Flip rows between last and first generation",
    "   -fc  Flip columns between last and first generation",
    "   -ff  Flip forward diagonals (/) between last and first generation",
    "   -fb  Flip backward diagonals (\\) between last and first generation",
    "   -fq  Flip quadrants (+) between last and first generation",
    "",
    "   -er  Enforce symmetry on rows",
    "   -ec  Enforce symmetry on columns",
    "   -ep  Enforce point symmetry around center",
    "   -ef  Enforce symmetry on forward diagonal",
    "   -eb  Enforce symmetry on backward diagonal",
    "",
    "   -uf  (default) Unknown cells first choice is OFF",
    "   -uo  Unknown cells first choice is ON",
    "   -ug  Unknown cells first choice follows previous or next generation",
    "",
    "   -wd  (default) Select DB/JS getNormalUnknown search method",
    "   -wn  Select WinLifeSearch's getNormalUnknown search method",
    "   -ws  Select WinLifeSearch's KS getSmartUnknown search method",
    "",
    "   -sg  Set search order from lowest generation to highest",
    "   -Sg  Like -og but select old search order code",
    "   -sw  Set search order to find wide objects first",
    "   -sm  Set search order from middle column outwards",
    "   -sr  Set search order from top to bottom",
    "   -sc  Set search order from left to right",
    "   -sf  Set search order from top left to down right",
    "   -sb  Set search order from top right to down left",
    "   -sO  Set search order circular outwards from the center",
    "   -si  Invert search order",
    "",
    "   -a   Find all objects (even those with subPeriods)",
    "   -b   Batch. Don't enter command mode",
    "   -D   Enter debug mode if the code is compiled with -DDEBUGFLAG",
    "   -R   Use Life rules specified by born,live values",
    "",
    "   -v n   View object every n seconds",
    "   -V n   Also prints rle of object",
    "   -v n:l (default) Shows objects in expanded lif format",
    "   -v n:b Shows object in UTF8 block characters",
    "   -v n:L/B Same as the lowercase options, but adds additional information",
    "            in lif and block output:",
    "            Symmetry is show in bright and dark",
    "            First cell to be searched is highlighted",
    "",
    "   -l   file  Load status from file",
    "   -i   file  Read initial object setting both ON and OFF cells",
    "   -I   file  Like -i, but only sets ON cells",
    "   -j   file  Read initial object setting OFF cells deeply (all gens)",
    "   -d n:file  Dump status to file every n seconds",
    "   -o   file  Output objects to file (appending mode)",
    "   -q   Don't show screen output",
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
