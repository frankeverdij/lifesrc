#include "stdlib.h"
#include "loopcells.h"
#include "deadcell.h"
#include "enums.h"
#include "macros.h"

/*
 * Make the two specified cells belong to the same loop.
 * If the two cells already belong to loops, the loops are joined.
 * This will force the state of these two cells to follow each other.
 * Symmetry uses this feature, and so does setting stable cells.
 * If any cells in the loop are frozen, then they all are.
 */
void
loopCells(const Bool smartOn, Cell * cell1, Cell * cell2)
{
    Cell * cell;
    Bool frozen;

    if (cell2 == NULL) return;

    if (cell1 == cell2) return;

    if (!smartOn)
    {
    /*
     * Check simple cases of equality, or of either cell
     * being the deadCell.
     */
    if ((cell1 == deadCell) || (cell2 == deadCell))
        FATAL("Attemping to use deadCell in a loop\n");

    /*
     * Make the cells belong to their own loop if required.
     * This will simplify the code.
     */
    if (cell1->loop == NULL)
        cell1->loop = cell1;

    if (cell2->loop == NULL)
        cell2->loop = cell2;
    }
    /*
     * See if the second cell is already part of the first cell's loop.
     * If so, they they are already joined.  We don't need to
     * check the other direction.
     */
    for (cell = cell1->loop; cell != cell1; cell = cell->loop)
    {
        if (cell == cell2) return;
    }

    /*
     * The two cells belong to separate loops.
     * Break each of those loops and make one big loop from them.
     */
    cell = cell1->loop;
    cell1->loop = cell2->loop;
    cell2->loop = cell;

    /*
     * See if any of the cells in the loop are frozen.
     * If so, then mark all of the cells in the loop frozen
     * since they effectively are anyway.  This lets the
     * user see that fact.
     */
    frozen = cell1->frozen;

    for (cell = cell1->loop; cell != cell1; cell = cell->loop)
    {
        if (cell->frozen)
            frozen = TRUE;
    }

    if (frozen)
    {
        cell1->frozen = TRUE;

        for (cell = cell1->loop; cell != cell1; cell = cell->loop)
            cell->frozen = TRUE;
    }
}
