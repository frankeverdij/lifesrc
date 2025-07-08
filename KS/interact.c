/*
 * Life search program - user interactions module.
 * Author: David I. Bell.
 */

#include "lifesrc.h"
#include "subperiods.h"
#include "loopcells.h"

#define VERSION "3.5"


/*
 * Local data.
 */
static Bool setall;         /* set all cells from initial file */
static Bool setdeep;
static Bool islife;         /* whether the rules are for standard Life */
static char rulestring[20]; /* rule string for printouts */
static int foundcount;      /* number of objects found */
static char * initfile;     /* file containing initial cells */
static char * loadfile;     /* file to load state from */


/*
 * Local procedures
 */
static void usage(void);
static void excludecone(int, int, int);
static void freezecell(int, int);
static Bool loadstate(const char *);
static Status readfile(char *);
static Bool setrules(char *);
static long getnum(char **, int);


/*
 * Table of addresses of parameters which are loaded and saved.
 * Changing this table may invalidate old dump files, unless new
 * parameters are added at the end and default to zero.
 * When changed incompatibly, the dump file version should be incremented.
 * The table is ended with a NULL pointer.
 */
static int * param_table[] =
{
    &curstatus,
    &rowmax, &colmax, &genmax, &rowtrans, &coltrans,
    &rowsym, &colsym, &pointsym, &fwdsym, &bwdsym,
    &fliprows, &flipcols, &flipquads,
    &parent, &allobjects, &nearcols, &maxcount,
    &userow, &usecol, &colcells, &colwidth, &follow,
    &orderwide, &ordergens, &ordermiddle, &followgens,
    &diagsort, &symmetry, &trans_rotate, &trans_flip, &trans_x, &trans_y,
    &knightsort, &chooseUnknown,
    &smart, &smartwindow, &smartthreshold, 
    &foundcount,
    NULL
};

void smartinit(void)
{
    smart = TRUE;
    smartwindow = 50;
    smartthreshold = 4;
    smartstatlen = 0;
    smartstatwnd = 0;
    smartstatsumlen = 0;
    smartstatsumwnd = 0;
    smartstatsumlenc = 0;
    smartstatsumwndc = 0;
    smarton = TRUE;
    combine = FALSE;
    combining = FALSE;
}

long showcount()
{
    static long tot = 0;

    if (viewcount<0) {
        tot=0;
    } else {
        tot += viewcount;
    }
    viewcount = 0;

    return tot;
}

// copy my format to dbells format...
// ... and make a backup of the current state (KAS)
Bool set_initial_cells(void)
{
    Cell * cell;
    Cell ** setpos;
    Bool change;
    int i,j,g;

    newset = settable;
    nextset = settable;

    // now let's try all UNK cells for ON and OFF state
    // set those which allow only one

    setpos = newset;
    do {
        change = FALSE;
        for(g=0;g<genmax;g++)
        {
            for(i=0;i<colmax;i++)
            {
                for(j=0;j<rowmax;j++)
                {
                    cell = findcell(j+1,i+1,g);
                    if (cell->active && (cell->state == UNK))
                    {
                        if (proceed(cell, OFF, TRUE))
                        {
                            backup();
                            if (proceed(cell, ON, TRUE))
                            {
                                backup();
                            } else {
                                // OFF possible, ON impossible
                                if (setpos != newset) backup();
                                if (proceed(cell, OFF, TRUE))
                                {
                                    change = TRUE;
                                } else {
                                    // we should never get here
                                    // because it's already tested that the OFF state is possible
                                    fprintf(stderr, "Program inconsistency found\n");
                                    return FALSE;
                                }
                            }                            
                        } else {
                            // can't set OFF state
                            // let's try ON state
                            if (setpos != newset) backup();
                            if (proceed(cell, ON, TRUE))
                            {
                                change = TRUE;
                            } else {
                                // can't set neither ON nor OFF state
                                printf("Inconsistent UNK state for cell (col %d,row %d,gen %d)\n",i+1,j+1,g);
                                return FALSE;
                            }
                        }
                    }
                }
            }
        }
    } while (change);

    newset = settable;
    nextset = settable;

    return TRUE;
}

