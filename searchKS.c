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
#define    EXTERN extern

#include "lifesrc.h"
#include "implicationKS.h"


/*
 * Local variables
 */
static int smartstatlen = 0;
static int smartstatwnd = 0;
static int smartstatsumlen = 0;
static int smartstatsumwnd = 0;
static int smartstatsumlenc = 0;
static int smartstatsumwndc = 0;

static State smartChoice = UNK;
static int smartlen0;
static int smartlen1;

static int cellCount = 0; /* number of set cells */
static State prevState; /* the state of the last free cell before backup() */

static Cell * (*getunknown)(void);

static void setState(Cell * const cell, const State state)
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

    --cellCount; // take all loops as a single cell

    c1 = cell;

    do {
        setState(cell, UNK);
        cell->free = TRUE;

        cell = cell->loop;
    } while (cell != c1);
}


void shortsetcell(Cell * cell, const State state)
{
    Cell * c1 = cell, * c2;

    do {
        setState(cell, state);
        cell->free = FALSE;

        if (cell->active) {
            *newSet++ = cell;
            *searchSet++ = searchList[searchIdx];
            for (; (c2 = searchList[searchIdx]); searchIdx++)
            {
                if (c2->state == UNK)
                {
                    break;
                }
            }
        }
        cell = cell->loop;
    } while (c1 != cell);

    ++cellCount; // take whole loop as a single cell

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
    Cell *c1, *c2;

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

        c1 = cell;

        do {
            setState(cell, state);
            cell->free = free;

            if (cell->active) {
                *newSet++ = cell;
                *searchSet++ = searchList[searchIdx];
                for (; (c2 = searchList[searchIdx]); searchIdx++)
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

        ++cellCount; // take whole loop as a single cell

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
    return SUMTODESCKS(cell->future->state, cell->state, cell->sumNear);
}


/*
 * Consistify a cell.
 * This means examine this cell in the previous generation, and
 * make sure that the previous generation can validly produce the
 * current cell.  Returns FALSE if the cell is inconsistent.
 */
static Bool consistify(Cell * cell)
{
    Cell * prevCell;
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

    prevCell = cell->past;
    desc = SUMTODESCKS(cell->state, prevCell->state, prevCell->sumNear);

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
        !setcell(prevCell, ((flags & IMPC1) != 0) ? ON : OFF, FALSE)) return FALSE;

    if ((flags & IMPUN) != 0)
    {
        // let's change the parent neighborhood
        state = ((flags & IMPUN1) != 0) ? ON : OFF;

        DPRINTF("Forcing unknown neighbors of cell %d %d %d %s\n",
            prevCell->row, prevCell->col, prevCell->gen, (state == ON) ? "on" : "off");

        if (prevCell->cul->state == UNK)
            shortsetcell(prevCell->cul, state);
        if (prevCell->cu->state == UNK)
            shortsetcell(prevCell->cu, state);
        if (prevCell->cur->state == UNK)
            shortsetcell(prevCell->cur, state);
        if (prevCell->cl->state == UNK)
            shortsetcell(prevCell->cl, state);
        if (prevCell->cr->state == UNK)
            shortsetcell(prevCell->cr, state);
        if (prevCell->cdl->state == UNK)
            shortsetcell(prevCell->cdl, state);
        if (prevCell->cd->state == UNK)
            shortsetcell(prevCell->cd, state);
        if (prevCell->cdr->state == UNK)
            shortsetcell(prevCell->cdr, state);
    }

    DPRINTF("Implications successful for prevCell %d %d %d %d\n", prevCell->row, prevCell->col, prevCell->gen, prevCell->state);

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
    if (nextSet == newSet)
        return CONSISTENT;

    /*
     * Get the next cell to examine, and check it out for symmetry
     * and for consistency with its previous and next generations.
     */
    cell = *nextSet++;

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

    nextSet = newSet;

    while (nextSet != setTable)
    {
        cell = *--nextSet;
        --searchSet;

        DPRINTF("backing up cell %d %d %d, was %s, %s\n",
            cell->row, cell->col, cell->gen,
            ((cell->state == ON) ? "on" : "off"),
            ((cell->free) ? "free": "forced"));

        if (!cell->free) continue;

        // free cell found
        // record old status
        prevState = cell->state;

        searchIdx = (*searchSet)->index;

        // reset the stack and return the cell
        while (newSet != nextSet) {
            rescell(*--newSet);
        }

        return cell;
    }

    // free cell not found
    // let's reset the stack anyway

    while (newSet != nextSet) {
        rescell(*--newSet);
    }

    return NULL;
}


/*
 * Do checking based on setting the specified cell.
 * Returns ERROR if an inconsistency was found.
 */
static Bool
go(Cell * cell, State state, Bool free)
{
    Cell ** setpos;

    for (;;)
    {
        setpos = nextSet;

        if (proceed(cell, state, free))
        {
            return TRUE;
        }

        if ((setpos == nextSet) && free)
        {
            // no cell added to stack
            // no backup required
            // but prevState is not defined now
            state = (ON + OFF) - state;
        } else {
            ++stepConfl;
            cell = backup();

            if (cell == NULL)
            {
                return FALSE;
            }

            state = (ON + OFF) - prevState;
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

    for (int i = searchIdx; (cell = searchList[i]) ; i++)
    {
        if (cell->state == UNK)
        {
            if (cell->choose)
            {
                searchIdx = i;

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
    setpos = newSet;

    // remember cell count to calculate the change
    cellno = cellCount;

    // test the cell
    if (proceed(cell, ON, TRUE))
    {
        smartlen1 = cellCount - cellno;

        // back up
        backup(); 

        // and now let's try the OFF choice

        if (proceed(cell, OFF, TRUE))
        {
            smartlen0 = cellCount - cellno;
            smartChoice = (smartlen1 > smartlen0) ? ON : OFF;

            // back up
            backup();

            return TRUE;

        } else {
            // OFF state inconsistent
            // makes a good candidate
            smartChoice = OFF;

            // back up if something changed
            if (setpos != newSet) backup();

            return FALSE;
        }

    } else {
        // ON state inconsistent
        // it's actually a good candidate
        smartChoice = ON;

        // back up if something changed
        if (setpos != newSet) backup();

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

    // Move the searchList over all known cells
    for (; (cell = searchList[searchIdx]); searchIdx++)
    {
        if ((cell->state == UNK) && (cell->choose))
        {
            break;
        }
    }

    // Return NULL if no unknown cells
    if (cell == NULL) return NULL;

    // Prepare threshold
    threshold = smartThreshold;
    if (threshold <= 0) threshold = MAX_CELLS;

    // Prepare the dummy maximum
    max = 2; // at least 3 cells must change
    bestlen0 = 1;
    bestlen1 = 1;
//    bestcomb = 0;

    best = NULL;

    wnd = 0;

    window = smartWindow;
    idx = searchIdx;

    while ((cell = searchList[idx]) && (window > 0) && (max < threshold)) {
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
                        bestchoice = smartChoice;
                        // bestcomb = smartcomb; -- it's equal anyway
                        bestlen1 = smartlen1;
                        bestlen0 = smartlen0;
                        max = bestlen0 + bestlen1;
                    }
/*                }
                else if (bestcomb < smartcomb)
                {
                    best = cell;
                    bestchoice = smartChoice;
                    bestcomb = smartcomb;
                    bestlen1 = smartlen1;
                    bestlen0 = smartlen0;
                    max = bestlen0 + bestlen1;
                }*/
            } else {
                // the cell can be set only one way
                best = cell;
                bestchoice = smartChoice;
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

        if (smartOn)
        {
            // propose the shorter tree
            smartChoice = bestchoice;
        } else {
            smartChoice = UNK;
        }
        return best;
    }

    // fall back to standard
    smartChoice = UNK;

    // Just return the first UNK cell
    // This shouldn't be often anyway

    return searchList[searchIdx];
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
    if (smartChoice != UNK) return smartChoice;

    /*
     * If we are following cells in other generations,
     * then try to do that.
     */
    if (followGens)
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
search(const Bool batch)
{
    Cell * cell;
    Bool free;
    State state;

    if (smartOn == 2)
    {
        getunknown = &getsmartunknown;
    }
    else
    {
        getunknown = &getnormalunknown;
    }
    
    cell = getunknown();

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
        state = (ON + OFF) - prevState;
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
            printGen(curGen);
            return NOT_EXIST;
        }

        /*
         * If it is time to dump our state, then do that.
         */
        if (dumpFlag)
        {
            dumpState(dumpFile);
            dumpFlag = FALSE;
        }

        /*
         * If it is time to view the progress,then show it.
         */
        ++viewCount;
        if (viewFlag)
        {
            printGen(curGen);
            viewFlag = FALSE;
        }

        /*
         * Get the next unknown cell and choose its state.
         */
        cell = getunknown();

        if (cell == NULL)
        {
            return FOUND;
        }

        state = choose(cell);
        free = TRUE;
    }
}

/* END CODE */
