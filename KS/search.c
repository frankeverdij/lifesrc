/*
 * Life search program - actual search routines.
 * Author: David I. Bell.
 * Based on the algorithms by Dean Hickerson that were
 * included with the "xlife 2.0" distribution.  Thanks!
 * Changes for arbitrary Life rules by Nathan S. Thompson.


  ****** Heavily modified. Modifications not noted consistently.  -JES ******
 */

/*
 * Define this as a null value so as to define the global variables
 * defined in lifesrc.h here.
 */
#define    EXTERN

//#include <windows.h>
//#include "wls.h"
#include "lifesrc.h"
//#include <search.h>
#include "implication.h"
#include "allocatecell.h"
#include "linkcell.h"
#include "setstate.h"
#include "mapcell.h"
#include "symcell.h"
#include "sortorder.h"
#include "enums.h"


#define SUMCOUNT 8
//#define SUMTODESC(a, b, c)  ((a) + 2 * (b) + 4 * (c))
//extern volatile int abortthread;
extern int symmetry;
//extern int stoponstep;

/*
 * Table of implications.
 * Given the state of a cell and its neighbors in one generation,
 * this table determines deductions about the cell and its neighbors
 * in the previous generation.
 * The table is indexed by the descriptor value of a cell.
 */
static FLAGS implic[2304];

/*
 * Other local data.
 */
static    Cell **    searchlist;    /* current list of cells to search */
static    int    searchidx;      /* index of first unknown cell in searchlist[] */
static Cell *    cellTable[MAXCELLS];    /* table of usual cells */


/*
 * Local procedures
 */
static State choose(Cell *);
static Cell * getnormalunknown(void);
static Cell * getsmartunknown(void); // KAS
static Bool consistify(Cell *);
static Bool consistify10(Cell *);
static Cell * (*getunknown)(void);


void setState(Cell * const cell, const State state)
{
    /* backup previous state */
    int diffState = state - cell->state;
    /* set cell state */
    cell->state = state;
    /* correct the neighbor sum for cells touching this cell */
    cell->cul->sumNear += diffState;
    cell->cu->sumNear += diffState;
    cell->cur->sumNear += diffState;
    cell->cl->sumNear += diffState;
    cell->cr->sumNear += diffState;
    cell->cdl->sumNear += diffState;
    cell->cd->sumNear += diffState;
    cell->cdr->sumNear += diffState;

    return;
}

/*
 * Initialize the table of cells.
 * Each cell in the active area is set to unknown state.
 * Boundary cells are set to zero state.
 */
void
initcells()
{
    int row, col, gen;
    int i;
    Bool edge;
    Cell * cell;
    Cell * cell2;

    inited = FALSE;
    searchlist = NULL;


    if ((rowmax <= 0) || (rowmax > ROWMAX) ||
        (colmax <= 0) || (colmax > COLMAX) ||
        (genmax <= 0) || (genmax > GENMAX) ||
        (rowtrans < -TRANSMAX) || (rowtrans > TRANSMAX) ||
        (coltrans < -TRANSMAX) || (coltrans > TRANSMAX))
    {
        ttystatus("ROW, COL, GEN, or TRANS out of range\n");
        exit(1);
    }

    for (i = 0; i < MAXCELLS; i++)
        cellTable[i] = allocateCell();

    /*
     * Link the cells together.
     */
    for (col = 0; col <= colmax+1; col++)
    {
        for (row = 0; row <= rowmax+1; row++)
        {
            for (gen = 0; gen < genmax; gen++)
            {
                edge = ((row == 0) || (col == 0) ||
                    (row > rowmax) || (col > colmax));

                cell = findcell(row, col, gen);
                cell->gen = gen;
                cell->row = row;
                cell->col = col;

                cell->active = TRUE;
                cell->choose = TRUE;

                /*
                 * If this is not an edge cell, then its state
                 * is unknown and it needs linking to its
                 * neighbors.
                 */
                if (!edge)
                {
                    linkCell(cell);
                    setState(cell, UNK);
                    cell->free = TRUE;
                }

                /*
                 * Map time forwards and backwards,
                 * wrapping around at the ends.
                 */
                cell->past = findcell(row, col,
                    (gen+genmax-1) % genmax);

                cell->future = findcell(row, col,
                    (gen+1) % genmax);

                /*
                 * If this is not an edge cell, and
                 * there is some symmetry, then put
                 * this cell in the same loop as the
                 * next symmetrical cell.
                 */
                if(symmetry && !edge)
                {
                    loopcells(cell, symCell(cell));
                }
            }
        }
    }

    /*
     * Now for the symmetry
     * Let's look for all loops
     * and select one cell from each loop as active
     */

    for (col = 1; col <= colmax; col++)
    {
        for (row = 1; row <= rowmax; row++)
        {
            for (gen = 0; gen < genmax; gen++)
            {
                cell = findcell(row, col, gen);

                if (cell->active)
                {
                    cell2 = cell->loop;
                    while (cell2 != cell)
                    {
                        cell2->active = FALSE;
                        cell2 = cell2->loop;
                    }
                }
            }
        }
    }


    /*
     * If there is a non-standard mapping between the last generation
     * and the first generation, then change the future and past pointers
     * to implement it.  This is for translations and flips.
     */
    if (rowtrans || coltrans || fliprows || flipcols || flipquads)
    {
        for (row = 0; row <= rowmax+1; row++)
        {
            for (col = 0; col <= colmax+1; col++)
            {
                cell = findcell(row, col, 0);
                cell2 = mapCell(cell);
                cell->past = cell2;
                cell2->future = cell;
            }
        }
    }

    if (smart) {
        getunknown = getsmartunknown; // KAS
    } else {
        getunknown = getnormalunknown;
    }

    newset = settable;
    nextset = settable;

    initsearchorder();

    searchset = searchtable;

    curstatus = OK;
    initimplic(bornrules, liverules, implic);

    inited = TRUE;
}