int
main(argc, argv)
    int argc;
    char ** argv;
{
    char * str;

    if (--argc <= 0)
    {
        usage();
        exit(1);
    }

    argv++;

    if (!setrules("3/23"))
    {
        fprintf(stderr, "Cannot set Life rules!\n");
        exit(1);
    }

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
            case 'q':
                quiet = TRUE; /* don't output */
                break;

            case 'r':         /* rows */
                rowmax = atoi(str);
                break;

            case 'c':         /* columns */
                colmax = atoi(str);
                break;

            case 'g':         /* generations */
                genmax = atoi(str);
                break;

            case 't':         /* translation */
                switch (*str++)
                {
                    case 'r':
                        rowtrans = atoi(str);
                        break;

                    case 'c':
                        coltrans = atoi(str);
                        break;

                    default:
                        fprintf(stderr, "Bad translate\n");
                        exit(1);
                }

                break;

            case 'f':            /* flip cells */
                switch (*str++)
                {
                    case 'r':
                        fliprows = 1;

                        if (*str)
                            fliprows = atoi(str);

                        break;

                    case 'c':
                        flipcols = 1;

                        if (*str)
                            flipcols = atoi(str);

                        break;

                    case 'q':
                        flipquads = TRUE;
                        break;

                    case 'g':
                        followgens = TRUE;
                        break;

                    case 'o':
                        chooseUnknown = ON;
                        break;

                    case 's':
                        smartinit();
                        break;

                    case '\0':
                        follow = TRUE;
                        break;

                    default:
                        fprintf(stderr, "Bad flip\n");
                        exit(1);
                }

                break;

            case 's':            /* symmetry */
                switch (*str++)
                {
                    case 'r':
                        rowsym = 1;

                        if (*str)
                            rowsym = atoi(str);

                        break;

                    case 'c':
                        colsym = 1;

                        if (*str)
                            colsym = atoi(str);

                        break;

                    case 'p':
                        pointsym = TRUE;
                        break;

                    case 'f':
                        fwdsym = TRUE;
                        break;

                    case 'b':
                        bwdsym = TRUE;
                        break;

                    default:
                        fprintf(stderr, "Bad symmetry\n");
                        exit(1);
                }

                break;

            case 'n':            /* near cells */
                switch (*str++)
                {
                    case 'c':
                        nearcols = atoi(str);
                        break;

                    default:
                        fprintf(stderr, "Bad near\n");
                        exit(1);
                }

                break;

            case 'w':            /* max width */
                switch (*str++)
                {
                    case 'c':
                        colwidth = atoi(str);
                        break;

                    default:
                        fprintf(stderr, "Bad width\n");
                        exit(1);
                }

                break;

            case 'u':            /* use row or column */
                switch (*str++)
                {
                    case 'r':
                        userow = atoi(str);
                        break;

                    case 'c':
                        usecol = atoi(str);
                        break;

                    default:
                        fprintf(stderr, "Bad use\n");
                        exit(1);
                }

                break;

            case 'd':            /* dump frequency */
                dumpfreq = atol(str) * DUMPMULT;
                dumpfile = DUMPFILE;

                if ((argc > 0) && (**argv != '-'))
                {
                    argc--;
                    dumpfile = *argv++;
                }

                break;

            case 'v':            /* view frequency */
                viewfreq = atol(str) * VIEWMULT;
                break;

            case 'l':            /* load file */
                if ((argc <= 0) || (**argv == '-'))
                {
                    fprintf(stderr, "Missing load file name\n");
                    exit(1);
                }

                loadfile = *argv++;
                argc--;
                break;

            case 'i':            /* initial file */
                if (*str == 'd')
                {
                    setall = TRUE;
                    setdeep = TRUE;
                }
                else if (*str != 'n')
                {
                    setall = TRUE;
                }

                if ((argc <= 0) || (**argv == '-'))
                {
                    fprintf(stderr, "Missing initial file name\n");
                    exit(1);
                }

                initfile = *argv++;
                argc--;
                break;

            case 'o':
                if ((*str == '\0') || isdigit(*str))
                {
                    /*
                     * Output file name
                     */
                    outputcols = atol(str);

                    if ((argc <= 0) || (**argv == '-'))
                    {
                        fprintf(stderr,
                        "Missing output file name\n");
                        exit(1);
                    }

                    outputfile = *argv++;
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
                            orderwide = TRUE;
                            break;

                        case 'g':
                            ordergens = TRUE;
                            break;

                        case 'm':
                            ordermiddle = TRUE;
                            break;

                        case 'b':
                            diagsort = TRUE;
                            break;

                        default:
                            fprintf(stderr,
                            "Bad ordering option\n");
                            exit(1);
                    }
                }

                break;

            case 'm':            /* max cell count */
                switch (*str++)
                {
                    case 'c':
                        colcells = atoi(str);
                        break;

                    case 't':
                        maxcount = atoi(str);
                        break;

                    default:
                        fprintf(stderr, "Bad maximum\n");
                        exit(1);
                }

                break;

            case 'p':            /* find parents only */
                parent = TRUE;
                break;

            case 'a':
                allobjects = TRUE;    /* find all objects */
                break;

            case 'D':            /* debugging output */
                debug = TRUE;
                break;

            case 'R':            /* set rules */
                if (!setrules(str))
                {
                    fprintf(stderr, "Bad rule string\n");
                    exit(1);
                }

                break;

            default:
                fprintf(stderr, "Unknown option -%c\n",
                    str[-1]);

                exit(1);
        }
    }

    if (parent && (rowtrans || coltrans || flipquads ||
        fliprows || flipcols))
    {
        fprintf(stderr, "Cannot specify translations or flips with -p\n");
        exit(1);
    }

    if ((pointsym != 0) + (rowsym || colsym) + (fwdsym || bwdsym) > 1)
    {
        fprintf(stderr, "Conflicting symmetries specified\n");
        exit(1);
    }

    if ((fwdsym || bwdsym || flipquads) && (rowmax != colmax))
    {
        fprintf(stderr, "Rows must equal cols with -sf, -sb, or -fq\n");
        exit(1);
    }

    if ((rowtrans || coltrans) + (flipquads != 0) > 1)
    {
        fprintf(stderr, "Conflicting translation or flipping specified\n");
        exit(1);
    }

    if ((rowtrans && fliprows) || (coltrans && flipcols))
    {
        fprintf(stderr, "Conflicting translation or flipping specified\n");
        exit(1);
    }

    if ((userow < 0) || (userow > rowmax))
    {
        fprintf(stderr, "Bad row for -ur\n");
        exit(1);
    }

    if ((usecol < 0) || (usecol > colmax))
    {
        fprintf(stderr, "Bad column for -uc\n");
        exit(1);
    }

    symmetry = 0;
    if (rowsym)
    {
        if (colsym)
        {
            symmetry = 6;
        }
        else
        {
            symmetry = 2;
        }
    }
    else
    {
        if (colsym)
        {
            symmetry = 1;
        }
    }

    if (fwdsym)
    {
        if (bwdsym)
        {
            symmetry = 7;
        }
        else
        {
            symmetry = 3;
        }
    }
    else
    {
        if (bwdsym)
        {
            symmetry = 4;
        }
    }
    
    if (pointsym)
    {
        symmetry = 8;
    }
    
    if (colsym && rowsym && fwdsym && bwdsym)
    {
        symmetry = 9;
    }

    newset = NULL;
    nextset = NULL;
    outputlastcols = 0;
    fullcolumns = 0;
    curstatus = OK;
    g0oncellcount = 0; // KAS
    cellcount = 0; // KAS
    smartchoice = UNK; // KAS

    viewcount = -1; // KAS


    /*
     * Check for loading state from file or reading initial
     * object from file.
     */
    if (loadfile)
    {
        if (loadstate(loadfile) != OK)
        {
            exit(1);
        }
    }
    else
    {
        initcells();

        if (initfile)
        {
            if (readfile(initfile) != OK)
            {
                exit(1);
            }
            //baseset = nextset;
        }

        set_initial_cells();

        /*
         * set_initial_cells() cannot be called if the searchlist is not
         * initialised, but then set cells will not be excluded from the
          * searchlist, so let's call initsearchorder() again.
         */
        initsearchorder();
    }

    /*
     * If we are looking for parents, then set the current generation
     * to the last one so that it can be input easily.  Then get the
     * commands to initialize the cells, unless we were told to not wait.
     */
    if (parent) curgen = genmax - 1;

    printgen(0);

    inited = TRUE;

    /*
     * Initial commands are complete, now look for the object.
     */
    while (TRUE)
    {
        if (curstatus == OK)
            curstatus = search();

        if ((curstatus == FOUND) && !allobjects && subPeriods())
        {
            curstatus = OK;
            continue;
        }

        if (dumpfreq)
        {
            dumpcount = 0;
            dumpstate(dumpfile);
        }

        quitok = (curstatus == NOTEXIST);

        curgen = 0;

        /*
         * Here if results are going to a file.
         */
        if (curstatus == FOUND)
        {
            curstatus = OK;

            if (!quiet)
            {
                printgen(0);
                fprintf(stderr, "Object %d found.\n", ++foundcount);
            }

            writegen(outputfile, TRUE);

            if (allobjects)
            {
                continue;
            }
        }

        if (foundcount == 0)
        {
            fprintf(stderr, "No objects found\n");
            exit(1);
        }


        if (!quiet)
            printf("Search completed, file \"%s\" contains %d object%s\n",
                outputfile, foundcount, (foundcount == 1) ? "" : "s");

        exit(0);
    }
}


