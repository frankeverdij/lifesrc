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

#include "lifesrc.h"

/*
 * Local procedures
 */
static Cell * (*getunknown)(void);


/*
 * Local variables
 */
static int smartstatlen = 0;
static int smartstatwnd = 0;
static int smartstatsumlen = 0;
static int smartstatsumwnd = 0;
static int smartstatsumlenc = 0;
static int smartstatsumwndc = 0;

static State smartchoice = UNK;
static int smartlen0;
static int smartlen1;

static int cellcount = 0; /* number of set cells */
static State prevstate; /* the state of the last free cell before backup() */


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


void shortsetcell(Cell * cell, const State state, Bool free)
{
    Cell * c1 = cell, * c2;

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
            free = FALSE;
        }
        cell = cell->loop;
    } while (c1 != cell);

    ++cellcount; // take whole loop as a single cell

    return;
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
    if (cell->state == state)
    {
        DPRINTF("setcell %d %d %d to state %s already set\n",
            cell->row, cell->col, cell->gen,
            (state == ON) ? "on" : "off");

        return TRUE;
    }

    if (cell->state == UNK)
    {
        DPRINTF("setCell %d %d %d to %s, %s successful\n",
            cell->row, cell->col, cell->gen,
            (free ? "free" : "forced"), ((state == ON) ? "on" : "off"));

        shortsetcell(cell, state, free);

        return TRUE;
    }

    DPRINTF("setcell %d %d %d to state %s inconsistent\n",
        cell->row, cell->col, cell->gen,
        (state == ON) ? "on" : "off");

    return FALSE;
}


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

        DPRINTF("Forcing unknown neighbors of cell %d %d %d %s\n",
            prevcell->row, prevcell->col, prevcell->gen, (state == ON) ? "on" : "off");

        if (prevcell->cul->state == UNK)
            shortsetcell(prevcell->cul, state, FALSE);
        if (prevcell->cu->state == UNK)
            shortsetcell(prevcell->cu, state, FALSE);
        if (prevcell->cur->state == UNK)
            shortsetcell(prevcell->cur, state, FALSE);
        if (prevcell->cl->state == UNK)
            shortsetcell(prevcell->cl, state, FALSE);
        if (prevcell->cr->state == UNK)
            shortsetcell(prevcell->cr, state, FALSE);
        if (prevcell->cdl->state == UNK)
            shortsetcell(prevcell->cdl, state, FALSE);
        if (prevcell->cd->state == UNK)
            shortsetcell(prevcell->cd, state, FALSE);
        if (prevcell->cdr->state == UNK)
            shortsetcell(prevcell->cdr, state, FALSE);
    }

    DPRINTF("Implications successful for prevCell %d %d %d %d\n", prevcell->row, prevcell->col, prevcell->gen, prevcell->state);

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
examinenext(void)
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

    DPRINTF("Examining saved cell %d %d %d (%s) for consistency\n",
        cell->row, cell->col, cell->gen,
        (cell->free ? "free" : "forced"));

    return consistify10(cell) ? OK : ERROR;
}


/*
 * Set a cell to the specified value and determine all consequences we
 * can from the choice.  Consequences are a contradiction or a consistency.
 */
Bool
proceed(Cell * cell, State state, Bool free)
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
backup(void)
{
    Cell * cell;

    // first let's find how far to backup

    nextset = newset;

    while (nextset != settable)
    {
        cell = *--nextset;
        --searchset;

        DPRINTF("backing up cell %d %d %d, was %s, %s\n",
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

        if (proceed(cell, state, free))
        {
            return TRUE;
        }

        if ((setpos == nextset) && free)
        {
            // no cell added to stack
            // no backup required
            // but prevstate is not defined now
            state = (ON + OFF) - state;
        } else {
            cell = backup();

            if (cell == NULL)
            {
                return FALSE;
            }

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
getnormalunknown(void)
{
    Cell * cell;

    for (int i = searchidx; (cell = searchlist[i]); i++)
    {
        if (cell->state == UNK)
        {
            if (cell->choose)
            {
                searchidx = i;

                return cell;
            }
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
    Cell ** setpos;

    // known and inactive cells are unimportant
    if (cell->state != UNK) return 2;

    // remember set position for proper backup
    setpos = newset;

    // remember cell count to calculate the change
    cellno = cellcount;

    // test the cell
    if (proceed(cell, ON, TRUE))
    {
        smartlen1 = cellcount - cellno;

        // back up
        backup(); 

        // and now let's try the OFF choice

        if (proceed(cell, OFF, TRUE))
        {
            smartlen0 = cellcount - cellno;
            smartchoice = (smartlen1 > smartlen0) ? ON : OFF;

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
getsmartunknown(void)
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
    int max, window, threshold, bestlen1, bestlen0, wnd, n1, n2, a, b, c, d;

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
    if (threshold <= 0) threshold = MAX_CELLS;

    // Prepare the dummy maximum
    max = 2; // at least 3 cells must change
    bestlen0 = 1;
    bestlen1 = 1;
//    bestcomb = 0;

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
//                if (bestcomb == smartcomb)
//                {
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
/*                }
                else if (bestcomb < smartcomb)
                {
                    best = cell;
                    bestchoice = smartchoice;
                    bestcomb = smartcomb;
                    bestlen1 = smartlen1;
                    bestlen0 = smartlen0;
                    max = bestlen0 + bestlen1;
                }*/
            } else {
                // the cell can be set only one way
                best = cell;
                bestchoice = smartchoice;
                max = MAX_CELLS + 1;
                window = 0;
            }
        }
        idx++;
    }

    // Found something?
    if (best != NULL)
    {
        if (MAX_CELLS >= max)
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
choose(const Cell * cell)
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


/*
 * The top level search routine.
 * Returns if an object is found, or is impossible.
 */
Status
search(void)
{
    Cell * cell;
    Bool free;
    State state;

    if (smarton) {
        getunknown = &getsmartunknown; // KAS
    } else {
        getunknown = &getnormalunknown;
    }

    cell = (*getunknown)();

    if (cell == NULL)
    {
        /*
         * nothing to search so we are at a solution
         * let's start search for another one
         */
        cell = backup();

        if (cell == NULL)
            return ERROR;

        free = FALSE;
        state = (ON + OFF) - prevstate;
    }
    else
    {
        state = choose(cell);
        free = TRUE;
    }

    for (;;)
    {
        /*
         * Set the state of the new cell.
         */
        if (!go(cell, state, free)) 
        {
            printgen(curgen);
            return NOTEXIST;
        }

        /*
         * If it is time to dump our state, then do that.
         */
        if (dumpfreq && (++dumpcount >= dumpfreq))
        {
            dumpstate(dumpfile);
            dumpcount = 0;
        }

        /*
         * If it is time to view the progress,then show it.
         */
        if (viewfreq && (++viewcount >= viewfreq))
        {
            printgen(curgen);
        }

        /*
         * Get the next unknown cell and choose its state.
         */
        cell = (*getunknown)();

        if (cell == NULL)
        {
            return FOUND;
        }

        state = choose(cell);
        free = TRUE;
    }
}

/* END CODE */