#if 0
/*
 * The sort routine for searching.
 */
static int
ordersortfunc(const void * xxx1, const void * xxx2)
{
    Cell ** arg1;
    Cell ** arg2;
    Cell * c1;
    Cell * c2;
    int midcol;
    int midrow;
    int dif1;
    int dif2;

    arg1 = (Cell**)xxx1;
    arg2 = (Cell**)xxx2;

    c1 = *arg1;
    c2 = *arg2;

    /*
     * If on equal position or not ordering by all generations
     * then sort primarily by generations
     */
    if (((c1->row == c2->row) && (c1->col == c2->col)) || !ordergens)
    {
        // Put generation 0 first
        // or if calculating parents, put generation 0 last
        if (parent)
        {
            if (c1->gen < c2->gen) return 1;
            if (c1->gen > c2->gen) return -1;
        } else {
            if (c1->gen < c2->gen) return -1;
            if (c1->gen > c2->gen) return 1;
        }
        // if we are here, it is the same cell
    }

    if(diagsort) {
        if(c1->col+c1->row > c2->col+c2->row) return 1;
        if(c1->col+c1->row < c2->col+c2->row) return -1;
        if(abs(c1->col-c1->row) > abs(c2->col-c2->row)) return (orderwide)?1:(-1);
        if(abs(c1->col-c1->row) < abs(c2->col-c2->row)) return (orderwide)?(-1):1;
    }
    if(knightsort) {
        if(c1->col*2+c1->row > c2->col*2+c2->row) return 1;
        if(c1->col*2+c1->row < c2->col*2+c2->row) return -1;
        if(abs(c1->col-c1->row) > abs(c2->col-c2->row)) return (orderwide)?1:(-1);
        if(abs(c1->col-c1->row) < abs(c2->col-c2->row)) return (orderwide)?(-1):1;
    }
    
    /*
     * Sort on the column number.
     * By default this is from left to right.
     * But if middle ordering is set, the ordering is from the center
     * column outwards.
     */
    if (ordermiddle)
    {
        midcol = (colmax + 1) / 2;

        dif1 = abs(c1->col - midcol);

        dif2 = abs(c2->col - midcol);

        if (dif1 < dif2) return -1;

        if (dif1 > dif2) return 1;
    } else {
        if (c1->col < c2->col) return -1;

        if (c1->col > c2->col) return 1;
    }

    /*
     * Sort on the row number.
     * By default, this is from the middle row outwards.
     * But if wide ordering is set, the ordering is from the edge
     * inwards.  Note that we actually set the ordering to be the
     * opposite of the desired order because the initial setting
     * for new cells is OFF.
     */
    midrow = (rowmax + 1) / 2;

    dif1 = abs(c1->row - midrow);

    dif2 = abs(c2->row - midrow);

    if (dif1 < dif2) return (orderwide ? -1 : 1);

    if (dif1 > dif2) return (orderwide ? 1 : -1);

    return 0;
}
#endif