/*
 * Exclude all cells within the previous light cone centered at the
 * specified cell from searching.
 */
static void
excludecone(int row, int col, int gen)
{
    int tgen;
    int trow;
    int tcol;
    int dist;

    for (tgen = genmax; tgen >= gen; tgen--)
    {
        dist = tgen - gen;

        for (trow = row - dist; trow <= row + dist; trow++)
        {
            for (tcol = col - dist; tcol <= col + dist; tcol++)
            {
                findcell(trow, tcol, tgen)->choose = FALSE;
            }
        }
    }
}


/*
 * Freeze all generations of the specified cell.
 * A frozen cell can be ON or OFF, but must be the same in all generations.
 * This routine marks them as frozen, and also inserts all the cells of
 * the generation into the same loop so that they will be forced
 * to have the same state.
 */
void
freezecell(row, col)
    int row;
    int col;
{
    int gen;
    Cell * cell0;
    Cell * cell;

    cell0 = findcell(row, col, 0);

    for (gen = 0; gen < genmax; gen++)
    {
        cell = findcell(row, col, gen);

        cell->frozen = TRUE;

        loopcells(cell0, cell);
    }
}


/*
 * Print out the current status of the specified generation.
 * This also sets the current generation.
 */
void
printgen(gen)
    int gen;
{
    int row;
    int col;
    int count;
    Cell * cell;
    char * msg;

    curgen = gen;

    switch (curstatus)
    {
        case NOTEXIST:    msg = "No such object"; break;
        case FOUND:    msg = "Found object"; break;
        default:    msg = ""; break;
    }

    count = 0;

    for (row = 1; row <= rowmax; row++)
    {
        for (col = 1; col <= colmax; col++)
        {
            count += (findcell(row, col, gen)->state == ON);
        }
    }

    if (islife)
    {
        printf("%s (gen %d, cells %d totalcount %ld)", msg, gen, count, showcount());
    }
    else
    {
        printf("%s (rule %s, gen %d, cells %di totalcount %ld)",
            msg, rulestring, gen, count, showcount());
    }

    printf(" -r%d -c%d -g%d", rowmax, colmax, genmax);

    if (rowtrans)
        printf(" -tr%d", rowtrans);

    if (coltrans)
        printf(" -tc%d", coltrans);

    if (fliprows == 1)
        printf(" -fr");

    if (fliprows > 1)
        printf(" -fr%d", fliprows);

    if (flipcols == 1)
        printf(" -fc");

    if (flipcols > 1)
        printf(" -fc%d", flipcols);

    if (flipquads)
        printf(" -fq");

    if (rowsym == 1)
        printf(" -sr");

    if (rowsym > 1)
        printf(" -sr%d", rowsym);

    if (colsym == 1)
        printf(" -sc");

    if (colsym > 1)
        printf(" -sc%d", colsym);

    if (pointsym)
        printf(" -sp");

    if (fwdsym)
        printf(" -sf");

    if (bwdsym)
        printf(" -sb");

    if (ordergens || orderwide || ordermiddle)
    {
        printf(" -o");

        if (ordergens)
            printf("g");

        if (orderwide)
            printf("w");

        if (ordermiddle)
            printf("m");
    }

    if (follow)
        printf(" -f");

    if (followgens)
        printf(" -fg");

    if (parent)
        printf(" -p");

    if (allobjects)
        printf(" -a");

    if (userow)
        printf(" -ur%d", userow);

    if (usecol)
        printf(" -uc%d", usecol);

    if (nearcols)
        printf(" -nc%d", nearcols);

    if (maxcount)
        printf(" -mt%d", maxcount);

    if (colcells)
        printf(" -mc%d", colcells);

    if (colwidth)
        printf(" -wc%d", colwidth);

    if (viewfreq)
        printf(" -v%ld", viewfreq / VIEWMULT);

    if (dumpfreq)
        printf(" -d%ld %s", dumpfreq / DUMPMULT, dumpfile);

    if (outputfile)
    {
        if (outputcols)
            printf(" -o%d %s", outputcols, outputfile);
        else
            printf(" -o %s", outputfile);

        if (foundcount)
            printf(" [%d]", foundcount);
    }

    printf("\n");

    for (row = 1; row <= rowmax; row++)
    {
        for (col = 1; col <= colmax; col++)
        {
            cell = findcell(row, col, gen);

            switch (cell->state)
            {
                case OFF:
                    msg = ".";
                    break;

                case ON:
                    msg = "O";
                    break;

                case UNK:
                    msg = "?";

                    if (cell->frozen)
                        msg = "+";

                    if (!cell->choose)
                        msg = "X";

                    break;
            }

            /*
             * If wide output, print only one character,
             * else print both characters.
             */
            printf("%s", msg);
            if (colmax < 40) printf(" ");
        }

        printf("\n");
    }

}


