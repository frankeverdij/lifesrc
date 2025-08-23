/*
 * Life search program - actual search routines.
 * Author: David I. Bell.
 * Based on the algorithms by Dean Hickerson that were
 * included with the "xlife 2.0" distribution.  Thanks!
 * Changes for arbitrary Life rules by Nathan S. Thompson.
 */

/*
 * Define this as a null value so as to define the global variables
 * defined in lifesrc.h here.
 */
#define EXTERN

#include "lifesrc.h"
#include "state.h"
#include "flags.h"
#include "transition.h"
#include "implication.h"
#include "nextstate.h"
#include "description.h"
#include "sortorder.h"
#include "loopcells.h"
#include "allocatecell.h"
#include "linkcell.h"
#include "symcell.h"
#include "mapcell.h"
#include "findcell.h"
#include "tty.h"


/*
 * Local variables
 */
static int cellCount = 0; /* number of set cells */


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
resCell(Cell * cell)
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


/*
 * Set the state of a cell to the specified state.
 * The state is either ON or OFF.
 * Returns ERROR if the setting is inconsistent.
 * If the cell is newly set, then it is added to the set table.
 */
Bool
setCell(Cell * const cell, const State state, const Bool free)
{
    if (cell->state == state)
    {
        DPRINTF("setCell %d %d %d to state %s already set\n",
            cell->row, cell->col, cell->gen,
            (state == ON) ? "on" : "off");

        return TRUE;
    }

    if (cell->state == UNK)
    {
        DPRINTF("setCell %d %d %d to %s, %s successful\n",
            cell->row, cell->col, cell->gen,
            (free ? "free" : "forced"), ((state == ON) ? "on" : "off"));

        *newSet++ = cell;
        setState(cell, state);
        cell->free = free;

        return TRUE;
    }

    DPRINTF("setCell %d %d %d to state %s inconsistent\n",
        cell->row, cell->col, cell->gen,
        (state == ON) ? "on" : "off");

    return FALSE;
}


void shortSetCell(Cell * const cell, const State state)
{
    if (cell->state == UNK)
    {
        *newSet++ = cell;
        setState(cell, state);
        cell->free = FALSE;
    }

    return;
}

#if 0
/*
 * Calculate the current descriptor for a cell.
 */
static int
getDesc(const Cell * const cell)
{
    return SUMTODESC(cell->state, cell->sumNear);
}
#endif

/*
 * Consistify a cell.
 * This means examine this cell in the previous generation, and
 * make sure that the previous generation can validly produce the
 * current cell.  Returns ERROR if the cell is inconsistent.
 */