/*
 * Order the cells to be searched by building the search table list.
 * This list is built backwards from the intended search order.
 * The default is to do searches from the middle row outwards, and
 * from the left to the right columns.  The order can be changed though.
 */
void
initsearchorder()
{
    int row, col, gen;
    int count;
    Cell * cell;
    Cell * table[MAXCELLS];
    globals_struct g;
    g.colMax = colmax;
    g.rowMax = rowmax;
    g.parent = parent;
    g.orderGens = ordergens;
    g.orderMiddle = ordermiddle;
    g.orderWide = orderwide;
    g.orderInvert = 0;
    if (diagsort)
        g.sortOrder = DIAG;
    else if (knightsort)
        g.sortOrder = KNIGHT;
    else
        g.sortOrder = DEFAULT;
    /*
     * Make a table of cells that will be searched.
     * Ignore cells that are not relevant to the search due to symmetry.
     */
    count = 0;

    for (gen = 0; gen < genmax; gen++)
    {
        for (col = 1; col <= colmax; col++)
        {
            for (row = 1; row <= rowmax; row++)
            {
                cell = findcell(row, col, gen);
                // cells must be already loaded!!!
                if ((cell->active) && (cell->state == UNK) && (cell->choose))
                {
                    table[count++] = findcell(row, col, gen);
                }
            }
        }
    }

    /*
     * Now sort the table based on our desired search order.
     */
    qsort_r((char *) table, count, sizeof(Cell *), orderSortFunc, &g);

    /*
     * If we've been here before, wipe the old searchlist
     */
    if (searchlist) free(searchlist);

    /*
     * Finally build the search list from the table elements in the
     * final order.
     */
    searchlist = (Cell **) malloc(sizeof(Cell *) * (count + 1));

    for (int i = 0; i < count; i++)
    {
        searchlist[i] = table[i];
        searchlist[i]->index = i;
    }
    searchlist[count] = NULL;
    searchidx = 0;

}

/*
 * Set the state of a cell back to UNK/FREE
 * Proceed through the loop if present
 */

void
rescell(Cell * cell)
{
    Cell * c1;

    if (cell->state == UNK) return;

    --cellcount; // take all loops as a single cell

    c1 = cell;

    do {
        setState(cell, UNK);
        cell->free = TRUE;

        cell = cell->loop;
    } while (cell != c1);
}

/*
 * Set the state of a cell to the specified state.
 * The state is either ON or OFF.
 * Returns ERROR if the setting is inconsistent.
 * If the cell is newly set, then it is added to the set table.
 */

Bool
setcell(Cell * cell, State state, Bool free)
{
    Cell * c1, * c2;
    if (cell->state == state)
    {
        DPRINTF4("setcell %d %d %d to state %s already set\n",
            cell->row, cell->col, cell->gen,
            (state == ON) ? "on" : "off");

        return TRUE;
    }

    if (cell->state != UNK)
    {
        DPRINTF4("setcell %d %d %d to state %s inconsistent\n",
            cell->row, cell->col, cell->gen,
            (state == ON) ? "on" : "off");

        return FALSE;
    }

    c1 = cell;

    DPRINTF5("setCell %d %d %d to %s, %s successful\n",
        cell->row, cell->col, cell->gen,
        (free ? "free" : "forced"), ((state == ON) ? "on" : "off"));

    do {
        setState(cell, state);
        cell->free = free;

        if (cell->active) {
            *newset++ = cell;
            *searchset++ = searchlist[searchidx];
            for (; (c2 = searchlist[searchidx]); searchidx++)
            {
                if (c2->state == UNK)
                {
                    break;
                }
            }
            free = FALSE; // all following cells in the loop are not free

        }
        cell = cell->loop;
    } while (c1 != cell);

    ++cellcount; // take whole loop as a single cell

    return TRUE;
}


void shortsetcell (Cell * cell, const State state)
{
    Cell * c1 = cell, * c2;

    do {
        setState(cell, state);
        cell->free = FALSE;

        if (cell->active) {
            *newset++ = cell;
            *searchset++ = searchlist[searchidx];
            for (; (c2 = searchlist[searchidx]); searchidx++)
            {
                if (c2->state == UNK)
                {
                    break;
                }
            }
        }
        cell = cell->loop;
    } while (c1 != cell);

    ++cellcount; // take whole loop as a single cell

    return;
}