/*
 * Write the current generation to the specified file.
 * Empty rows and columns are not written.
 * If no file is specified, it is asked for.
 * Filename of "." means write to stdout.
 */
void
writegen(file, append)
    char * file;        /* file name (or NULL) */
    Bool append;        /* TRUE to append instead of create */
{
    FILE * fp;
    Cell * cell;
    int row;
    int col;
    int ch;
    int minrow, maxrow, mincol, maxcol;

    if (*file == '\0')
        return;

    fp = stdout;

    if (strcmp(file, "."))
        fp = fopen(file, append ? "a" : "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Cannot create \"%s\"\n", file);

        return;
    }

    /*
     * First find the minimum bounds on the object.
     */
    minrow = rowmax;
    mincol = colmax;
    maxrow = 1;
    maxcol = 1;

    for (row = 1; row <= rowmax; row++)
    {
        for (col = 1; col <= colmax; col++)
        {
            cell = findcell(row, col, curgen);

            if (cell->state == OFF)
                continue;

            if (row < minrow)
                minrow = row;

            if (row > maxrow)
                maxrow = row;

            if (col < mincol)
                mincol = col;

            if (col > maxcol)
                maxcol = col;
        }
    }

    if (minrow > maxrow)
    {
        minrow = 1;
        maxrow = 1;
        mincol = 1;
        maxcol = 1;
    }

    if (fp == stdout)
        fprintf(fp, "#\n");

    /*
     * Now write out the bounded area.
     */
    for (row = minrow; row <= maxrow; row++)
    {
        for (col = mincol; col <= maxcol; col++)
        {
            cell = findcell(row, col, curgen);

            switch (cell->state)
            {
                case OFF:    
                    ch = '.'; 
                        break;

                case ON:    
                    ch = '*'; 
                    break;

                case UNK:    
                    ch = cell->choose ? '?' : 'X';
                    break;

                default:
                    fprintf(stderr, "Bad cell state");
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
        fprintf(stderr, "Error writing \"%s\"\n", file);

        return;
    }

    if (fp != stdout)
        fprintf(stderr, "\"%s\" written\n", file);

    quitok = TRUE;
}


/*
 * Dump the current state of the search in the specified file.
 * If no file is specified, it is asked for.
 */
void dumpstate(const char * file)
{
    FILE * fp;
    Cell ** set;
    Cell * cell;
    int ** param;

    if (*file == '\0')
        return;

    fp = fopen(file, "wt");

    if (fp == NULL)
    {
        fprintf(stderr, "Cannot create \"%s\"\n", file);

        return;
    }

    /*
     * Dump out the version so we can detect incompatible formats.
     */
    fprintf(fp, "V %d\n", DUMPVERSION);

    /*
     * Dump out the parameter values.
     */
    fprintf(fp, "P");

    for (param = param_table; *param; param++)
        fprintf(fp, " %d", **param);

    fprintf(fp, "\n");

    /*
     * Dump out the life rule
     */

    fprintf(fp, "R %s\n", rulestring);

    /*
     * Dump out those cells which have a setting.
     */
    set = settable;

    while (set != nextset)
    {
        cell = *set++;

        fprintf(fp, "S %d %d %d %d %d\n", cell->row, cell->col,
            cell->gen, (cell->state == ON) ? 1 : 0, cell->free);
    }

    fprintf(fp, "E\n");

    if (fclose(fp))
    {
        fprintf(stderr, "Error writing \"%s\"\n", file);

        return;
    }

    if (!quiet) fprintf(stderr, "State dumped to \"%s\"\n", file);
}


/*
 * Load a previously dumped state from a file.
 * Warning: Almost no checks are made for validity of the state.
 * Returns OK on success, ERROR on failure.
 */
Bool loadstate(const char * file)
{
    FILE * fp;
    char * cp;
    int row;
    int col;
    int gen;
    State state;
    Bool free;
    Cell * cell;
    int ** param;
    char buf[LINESIZE];
    int ver;

    Status status;

    if(file[0]=='\0') return FALSE;

    fp = fopen(file, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open state file \"%s\"\n", file);

        return FALSE;
    }

//*********************************************
// Read and check the file version
//*********************************************

    buf[0] = '\0';
    fgets(buf, LINESIZE, fp);

    if (buf[0] != 'V')
    {
        fprintf(stderr, "Missing version line in file \"%s\"\n", file);
        fclose(fp);

        return FALSE;
    }

    cp = &buf[1];
    ver = getnum(&cp, 0);

    if (DUMPVERSION != ver)
    {
        fprintf(stderr, "Incorrect version of the dump file: expected %d, found %d", DUMPVERSION, ver);
        fclose(fp);
        return FALSE;
    }

//*********************************************
// Read parameters
//*********************************************

    fgets(buf, LINESIZE, fp);

    /*
     * Load up all of the parameters from the parameter line.
     * If parameters are missing at the end, they are defaulted to zero.
     */
    if (buf[0] != 'P')
    {
        fprintf(stderr, "Missing parameter line in state file\n");
        fclose(fp);

        return FALSE;
    }

    cp = &buf[1];

    for (param = param_table; *param; param++)
        **param = getnum(&cp, 0);

//*********************************************
// Initialise
//*********************************************

    initcells();

//*********************************************
// Read life rule
//*********************************************

    fgets(buf, LINESIZE, fp);

    /*
     * Set the life rules if they were specified.
     * This line is optional.
     */
    if (buf[0] != 'R')
    {
        fprintf(stderr, "Missing rule line in state file\n");
        fclose(fp);

        return FALSE;
    }
    cp = &buf[strlen(buf) - 1];

    if (*cp == '\n') *cp = '\0';

    cp = &buf[1];

    while (isblank(*cp)) cp++;

    if (!setrules(cp))
    {
        fprintf(stderr, "Bad Life rules in state file\n");
        fclose(fp);

        return FALSE;
    }


//*********************************************
// Set the search order
//*********************************************

    initsearchorder();

//*********************************************
// Process cells in the stack
//*********************************************

    for (;;)
    {
        buf[0] = '\0';
        fgets(buf, LINESIZE, fp);

        if (buf[0] != 'S')
            break;

        cp = &buf[1];
        row = getnum(&cp, 0);
        col = getnum(&cp, 0);
        gen = getnum(&cp, 0);
        state = (getnum(&cp, 0) != 0) ? ON : OFF;
        free = getnum(&cp, 0);

        cell = findcell(row, col, gen);

        if (!setcell(cell, state, free))
        {
            fprintf(stderr, 
                "Inconsistently setting cell at r%d c%d g%d \n",
                row, col, gen);

            fclose(fp);

            return FALSE;
        }
    }

//*********************************************
// Check the consistency
//*********************************************

    do {
        status = examinenext();
    } while (status == OK);

    if (status != CONSISTENT) {
        fprintf(stderr, "Inconsistent cell status\n");
        fclose(fp);

        return FALSE;
    }

//*********************************************
// Check the presence of the 'end' line
//*********************************************

    if (buf[0] != 'E')
    {
        fprintf(stderr, "Missing end of file line in state file\n");
        fclose(fp);

        return FALSE;
    }

    if (fclose(fp))
    {
        fprintf(stderr, "Error reading \"%s\"\n", file);

        return FALSE;
    }

    fprintf(stderr, "State loaded from \"%s\"\n", file);
    return TRUE;
}