static Bool
consistify(Cell * const cell)
{
    Cell * prevCell;
    int desc;
    State state;
    Flags flags;

    /*
     * First check the transit table entry for the previous
     * generation.  Make sure that this cell matches the ON or
     * OFF state demanded by the transit table.  If the current
     * cell is unknown but the transit table knows the answer,
     * then set the now known state of the cell.
     */
    prevCell = cell->past;
    desc = SUMTODESC(prevCell->state, prevCell->sumNear);
    state = transit[desc];

    /*
     * This was the first iteration of the code logic which determines
     * either the setting of an unknown cell state from the previous cell,
     * or exiting if the cell state given from the transit table contradicts
     * the current known cell state.

    if (state != UNK)
        if (state != cell->state)
            if (setCell(cell, state, FALSE) == ERROR)
                return ERROR;
    cellState = cell->state;
    if (cellState == UNK)
        return OK;

     * The code below is equivalent and faster.
     */

    if (cell->state == UNK)
    {
        if (state != UNK)
        {
            *newSet++ = cell;
            setState(cell, state);
            cell->free = FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else if ((cell->state ^ state) == ON)
        return FALSE;

    /*
     * Now look up the previous generation in the implic table.
     * If this cell implies anything about the cell or its neighbors
     * in the previous generation, then handle that.
     */

    /*
     * The shift of 'flags' depending on cell->state is a trick to save
     * an 'if' statement since the bitflags for set cells is exactly the same
     * as for unset cells, except they are left shifted by 4 bits.
     */

    flags = implic[desc] >> 4 * cell->state;

    DPRINTF("Implication flags %x\n", flags);

    if (flags & N0IC0)
        if (!setCell(prevCell, OFF, FALSE))
            return FALSE;

    if (flags & N0IC1)
        if (!setCell(prevCell, ON, FALSE))
            return FALSE;

    if (flags & N0ICUN1)
    {
        /*
         * For each unknown neighbor, set its state as indicated.
         * Return an error if any neighbor is inconsistent.
         */
        DPRINTF("Forcing unknown neighbors of cell %d %d %d %s\n",
            prevCell->row, prevCell->col, prevCell->gen, "on");

        shortSetCell(prevCell->cul, ON);
        shortSetCell(prevCell->cu, ON);
        shortSetCell(prevCell->cur, ON);
        shortSetCell(prevCell->cl, ON);
        shortSetCell(prevCell->cr, ON);
        shortSetCell(prevCell->cdl, ON);
        shortSetCell(prevCell->cd, ON);
        shortSetCell(prevCell->cdr, ON);
        
        DPRINTF("Implications successful\n");

        return TRUE;
    }
    
    if (flags & N0ICUN0)
    {
        DPRINTF("Forcing unknown neighbors of cell %d %d %d %s\n",
            prevCell->row, prevCell->col, prevCell->gen, "off");

        shortSetCell(prevCell->cul, OFF);
        shortSetCell(prevCell->cu, OFF);
        shortSetCell(prevCell->cur, OFF);
        shortSetCell(prevCell->cl, OFF);
        shortSetCell(prevCell->cr, OFF);
        shortSetCell(prevCell->cdl, OFF);
        shortSetCell(prevCell->cd, OFF);
        shortSetCell(prevCell->cdr, OFF);
    }

    DPRINTF("Implications successful for prevCell %d %d %d %d\n", prevCell->row, prevCell->col, prevCell->gen, prevCell->state);

    return TRUE;
}


/*
 * See if a cell and its neighbors are consistent with the cell and its
 * neighbors in the next generation.
 */
static Bool
consistify10(Cell * const cell)
{
    if (!consistify(cell))
        return FALSE;

    if (!consistify(cell->future))
        return FALSE;

    if (!consistify(cell->cul->future))
        return FALSE;

    if (!consistify(cell->cu->future))
        return FALSE;

    if (!consistify(cell->cur->future))
        return FALSE;

    if (!consistify(cell->cl->future))
        return FALSE;

    if (!consistify(cell->cr->future))
        return FALSE;

    if (!consistify(cell->cdl->future))
        return FALSE;

    if (!consistify(cell->cd->future))
        return FALSE;

    if (!consistify(cell->cdr->future))
        return FALSE;

    return TRUE;
}


/*
 * Examine the next choice of cell settings.
 */
static Status
examineNext(void)
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

    if (cell->loop && (!setCell(cell->loop, cell->state, FALSE)))
    {
        return ERROR;
    }

    return consistify10(cell) ? OK : ERROR;
}


/*
 * Set a cell to the specified value and determine all consequences we
 * can from the choice.  Consequences are a contradiction or a consistency.
 */
Bool
Proceed(Cell * cell, const State state, const Bool free)
{
    int status;

    if (!setCell(cell, state, free))
        return FALSE;

    do {
        status = examineNext();
    } while (status == OK);

    return (status == CONSISTENT) ? TRUE : FALSE;
}


/*
 * Back up the list of set cells to undo choices.
 * Returns the cell which is to be tried for the other possibility.
 * Returns NULL on an "object cannot exist" error.
 */
Cell *
Backup(void)
{
    Cell * cell;


    while (newSet != baseSet)
    {
        cell = *--newSet;

        DPRINTF("backing up cell %d %d %d, was %s, %s\n",
            cell->row, cell->col, cell->gen,
            ((cell->state == ON) ? "on" : "off"),
            (cell->free ? "free": "forced"));

        if (!cell->free)
        {
            setState(cell, UNK);
            cell->free = TRUE;

            continue;
        }

        nextSet = newSet;
        searchIdx = cell->index;

        return cell;
    }

    nextSet = baseSet;
    searchIdx = 0;

    return NULL;
}


/*
 * Do checking based on setting the specified cell.
 * Returns ERROR if an inconsistency was found.
 */
static Bool
go(Cell * cell, State state, Bool free)
{
    quitOk = FALSE;

    for (;;)
    {
        if (Proceed(cell, state, free))
        {
            return TRUE;
        }

        ++stepConfl;
        cell = Backup();

        if (cell == NULL)
        {
            return FALSE;
        }

        free = FALSE;
        state = (ON + OFF) - cell->state;
        setState(cell, UNK);
    }
}


/*
 * Find another unknown cell in a normal search.
 * Returns NULL if there are no more unknown cells.
 */
static Cell *
getNormalUnknown(void)
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

        if ((cell->past->state == OFF) ||
            (cell->future->state == OFF))
        {
            return OFF;
        }
    }

    return chooseUnknown;
}


/*
 * The top level search routine.
 * Returns if an object is found, or is impossible.
 */
Status
Search(const Bool batch)
{
    Cell * cell;
    Bool free;
    State state;

    cell = getNormalUnknown();

    if (cell == NULL)
    {
        cell = Backup();

        if (cell == NULL)
            return ERROR;

        free = FALSE;
        state = (ON + OFF) - cell->state;
        setState(cell, UNK);
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
            return NOT_EXIST;

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
        if (viewFlag)
        {
            printGen(curGen);
            viewFlag = FALSE;
        }

        /*
         * Check for commands.
         */
        if (!batch)
        {
            if (ttyCheck())
                getCommands();
        }

        /*
         * Get the next unknown cell and choose its state.
         */
        cell = getNormalUnknown();

        if (cell == NULL)
            return FOUND;

        state = choose(cell);
        free = TRUE;
    }
}

/* END CODE */