/*static __inline int
sumtodesc(State futurestate, State currentstate, int neighborsum)
{
    // UNK = 0
    // ON = 1
    // OFF = 9

    // using the following expression, all different
    // combinations are mapped to different numbers
    // if you don't believe it, just try it
    
    return (neighborsum*10 + currentstate*3 + futurestate);
}*/

/*
 * Calculate the current descriptor for a cell.
 */
static __inline short
getdesc(Cell * cell)
{
    return SUMTODESC(cell->future->state, cell->state, cell->sumNear);
}

/*
 * Consistify a cell.
 * This means examine this cell in the previous generation, and
 * make sure that the previous generation can validly produce the
 * current cell.  Returns FALSE if the cell is inconsistent.
 */
static Bool consistify(Cell * cell)
{
    Cell * prevcell;
    int desc;
    State state;
    FLAGS flags;

    /*
     * If we are searching for parents and this is generation 0, then
     * the cell is consistent with respect to the previous generation.
     */
    if (parent && (cell->gen == 0))
        return TRUE;

    // Now get the descriptor for the cell, its parent and its parent neighborhood

    prevcell = cell->past;
    desc = SUMTODESC(cell->state, prevcell->state, prevcell->sumNear);

    // the implic table will tell us everything we need to know

    flags = implic[desc];

    // first check if the state is consistent

    if (flags == IMPBAD) return FALSE;

    // the state is consistent
    // now for the implications

    // change the cell if needed
    if (((flags & IMPN) != 0) &&
        !setcell(cell, ((flags & IMPN1) != 0) ? ON : OFF, FALSE)) return FALSE;

    // change the parent cell if needed
    if (((flags & IMPC) != 0) &&
        !setcell(prevcell, ((flags & IMPC1) != 0) ? ON : OFF, FALSE)) return FALSE;

    if ((flags & IMPUN) != 0)
    {
        // let's change the parent neighborhood
        state = ((flags & IMPUN1) != 0) ? ON : OFF;

        DPRINTF4("Forcing unknown neighbors of cell %d %d %d %s\n",
            prevcell->row, prevcell->col, prevcell->gen, (state == ON) ? "on" : "off");

        if (prevcell->cul->state == UNK)
            shortsetcell(prevcell->cul, state);
        if (prevcell->cu->state == UNK)
            shortsetcell(prevcell->cu, state);
        if (prevcell->cur->state == UNK)
            shortsetcell(prevcell->cur, state);
        if (prevcell->cl->state == UNK)
            shortsetcell(prevcell->cl, state);
        if (prevcell->cr->state == UNK)
            shortsetcell(prevcell->cr, state);
        if (prevcell->cdl->state == UNK)
            shortsetcell(prevcell->cdl, state);
        if (prevcell->cd->state == UNK)
            shortsetcell(prevcell->cd, state);
        if (prevcell->cdr->state == UNK)
            shortsetcell(prevcell->cdr, state);
    }

    DPRINTF4("Implications successful for prevCell %d %d %d %d\n", prevcell->row, prevcell->col, prevcell->gen, prevcell->state);

    return TRUE;
}


/*
 * See if a cell and its neighbors are consistent with the cell and its
 * neighbors in the next generation.
 */
static Bool
consistify10(Cell * cell)
{
    if (!consistify(cell))
        return FALSE;

    cell = cell->future;

    return consistify(cell)
       && consistify(cell->cul)
       && consistify(cell->cu)
       && consistify(cell->cur)
       && consistify(cell->cl)
       && consistify(cell->cr)
       && consistify(cell->cdl)
       && consistify(cell->cd)
       && consistify(cell->cdr);
}


/*
 * Examine the next choice of cell settings.
 */
Status
examinenext()
{
    Cell * cell;

    /*
     * If there are no more cells to examine, then what we have
     * is consistent.
     */
    if (nextset == newset)
        return CONSISTENT;

    /*
     * Get the next cell to examine, and check it out for symmetry
     * and for consistency with its previous and next generations.
     */
    cell = *nextset++;

    DPRINTF4("Examining saved cell %d %d %d (%s) for consistency\n",
        cell->row, cell->col, cell->gen,
        (cell->free ? "free" : "forced"));

    return consistify10(cell) ? OK : ERROR1;
}