/*
 * Read a file containing initial settings for either gen 0 or the last gen.
 * If setall is TRUE, both the ON and the OFF cells will be set.
 * Returns OK on success, ERROR on error.
 */
static Status
readfile(file)
    char * file;
{
    FILE * fp;
    char * cp;
    char ch;
    int row;
    int col;
    int activegen;
    int mingen;
    int maxgen;
    int gen;
    State state;
    char buf[LINESIZE];
    Cell * cell;

    if (*file == '\0')
        return OK;

    fp = fopen(file, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open \"%s\"\n", file);

        return ERROR;
    }

    activegen = (parent ? (genmax - 1) : 0);
    row = 0;

    while (fgets(buf, LINESIZE, fp))
    {
        row++;
        cp = buf;
        col = 0;

        while (*cp && (*cp != '\n'))
        {
            mingen = activegen;
            maxgen = activegen;

            col++;
            ch = *cp++;

            switch (ch)
            {
                case '?':
                    continue;

                case 'x':
                case 'X':
                    excludecone(row, col, activegen);
                    continue;

                case '+':
                    freezecell(row, col);
                    continue;

                case '.':
                case ' ':
                    if (!setall)
                        continue;

                    if (setdeep)
                    {
                        mingen = 0;
                        maxgen = genmax;
                    }

                    state = OFF;
                    break;

                case ':':
                    mingen = 0;
                    maxgen = genmax;
                    state = OFF;
                    break;

                case 'O':
                case 'o':
                case '*':
                    state = ON;
                    break;

                default:
                    fprintf(stderr, "Bad file format in line %d\n",
                        row);
                    fclose(fp);

                    return ERROR;
            }
            for (gen = mingen; gen <= maxgen; gen++)
            {
            cell = findcell(row, col, gen);
            if (!proceed(cell, state, FALSE))
            {
                fprintf(stderr, "Inconsistent state for cell %d %d\n",
                    row, col);
                fclose(fp);

                return ERROR;
            }
            }
        }
    }

    if (fclose(fp))
    {
        fprintf(stderr, "Error reading \"%s\"\n", file);

        return ERROR;
    }

    return OK;
}