/*
 * Set a cell to the specified value and determine all consequences we
 * can from the choice.  Consequences are a contradiction or a consistency.
 */
Bool
proceed(cell, state, free)
    Cell * cell;
    State state;
    Bool free;
{
    int status;

    if (!setcell(cell, state, free))
        return FALSE;

    do {
        status = examinenext();
    } while (status == OK);

    return (status == CONSISTENT);
}


/*
 * Back up the list of set cells to undo choices.
 * Returns the cell which is to be tried for the other possibility.
 * Returns NULL on an "object cannot exist" error.
 */
Cell *
backup()
{
    Cell * cell;

    // first let's find how far to backup

    nextset = newset;

    while (nextset != settable)
    {
        cell = *--nextset;
        --searchset;

        DPRINTF5("backing up cell %d %d %d, was %s, %s\n",
            cell->row, cell->col, cell->gen,
            ((cell->state == ON) ? "on" : "off"),
            ((cell->free) ? "free": "forced"));

        if (!cell->free) continue;

        // free cell found
        // record old status
        prevstate = cell->state;

        searchidx = (*searchset)->index;

        // reset the stack and return the cell
        while (newset != nextset) {
            rescell(*--newset);
        }

        return cell;
    }

    // free cell not found
    // let's reset the stack anyway

    while (newset != nextset) {
        rescell(*--newset);
    }

    return NULL;
}


/*
 * Do checking based on setting the specified cell.
 * Returns ERROR if an inconsistency was found.
 */
Bool
go(Cell * cell, State state, Bool free)
{
    Cell ** setpos;

    for (;;)
    {
        setpos = nextset;

        if (proceed(cell, state, free)) return TRUE;

        if ((setpos == nextset) && free)
        {
            // no cell added to stack
            // no backup required
            // but prevstate is not defined now
            state = (ON + OFF) - state;
        } else {
            cell = backup();

            if (cell == NULL) return FALSE;

            state = (ON + OFF) - prevstate;
        }
        free = FALSE;
    }
}


/*
 * Find another unknown cell in a normal search.
 * Returns NULL if there are no more unknown cells.
 */
static Cell *
getnormalunknown()
{
    Cell * cell;

    for (int i = searchidx; (cell = searchlist[i]); i++)
    {
        if ((cell->state == UNK) && (cell->choose))
        {
                searchidx = i;
                return cell;
        }
    }

    return NULL;
}


// calculate how many cells will change
// if we change the current cell to ON or OFF
// set smartlen1 and smartlen0 to appropriate numbers

static Bool getsmartnumbers(Cell * cell)
{
    int cellno;
    int comb0, comb1;
    Cell ** setpos;

    // known and inactive cells are unimportant
    if (cell->state != UNK) return 2;

    // remember set position for proper backup
    setpos = newset;

    // remember cell count to calculate the change
    cellno = cellcount;

    comb0 = comb1 = differentcombinedcells + setcombinedcells;
    // test the cell
    if (proceed(cell, ON, TRUE))
    {
        smartlen1 = cellcount - cellno;
        comb1 = differentcombinedcells  + setcombinedcells - comb1;

        // back up
        backup(); 

        // and now let's try the OFF choice

        if (proceed(cell, OFF, TRUE))
        {
            smartlen0 = cellcount - cellno;
            comb0 = differentcombinedcells + setcombinedcells - comb0;

            if (smarton)
            {
                if (comb0 == comb1)
                {
                    smartcomb = comb0;
                    smartchoice = (smartlen1 > smartlen0) ? ON : OFF;
                }
                else if (comb0 > comb1)
                {
                    smartcomb = comb0;
                    smartchoice = OFF;
                }
                else
                {
                    smartcomb = comb1;
                    smartchoice = ON;
                }
            }
            else
            {
                smartcomb = 0;
                smartchoice = (smartlen1 > smartlen0) ? ON : OFF;
            }

            // back up
            backup();

            return TRUE;

        } else {
            // OFF state inconsistent
            // makes a good candidate
            smartchoice = OFF;

            // back up if something changed
            if (setpos != newset) backup();

            return FALSE;
        }

    } else {
        // ON state inconsistent
        // it's actually a good candidate
        smartchoice = ON;

        // back up if something changed
        if (setpos != newset) backup();

        return FALSE;

    }
}

// Smart cell ordering

static Cell *
getsmartunknown()
{
    Cell * cell;
    Cell * best;

    // The assignment in the following codeline is debatable,
    // but since the original code also did not initialise
    // this variable, chances are good that this defaulted to `0`
    // which in the original code was being interpreted as a 'UNK',
    // hence the initialisation as 'UNK'
    State bestchoice = UNK;

    int idx;
    int max, window, threshold, bestlen1, bestlen0, bestcomb, wnd, n1, n2, a, b, c, d;

    // Move the searchlist over all known cells
    for (; (cell = searchlist[searchidx]); searchidx++)
    {
        if ((cell->state == UNK) && (cell->choose))
        {
            break;
        }
    }

    // Return NULL if no unknown cells
    if (cell == NULL) return NULL;

    // Prepare threshold
    threshold = smartthreshold;
    if (threshold <= 0) threshold = MAXCELLS;

    // Prepare the dummy maximum
    max = 2; // at least 3 cells must change
    bestlen0 = 1;
    bestlen1 = 1;
    bestcomb = 0;

    best = NULL;

    wnd = 0;

    window = smartwindow;
    idx = searchidx;

    while ((cell = searchlist[idx]) && (window > 0) && (max < threshold)) {
        ++wnd;
        --window; // count known cells too
        if ((cell->state == UNK) && (cell->choose))
        {
            if (getsmartnumbers(cell))
            {
                if (bestcomb == smartcomb)
                {
                    // (smartlen0, smartlen1) is better than (bestlen0, bestlen1) if
                    // 1/2**smartlen0 + 1/2**smartlen1 < 1/2**bestlen0 + 1/2**bestlen1
                    // i.e.
                    // 2**(b0+b1+s0) + 2**(b0+b1+s1) < 2**(s0+s1+b0) + 2**(s0+s1+b1)
                    //
                    // both sides are binary numbers with one or two 1's
                    // so let's compare the exponents

                    n1 = smartlen0 + smartlen1;
                    n2 = n1 + bestlen1;
                    n1 += bestlen0;

                    if (n1 > n2) 
                    {
                        a = n1;
                        b = n2;
                    } else if (n1 == n2) {
                        a = n1 + 1;
                        b = -1;
                    } else {
                        a = n2;
                        b = n1;
                    }

                    // max = bestlen0 + bestlen1

                    n1 = max + smartlen0;
                    n2 = max + smartlen1;

                    if (n1 > n2)
                    {
                        c = n1;
                        d = n2;
                    } else if (n1 == n2) {
                        c = n1 + 1;
                        d = -1;
                    } else {
                        c = n2;
                        d = n1;
                    }

                    if ((a > c) || ((a = c) && (b > d)))
                    {
                        best = cell;
                        bestchoice = smartchoice;
                        // bestcomb = smartcomb; -- it's equal anyway
                        bestlen1 = smartlen1;
                        bestlen0 = smartlen0;
                        max = bestlen0 + bestlen1;
                    }
                }
                else if (bestcomb < smartcomb)
                {
                    best = cell;
                    bestchoice = smartchoice;
                    bestcomb = smartcomb;
                    bestlen1 = smartlen1;
                    bestlen0 = smartlen0;
                    max = bestlen0 + bestlen1;
                }
            } else {
                // the cell can be set only one way
                best = cell;
                bestchoice = smartchoice;
                max = MAXCELLS + 1;
                window = 0;
            }
        }
        idx++;
    }

    // Found something?
    if (best != NULL)
    {
        if (MAXCELLS >= max)
        {
            smartstatsumwnd += wnd;
            ++smartstatsumwndc;
            smartstatsumlen += max;
            ++smartstatsumlenc;
        
            if (smartstatsumwndc >= 100000)
            {
                smartstatwnd = smartstatsumwnd / smartstatsumwndc;
                smartstatsumwnd = 0;
                smartstatsumwndc = 0;
                if (smartstatsumlenc >= 10000)
                {
                    smartstatlen = smartstatsumlen / smartstatsumlenc;
                    smartstatsumlen = 0;
                    smartstatsumlenc = 0;
                }
            }
        }

        if (smarton)
        {
            // propose the shorter tree
            smartchoice = bestchoice;
        } else {
            smartchoice = UNK;
        }
        return best;
    }

    // fall back to standard
    smartchoice = UNK;

    // Just return the first UNK cell
    // This shouldn't be often anyway

    return searchlist[searchidx];
}