/*
 * Read a number from a string, eating any leading or trailing blanks.
 * Returns the value, and indirectly updates the string pointer.
 * Returns specified default if no number was found.
 */
static long
getnum(cpp, defnum)
    char ** cpp;
    int defnum;
{
    char * cp;
    long num;
    Bool isneg;

    isneg = FALSE;
    cp = *cpp;

    while (isblank(*cp))
        cp++;

    if (*cp == '-')
    {
        cp++;
        isneg = TRUE;
    }

    if (!isdigit(*cp))
    {
        *cpp = cp;

        return defnum;
    }

    num = 0;

    while (isdigit(*cp))
        num = num * 10 + (*cp++ - '0');

    if (isneg)
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
static Bool
setrules(cp)
    char * cp;
{
    int i;
    unsigned int bits;

    for (i = 0; i < 9; i++)
    {
        bornrules[i] = FALSE;
        liverules[i] = FALSE;
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
                bornrules[i] = TRUE;

            if (bits & 0x02)
                liverules[i] = TRUE;

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
            bornrules[*cp++ - '0'] = TRUE;

        if ((*cp != ',') && (*cp != '/'))
            return FALSE;

        cp++;

        if ((*cp == 's') || (*cp == 'S'))
            cp++;

        while ((*cp >= '0') && (*cp <= '8'))
            liverules[*cp++ - '0'] = TRUE;

        if (*cp)
            return FALSE;
    }

    /*
     * Construct the rule string for printouts and see if this
     * is the normal Life rule.
     */
    cp = rulestring;

    *cp++ = 'B';

    for (i = 0; i < 9; i++)
    {
        if (bornrules[i])
            *cp++ = '0' + i;
    }

    *cp++ = '/';
    *cp++ = 'S';

    for (i = 0; i < 9; i++)
    {
        if (liverules[i])
            *cp++ = '0' + i;
    }

    *cp = '\0';

    islife = (strcmp(rulestring, "B3/S23") == 0);

    return TRUE;
}


/*
 * Print usage text.
 */
static void
usage()
{
    char ** cpp;
    static char * text[] =
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
    "   -ow  Set search order to find wide objects first",
    "   -og  Set search order to examine all gens in a column before next column",
    "   -om  Set search order to examine from middle column outwards",
    "   -p   Only look for parents of last generation",
    "   -a   Find all objects (even those with subperiods)",
    "   -v   View object every N thousand searches",
    "   -d   Dump status to file every N thousand searches",
    "   -l   Load status from file",
    "   -ln  Load status without entering command mode",
    "   -b   Batch. Don't enter command mode",
    "   -i   Read initial object setting both ON and OFF cells",
    "   -in  Read initial object from file setting only ON cells",
    "   -o   Output objects to file (appending) every N columns",
    "   -R   Use Life rules specified by born,live values",
    NULL
    };

    printf("Program to search for Life oscillators or spaceships (version %s)\n", VERSION);

    for (cpp = text; *cpp; cpp++)
        fprintf(stderr, "%s\n", *cpp);
}

/* END CODE */