/*
 * Choose a state for an unknown cell, either OFF or ON.
 * Normally, we try to choose OFF cells first to terminate an object.
 * But for follow generations mode, we try to choose the same setting
 * as a nearby generation.
 */

static State
choose(cell)
    Cell * cell;
{
    /* 
     * if something pre-set by the select algorithm,
     * use the selection
     */

    if (smartchoice != UNK) return smartchoice;

    /*
     * If we are following cells in other generations,
     * then try to do that.
     */

    if (followgens)
    {
        if ((cell->past->state == ON) ||
            (cell->future->state == ON))
        {
            return ON;
        }

    }

    /* 
     * In all other cases
     * try the OFF state first
     */

    return chooseUnknown;
}

Cell * combinebackup(void);

/*
 * The top level search routine.
 * Returns if an object is found, or is impossible.
 */
Status
search(const Bool batch)
{
    Cell * cell;
    Bool free;
    Bool needwrite;
    State state;

    cell = (*getunknown)();

    if (cell == NULL)
    {
        // nothing to search
        // so we are at a solution
        // let's start search for another one

        cell = backup();

        if (cell == NULL)
            return ERROR1;

        free = FALSE;
        state = (ON + OFF) - prevstate;

    } else {

        state = choose(cell);
        free = TRUE;

    }

    for (;;) {
        if(ttycheck()) 
        {
            if (!batch) {
                getcommands();
            } else {
                exit(0);
            }
        }
        // Set the state of the new cell.

        if (!go(cell, state, free)) 
        {
            printgen(curgen);

            return NOTEXIST;
        }


        // If it is time to dump our state, then do that.

        if (dumpfreq && (++dumpcount >= dumpfreq))
        {
            dumpcount = 0;
            dumpstate(dumpfile);
        }


        // If we have enough columns found, then remember to
        // write it to the output file.  Also keep the last
        // columns count values up to date.

        needwrite = FALSE;

        if (outputcols &&
            (fullcolumns >= outputlastcols + outputcols))
        {
            outputlastcols = fullcolumns;
            needwrite = TRUE;
        }

        if (outputlastcols > fullcolumns)
            outputlastcols = fullcolumns;

        // If it is time to view the progress,then show it.

        if (needwrite || (viewfreq && (++viewcount >= viewfreq)))
        {
            printgen(curgen);
        }

        // Write the progress to the output file if needed.
        // This is done after viewing it so that the write
        // message will stay visible for a while.

        if (needwrite)
        {
            writegen(outputfile, TRUE);
        }


        // Get the next unknown cell and choose its state.

        cell = (*getunknown)();

        if (cell == NULL)
            return FOUND;

        state = choose(cell);
        free = TRUE;
    }
}

static int auxCellCount = 0; /* cells in auxillary table */
static Cell * auxTable[MAXCELLS]; /* table of auxillary cells */

/*
 * Find a cell given its coordinates.
 * Most coordinates range from 0 to colmax+1, 0 to rowmax+1, and 0 to genmax-1.
 * Cells within this range are quickly found by indexing into celltable.
 * Cells outside of this range are handled by searching an auxillary table,
 * and are dynamically created as necessary.
 */
Cell * findcell(int row, int col, int gen)
{
    Cell * cell;
    int i;

    /*
     * If the cell is a normal cell, then we know where it is.
     */
    if ((row >= 0) && (row <= rowmax + 1) &&
        (col >= 0) && (col <= colmax + 1) &&
        (gen >= 0) && (gen < genmax))
    {
        return cellTable[(col * (rowmax + 2) + row) * genmax + gen];
    }

    /*
     * See if the cell is already allocated in the auxillary table.
     */
    for (i = 0; i < auxCellCount; i++)
    {
        cell = auxTable[i];

        if ((cell->row == row) && (cell->col == col) &&
            (cell->gen == gen))
        {
            return cell;
        }
    }

    /*
     * Need to allocate the cell and add it to the auxillary table.
     */
    cell = allocateCell();
    cell->row = row;
    cell->col = col;
    cell->gen = gen;

    auxTable[auxCellCount++] = cell;

    return cell;
}

/* END CODE */
